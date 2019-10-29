// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Common.h"

#include <array>
#include <assert.h>

#include "glslang/Include/BaseTypes.h"
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
		using ExprInfo			= ShaderTrace::ExprInfo;
		using VarNames_t		= ShaderTrace::VarNames_t;
		using Sources_t			= ShaderTrace::Sources_t;
		using SourceLocation	= ShaderTrace::SourceLocation;

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
			bool			modified	= false;
		};

		using VarStates	= std::unordered_map< uint64_t, VariableState >;
		using Pending	= std::vector< uint64_t >;


		uint32_t			lastPosition	= ~0u;
		VarStates			_states;
		Pending				_pending;
		SourceLocation		_lastLoc;


		bool AddState (const ExprInfo &expr, TBasicType type, uint32_t rows, uint32_t cols, const uint32_t *data,
					   const VarNames_t &varNames, const Sources_t &src, INOUT std::string &result);

		bool Flush (const VarNames_t &varNames, const Sources_t &src, INOUT std::string &result);

		static uint64_t		HashOf (VariableID id, uint32_t col)	{ return (uint64_t(id) << 32) | col; }
		static VariableID	VarFromHash (uint64_t h)				{ return VariableID(h >> 32); }
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
		uint32_t		row_size	= (type >> 8) & 0xF;					// for scalar, vector and matrix
		uint32_t		col_size	= std::max(1u, (type >> 12) & 0xF );	// only for matrix
		uint32_t const*	data		= data_ptr;
		Trace*			trace		= nullptr;

		CHECK_ERR( (t_basic == TBasicType::EbtVoid and row_size == 0) or (row_size > 0 and row_size <= 4) );
		CHECK_ERR( col_size > 0 and col_size <= 4 );

		data_ptr += (row_size * col_size) * (t_basic == TBasicType::EbtDouble or t_basic == TBasicType::EbtUint64 or t_basic == TBasicType::EbtInt64 ? 2 : 1);

		for (auto& sh : shaders) {
			if ( sh.lastPosition == prev_pos ) {
				trace = &sh;
				break;
			}
		}
		
		if ( not trace )
		{
			if ( prev_pos == _initialPosition )
			{
				shaders.push_back( Trace{} );
				result.resize( shaders.size() );
				trace = &shaders.back();
			}
			else
			{
				// this entry from another shader, skip it
				continue;
			}
		}
		
		CHECK_ERR( expr_id < _exprLocations.size() );

		auto&	expr = _exprLocations[expr_id];
		auto&	str  = result[std::distance( shaders.data(), trace )];

		CHECK_ERR( trace->AddState( expr, t_basic, row_size, col_size, data, _varNames, _sources, INOUT str ));

		trace->lastPosition = pos;
	}

	for (size_t i = 0; i < shaders.size(); ++i)
	{
		CHECK_ERR( shaders[i].Flush( _varNames, _sources, INOUT result[i] ));
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
			std::memcpy( OUT &value.i[valueIndex], &data[dataIndex++], sizeof(int) );
			break;
		
		case TBasicType::EbtBool :
			value.b[valueIndex] = (data[dataIndex++] != 0);
			break;
		
		case TBasicType::EbtUint :
			value.u[valueIndex] = data[dataIndex++];
			break;
		
		case TBasicType::EbtFloat :
			std::memcpy( OUT &value.f[valueIndex], &data[dataIndex++], sizeof(float) );
			break;
		
		case TBasicType::EbtDouble :
			std::memcpy( OUT &value.d[valueIndex], &data[dataIndex], sizeof(double) );
			dataIndex += 2;
			break;
		
		case TBasicType::EbtInt64 :
			std::memcpy( OUT &value.d[valueIndex], &data[dataIndex], sizeof(int64_t) );
			dataIndex += 2;
			break;
		
		case TBasicType::EbtUint64 :
			std::memcpy( OUT &value.u64[valueIndex], &data[dataIndex], sizeof(uint64_t) );
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
bool Trace::AddState (const ExprInfo &expr, TBasicType type, uint32_t rows, uint32_t cols, const uint32_t *data,
					  const VarNames_t &varNames, const Sources_t &sources, INOUT std::string &result)
{
	if ( not (_lastLoc == expr.range) )
		CHECK_ERR( Flush( varNames, sources, INOUT result ));

	const auto	AppendID = [this] (uint64_t newID) {
		for (auto& id : _pending) {
			if ( id == newID )
				return;
		}
		_pending.push_back( newID );
	};

	for (uint32_t col = 0; col < cols; ++col)
	{
		uint64_t	id   = HashOf( expr.varID, col );
		auto		iter = _states.find( id );

		if ( iter == _states.end() or expr.varID == VariableID::Unknown )
			iter = _states.insert_or_assign( id, VariableState{ {}, type, rows, false } ).first;


		VariableState&	var = iter->second;

		CHECK( expr.varID == VariableID::Unknown or var.type == type );
		var.type	 = type;
		var.modified = true;

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

		AppendID( id );

		for (auto& var_id : expr.vars) {
			AppendID( HashOf( var_id, 0 ));
			AppendID( HashOf( var_id, 1 ));
			AppendID( HashOf( var_id, 2 ));
			AppendID( HashOf( var_id, 3 ));
		}
	}

	_lastLoc = expr.range;
	return true;
}
	
/*
=================================================
	TypeToString
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
	Trace::Flush
=================================================
*/
bool Trace::Flush (const VarNames_t &varNames, const Sources_t &sources, INOUT std::string &result)
{
	const auto	Convert = [this, &varNames, OUT &result] (uint64_t varHash) -> bool
	{
		VariableID	id	 = VarFromHash( varHash );
		auto		iter = _states.find( varHash );

		if ( iter == _states.end() )
			return false;

		auto	name = varNames.find( id );
		if ( name != varNames.end() )
			result += std::string("//") + (iter->second.modified ? "> " : "  ") + name->second + ": ";
		else
			result += std::string("//") + (iter->second.modified ? "> (out): " : "  (temp): ");

		iter->second.modified = false;

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
		return true;
	};

	if ( _pending.empty() )
		return true;

	for (auto& h : _pending) {
		Convert( h );
	}
	_pending.clear();

	CHECK_ERR( _lastLoc.sourceId < sources.size() );
	const auto&		src			= sources[ _lastLoc.sourceId ];
	const uint32_t	start_line	= std::max( 1u, _lastLoc.begin.Line() ) - 1;	// because first line is 1
	const uint32_t	end_line	= std::max( 1u, _lastLoc.end.Line() ) - 1;
	const uint32_t	start_col	= std::max( 1u, _lastLoc.begin.Column() ) - 1;
	const uint32_t	end_col		= std::max( 1u, _lastLoc.end.Column() ) - 1;

	CHECK_ERR( start_line < src.lines.size() );
	CHECK_ERR( end_line < src.lines.size() );

	if ( _lastLoc.sourceId == 0 and end_line == 0 and end_col == 0 )
		result += "no source\n";
	else
	for (uint32_t i = start_line; i <= end_line; ++i)
	{
		result += std::to_string(i+1) + ". ";

		size_t	start	= src.lines[i].first;
		size_t	end		= src.lines[i].second;
		
		/*if ( i == end_line ) {
			CHECK_ERR( start + end_col < end );
			end = start + end_col;
		}*/
		if ( i == start_line ) {
			//CHECK_ERR( start + start_col < end );
			start += start_col;
		}

		if ( start < end )
		{
			result += src.code.substr( start, end - start );
			result += '\n';
		}
		else
			result += "invalid source location\n";
	}
	result += '\n';

	return true;
}
