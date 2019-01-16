// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Common.h"

#include <algorithm>
#include <cstring>
#include <assert.h>

#include "glslang/glslang/Include/BaseTypes.h"
using namespace glslang;


# if !(defined(RDOC_X64) || defined(RDOC_X86))
/*
=================================================
	for RenderDoc compatibility
=================================================
*/
namespace {
	template <typename T> using rdcarray = std::vector<T>;

	enum class ShaderEvents : uint32_t
	{
		NoEvent = 0,
		SampleLoadGather = 0x1,
		GeneratedNanOrInf = 0x2,
	};

	enum class VarType : uint32_t
	{
		Float = 0,
		Int,
		UInt,
		Double,
		Unknown = ~0U,
	};
	
	enum class RegisterType : uint32_t
	{
		Undefined,
		Input,
		Temporary,
		IndexedTemporary,
		Output,
	};

	union ShaderValue
	{
		float			fv[16];
		int32_t			iv[16];
		uint32_t		uv[16];
		double			dv[16];
		uint64_t		u64v[16];
	};

	struct ShaderVariable
	{
		uint32_t						rows			= 0;
		uint32_t						columns			= 0;
		std::string						name;
		VarType							type			= VarType::Unknown;
		ShaderValue						value			{};
		bool							displayAsHex	= false;
		bool							isStruct		= false;
		bool							rowMajor		= false;
		rdcarray<ShaderVariable>		members;
	};

	struct LocalVariableMapping
	{
	};

	struct RegisterRange
	{
		RegisterType					type		= RegisterType::Undefined;
		uint16_t						index		= 0xFFFF;
		uint16_t						component	= 0;
	};

	struct ShaderDebugState
	{
		rdcarray<ShaderVariable>		registers;
		rdcarray<ShaderVariable>		outputs;
		rdcarray<ShaderVariable>		indexableTemps;
		rdcarray<LocalVariableMapping>	locals;
		rdcarray<RegisterRange>			modified;
		uint32_t						nextInstruction	= 0;
		ShaderEvents					flags			= ShaderEvents::NoEvent;
	};

	struct LineColumnInfo
	{
		int32_t							fileIndex	= -1;
		uint32_t						lineStart	= 0;
		uint32_t						lineEnd		= 0;
		uint32_t						colStart	= 0;
		uint32_t						colEnd		= 0;
		rdcarray<std::string>			callstack;
	};
}

struct ShaderDebugTrace
{
	rdcarray<ShaderVariable>		inputs;
	rdcarray<ShaderVariable>		constantBlocks;
	rdcarray<ShaderDebugState>		states;
	rdcarray<LineColumnInfo>		lineInfo;
};
# endif

/*
=================================================
	TraceRdc
=================================================
*/
namespace {
	struct TraceRdc
	{
		using VariableID		= ShaderTrace::VariableID;
		using ExprInfo			= ShaderTrace::ExprInfo;
		using VarNames_t		= ShaderTrace::VarNames_t;
		using SourceLocation	= ShaderTrace::SourceLocation;

		struct VariableState
		{
			ShaderVariable	var;
			bool			modified	= true;
		};

		using VarStates		= std::unordered_map< VariableID, VariableState >;
		using PendingIDs	= std::vector< ExprInfo const* >;


		uint32_t			lastPosition	= ~0u;
		ShaderDebugTrace	debugTrace;
		SourceLocation		_lastLoc;
		VarStates			_states;
		PendingIDs			_pending;


		TraceRdc () {}
		bool AddState (const ExprInfo &expr, const VarNames_t &varNames, TBasicType type, uint32_t rows, uint32_t cols, const uint32_t *data);
		bool Flush ();
	};
}
/*
=================================================
	ConvertTraceToRdc
=================================================
*/
bool ShaderTrace::ConvertTraceToRdc (const void *ptr, uint64_t maxSize, OUT std::vector<ShaderDebugTrace> &result) const
{
	result.clear();

	const uint64_t			count		= *(static_cast<uint32_t const*>(ptr) + _posOffset / sizeof(uint32_t));
	uint32_t const*			start_ptr	= static_cast<uint32_t const*>(ptr) + _dataOffset / sizeof(uint32_t);
	uint32_t const*			end_ptr		= start_ptr + std::min( count, maxSize / sizeof(uint32_t) );
	std::vector<TraceRdc>	shaders;

	for (auto data_ptr = start_ptr; data_ptr < end_ptr;)
	{
		uint32_t		pos			= uint32_t(std::distance( start_ptr, data_ptr ));
		uint32_t		prev_pos	= *(data_ptr++);
		uint32_t		expr_id		= *(data_ptr++);
		uint32_t		type		= *(data_ptr++);
		TBasicType		t_basic		= TBasicType(type & 0xFF);
		uint32_t		row_size	= (type >> 8) & 0xF;		// for scalar, vector and matrix
		uint32_t		col_size	= (type >> 12) & 0xF;		// only for matrix
		TraceRdc*		trace		= nullptr;

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
		
		if ( prev_pos == ~0u or not trace ) {
			shaders.push_back( TraceRdc{} );
			trace = &shaders.back();
		}

		auto&	expr = _exprLocations[expr_id];

		CHECK_ERR( trace->AddState( expr, _varNames, t_basic, row_size, col_size, data_ptr ));

		trace->lastPosition = pos;

		data_ptr += (row_size * std::max(1u, col_size));
	}
	
	CHECK_ERR( shaders.size() );
	result.reserve( shaders.size() );

	for (auto& sh : shaders)
	{
		CHECK_ERR( sh.Flush() );

		// add the last state
		{
			ShaderDebugState		state = {};
			state.nextInstruction	= uint32_t(sh.debugTrace.lineInfo.size());
			state.flags				= ShaderEvents::NoEvent;

			sh.debugTrace.states.push_back( state );
		}

		result.push_back( std::move(sh.debugTrace) );
	}

	return true;
}

