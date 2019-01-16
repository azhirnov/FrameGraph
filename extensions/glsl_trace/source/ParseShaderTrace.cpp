// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Common.h"

#include <array>
#include <assert.h>

#include "glslang/glslang/Include/BaseTypes.h"
using namespace glslang;

using VariableID = ShaderTrace::VariableID;

namespace std
{
	inline std::string  to_string (bool value)
	{
		return value ? "true" : "false";
	}
}

/*
=================================================
	Trace
=================================================
*/
namespace {
	struct Trace
	{
		using ExprInfo		= ShaderTrace::ExprInfo;
		using VarNames_t	= ShaderTrace::VarNames_t;
		using Sources_t		= ShaderTrace::Sources_t;

		union Value
		{
			std::array< int32_t, 4 >	i;
			std::array< uint32_t, 4 >	u;
			std::array< bool, 4 >		b;
			std::array< float, 4 >		f;
			std::array< double, 4 >		d;
			std::array< int64_t, 4 >	i64;
			std::array< uint64_t, 4 >	u64;
		};

		struct VariableState
		{
			Value			value		{};
			TBasicType		type		= TBasicType::EbtVoid;
			uint32_t		count		= 0;
		};

		using VarStates = std::unordered_map< uint64_t, VariableState >;


		uint32_t	lastPosition	= ~0u;
		VarStates	_states;


		bool AddState (const ExprInfo &expr, TBasicType type, uint32_t rows, uint32_t cols, const uint32_t *data);
		bool ToString (const ExprInfo &expr, const VarNames_t &varNames, const Sources_t &src, OUT std::string &result) const;
	};
}
/*
=================================================
	ParseShaderTrace
=================================================
*/
bool ShaderTrace::ParseShaderTrace (const void *ptr, uint64_t maxSize, OUT std::vector<std::string> &result) const
{
	result.clear();

	const uint64_t		count		= *(static_cast<uint32_t const*>(ptr) + _posOffset / sizeof(uint32_t));
	uint32_t const*		start_ptr	= static_cast<uint32_t const*>(ptr) + _dataOffset / sizeof(uint32_t);
	uint32_t const*		end_ptr		= start_ptr + std::min( count, (maxSize - _dataOffset) / sizeof(uint32_t) );
	std::vector<Trace>	shaders;
	
	for (auto data_ptr = start_ptr; data_ptr < end_ptr;)
	{
		uint32_t		pos			= uint32_t(std::distance( start_ptr, data_ptr ));
		uint32_t		prev_pos	= *(data_ptr++);
		uint32_t		expr_id		= *(data_ptr++);
		uint32_t		type		= *(data_ptr++);
		TBasicType		t_basic		= TBasicType(type & 0xFF);
		uint32_t		row_size	= (type >> 8) & 0xF;		// for scalar, vector and matrix
		uint32_t		col_size	= (type >> 12) & 0xF;		// only for matrix
		Trace*			trace		= nullptr;

		CHECK_ERR( prev_pos == ~0u or prev_pos < pos );
		CHECK_ERR( expr_id < _exprLocations.size() );
		CHECK_ERR( (t_basic == TBasicType::EbtVoid and row_size == 0) or (row_size > 0 and row_size <= 4) );
		CHECK_ERR( col_size >= 0 and col_size <= 4 );

		for (auto& sh : shaders) {
			if ( sh.lastPosition == prev_pos ) {
				trace = &sh;
				break;
			}
		}
		
		if ( prev_pos == ~0u or not trace )
		{
			shaders.push_back( Trace{} );
			result.resize( shaders.size() );
			trace = &shaders.back();
		}

		auto&	expr = _exprLocations[expr_id];

		CHECK_ERR( trace->AddState( expr, t_basic, row_size, col_size, data_ptr ));
		CHECK_ERR( trace->ToString( expr, _varNames, _sources, INOUT result[std::distance( shaders.data(), trace )] ));

		trace->lastPosition = pos;

		data_ptr += (row_size * std::max(1u, col_size)) * (t_basic == TBasicType::EbtDouble or t_basic == TBasicType::EbtUint64 or t_basic == TBasicType::EbtInt64 ? 2 : 1);
	}
	return true;
}

/*
=================================================
	CopyValue
=================================================
*/
inline void CopyValue (TBasicType type, INOUT Trace::Value &value, uint32_t valueIndex, const uint32_t *data, INOUT uint32_t &dataIndex)
{
	switch ( type )
	{
		case TBasicType::EbtInt :
			memcpy( OUT &value.i[valueIndex], &data[dataIndex++], sizeof(int) );
			break;
		
		case TBasicType::EbtBool :
			value.b[valueIndex] = (data[dataIndex++] != 0);
			break;
		
		case TBasicType::EbtUint :
			value.u[valueIndex] = data[dataIndex++];
			break;
		
		case TBasicType::EbtFloat :
			memcpy( OUT &value.f[valueIndex], &data[dataIndex++], sizeof(float) );
			break;
		
		case TBasicType::EbtDouble :
			memcpy( OUT &value.d[valueIndex], &data[dataIndex], sizeof(double) );
			dataIndex += 2;
			break;
		
		case TBasicType::EbtInt64 :
			memcpy( OUT &value.d[valueIndex], &data[dataIndex], sizeof(int64_t) );
			dataIndex += 2;
			break;
		
		case TBasicType::EbtUint64 :
			memcpy( OUT &value.u64[valueIndex], &data[dataIndex], sizeof(uint64_t) );
			dataIndex += 2;
			break;
		
		default :
			CHECK( !"not supported" );
			break;
	}
}

