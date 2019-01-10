// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include <array>
#include "DebugInfo.h"

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
			std::array< int32_t, 16 >	i;
			std::array< uint32_t, 16 >	u;
			std::array< float, 16 >		f;
			std::array< double, 16 >	d;
		};

		struct VariableState
		{
			Value			value		{};
			TBasicType		type		= TBasicType::EbtVoid;
			uint32_t		rows		= 0;
			uint32_t		columns		= 0;
		};

		using VarStates = std::unordered_map< VariableID, VariableState >;


		uint32_t	lastPosition	= ~0u;
		VarStates	_states;


		bool AddState (VariableID id, TBasicType type, uint32_t rows, uint32_t cols, const uint32_t *data);
		bool ToString (const ExprInfo &expr, const VarNames_t &varNames, const Sources_t &src, OUT std::string &result) const;
	};
}
/*
=================================================
	GetDebugOutput
=================================================
*/
bool ShaderTrace::GetDebugOutput (const void *ptr, uint64_t maxSize, OUT std::vector<std::string> &result) const
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
		uint32_t		row_size	= (type >> 8) & 0xFF;		// for scalar, vector and matrix
		uint32_t		col_size	= (type >> 16) & 0xFF;		// only for matrix
		Trace*			trace		= nullptr;

		CHECK_ERR( prev_pos == ~0u or prev_pos < pos );
		CHECK_ERR( expr_id < _exprLocations.size() );
		CHECK_ERR( row_size > 0 and row_size <= 4 );
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

		CHECK_ERR( trace->AddState( expr.varID, t_basic, row_size, col_size, data_ptr ));
		CHECK_ERR( trace->ToString( expr, _varNames, _sources, INOUT result[std::distance( shaders.data(), trace )] ));

		trace->lastPosition = pos;

		data_ptr += (row_size * std::max(1u, col_size));
	}
	return true;
}

/*
=================================================
	Trace::AddState
=================================================
*/
bool Trace::AddState (VariableID id, TBasicType type, uint32_t rows, uint32_t cols, const uint32_t *data)
{
	VariableState	var;	
	var.type	= type;
	var.rows	= rows;
	var.columns	= cols;
	
	cols = std::max( 1u, cols );

	switch ( type )
	{
		case TBasicType::EbtInt : {
			for (uint32_t c = 0, j = 0; c < cols; ++c)
			for (uint32_t r = 0; r < rows; ++r) {
				memcpy( OUT &var.value.i[c*4 + r], &data[j++], sizeof(int) );
			}
			break;
		}
		case TBasicType::EbtUint : {
			for (uint32_t c = 0, j = 0; c < cols; ++c)
			for (uint32_t r = 0; r < rows; ++r) {
				var.value.u[c*4 + r] = data[j++];
			}
			break;
		}
		case TBasicType::EbtFloat : {
			for (uint32_t c = 0, j = 0; c < cols; ++c)
			for (uint32_t r = 0; r < rows; ++r) {
				memcpy( OUT &var.value.f[c*4 + r], &data[j++], sizeof(float) );
			}
			break;
		}
		default :
			RETURN_ERR( "not supported" );
	}

	_states.insert_or_assign( id, var );
	return true;
}
	
/*
=================================================
	Trace::AddState
=================================================
*/
template <typename T>
static std::string  TypeToString (uint32_t rows, uint32_t columns, const std::array<T,16> &values)
{
	std::string		str;
	const bool		is_matrix	= (columns > 1);

	columns = std::max( 1u, columns );

	if ( rows > 1 )
	{
		str += std::to_string( rows );

		if ( columns > 1 )
			str += "x" + std::to_string( columns );
	}

	str += " {";
	for (uint32_t c = 0; c < columns; ++c)
	{
		if ( is_matrix )
			str += "{";

		for (uint32_t r = 0; r < rows; ++r) {
			str += (r ? ", " : "") + std::to_string( values[c*4 + r] );
		}
		
		if ( is_matrix )
			str += "}, ";
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
	const auto	Convert = [this, &varNames, &result] (VariableID id) -> bool
	{
		auto	iter = _states.find( id );
		if ( iter == _states.end() )
			return false;

		auto	name = varNames.find( id );
		CHECK_ERR( name != varNames.end() );

		result += "// " + name->second + ": ";

		switch ( iter->second.type )
		{
			case TBasicType::EbtFloat : {
				result += "float";
				result += TypeToString( iter->second.rows, iter->second.columns, iter->second.value.f );
				break;
			}
			case TBasicType::EbtInt : {
				result += "int";
				result += TypeToString( iter->second.rows, iter->second.columns, iter->second.value.i );
				break;
			}
			case TBasicType::EbtUint : {
				result += "uint";
				result += TypeToString( iter->second.rows, iter->second.columns, iter->second.value.u );
				break;
			}
			default :
				RETURN_ERR( "not supported" );
		}
		return true;
	};

	CHECK_ERR( Convert( expr.varID ));

	for (auto& id : expr.vars) {
		Convert( id );
	}

	CHECK_ERR( expr.range.sourceId < sources.size() );
	const auto&		src			= sources[ expr.range.sourceId ];
	const uint32_t	start_line	= std::max( 1u, expr.range.begin.Line() ) - 1;	// becouse first line is 1
	const uint32_t	end_line	= std::max( 1u, expr.range.end.Line() ) - 1;
	const uint32_t	start_col	= std::max( 1u, expr.range.begin.Column() ) - 1;
	const uint32_t	end_col		= std::max( 1u, expr.range.end.Column() ) - 1;

	CHECK_ERR( start_line < src.lines.size() );
	CHECK_ERR( end_line < src.lines.size() );

	if ( expr.range.sourceId == 0 and end_line == 0 and end_col == 0 )
		result += "no source\n\n";
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

		result += std::to_string(i) + ". ";
		result += src.code.substr( start, end - start );
		result += "\n\n";
	}

	return true;
}