/*
=================================================
	CopyValue
=================================================
*/
inline void CopyValue (TBasicType type, INOUT ShaderValue &value, uint32_t valueIndex, const uint32_t *data, INOUT uint32_t &dataIndex)
{
	switch ( type )
	{
		case TBasicType::EbtBool :
		case TBasicType::EbtInt :
			memcpy( OUT &value.iv[valueIndex], &data[dataIndex++], sizeof(int) );
			break;
		
		case TBasicType::EbtUint :
			value.uv[valueIndex] = data[dataIndex++];
			break;
		
		case TBasicType::EbtFloat :
			memcpy( OUT &value.fv[valueIndex], &data[dataIndex++], sizeof(float) );
			break;
		
		case TBasicType::EbtDouble :
			memcpy( OUT &value.dv[valueIndex], &data[dataIndex], sizeof(double) );
			dataIndex += 2;
			break;
	}
}

/*
=================================================
	TraceRdc::AddState
=================================================
*/
bool TraceRdc::AddState (const ExprInfo &expr, const VarNames_t &varNames, TBasicType type, uint32_t rows, uint32_t cols, const uint32_t *data)
{
	if ( not (_lastLoc == expr.range) )
		CHECK_ERR( Flush() );

	auto iter = _states.find( expr.varID );

	if ( iter == _states.end() or expr.varID == VariableID::Unknown )
	{
		VariableState	temp{};
		iter = _states.insert_or_assign( expr.varID, std::move(temp) ).first;
	}
	
	iter->second.modified = true;

	cols = std::max( 1u, cols );

	ShaderVariable&  var = iter->second.var;

	switch ( type ) {
		case TBasicType::EbtVoid : var.type = VarType::Unknown; break;
		case TBasicType::EbtBool :
		case TBasicType::EbtInt : var.type = VarType::Int; break;
		case TBasicType::EbtUint : var.type = VarType::UInt; break;
		case TBasicType::EbtFloat : var.type = VarType::Float; break;
		case TBasicType::EbtDouble : var.type = VarType::Double; break;
		default : CHECK( !"not supported" ); break;
	}
	
	var.columns	= std::min( 4u, std::max( var.columns, rows ));
	var.rows	= std::min( 4u, std::max( var.rows, cols ));

	auto	name = varNames.find( expr.varID );
	if ( name != varNames.end() )
		var.name = name->second;
	else
		var.name = "<result>";

	if ( expr.swizzle )
	{
		ASSERT( cols == 1 );
		
		for (uint32_t i = 0; i < rows; ++i)
		{
			uint32_t	sw = (expr.swizzle >> (i*3)) & 7;
			ASSERT( sw > 0 and sw <= 4 );
			var.columns = std::max( var.columns, sw );
		}
			
		for (uint32_t r = 0, j = 0; r < rows; ++r)
		{
			uint32_t	sw = (expr.swizzle >> (r*3)) & 7;
			CopyValue( type, INOUT var.value, sw-1, data, INOUT j );
		}
	}
	else
	{
		for (uint32_t c = 0, j = 0; c < cols; ++c)
		for (uint32_t r = 0; r < rows; ++r) {
			CopyValue( type, INOUT var.value, c*4 + r, data, INOUT j );
		}
	}

	const bool	has_source_location	= not (expr.range.sourceId == 0 and expr.range.end.Line() == 0 and expr.range.end.Column() == 0);
	const bool	is_builtin			= (var.name.size() > 3 and var.name[0] == 'g' and var.name[1] == 'l' and var.name[2] == '_');

	if ( has_source_location and not is_builtin )
	{
		_pending.push_back( &expr );
	}

	_lastLoc = expr.range;
	return true;
}

/*
=================================================
	TraceRdc::Flush
=================================================
*/
bool TraceRdc::Flush ()
{
	if ( _pending.empty() )
		return true;

	// line info
	{
		auto&	expr = *_pending.front();

		LineColumnInfo	line;
		line.lineStart	= expr.range.begin.Line();
		line.colStart	= expr.range.begin.Column();
		line.lineEnd	= expr.range.end.Line();
		line.colEnd		= expr.range.end.Column();
		//line.callstack	// TODO

		debugTrace.lineInfo.push_back( line );
	}

	std::vector<std::pair<VariableID, bool>>	temporary_ids;

	const auto	AppendVarID = [&temporary_ids] (VariableID id, bool modified)
	{
		for (auto& item : temporary_ids) {
			if ( id == item.first ) {
				item.second |= modified;
				return;
			}
		}
		temporary_ids.push_back({ id, modified });
	};

	for (auto* expr : _pending)
	{
		AppendVarID( expr->varID, true );

		for (auto& id : expr->vars) {
			AppendVarID( id, false );
		}
	}
	
	ShaderDebugState		state = {};
	state.nextInstruction	= uint32_t(debugTrace.lineInfo.size()-1);
	state.flags				= ShaderEvents::NoEvent;

	for (auto& item : temporary_ids)
	{
		auto	iter = _states.find( item.first );
		if ( iter == _states.end() )
			continue;
		
		if ( iter->second.modified or item.second )
		{
			RegisterRange	range;
			range.index		= uint16_t(state.registers.size());
			range.type		= RegisterType::Temporary;
			range.component	= 0;				// TODO: ???
			state.modified.push_back( range );
		}

		iter->second.modified = false;
		state.registers.push_back( iter->second.var );
	}
	
	debugTrace.states.push_back( std::move(state) );
	_pending.clear();

	return true;
}