/*
=================================================
	Trace::AddState
=================================================
*/
bool Trace::AddState (const ExprInfo &expr, TBasicType type, uint32_t rows, uint32_t cols, const uint32_t *data)
{
	cols = std::max( 1u, cols );

	for (uint32_t col = 0; col < cols; ++col)
	{
		uint64_t	id   = (uint64_t(expr.varID) << 32) | col;
		auto		iter = _states.find( id );

		if ( iter == _states.end() or expr.varID == VariableID::Unknown )
			iter = _states.insert_or_assign( id, VariableState{ {}, type, rows } ).first;


		VariableState&	var = iter->second;

		CHECK( expr.varID == VariableID::Unknown or var.type == type );
		var.type = type;	

		if ( var.type == TBasicType::EbtVoid )
		{
			var.value = {};
			var.count = 0;
		}
		else
		if ( expr.swizzle )
		{
			for (uint32_t i = 0; i < rows; ++i)
			{
				uint32_t	sw = (expr.swizzle >> (i*3)) & 7;
				ASSERT( sw > 0 and sw <= 4 );
				var.count = std::max( var.count, sw );
			}
			
			for (uint32_t r = 0, j = 0; r < rows; ++r)
			{
				uint32_t	sw = (expr.swizzle >> (r*3)) & 7;
				CopyValue( type, INOUT var.value, sw-1, data, INOUT j );
			}
		}
		else
		{
			for (uint32_t r = 0, j = 0; r < rows; ++r) {
				CopyValue( type, INOUT var.value, r, data, INOUT j );
			}
		}
		
		var.count = std::max( var.count, rows );
		var.count = std::min( var.count, 4u );
	}
	return true;
}
	
/*
=================================================
	Trace::AddState
=================================================
*/
template <typename T>
static std::string  TypeToString (uint32_t rows, const std::array<T,4> &values)
{
	std::string		str;

	if ( rows > 1 )
		str += std::to_string( rows );

	str += " {";

	for (uint32_t r = 0; r < rows; ++r) {
		str += (r ? ", " : "") + std::to_string( values[r] );
	}
	
	str += "}\n";
	return str;
}

/*
=================================================
	Trace::AddState
=================================================
*/
bool Trace::ToString (const ExprInfo &expr, const VarNames_t &varNames, const Sources_t &sources, OUT std::string &result) const
{
	const auto	Convert = [this, &varNames, OUT &result] (VariableID varID, bool isOutput) -> bool
	{
		for (uint32_t i = 0; i < 4; ++i)
		{
			uint64_t	id   = (uint64_t(varID) << 32) | i;
			auto		iter = _states.find( id );

			if ( iter == _states.end() )
				continue;

			auto	name = varNames.find( varID );
			if ( name != varNames.end() )
				result += std::string("//") + (isOutput ? "> " : "  ") + name->second + ": ";
			else
				result += std::string("//") + (isOutput ? "> (out): " : "  (temp): ");

			switch ( iter->second.type )
			{
				case TBasicType::EbtVoid : {
					result += "void\n";
					break;
				}
				case TBasicType::EbtFloat : {
					result += "float";
					result += TypeToString( iter->second.count, iter->second.value.f );
					break;
				}
				case TBasicType::EbtDouble : {
					result += "double";
					result += TypeToString( iter->second.count, iter->second.value.d );
					break;
				}
				case TBasicType::EbtInt : {
					result += "int";
					result += TypeToString( iter->second.count, iter->second.value.i );
					break;
				}
				case TBasicType::EbtBool : {
					result += "bool";
					result += TypeToString( iter->second.count, iter->second.value.b );
					break;
				}
				case TBasicType::EbtUint : {
					result += "uint";
					result += TypeToString( iter->second.count, iter->second.value.u );
					break;
				}
				case TBasicType::EbtInt64 : {
					result += "long";
					result += TypeToString( iter->second.count, iter->second.value.i64 );
					break;
				}
				case TBasicType::EbtUint64 : {
					result += "ulong";
					result += TypeToString( iter->second.count, iter->second.value.u64 );
					break;
				}
				default :
					RETURN_ERR( "not supported" );
			}
		}
		return true;
	};

	CHECK_ERR( Convert( expr.varID, true ));

	for (auto& id : expr.vars) {
		Convert( id, false );
	}

	CHECK_ERR( expr.range.sourceId < sources.size() );
	const auto&		src			= sources[ expr.range.sourceId ];
	const uint32_t	start_line	= std::max( 1u, expr.range.begin.Line() ) - 1;	// because first line is 1
	const uint32_t	end_line	= std::max( 1u, expr.range.end.Line() ) - 1;
	const uint32_t	start_col	= std::max( 1u, expr.range.begin.Column() ) - 1;
	const uint32_t	end_col		= std::max( 1u, expr.range.end.Column() ) - 1;

	CHECK_ERR( start_line < src.lines.size() );
	CHECK_ERR( end_line < src.lines.size() );

	if ( expr.range.sourceId == 0 and end_line == 0 and end_col == 0 )
		result += "no source\n";
	else
	for (uint32_t i = start_line; i <= end_line; ++i)
	{
		size_t	start	= src.lines[i].first;
		size_t	end		= src.lines[i].second;
		size_t	length	= (end - start);
		
		/*if ( i == end_line ) {
			CHECK_ERR( end_col < length );
			end = start + end_col;
		}*/
		if ( i == start_line ) {
			CHECK_ERR( start_col < length );
			start += start_col;
		}
		CHECK_ERR( start < end );

		result += std::to_string(i+1) + ". ";
		result += src.code.substr( start, end - start );
		result += '\n';
	}
	result += '\n';

	return true;
}
