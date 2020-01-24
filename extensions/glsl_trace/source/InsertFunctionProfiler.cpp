// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	docs:
	https://github.com/KhronosGroup/GLSL/blob/master/extensions/ext/GL_EXT_shader_realtime_clock.txt
	https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_shader_clock.txt
	https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VK_KHR_shader_clock
*/

#include "Common.h"

// glslang includes
#include "glslang/Include/revision.h"
#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/Include/intermediate.h"

using namespace glslang;
using VariableID = ShaderTrace::VariableID;

namespace
{
	//
	// Debug Info
	//

	struct DebugInfo
	{
	public:
		using SrcLoc		= ShaderTrace::SourceLocation;
		using SrcPoint		= ShaderTrace::SourcePoint;
		using ExprInfos_t	= ShaderTrace::ExprInfos_t;
		using VarNames_t	= ShaderTrace::VarNames_t;

		struct StackFrame
		{
			TIntermNode*		node			= nullptr;
			SrcLoc				loc;
		};

		struct VariableInfo
		{
			VariableID				id;
			std::string				name;
			std::vector<TSourceLoc>	locations;
		};

		struct FnCallLocation
		{
			std::string		fnName;
			TSourceLoc		loc;

			bool operator == (const FnCallLocation &) const;
		};

		struct FnCallLocationHash {
			size_t operator () (const FnCallLocation &) const;
		};

		struct FieldInfo
		{
			int		baseId		= 0;
			int		fieldIndex	= 0;

			bool operator == (const FieldInfo &) const;
		};

		struct FieldInfoHash {
			size_t operator () (const FieldInfo &) const;
		};

		using SymbolLocations_t		= std::unordered_map< int, std::vector<TIntermSymbol *> >;
		using RequiredFunctions_t	= std::unordered_set< TString >;
		using CachedSymbols_t		= std::unordered_map< TString, TIntermSymbol *>;
		using VariableInfoMap_t		= std::unordered_map< int, VariableInfo >;
		using FnCallMap_t			= std::unordered_map< FnCallLocation, VariableInfo, FnCallLocationHash >;
		using StructFieldMap_t		= std::unordered_map< FieldInfo, VariableInfo, FieldInfoHash >;
		using CallStack_t			= std::vector< StackFrame >;
		using FileMap_t				= std::unordered_map< std::string, uint32_t >;
		using StartTimeStack_t		= std::vector< TIntermSymbol* >;
		using StartTimeNodes_t		= std::vector< TIntermSymbol* >;

		static constexpr int	InvalidSymbolID = -1;


	public:
		CallStack_t				_callStack;
		StartTimeStack_t		_startTimeStack;
		StartTimeNodes_t		_uniqueStartTimes;

		RequiredFunctions_t		_requiredFunctions;
		CachedSymbols_t			_cachedSymbols;

		ExprInfos_t &			_exprLocations;
		VariableInfoMap_t		_varInfos;
		FnCallMap_t				_fnCallMap;
		StructFieldMap_t		_fieldMap;

		int						_maxSymbolId				= 0;
		bool					_startedUserDefinedSymbols	= false;

		bool					_shaderSubgroupClock		= false;
		bool					_shaderDeviceClock			= false;

		TIntermSymbol *			_dbgStorage					= nullptr;

		FileMap_t const&		_includedFilesMap;

		const EShLanguage		_shLang;


	public:
		DebugInfo (EShLanguage shLang, OUT ExprInfos_t &exprLoc, const FileMap_t &includedFiles, bool shaderSubgroupClock, bool shaderDeviceClock) :
			_exprLocations{ exprLoc },
			_shaderSubgroupClock{ shaderSubgroupClock },
			_shaderDeviceClock{ shaderDeviceClock },
			_includedFilesMap{ includedFiles },
			_shLang{ shLang }
		{}

		void Enter (TIntermNode* node);
		void Leave (TIntermNode* node);

		bool PushStartTime (TIntermSymbol* node);
		bool PopStartTime ();
		TIntermSymbol*  GetStartTime ();
		TIntermSymbol*	CreateStartTimeSymbolNode ();

		CallStack_t const&	GetCallStack ()			const				{ return _callStack; }
		TIntermAggregate*	GetCurrentFunction ();

		uint32_t		GetSourceLocation (TIntermNode* node, const TSourceLoc &curr);
		uint32_t		GetCustomSourceLocation (TIntermNode* node, const TSourceLoc &curr);
		uint32_t		GetCustomSourceLocation2 (TIntermNode* node, const TSourceLoc &begin, const TSourceLoc &end);

		SrcLoc const&	GetCurrentLocation () const						{ return _callStack.back().loc; }
		void			AddLocation (const TSourceLoc &loc);
		void			AddLocation (const SrcLoc &src);

		void						RequestFunc (const TString &fname)	{ _requiredFunctions.insert( fname ); }
		RequiredFunctions_t const&	GetRequiredFunctions ()		const	{ return _requiredFunctions; }

		void			AddSymbol (TIntermSymbol* node, bool isUserDefined);
		int				GetUniqueSymbolID ();

		void CacheSymbolNode (TIntermSymbol* node, bool isUserDefined)
		{
			_cachedSymbols.insert({ node->getName(), node });
			AddSymbol( node, isUserDefined );
		}

		TIntermSymbol*  GetCachedSymbolNode (const TString &name)
		{
			auto	iter = _cachedSymbols.find( name );
			return	iter != _cachedSymbols.end() ? iter->second : nullptr;
		}


		TIntermSymbol*	GetDebugStorage ()						const	{ CHECK( _dbgStorage );  return _dbgStorage; }
		bool			SetDebugStorage (TIntermSymbol* symb);
		TIntermBinary*  GetDebugStorageField (const char* name)	const;
	

		bool			UpdateSymbolIDs ();
		bool			PostProcess (OUT VarNames_t &);

		EShLanguage		GetShaderType ()	const						{ return _shLang; }


	private:
		void		_GetVariableID (TIntermNode* node, OUT VariableID &id, OUT uint32_t &swizzle);
		uint32_t	_GetSourceId (const TSourceLoc &) const;
	};


	struct FunctionScope
	{
		DebugInfo &						_info;
		TIntermNode *					_node;
		ShaderTrace::SourceLocation		_location;
		bool							_hasLocation = false;

		FunctionScope (TIntermNode* node, DebugInfo &info) : _info{info}, _node{node}
		{
			_info.Enter( _node );
		}

		~FunctionScope ()
		{
			_info.Leave( _node );
			
			// propagate source location to root
			if ( _hasLocation )
				_info.AddLocation( _location );
		}

		void SetLoc (const ShaderTrace::SourceLocation &loc)
		{
			_location		= loc;
			_hasLocation	= true;
		}
	};
//-----------------------------------------------------------------------------



bool RecursiveProcessNode (TIntermNode* node, DebugInfo &dbgInfo);
void CreateShaderDebugStorage (uint32_t setIndex, DebugInfo &dbgInfo, OUT uint64_t &posOffset, OUT uint64_t &dataOffset);
void CreateShaderBuiltinSymbols (TIntermNode* root, DebugInfo &dbgInfo);
bool CreateDebugTraceFunctions (TIntermNode* root, uint32_t initialPosition, DebugInfo &dbgInfo);
TIntermAggregate*  RecordShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo);

/*
=================================================
	CombineHash
=================================================
*/
inline size_t  CombineHash (size_t lhs, size_t rhs)
{
	const size_t	mask	= (sizeof(lhs)*8 - 1);
	size_t			shift	= 4;

	shift &= mask;
	return lhs ^ ((rhs << shift) | (rhs >> ( ~(shift-1) & mask )));
}

/*
=================================================
	Enter
=================================================
*/
void  DebugInfo::Enter (TIntermNode* node)
{
	CHECK( node );

	StackFrame	frame;
	frame.node			= node;
	frame.loc.sourceId	= _GetSourceId( node->getLoc() );
	frame.loc.begin		= SrcPoint{ uint32_t(node->getLoc().line), uint32_t(node->getLoc().column) };
	frame.loc.end		= frame.loc.begin;

	_callStack.push_back( frame );
}

/*
=================================================
	Leave
=================================================
*/
void  DebugInfo::Leave (TIntermNode* node)
{
	CHECK( _callStack.back().node == node );
	_callStack.pop_back();
}

/*
=================================================
	PushStartTime
=================================================
*/
bool  DebugInfo::PushStartTime (TIntermSymbol* node)
{
	CHECK_ERR( node );

	_startTimeStack.push_back( node );
	return true;
}

/*
=================================================
	PopStartTime
=================================================
*/
bool  DebugInfo::PopStartTime ()
{
	CHECK_ERR( _startTimeStack.size() );

	_startTimeStack.pop_back();
	return true;
}

/*
=================================================
	GetStartTime
=================================================
*/
TIntermSymbol*  DebugInfo::GetStartTime ()
{
	CHECK_ERR( _startTimeStack.size() );

	return _startTimeStack.back();
}

/*
=================================================
	CreateStartTimeSymbolNode
=================================================
*/
TIntermSymbol*	DebugInfo::CreateStartTimeSymbolNode ()
{
	TPublicType			uint_type;	uint_type.init({});
	uint_type.basicType				= TBasicType::EbtUint;
	uint_type.vectorSize			= 4;
	uint_type.qualifier.storage		= TStorageQualifier::EvqTemporary;
	uint_type.qualifier.precision	= TPrecisionQualifier::EpqHigh;

	TIntermSymbol*	node = new TIntermSymbol{ InvalidSymbolID, TString{"dbg_StartTime_"} + std::to_string(_uniqueStartTimes.size()).c_str(), TType{uint_type} };

	_uniqueStartTimes.push_back( node );

	return node;
}

/*
=================================================
	GetCurrentFunction
=================================================
*/
TIntermAggregate*  DebugInfo::GetCurrentFunction ()
{
	for (auto iter = _callStack.rbegin(), end = _callStack.rend(); iter != end; ++iter)
	{
		TIntermAggregate*	aggr = iter->node->getAsAggregate();

		if ( aggr and aggr->getOp() == TOperator::EOpFunction )
			return aggr;
	}
	return nullptr;
}

/*
=================================================
	GetSourceLocation
=================================================
*/
uint32_t  DebugInfo::GetSourceLocation (TIntermNode* node, const TSourceLoc &curr)
{
	VariableID	id;
	uint32_t	swizzle;
	_GetVariableID( node, OUT id, OUT swizzle );

	SrcPoint	point	{ uint32_t(curr.line), uint32_t(curr.column) };
	SrcLoc		range	= _callStack.back().loc;
	uint32_t	src_id	= _GetSourceId( curr );

	ASSERT( range.sourceId != ~0u );
	ASSERT( range.sourceId == src_id );
	
	range.begin.value = std::min( range.begin.value, point.value );
	range.end.value   = std::max( range.end.value,   point.value );

	_exprLocations.push_back({ id, swizzle, range, point, {} });
	return uint32_t(_exprLocations.size()-1);
}

/*
=================================================
	GetCustomSourceLocation
=================================================
*/
uint32_t  DebugInfo::GetCustomSourceLocation (TIntermNode* node, const TSourceLoc &curr)
{
	VariableID	id;
	uint32_t	swizzle;
	_GetVariableID( node, OUT id, OUT swizzle );

	SrcLoc	range{ 0, uint32_t(curr.line), uint32_t(curr.column) };
	range.sourceId = _GetSourceId( curr );

	_exprLocations.push_back({ id, swizzle, range, range.begin, {} });
	return uint32_t(_exprLocations.size()-1);
}

/*
=================================================
	GetCustomSourceLocation2
=================================================
*/
uint32_t  DebugInfo::GetCustomSourceLocation2 (TIntermNode* node, const TSourceLoc &begin, const TSourceLoc &end)
{
	VariableID	id;
	uint32_t	swizzle;
	_GetVariableID( node, OUT id, OUT swizzle );

	SrcLoc	range { 0, uint32_t(begin.line), uint32_t(begin.column) };
	SrcLoc	range2{ 0, uint32_t(end.line),   uint32_t(end.column)   };
	range.sourceId  = _GetSourceId( begin );
	range2.sourceId = _GetSourceId( end );
	
	ASSERT( range.sourceId == range2.sourceId );

	range.begin.value = std::min( range.begin.value, range2.begin.value );
	range.end.value   = std::max( range.end.value,   range2.end.value );

	_exprLocations.push_back({ id, swizzle, range, range.begin, {} });
	return uint32_t(_exprLocations.size()-1);
}

/*
=================================================
	AddLocation
=================================================
*/
void  DebugInfo::AddLocation (const TSourceLoc &loc)
{
	return AddLocation({ _GetSourceId( loc ), uint32_t(loc.line), uint32_t(loc.column) });
}

void  DebugInfo::AddLocation (const SrcLoc &src)
{
	if ( _callStack.empty() )
		return;

	auto&	dst = _callStack.back().loc;

	if ( dst.sourceId == 0 and dst.begin.value == 0 and dst.end.value == 0 )
	{
		dst = src;
		return;
	}

	CHECK( src.sourceId == dst.sourceId );
	dst.begin.value = std::min( dst.begin.value, src.begin.value );
	dst.end.value	= std::max( dst.end.value,	 src.end.value );
}

/*
=================================================
	SetDebugStorage
=================================================
*/
bool  DebugInfo::SetDebugStorage (TIntermSymbol* symb)
{
	if ( _dbgStorage )
		return true;

	CHECK_ERR( symb and symb->getType().isStruct() );

	_dbgStorage = symb;
	return true;
}

/*
=================================================
	GetDebugStorageField
=================================================
*/
TIntermBinary*  DebugInfo::GetDebugStorageField (const char* name) const
{
	CHECK_ERR( _dbgStorage );
		
	TPublicType		index_type;		index_type.init({});
	index_type.basicType			= TBasicType::EbtInt;
	index_type.qualifier.storage	= TStorageQualifier::EvqConst;

	for (auto& field : *_dbgStorage->getType().getStruct())
	{
		if ( field.type->getFieldName() == name )
		{
			const auto				index			= std::distance( _dbgStorage->getType().getStruct()->data(), &field );
			TConstUnionArray		index_Value(1);	index_Value[0].setIConst( int(index) );
			TIntermConstantUnion*	field_index		= new TIntermConstantUnion{ index_Value, TType{index_type} };
			TIntermBinary*			field_access	= new TIntermBinary{ TOperator::EOpIndexDirectStruct };
			field_access->setType( *field.type );
			field_access->setLeft( _dbgStorage );
			field_access->setRight( field_index );
			return field_access;
		}
	}
	return nullptr;
}
	
/*
=================================================
	AddSymbol
=================================================
*/
void  DebugInfo::AddSymbol (TIntermSymbol* node, bool isUserDefined)
{
	ASSERT( node );
	ASSERT( isUserDefined or not _startedUserDefinedSymbols );

	_maxSymbolId = Max( _maxSymbolId, node->getId() );

	// register symbol
	VariableID	id;
	uint32_t	sw;
	_GetVariableID( node, OUT id, OUT sw );
}

/*
=================================================
	GetUniqueSymbolID
=================================================
*/
int  DebugInfo::GetUniqueSymbolID ()
{
	_startedUserDefinedSymbols = true;
	return ++_maxSymbolId;
}

/*
=================================================
	TSourceLoc::operator ==
=================================================
*/
inline bool  operator == (const TSourceLoc &lhs, const TSourceLoc &rhs)
{
	if ( lhs.name != rhs.name )
	{
		if ( lhs.name == nullptr  or
			 rhs.name == nullptr  or
			*lhs.name != *rhs.name )
			return false;
	}

	return	lhs.string	== rhs.string	and
			lhs.line	== rhs.line		and
			lhs.column	== rhs.column;
}

inline bool  operator != (const TSourceLoc &lhs, const TSourceLoc &rhs)
{
	return not (lhs == rhs);
}

inline bool  operator < (const TSourceLoc &lhs, const TSourceLoc &rhs)
{
	if ( lhs.name != rhs.name )
	{
		if ( lhs.name == nullptr  or
			 rhs.name == nullptr )
			return false;

		if ( *lhs.name != *rhs.name )
			return *lhs.name < *rhs.name;
	}

	return	lhs.string	!= rhs.string	? lhs.string < rhs.string	:
			lhs.line	!= rhs.line		? lhs.line	 < rhs.line		:
										  lhs.column < rhs.column;
}

/*
=================================================
	IsBuiltinFunction
=================================================
*/
inline bool  IsBuiltinFunction (TOperator op)
{
	return	(op > TOperator::EOpRadians			and op < TOperator::EOpKill)	or
			(op > TOperator::EOpArrayLength		and op < TOperator::EOpClip);
}

/*
=================================================
	IsDebugFunction
=================================================
*/
inline bool  IsDebugFunction (TIntermOperator* node)
{
	auto*	aggr = node->getAsAggregate();

	if ( not (aggr and aggr->getOp() == TOperator::EOpFunctionCall) )
		return false;

	return aggr->getName().rfind( "dbg_EnableProfiling", 0 ) == 0;
}

/*
=================================================
	GetFunctionName
=================================================
*/
std::string  GetFunctionName (TIntermOperator *op)
{
	if ( TIntermAggregate* aggr = op->getAsAggregate(); aggr and aggr->getOp() == TOperator::EOpFunctionCall )
	{
		auto	pos = aggr->getName().find( '(' );
		return pos != TString::npos ? aggr->getName().substr( 0, pos ).c_str() : aggr->getName().c_str();
	}

	switch ( op->getOp() )
	{
#	if 0
		case TOperator::EOpConvInt8ToBool :
		case TOperator::EOpConvUint8ToBool :
		case TOperator::EOpConvInt16ToBool :
		case TOperator::EOpConvUint16ToBool :
		case TOperator::EOpConvIntToBool :
		case TOperator::EOpConvUintToBool :
		case TOperator::EOpConvInt64ToBool :
		case TOperator::EOpConvUint64ToBool :
		case TOperator::EOpConvFloat16ToBool :
		case TOperator::EOpConvFloatToBool :
		case TOperator::EOpConvDoubleToBool :  return suffix.size() ? "b" + suffix : "bool";

		case TOperator::EOpConvBoolToInt :
		case TOperator::EOpConvInt8ToInt :
		case TOperator::EOpConvUint8ToInt :
		case TOperator::EOpConvInt16ToInt :
		case TOperator::EOpConvUint16ToInt :
		case TOperator::EOpConvUintToInt :
		case TOperator::EOpConvInt64ToInt :
		case TOperator::EOpConvUint64ToInt :
		case TOperator::EOpConvFloat16ToInt :
		case TOperator::EOpConvFloatToInt :
		case TOperator::EOpConvDoubleToInt : return suffix.size() ? "i" + suffix : "int";
			
		case TOperator::EOpConvBoolToInt8 :
		case TOperator::EOpConvUint8ToInt8 :
		case TOperator::EOpConvInt16ToInt8 :
		case TOperator::EOpConvUint16ToInt8 :
		case TOperator::EOpConvIntToInt8 :
		case TOperator::EOpConvUintToInt8 :
		case TOperator::EOpConvInt64ToInt8 :
		case TOperator::EOpConvUint64ToInt8 :
		case TOperator::EOpConvFloat16ToInt8 :
		case TOperator::EOpConvFloatToInt8 :
		case TOperator::EOpConvDoubleToInt8 : return suffix.size() ? "i8" + suffix : "int8_t";

		case TOperator::EOpConvBoolToUint8 :
		case TOperator::EOpConvInt8ToUint8 :
		case TOperator::EOpConvInt16ToUint8 :
		case TOperator::EOpConvUint16ToUint8 :
		case TOperator::EOpConvIntToUint8 :
		case TOperator::EOpConvUintToUint8 :
		case TOperator::EOpConvInt64ToUint8 :
		case TOperator::EOpConvUint64ToUint8 :
		case TOperator::EOpConvFloat16ToUint8 :
		case TOperator::EOpConvFloatToUint8 :
		case TOperator::EOpConvDoubleToUint8 : return suffix.size() ? "u8" + suffix : "uint8_t";
			
		case TOperator::EOpConvBoolToInt16 :
		case TOperator::EOpConvInt8ToInt16 :
		case TOperator::EOpConvUint8ToInt16 :
		case TOperator::EOpConvUint16ToInt16 :
		case TOperator::EOpConvIntToInt16 :
		case TOperator::EOpConvUintToInt16 :
		case TOperator::EOpConvInt64ToInt16 :
		case TOperator::EOpConvUint64ToInt16 :
		case TOperator::EOpConvFloat16ToInt16 :
		case TOperator::EOpConvFloatToInt16 :
		case TOperator::EOpConvDoubleToInt16 : return suffix.size() ? "i16" + suffix : "uint16_t";

		case TOperator::EOpConvBoolToUint16 :
		case TOperator::EOpConvInt8ToUint16 :
		case TOperator::EOpConvUint8ToUint16 :
		case TOperator::EOpConvInt16ToUint16 :
		case TOperator::EOpConvIntToUint16 :
		case TOperator::EOpConvUintToUint16 :
		case TOperator::EOpConvInt64ToUint16 :
		case TOperator::EOpConvUint64ToUint16 :
		case TOperator::EOpConvFloat16ToUint16 :
		case TOperator::EOpConvFloatToUint16 :
		case TOperator::EOpConvDoubleToUint16 : return suffix.size() ? "u16" + suffix : "uint16_t";
			
		case TOperator::EOpConvBoolToUint :
		case TOperator::EOpConvInt8ToUint :
		case TOperator::EOpConvUint8ToUint :
		case TOperator::EOpConvInt16ToUint :
		case TOperator::EOpConvUint16ToUint :
		case TOperator::EOpConvIntToUint :
		case TOperator::EOpConvInt64ToUint :
		case TOperator::EOpConvUint64ToUint :
		case TOperator::EOpConvFloat16ToUint :
		case TOperator::EOpConvFloatToUint :
		case TOperator::EOpConvDoubleToUint : return suffix.size() ? "u" + suffix : "uint";
			
		case TOperator::EOpConvBoolToInt64 :
		case TOperator::EOpConvInt8ToInt64 :
		case TOperator::EOpConvUint8ToInt64 :
		case TOperator::EOpConvInt16ToInt64 :
		case TOperator::EOpConvUint16ToInt64 :
		case TOperator::EOpConvIntToInt64 :
		case TOperator::EOpConvUintToInt64 :
		case TOperator::EOpConvUint64ToInt64 :
		case TOperator::EOpConvFloat16ToInt64 :
		case TOperator::EOpConvFloatToInt64 :
		case TOperator::EOpConvDoubleToInt64 : return suffix.size() ? "i64" + suffix : "int64_t";

		case TOperator::EOpConvBoolToUint64 :
		case TOperator::EOpConvInt8ToUint64 :
		case TOperator::EOpConvUint8ToUint64 :
		case TOperator::EOpConvInt16ToUint64 :
		case TOperator::EOpConvUint16ToUint64 :
		case TOperator::EOpConvIntToUint64 :
		case TOperator::EOpConvUintToUint64 :
		case TOperator::EOpConvInt64ToUint64 :
		case TOperator::EOpConvFloat16ToUint64 :
		case TOperator::EOpConvFloatToUint64 :
		case TOperator::EOpConvDoubleToUint64 : return suffix.size() ? "u64" + suffix : "uint64_t";
			
		case TOperator::EOpConvBoolToFloat16 :
		case TOperator::EOpConvInt8ToFloat16 :
		case TOperator::EOpConvUint8ToFloat16 :
		case TOperator::EOpConvInt16ToFloat16 :
		case TOperator::EOpConvUint16ToFloat16 :
		case TOperator::EOpConvIntToFloat16 :
		case TOperator::EOpConvUintToFloat16 :
		case TOperator::EOpConvInt64ToFloat16 :
		case TOperator::EOpConvUint64ToFloat16 :
		case TOperator::EOpConvFloatToFloat16 :
		case TOperator::EOpConvDoubleToFloat16 : return suffix.size() ? "f16" + suffix : "float16_t";
			
		case TOperator::EOpConvBoolToFloat :
		case TOperator::EOpConvInt8ToFloat :
		case TOperator::EOpConvUint8ToFloat :
		case TOperator::EOpConvInt16ToFloat :
		case TOperator::EOpConvUint16ToFloat :
		case TOperator::EOpConvIntToFloat :
		case TOperator::EOpConvUintToFloat :
		case TOperator::EOpConvInt64ToFloat :
		case TOperator::EOpConvUint64ToFloat :
		case TOperator::EOpConvFloat16ToFloat :
		case TOperator::EOpConvDoubleToFloat : return suffix.size() ? suffix : "float";
			
		case TOperator::EOpConvBoolToDouble :
		case TOperator::EOpConvInt8ToDouble :
		case TOperator::EOpConvUint8ToDouble :
		case TOperator::EOpConvInt16ToDouble :
		case TOperator::EOpConvUint16ToDouble :
		case TOperator::EOpConvIntToDouble :
		case TOperator::EOpConvUintToDouble :
		case TOperator::EOpConvInt64ToDouble :
		case TOperator::EOpConvUint64ToDouble :
		case TOperator::EOpConvFloat16ToDouble :
		case TOperator::EOpConvFloatToDouble : return suffix.size() ? "d" + suffix : "double";
#	endif
#	if 0
		case TOperator::EOpAssign : return "=";
		case TOperator::EOpAddAssign : return "+=";
		case TOperator::EOpSubAssign : return "-=";
		case TOperator::EOpMulAssign : return "*=";
		case TOperator::EOpVectorTimesMatrixAssign : return "*=";
		case TOperator::EOpVectorTimesScalarAssign : return "*=";
		case TOperator::EOpMatrixTimesScalarAssign : return "*=";
		case TOperator::EOpMatrixTimesMatrixAssign : return "*=";
		case TOperator::EOpDivAssign : return "/=";
		case TOperator::EOpModAssign : return "%=";
		case TOperator::EOpAndAssign : return "&=";
		case TOperator::EOpInclusiveOrAssign : return "|=";
		case TOperator::EOpExclusiveOrAssign : return "^=";
		case TOperator::EOpLeftShiftAssign : return "<<=";
		case TOperator::EOpRightShiftAssign : return ">>=";
		case TOperator::EOpNegative : return "-";
		case TOperator::EOpLogicalNot : return "!";
		case TOperator::EOpVectorLogicalNot : return "not";
		case TOperator::EOpBitwiseNot : return "~";
		case TOperator::EOpPostIncrement : return "++";
		case TOperator::EOpPostDecrement : return "--";
		case TOperator::EOpPreIncrement : return "++";
		case TOperator::EOpPreDecrement : return "--";
		case TOperator::EOpAdd : return "+";
		case TOperator::EOpSub : return "-";
		case TOperator::EOpMul : return "*";
		case TOperator::EOpDiv : return "/";
		case TOperator::EOpMod : return "%";
		case TOperator::EOpRightShift : return ">>";
		case TOperator::EOpLeftShift : return "<<";
		case TOperator::EOpAnd : return "&";
		case TOperator::EOpInclusiveOr : return "|";
		case TOperator::EOpExclusiveOr : return "^";
		case TOperator::EOpEqual : return "==";
		case TOperator::EOpNotEqual : return "!=";
		case TOperator::EOpVectorTimesScalar : return "*";
		case TOperator::EOpVectorTimesMatrix : return "*";
		case TOperator::EOpMatrixTimesVector : return "*";
		case TOperator::EOpMatrixTimesScalar : return "*";
		case TOperator::EOpLogicalOr : return "||";
		case TOperator::EOpLogicalXor : return "^^";
		case TOperator::EOpLogicalAnd : return "&&";
#	endif
		case TOperator::EOpVectorEqual : return "equal";
		case TOperator::EOpVectorNotEqual : return "notEqual";
		case TOperator::EOpLessThan : return "lessThan";
		case TOperator::EOpGreaterThan : return "greaterThan";
		case TOperator::EOpLessThanEqual : return "lessThanEqual";
		case TOperator::EOpGreaterThanEqual : return "greaterThanEqual";
		case TOperator::EOpRadians : return "radians";
		case TOperator::EOpDegrees : return "degress";
		case TOperator::EOpSin : return "sin";
		case TOperator::EOpCos : return "cos";
		case TOperator::EOpTan : return "tan";
		case TOperator::EOpAsin : return "asin";
		case TOperator::EOpAcos : return "acos";
		case TOperator::EOpAtan : return "atan";
		case TOperator::EOpSinh : return "sinh";
		case TOperator::EOpCosh : return "cosh";
		case TOperator::EOpTanh : return "tanh";
		case TOperator::EOpAsinh : return "asinh";
		case TOperator::EOpAcosh : return "acosh";
		case TOperator::EOpAtanh : return "atanh";
		case TOperator::EOpPow : return "pow";
		case TOperator::EOpExp : return "exp";
		case TOperator::EOpLog : return "log";
		case TOperator::EOpExp2 : return "exp2";
		case TOperator::EOpLog2 : return "log2";
		case TOperator::EOpSqrt : return "sqrt";
		case TOperator::EOpInverseSqrt : return "inversesqrt";
		case TOperator::EOpAbs : return "abs";
		case TOperator::EOpSign : return "sign";
		case TOperator::EOpFloor : return "floor";
		case TOperator::EOpTrunc : return "trunc";
		case TOperator::EOpRound : return "round";
		case TOperator::EOpRoundEven : return "roundEven";
		case TOperator::EOpCeil : return "ceil";
		case TOperator::EOpFract : return "fract";
		case TOperator::EOpModf : return "modf";
		case TOperator::EOpMin : return "min";
		case TOperator::EOpMax : return "max";
		case TOperator::EOpClamp : return "clamp";
		case TOperator::EOpMix : return "mix";
		case TOperator::EOpStep : return "step";
		case TOperator::EOpSmoothStep : return "smoothstep";
		case TOperator::EOpIsNan : return "isnan";
		case TOperator::EOpIsInf : return "isinf";
		case TOperator::EOpFma : return "fma";
		case TOperator::EOpFrexp : return "frexp";
		case TOperator::EOpLdexp : return "ldexp";
		case TOperator::EOpFloatBitsToInt : return "floatBitsToInt";
		case TOperator::EOpFloatBitsToUint : return "floatBitsToUint";
		case TOperator::EOpIntBitsToFloat : return "intBitsToFloat";
		case TOperator::EOpUintBitsToFloat : return "uintBitsToFloat";
		case TOperator::EOpDoubleBitsToInt64 : return "doubleBitsToInt64";
		case TOperator::EOpDoubleBitsToUint64 : return "doubleBitsToUint64";
		case TOperator::EOpInt64BitsToDouble : return "int64BitsToDouble";
		case TOperator::EOpUint64BitsToDouble : return "uint64BitsToDouble";
		case TOperator::EOpFloat16BitsToInt16 : return "float16BitsToInt16";
		case TOperator::EOpFloat16BitsToUint16 : return "float16BitsToUint16";
		case TOperator::EOpInt16BitsToFloat16 : return "int16BitsToFloat16";
		case TOperator::EOpUint16BitsToFloat16 : return "uint16BitsToFloat16";
		case TOperator::EOpPackSnorm2x16 : return "packSnorm2x16";
		case TOperator::EOpUnpackSnorm2x16 : return "unpackSnorm2x16";
		case TOperator::EOpPackUnorm2x16 : return "packUnorm2x16";
		case TOperator::EOpUnpackUnorm2x16 : return "unpackUnorm2x16";
		case TOperator::EOpPackSnorm4x8 : return "packSnorm4x8";
		case TOperator::EOpUnpackSnorm4x8 : return "unpackSnorm4x8";
		case TOperator::EOpPackUnorm4x8 : return "packUnorm4x8";
		case TOperator::EOpUnpackUnorm4x8 : return "unpackUnorm4x8";
		case TOperator::EOpPackHalf2x16 : return "packHalf2x16";
		case TOperator::EOpUnpackHalf2x16 : return "unpackHalf2x16";
		case TOperator::EOpPackDouble2x32 : return "packDouble2x32";
		case TOperator::EOpUnpackDouble2x32 : return "unpackDouble2x32";
		case TOperator::EOpPackInt2x32 : return "packInt2x32";
		case TOperator::EOpUnpackInt2x32 : return "unpackInt2x32";
		case TOperator::EOpPackUint2x32 : return "packUint2x32";
		case TOperator::EOpUnpackUint2x32 : return "unpackUint2x32";
		case TOperator::EOpPackFloat2x16 : return "packFloat2x16";
		case TOperator::EOpUnpackFloat2x16 : return "unpackFloat2x16";
		case TOperator::EOpPackInt2x16 : return "packInt2x16";
		case TOperator::EOpUnpackInt2x16 : return "unpackInt2x16";
		case TOperator::EOpPackUint2x16 : return "packUint2x16";
		case TOperator::EOpUnpackUint2x16 : return "unpackUint2x16";
		case TOperator::EOpPackInt4x16 : return "packInt4x16";
		case TOperator::EOpUnpackInt4x16 : return "unpackInt4x16";
		case TOperator::EOpPackUint4x16 : return "packUint4x16";
		case TOperator::EOpUnpackUint4x16 : return "unpackUint4x16";
		case TOperator::EOpPack16 : return "pack16";
		case TOperator::EOpPack32 : return "pack32";
		case TOperator::EOpPack64 : return "pack64";
		case TOperator::EOpUnpack32 : return "unpack32";
		case TOperator::EOpUnpack16 : return "unpack16";
		case TOperator::EOpUnpack8 : return "unpack8";
		case TOperator::EOpLength : return "length";
		case TOperator::EOpDistance : return "distance";
		case TOperator::EOpDot : return "dot";
		case TOperator::EOpCross : return "cross";
		case TOperator::EOpNormalize : return "normalize";
		case TOperator::EOpFaceForward : return "faceForward";
		case TOperator::EOpReflect : return "reflect";
		case TOperator::EOpRefract : return "refract";
		case TOperator::EOpMin3 : return "min3";
		case TOperator::EOpMax3 : return "max3";
		case TOperator::EOpMid3 : return "mid3";
		case TOperator::EOpDPdx : return "dFdx";
		case TOperator::EOpDPdy : return "dFdy";
		case TOperator::EOpFwidth : return "fwidth";
		case TOperator::EOpDPdxFine : return "dFdxFine";
		case TOperator::EOpDPdyFine : return "dFdyFine";
		case TOperator::EOpFwidthFine : return "fwidthFine";
		case TOperator::EOpDPdxCoarse : return "dFdxCoarse";
		case TOperator::EOpDPdyCoarse : return "dFdyCoarse";
		case TOperator::EOpFwidthCoarse : return "fwidthCoarse";
		case TOperator::EOpInterpolateAtCentroid : return "interpolateAtCentroid";
		case TOperator::EOpInterpolateAtSample : return "interpolateAtSample";
		case TOperator::EOpInterpolateAtOffset : return "interpolateAtOffset";
		case TOperator::EOpInterpolateAtVertex : return "interpolateAtVertexAMD";
		case TOperator::EOpMatrixTimesMatrix : return "matrixCompMult";
		case TOperator::EOpOuterProduct : return "outerProduct";
		case TOperator::EOpDeterminant : return "determinant";
		case TOperator::EOpMatrixInverse : return "inverse";
		case TOperator::EOpTranspose : return "transpose";
		case TOperator::EOpEmitVertex : return "EmitVertex";
		case TOperator::EOpEndPrimitive : return "EndPrimitive";
		case TOperator::EOpEmitStreamVertex : return "EmitStreamVertex";
		case TOperator::EOpEndStreamPrimitive : return "EndStreamPrimitive";
		case TOperator::EOpBarrier : return "barrier";
		case TOperator::EOpMemoryBarrier : return "memoryBarrier";
		case TOperator::EOpMemoryBarrierAtomicCounter : return "memoryBarrierAtomicCounter";
		case TOperator::EOpMemoryBarrierBuffer : return "memoryBarrierBuffer";
		case TOperator::EOpMemoryBarrierImage : return "memoryBarrierImage";
		case TOperator::EOpMemoryBarrierShared : return "memoryBarrierShared";
		case TOperator::EOpGroupMemoryBarrier : return "groupMemoryBarrier";
		case TOperator::EOpBallot : return "ballotARB";
		case TOperator::EOpReadInvocation : return "readInvocationARB";
		case TOperator::EOpReadFirstInvocation : return "readFirstInvocationARB";
		case TOperator::EOpAnyInvocation : return "anyInvocation";
		case TOperator::EOpAllInvocations : return "allInvocations";
		case TOperator::EOpAllInvocationsEqual : return "allInvocationsEqual";
		case TOperator::EOpSubgroupBarrier : return "subgroupBarrier";
		case TOperator::EOpSubgroupMemoryBarrier : return "subgroupMemoryBarrier";
		case TOperator::EOpSubgroupMemoryBarrierBuffer : return "subgroupMemoryBarrierBuffer";
		case TOperator::EOpSubgroupMemoryBarrierImage : return "subgroupMemoryBarrierImage";
		case TOperator::EOpSubgroupMemoryBarrierShared : return "subgroupMemoryBarrierShared";
		case TOperator::EOpSubgroupElect : return "subgroupElect";
		case TOperator::EOpSubgroupAll : return "subgroupAll";
		case TOperator::EOpSubgroupAny : return "subgroupAny";
		case TOperator::EOpSubgroupAllEqual : return "subgroupAllEqual";
		case TOperator::EOpSubgroupBroadcast : return "subgroupBroadcast";
		case TOperator::EOpSubgroupBroadcastFirst : return "subgroupBroadcastFirst";
		case TOperator::EOpSubgroupBallot : return "subgroupBallot";
		case TOperator::EOpSubgroupInverseBallot : return "subgroupInverseBallot";
		case TOperator::EOpSubgroupBallotBitExtract : return "subgroupBallotBitExtract";
		case TOperator::EOpSubgroupBallotBitCount : return "subgroupBallotBitCount";
		case TOperator::EOpSubgroupBallotInclusiveBitCount : return "subgroupBallotInclusiveBitCount";
		case TOperator::EOpSubgroupBallotExclusiveBitCount : return "subgroupBallotExclusiveBitCount";
		case TOperator::EOpSubgroupBallotFindLSB : return "subgroupBallotFindLSB";
		case TOperator::EOpSubgroupBallotFindMSB : return "subgroupBallotFindMSB";
		case TOperator::EOpSubgroupShuffle : return "subgroupShuffle";
		case TOperator::EOpSubgroupShuffleXor : return "subgroupShuffleXor";
		case TOperator::EOpSubgroupShuffleUp : return "subgroupShuffleUp";
		case TOperator::EOpSubgroupShuffleDown : return "subgroupShuffleDown";
		case TOperator::EOpSubgroupAdd : return "subgroupAdd";
		case TOperator::EOpSubgroupMul : return "subgroupMul";
		case TOperator::EOpSubgroupMin : return "subgroupMin";
		case TOperator::EOpSubgroupMax : return "subgroupMax";
		case TOperator::EOpSubgroupAnd : return "subgroupAnd";
		case TOperator::EOpSubgroupOr : return "subgroupOr";
		case TOperator::EOpSubgroupXor : return "subgroupXor";
		case TOperator::EOpSubgroupInclusiveAdd : return "subgroupInclusiveAdd";
		case TOperator::EOpSubgroupInclusiveMul : return "subgroupInclusiveMul";
		case TOperator::EOpSubgroupInclusiveMin : return "subgroupInclusiveMin";
		case TOperator::EOpSubgroupInclusiveMax : return "subgroupInclusiveMax";
		case TOperator::EOpSubgroupInclusiveAnd : return "subgroupInclusiveAnd";
		case TOperator::EOpSubgroupInclusiveOr : return "subgroupInclusiveOr";
		case TOperator::EOpSubgroupInclusiveXor : return "subgroupInclusiveXor";
		case TOperator::EOpSubgroupExclusiveAdd : return "subgroupExclusiveAdd";
		case TOperator::EOpSubgroupExclusiveMul : return "subgroupExclusiveMul";
		case TOperator::EOpSubgroupExclusiveMin : return "subgroupExclusiveMin";
		case TOperator::EOpSubgroupExclusiveMax : return "subgroupExclusiveMax";
		case TOperator::EOpSubgroupExclusiveAnd : return "subgroupExclusiveAnd";
		case TOperator::EOpSubgroupExclusiveOr : return "subgroupExclusiveOr";
		case TOperator::EOpSubgroupExclusiveXor : return "subgroupExclusiveXor";
		case TOperator::EOpSubgroupClusteredAdd : return "subgroupClusteredAdd";
		case TOperator::EOpSubgroupClusteredMul : return "subgroupClusteredMul";
		case TOperator::EOpSubgroupClusteredMin : return "subgroupClusteredMin";
		case TOperator::EOpSubgroupClusteredMax : return "subgroupClusteredMax";
		case TOperator::EOpSubgroupClusteredAnd : return "subgroupClusteredAnd";
		case TOperator::EOpSubgroupClusteredOr : return "subgroupClusteredOr";
		case TOperator::EOpSubgroupClusteredXor : return "subgroupClusteredXor";
		case TOperator::EOpSubgroupQuadBroadcast : return "subgroupQuadBroadcast";
		case TOperator::EOpSubgroupQuadSwapHorizontal : return "subgroupQuadSwapHorizontal";
		case TOperator::EOpSubgroupQuadSwapVertical : return "subgroupQuadSwapVertical";
		case TOperator::EOpSubgroupQuadSwapDiagonal : return "subgroupQuadSwapDiagonal";
		case TOperator::EOpSubgroupPartition : return "subgroupPartition";
		case TOperator::EOpSubgroupPartitionedAdd : return "subgroupPartitionedAdd";
		case TOperator::EOpSubgroupPartitionedMul : return "subgroupPartitionedMul";
		case TOperator::EOpSubgroupPartitionedMin : return "subgroupPartitionedMin";
		case TOperator::EOpSubgroupPartitionedMax : return "subgroupPartitionedMax";
		case TOperator::EOpSubgroupPartitionedAnd : return "subgroupPartitionedAnd";
		case TOperator::EOpSubgroupPartitionedOr : return "subgroupPartitionedOr";
		case TOperator::EOpSubgroupPartitionedXor : return "subgroupPartitionedXor";
		case TOperator::EOpSubgroupPartitionedInclusiveAdd : return "subgroupPartitionedInclusiveAdd";
		case TOperator::EOpSubgroupPartitionedInclusiveMul : return "subgroupPartitionedInclusiveMul";
		case TOperator::EOpSubgroupPartitionedInclusiveMin : return "subgroupPartitionedInclusiveMin";
		case TOperator::EOpSubgroupPartitionedInclusiveMax : return "subgroupPartitionedInclusiveMax";
		case TOperator::EOpSubgroupPartitionedInclusiveAnd : return "subgroupPartitionedInclusiveAnd";
		case TOperator::EOpSubgroupPartitionedInclusiveOr : return "subgroupPartitionedInclusiveOr";
		case TOperator::EOpSubgroupPartitionedInclusiveXor : return "subgroupPartitionedInclusiveXor";
		case TOperator::EOpSubgroupPartitionedExclusiveAdd : return "subgroupPartitionedExclusiveAdd";
		case TOperator::EOpSubgroupPartitionedExclusiveMul : return "subgroupPartitionedExclusiveMul";
		case TOperator::EOpSubgroupPartitionedExclusiveMin : return "subgroupPartitionedExclusiveMin";
		case TOperator::EOpSubgroupPartitionedExclusiveMax : return "subgroupPartitionedExclusiveMax";
		case TOperator::EOpSubgroupPartitionedExclusiveAnd : return "subgroupPartitionedExclusiveAnd";
		case TOperator::EOpSubgroupPartitionedExclusiveOr : return "subgroupPartitionedExclusiveOr";
		case TOperator::EOpSubgroupPartitionedExclusiveXor : return "subgroupPartitionedExclusiveXor";
		case TOperator::EOpMinInvocations : return "minInvocationsAMD";
		case TOperator::EOpMaxInvocations : return "maxInvocationsAMD";
		case TOperator::EOpAddInvocations : return "addInvocationsAMD";
		case TOperator::EOpMinInvocationsNonUniform : return "minInvocationsNonUniformAMD";
		case TOperator::EOpMaxInvocationsNonUniform : return "maxInvocationsNonUniformAMD";
		case TOperator::EOpAddInvocationsNonUniform : return "addInvocationsNonUniformAMD";
		case TOperator::EOpMinInvocationsInclusiveScan : return "minInvocationsInclusiveScanAMD";
		case TOperator::EOpMaxInvocationsInclusiveScan : return "maxInvocationsInclusiveScanAMD";
		case TOperator::EOpAddInvocationsInclusiveScan : return "addInvocationsInclusiveScanAMD";
		case TOperator::EOpMinInvocationsInclusiveScanNonUniform : return "minInvocationsInclusiveScanNonUniformAMD";
		case TOperator::EOpMaxInvocationsInclusiveScanNonUniform : return "maxInvocationsInclusiveScanNonUniformAMD";
		case TOperator::EOpAddInvocationsInclusiveScanNonUniform : return "addInvocationsInclusiveScanNonUniformAMD";
		case TOperator::EOpMinInvocationsExclusiveScan : return "minInvocationsExclusiveScanAMD";
		case TOperator::EOpMaxInvocationsExclusiveScan : return "maxInvocationsExclusiveScanAMD";
		case TOperator::EOpAddInvocationsExclusiveScan : return "addInvocationsExclusiveScanAMD";
		case TOperator::EOpMinInvocationsExclusiveScanNonUniform : return "minInvocationsExclusiveScanNonUniformAMD";
		case TOperator::EOpMaxInvocationsExclusiveScanNonUniform : return "maxInvocationsExclusiveScanNonUniformAMD";
		case TOperator::EOpAddInvocationsExclusiveScanNonUniform : return "addInvocationsExclusiveScanNonUniformAMD";
		case TOperator::EOpSwizzleInvocations : return "swizzleInvocationsAMD";
		case TOperator::EOpSwizzleInvocationsMasked : return "swizzleInvocationsMaskedAMD";
		case TOperator::EOpWriteInvocation : return "writeInvocationAMD";
		case TOperator::EOpMbcnt : return "mbcntAMD";
		case TOperator::EOpCubeFaceIndex : return "cubeFaceIndexAMD";
		case TOperator::EOpCubeFaceCoord : return "cubeFaceCoordAMD";
		case TOperator::EOpTime : return "timeAMD";
		case TOperator::EOpAtomicAdd : return "atomicAdd";
		case TOperator::EOpAtomicMin : return "atomicMin";
		case TOperator::EOpAtomicMax : return "atomicMax";
		case TOperator::EOpAtomicAnd : return "atomicAnd";
		case TOperator::EOpAtomicOr : return "atomicOr";
		case TOperator::EOpAtomicXor : return "atomicXor";
		case TOperator::EOpAtomicExchange : return "atomicExchange";
		case TOperator::EOpAtomicCompSwap : return "atomicCompSwap";
		case TOperator::EOpAtomicLoad : return "atomicLoad";
		case TOperator::EOpAtomicStore : return "atomicStore";
		case TOperator::EOpAny : return "any";
		case TOperator::EOpAll : return "all";
		case TOperator::EOpArrayLength : return ".length";
		case TOperator::EOpImageQuerySize : return "imageSize";
		case TOperator::EOpImageQuerySamples : return "imageSamples";
		case TOperator::EOpImageLoad : return "imageLoad";
		case TOperator::EOpImageStore : return "imageStore";
		case TOperator::EOpImageLoadLod : return "imageLoadLod";
		case TOperator::EOpImageStoreLod : return "imageStoreLod";
		case TOperator::EOpImageAtomicAdd : return "imageAtomicAdd";
		case TOperator::EOpImageAtomicMin : return "imageAtomicMin";
		case TOperator::EOpImageAtomicMax : return "imageAtomicMax";
		case TOperator::EOpImageAtomicAnd : return "imageAtomicAnd";
		case TOperator::EOpImageAtomicOr : return "imageAtomicOr";
		case TOperator::EOpImageAtomicXor : return "imageAtomicXor";
		case TOperator::EOpImageAtomicExchange : return "imageAtomicExchange";
		case TOperator::EOpImageAtomicCompSwap : return "imageAtomicCompSwap";
		case TOperator::EOpImageAtomicLoad : return "imageAtomicLoad";
		case TOperator::EOpImageAtomicStore : return "imageAtomicStore";
		case TOperator::EOpSubpassLoad : return "subpassLoad";
		case TOperator::EOpSubpassLoadMS : return "subpassLoadMS";
		case TOperator::EOpSparseImageLoad : return "sparseImageLoadARB";
		case TOperator::EOpSparseImageLoadLod : return "sparseImageLoadLodAMD";
		case TOperator::EOpTextureQuerySize : return "textureSize";
		case TOperator::EOpTextureQueryLod : return "textureQueryLod";
		case TOperator::EOpTextureQueryLevels : return "textureQueryLevels";
		case TOperator::EOpTextureQuerySamples : return "textureSamples";
		case TOperator::EOpTexture : return "texture";
		case TOperator::EOpTextureProj : return "textureProj";
		case TOperator::EOpTextureLod : return "textureLod";
		case TOperator::EOpTextureOffset : return "textureOffset";
		case TOperator::EOpTextureFetch : return "texelFetch";
		case TOperator::EOpTextureFetchOffset : return "texelFetchOffset";
		case TOperator::EOpTextureProjOffset : return "textureProjOffset";
		case TOperator::EOpTextureLodOffset : return "textureLodOffset";
		case TOperator::EOpTextureProjLod : return "textureProjLod";
		case TOperator::EOpTextureProjLodOffset : return "textureProjLodOffset";
		case TOperator::EOpTextureGrad : return "textureGrad";
		case TOperator::EOpTextureGradOffset : return "textureGradOffset";
		case TOperator::EOpTextureProjGrad : return "textureProjGrad";
		case TOperator::EOpTextureProjGradOffset : return "textureProjGradOffset";
		case TOperator::EOpTextureGather : return "textureGather";
		case TOperator::EOpTextureGatherOffset : return "textureGatherOffset";
		case TOperator::EOpTextureGatherOffsets : return "textureGatherOffsets";
		case TOperator::EOpTextureClamp : return "textureClampARB";
		case TOperator::EOpTextureOffsetClamp : return "textureOffsetClampARB";
		case TOperator::EOpTextureGradClamp : return "textureGradClampARB";
		case TOperator::EOpTextureGradOffsetClamp : return "textureGradOffsetClampARB";
		case TOperator::EOpTextureGatherLod : return "textureGatherLodAMD";
		case TOperator::EOpTextureGatherLodOffset : return "textureGatherLodOffsetAMD";
		case TOperator::EOpTextureGatherLodOffsets : return "textureGatherLodOffsetsAMD";
		case TOperator::EOpFragmentMaskFetch : return "fragmentMaskFetchAMD";
		case TOperator::EOpFragmentFetch : return "fragmentFetchAMD";
		case TOperator::EOpSparseTexture : return "sparseTextureARB";
		case TOperator::EOpSparseTextureLod : return "parseTextureLodARB";
		case TOperator::EOpSparseTextureOffset : return "sparseTextureOffsetARB";
		case TOperator::EOpSparseTextureFetch : return "sparseTextureFetchARB";
		case TOperator::EOpSparseTextureFetchOffset : return "sparseTextureFetchOffsetARB";
		case TOperator::EOpSparseTextureLodOffset : return "sparseTextureLodOffsetARB";
		case TOperator::EOpSparseTextureGrad : return "sparseTextureGradARB";
		case TOperator::EOpSparseTextureGradOffset : return "sparseTextureGradOffsetARB";
		case TOperator::EOpSparseTextureGather : return "sparseTextureGatherARB";
		case TOperator::EOpSparseTextureGatherOffset : return "sparseTextureGatherOffsetAARB";
		case TOperator::EOpSparseTextureGatherOffsets : return "sparseTextureGatherOffsetsARB";
		case TOperator::EOpSparseTexelsResident : return "sparseTexelsResidentARB";
		case TOperator::EOpSparseTextureClamp : return "sparseTextureClampARB";
		case TOperator::EOpSparseTextureOffsetClamp : return "sparseTextureOffsetClampARB";
		case TOperator::EOpSparseTextureGradClamp : return "sparseTextureGradClampARB";
		case TOperator::EOpSparseTextureGradOffsetClamp : return "sparseTextureGradOffsetClampARB";
		case TOperator::EOpSparseTextureGatherLod : return "sparseTextureGatherLodAMD";
		case TOperator::EOpSparseTextureGatherLodOffset : return "sparseTextureGatherLodOffsetAMD";
		case TOperator::EOpSparseTextureGatherLodOffsets : return "sparseTextureGatherLodOffsetsAMD";
		case TOperator::EOpImageSampleFootprintNV : return "textureFootprintNV";
		case TOperator::EOpImageSampleFootprintClampNV : return "textureFootprintClampNV";
		case TOperator::EOpImageSampleFootprintLodNV : return "textureFootprintLodNV";
		case TOperator::EOpImageSampleFootprintGradNV : return "textureFootprintGradNV";
		case TOperator::EOpImageSampleFootprintGradClampNV : return "textureFootprintGradClampNV";
		case TOperator::EOpAddCarry : return "uaddCarry";
		case TOperator::EOpSubBorrow : return "subBorrow";
		case TOperator::EOpUMulExtended : return "umulExtended";
		case TOperator::EOpIMulExtended : return "imulExtended";
		case TOperator::EOpBitfieldExtract : return "bitfieldExtract";
		case TOperator::EOpBitfieldInsert : return "bitfieldInsert";
		case TOperator::EOpBitFieldReverse : return "bitFieldReverse";
		case TOperator::EOpBitCount : return "bitCount";
		case TOperator::EOpFindLSB : return "findLSB";
		case TOperator::EOpFindMSB : return "findMSB";
		case TOperator::EOpTraceNV : return "traceNV";
		case TOperator::EOpReportIntersectionNV : return "reportIntersectionNV";
		case TOperator::EOpIgnoreIntersectionNV : return "ignoreIntersectionNV";
		case TOperator::EOpTerminateRayNV : return "terminateRayNV";
		case TOperator::EOpExecuteCallableNV : return "executeCallableNV";
		case TOperator::EOpWritePackedPrimitiveIndices4x8NV : return "writePackedPrimitiveIndices4x8NV";
	}
	CHECK(false);
	return "<unknown>";
}

/*
=================================================
	FnCallLocation::operator ==
=================================================
*/
inline bool  DebugInfo::FnCallLocation::operator == (const FnCallLocation &rhs) const
{
	return loc == rhs.loc and fnName == rhs.fnName;
}

/*
=================================================
	FnCallLocationHash::operator ()
=================================================
*/
inline size_t  DebugInfo::FnCallLocationHash::operator () (const FnCallLocation &value) const
{
	size_t	fn_hash		= std::hash<std::string>{}( value.fnName );
	size_t	name_hash	= value.loc.name ? std::hash<TString>{}( *value.loc.name ) : 0;

	return	CombineHash( CombineHash( fn_hash, name_hash ),
				CombineHash( CombineHash( size_t(value.loc.string) << 24, size_t(value.loc.line) << 8 ),
							 size_t(value.loc.column) << 18 ));
}

/*
=================================================
	FieldInfo::operator ==
=================================================
*/
inline bool  DebugInfo::FieldInfo::operator == (const FieldInfo &rhs) const
{
	return	baseId		== rhs.baseId	and
			fieldIndex	== rhs.fieldIndex;
}

/*
=================================================
	FieldInfoHash::operator ()
=================================================
*/
inline size_t  DebugInfo::FieldInfoHash::operator () (const FieldInfo &value) const
{
	return	size_t(value.baseId) ^
			(size_t(value.fieldIndex) << (sizeof(size_t)*8-16));
}

/*
=================================================
	GetVectorSwizzleMask
=================================================
*/
uint32_t  GetVectorSwizzleMask (TIntermBinary* binary)
{
	std::vector<uint32_t>		sw_mask;
	std::vector<TIntermBinary*>	swizzle_op;		swizzle_op.push_back( binary );

	CHECK_ERR( binary and (binary->getOp() == TOperator::EOpVectorSwizzle or binary->getOp() == TOperator::EOpIndexDirect) );

	// extract swizzle mask
	for (TIntermTyped* node = binary->getLeft();
		 node->getAsBinaryNode() and node->getAsBinaryNode()->getOp() == TOperator::EOpVectorSwizzle;)
	{
		swizzle_op.push_back( node->getAsBinaryNode() );

		node = swizzle_op.back()->getLeft();
	}

	binary = swizzle_op.back();

	const auto ProcessUnion = [&sw_mask] (TIntermConstantUnion *cu, const std::vector<uint32_t> &mask) -> bool
	{
		TConstUnionArray const&	cu_arr = cu->getConstArray();
		CHECK_ERR( cu_arr.size() == 1 and cu->getType().getBasicType() == EbtInt );
		CHECK_ERR( cu_arr[0].getType() == EbtInt and cu_arr[0].getIConst() >= 0 and cu_arr[0].getIConst() < 4 );

		if ( mask.empty() )
			sw_mask.push_back( cu_arr[0].getIConst() );
		else
			sw_mask.push_back( mask[ cu_arr[0].getIConst() ]);
		return true;
	};

	// optimize swizzle
	for (auto iter = swizzle_op.rbegin(); iter != swizzle_op.rend(); ++iter)
	{
		TIntermBinary*				bin		= (*iter);
		const std::vector<uint32_t>	mask	= sw_mask;

		sw_mask.clear();

		if ( TIntermAggregate* aggr = bin->getRight()->getAsAggregate() )
		{
			CHECK_ERR( aggr->getOp() == TOperator::EOpSequence );

			for (auto& node : aggr->getSequence())
			{
				if ( auto* cu = node->getAsConstantUnion() )
					CHECK_ERR( ProcessUnion( cu, mask ));
			}
		}
		else
		if ( auto* cu = bin->getRight()->getAsConstantUnion() )
		{
			CHECK_ERR( ProcessUnion( cu, mask ));
		}
		else
			RETURN_ERR( "not supported!" );
	}

	uint32_t	result	= 0;
	uint32_t	shift	= 0;
	for (auto& idx : sw_mask)
	{
		result |= ((idx + 1) << shift);
		shift  += 3;
	}
	return result;
}

/*
=================================================
	_GetVariableID
=================================================
*/
void  DebugInfo::_GetVariableID (TIntermNode* node, OUT VariableID &id, OUT uint32_t &swizzle)
{
	CHECK_ERR( node, void());

	id = VariableID(~0u);
	swizzle = 0;

	const VariableID	new_id = VariableID(uint32_t(_varInfos.size() + _fnCallMap.size() + _fieldMap.size()));

	if ( TIntermSymbol* symb = node->getAsSymbolNode() )
	{
		if ( symb->getId() == InvalidSymbolID )
			return;

		auto	iter = _varInfos.find( symb->getId() );
		if ( iter == _varInfos.end() )
			iter = _varInfos.insert({ symb->getId(), VariableInfo{new_id, symb->getName().c_str(), {}} }).first;

		auto&	locations = iter->second.locations;

		if ( locations.empty() or locations.back() != node->getLoc() )
			locations.push_back( node->getLoc() );

		id = iter->second.id;
		return;
	}

	if ( TIntermBinary* binary = node->getAsBinaryNode() )
	{
		// vector swizzle
		if ( binary->getOp() == TOperator::EOpVectorSwizzle or
			(binary->getOp() == TOperator::EOpIndexDirect and not binary->getLeft()->isArray() and
			(binary->getLeft()->isScalar() or binary->getLeft()->isVector())) )
		{
			swizzle = GetVectorSwizzleMask( binary );

			uint32_t	temp;
			return _GetVariableID( binary->getLeft(), OUT id, OUT temp );
		}
		else
		// matrix swizzle
		if ( binary->getOp() == TOperator::EOpIndexDirect and
			 binary->getLeft()->isMatrix() )
		{
			swizzle = GetVectorSwizzleMask( binary );

			uint32_t	temp;
			return _GetVariableID( binary->getLeft(), OUT id, OUT temp );
		}
		else
		// array element
		if ( binary->getOp() == TOperator::EOpIndexDirect and
			 binary->getLeft()->isArray() )
		{
			swizzle = 0;	// TODO

			uint32_t	temp;
			return _GetVariableID( binary->getLeft(), OUT id, OUT temp );
		}
		else
		if ( binary->getOp() == TOperator::EOpIndexIndirect )
		{}
		else
		if ( binary->getOp() == TOperator::EOpIndexDirectStruct )
		{
			return; // temp
		}
		else
			return;	// it hasn't any ID
	}

	if ( TIntermOperator* op = node->getAsOperator() )
	{
		// temporary variable returned by function
		if ( IsBuiltinFunction( op->getOp() ) or op->getOp() == TOperator::EOpFunctionCall )
		{
			auto	name = GetFunctionName( op ) + "()";
			auto	iter = _fnCallMap.find({ name, op->getLoc() });

			if ( iter == _fnCallMap.end() )
				iter = _fnCallMap.insert({ FnCallLocation{name, op->getLoc()}, VariableInfo{new_id, name, {}} }).first;
			
			auto&	locations = iter->second.locations;

			if ( locations.empty() or locations.back() != op->getLoc() )
				locations.push_back( node->getLoc() );
			
			id = iter->second.id;
			return;
		}
		return;	// it hasn't any ID
	}

	if ( node->getAsBranchNode() or node->getAsSelectionNode() )
		return;	// no ID for branches

	CHECK(false);
}

/*
=================================================
	_GetSourceId
=================================================
*/
uint32_t  DebugInfo::_GetSourceId (const TSourceLoc &loc) const
{
	if ( loc.name )
	{
		auto	iter = _includedFilesMap.find( loc.name->c_str() );
		CHECK( iter != _includedFilesMap.end() );
		CHECK( loc.string == 0 );

		return iter->second;
	}
	else
		return loc.string;
}

/*
=================================================
	UpdateSymbolIDs
=================================================
*/
bool  DebugInfo::UpdateSymbolIDs ()
{
	for (auto* symb : _uniqueStartTimes)
	{
		if ( symb->getId() == InvalidSymbolID )
		{
			symb->changeId( GetUniqueSymbolID() );
		}
	}
	return true;
}

/*
=================================================
	PostProcess
=================================================
*/
bool  DebugInfo::PostProcess (OUT VarNames_t &varNames)
{
	const auto	SearchInExpressions = [&] (VariableInfo &info)
	{
		std::sort(	info.locations.begin(), info.locations.end(),
					[] (auto& lhs, auto& rhs) { return lhs < rhs; });

		bool	is_unused = true;
		
		for (auto& expr : _exprLocations)
		{
			if ( expr.varID == info.id ) {
				is_unused = false;
				continue;
			}

			for (auto& loc : info.locations)
			{
				uint32_t	loc_line	= uint32_t(loc.line);
				uint32_t	loc_column	= uint32_t(loc.column);
				uint32_t	source_id	= _GetSourceId( loc );

				if ( source_id == 0 and loc_line == 0 and loc_column == 0 )
					continue;	// skip undefined location

				// check intersection
				if ( source_id != expr.range.sourceId )
					continue;

				if ( loc_line < expr.range.begin.Line() or loc_line > expr.range.end.Line() )
					continue;

				if ( loc_line == expr.range.begin.Line() and loc_column < expr.range.begin.Column() )
					continue;

				if ( loc_line == expr.range.end.Line() and loc_column > expr.range.end.Column() )
					continue;

				bool	exist = false;
				for (auto& v : expr.vars) {
					exist |= (v == info.id);
				}

				if ( not exist )
					expr.vars.push_back( info.id );

				is_unused = false;
			}
		}

		if ( not is_unused )
			varNames.insert_or_assign( info.id, std::move(info.name) );
	};

	for (auto& info : _varInfos) {
		SearchInExpressions( info.second );
	}
	for (auto& info : _fnCallMap) {
		SearchInExpressions( info.second );
	}
	for (auto& info : _fieldMap) {
		SearchInExpressions( info.second );
	}
	return true;
}
//-----------------------------------------------------------------------------



bool  ProcessFunctionDefinition (TIntermAggregate* node, DebugInfo &dbgInfo);
bool  ProcessFunctionDefinition2 (TIntermAggregate* node, DebugInfo &dbgInfo);
bool  RecursiveProcessSymbolNode (TIntermSymbol* node, DebugInfo &dbgInfo);
TIntermBinary*     AssignClock (TIntermSymbol* dst, const TSourceLoc &loc, DebugInfo &dbgInfo);
TIntermAggregate*  CreateAppendToTrace (TIntermTyped* exprNode, uint32_t sourceLoc, DebugInfo &dbgInfo);
TIntermAggregate*  CreateAddTimeToTrace2 (TIntermSymbol* startTimeNode, DebugInfo &dbgInfo);
TIntermAggregate*  CreateAddTimeToTrace (TIntermTyped* exprNode, TIntermSymbol* startTimeNode, DebugInfo &dbgInfo);

/*
=================================================
	RecursiveProcessNode
=================================================
*/
bool  RecursiveProcessNode (TIntermNode* node, DebugInfo &dbgInfo)
{
	if ( not node )
		return true;

	FunctionScope	fn_scope{ node, dbgInfo };
	
	if ( auto* aggr = node->getAsAggregate() )
	{
		bool	is_func = (aggr->getOp() == TOperator::EOpFunction);

		if ( is_func )
			CHECK_ERR( ProcessFunctionDefinition( aggr, dbgInfo ));

		for (auto& n : aggr->getSequence())
		{
			CHECK_ERR( RecursiveProcessNode( n, dbgInfo ));
		}

		if ( is_func )
			CHECK_ERR( ProcessFunctionDefinition2( aggr, dbgInfo ));

		if ( aggr->getOp() >= TOperator::EOpNegative	 or
			 aggr->getOp() == TOperator::EOpFunctionCall or
			 aggr->getOp() == TOperator::EOpParameters	 )
		{
			fn_scope.SetLoc( dbgInfo.GetCurrentLocation() );
		}
		return true;
	}

	if ( auto* unary = node->getAsUnaryNode() )
	{
		CHECK_ERR( RecursiveProcessNode( unary->getOperand(), dbgInfo ));
		fn_scope.SetLoc( dbgInfo.GetCurrentLocation() );
		return true;
	}

	if ( auto* binary = node->getAsBinaryNode() )
	{
		CHECK_ERR( RecursiveProcessNode( binary->getLeft(), dbgInfo ));
		CHECK_ERR( RecursiveProcessNode( binary->getRight(), dbgInfo ));
		fn_scope.SetLoc( dbgInfo.GetCurrentLocation() );
		return true;
	}

	if ( auto* op = node->getAsOperator() )
	{
		return true;
	}

	if ( auto* branch = node->getAsBranchNode() )
	{
		// record function time at the end
		if ( branch->getFlowOp() == TOperator::EOpReturn )
		{
			if ( auto* start_time_node = dbgInfo.GetStartTime() )
			{
				if ( not branch->getExpression() or
					 branch->getExpression()->getType().getBasicType() == TBasicType::EbtVoid )
				{
					branch->~TIntermBranch();
					new(branch) TIntermBranch{ TOperator::EOpReturn, CreateAddTimeToTrace2( start_time_node, dbgInfo )};
				}
				else
				{
					branch->~TIntermBranch();
					new(branch) TIntermBranch{ TOperator::EOpReturn, CreateAddTimeToTrace( branch->getExpression(), start_time_node, dbgInfo )};
				}
			}
		}

		fn_scope.SetLoc( dbgInfo.GetCurrentLocation() );
		CHECK_ERR( RecursiveProcessNode( branch->getExpression(), dbgInfo ));
		return true;
	}

	if ( auto* sw = node->getAsSwitchNode() )
	{
		CHECK_ERR( RecursiveProcessNode( sw->getCondition(), dbgInfo ));
		CHECK_ERR( RecursiveProcessNode( sw->getBody(), dbgInfo ));
		return true;
	}

	if ( auto* cunion = node->getAsConstantUnion() )
	{
		fn_scope.SetLoc( dbgInfo.GetCurrentLocation() );
		return true;
	}

	if ( auto* selection = node->getAsSelectionNode() )
	{
		CHECK_ERR( RecursiveProcessNode( selection->getCondition(), dbgInfo ));
		CHECK_ERR( RecursiveProcessNode( selection->getTrueBlock(), dbgInfo ));
		CHECK_ERR( RecursiveProcessNode( selection->getFalseBlock(), dbgInfo ));
		return true;
	}

	if ( auto* method = node->getAsMethodNode() )
	{
		return true;
	}

	if ( auto* symbol = node->getAsSymbolNode() )
	{
		fn_scope.SetLoc( dbgInfo.GetCurrentLocation() );
		CHECK_ERR( RecursiveProcessSymbolNode( symbol, dbgInfo ));
		return true;
	}

	if ( auto* typed = node->getAsTyped() )
	{
		fn_scope.SetLoc( dbgInfo.GetCurrentLocation() );
		return true;
	}

	if ( auto* loop = node->getAsLoopNode() )
	{
		CHECK_ERR( RecursiveProcessNode( loop->getBody(), dbgInfo ));
		CHECK_ERR( RecursiveProcessNode( loop->getTerminal(), dbgInfo ));
		CHECK_ERR( RecursiveProcessNode( loop->getTest(), dbgInfo ));
		return true;
	}

	return false;
}

/*
=================================================
	CreateGraphicsShaderDebugStorage
=================================================
*/
void  CreateGraphicsShaderDebugStorage (TTypeList* typeList, INOUT TPublicType &type)
{
	type.basicType = TBasicType::EbtInt;

	TType*	fragcoord_x	= new TType{type};		fragcoord_x->setFieldName( "fragCoordX" );
	type.qualifier.layoutOffset += sizeof(int32_t);

	TType*	fragcoord_y	= new TType{type};		fragcoord_y->setFieldName( "fragCoordY" );
	type.qualifier.layoutOffset += sizeof(int32_t);
	
	TType*	padding	= new TType{type};			padding->setFieldName( "padding1" );
	type.qualifier.layoutOffset += sizeof(int32_t);

	typeList->push_back({ fragcoord_x,	TSourceLoc{} });
	typeList->push_back({ fragcoord_y,	TSourceLoc{} });
	typeList->push_back({ padding,		TSourceLoc{} });
}

/*
=================================================
	CreateComputeShaderDebugStorage
=================================================
*/
void  CreateComputeShaderDebugStorage (TTypeList* typeList, INOUT TPublicType &type)
{
	type.basicType = TBasicType::EbtUint;
	
	TType*	thread_id_x	= new TType{type};		thread_id_x->setFieldName( "globalInvocationX" );
	type.qualifier.layoutOffset += sizeof(uint32_t);
	
	TType*	thread_id_y	= new TType{type};		thread_id_y->setFieldName( "globalInvocationY" );
	type.qualifier.layoutOffset += sizeof(uint32_t);
	
	TType*	thread_id_z	= new TType{type};		thread_id_z->setFieldName( "globalInvocationZ" );
	type.qualifier.layoutOffset += sizeof(uint32_t);
	
	typeList->push_back({ thread_id_x,	TSourceLoc{} });
	typeList->push_back({ thread_id_y,	TSourceLoc{} });
	typeList->push_back({ thread_id_z,	TSourceLoc{} });
}

/*
=================================================
	CreateRayTracingShaderDebugStorage
=================================================
*/
void  CreateRayTracingShaderDebugStorage (TTypeList* typeList, INOUT TPublicType &type)
{
	type.basicType = TBasicType::EbtUint;
	
	TType*	thread_id_x	= new TType{type};		thread_id_x->setFieldName( "launchID_x" );
	type.qualifier.layoutOffset += sizeof(uint32_t);
	
	TType*	thread_id_y	= new TType{type};		thread_id_y->setFieldName( "launchID_y" );
	type.qualifier.layoutOffset += sizeof(uint32_t);
	
	TType*	thread_id_z	= new TType{type};		thread_id_z->setFieldName( "launchID_z" );
	type.qualifier.layoutOffset += sizeof(uint32_t);
	
	typeList->push_back({ thread_id_x,	TSourceLoc{} });
	typeList->push_back({ thread_id_y,	TSourceLoc{} });
	typeList->push_back({ thread_id_z,	TSourceLoc{} });
}

/*
=================================================
	CreateShaderDebugStorage
=================================================
*/
void  CreateShaderDebugStorage (uint32_t setIndex, DebugInfo &dbgInfo, OUT uint64_t &posOffset, OUT uint64_t &dataOffset)
{
	// "layout(binding=x, std430) buffer dbg_ShaderTraceStorage { ... } dbg_ShaderTrace"
	
	TPublicType		uint_type;			uint_type.init({});
	uint_type.basicType					= TBasicType::EbtUint;
	uint_type.qualifier.storage			= TStorageQualifier::EvqBuffer;
	uint_type.qualifier.layoutMatrix	= TLayoutMatrix::ElmColumnMajor;
	uint_type.qualifier.layoutPacking	= TLayoutPacking::ElpStd430;
	uint_type.qualifier.precision		= TPrecisionQualifier::EpqHigh;
	uint_type.qualifier.layoutOffset	= 0;
	uint_type.qualifier.coherent		= true;
	
	TTypeList*		type_list	= new TTypeList{};
	TPublicType		temp		= uint_type;

	switch ( dbgInfo.GetShaderType() )
	{
		case EShLangVertex :
		case EShLangTessControl :
		case EShLangTessEvaluation :
		case EShLangGeometry :
		case EShLangFragment :
		case EShLangTaskNV :
		case EShLangMeshNV :			CreateGraphicsShaderDebugStorage( type_list, INOUT temp );		break;

		case EShLangCompute :			CreateComputeShaderDebugStorage( type_list, INOUT temp );		break;

		case EShLangRayGenNV :
		case EShLangIntersectNV :
		case EShLangAnyHitNV :
		case EShLangClosestHitNV :
		case EShLangMissNV :
		case EShLangCallableNV :		CreateRayTracingShaderDebugStorage( type_list, INOUT temp );	break;

		default :						CHECK(false); return;
	}

	uint_type.qualifier.layoutOffset = temp.qualifier.layoutOffset;

	TType*			position	= new TType{uint_type};		position->setFieldName( "position" );
	
	uint_type.qualifier.layoutOffset += sizeof(uint32_t);
	uint_type.qualifier.restrict	 = true;
	uint_type.qualifier.coherent	 = false;
	uint_type.arraySizes			 = new TArraySizes{};
	uint_type.arraySizes->addInnerSize();
	
	TType*			data_arr	= new TType{uint_type};		data_arr->setFieldName( "outData" );

	type_list->push_back({ position,	TSourceLoc{} });
	type_list->push_back({ data_arr,	TSourceLoc{} });

	TQualifier		block_qual;	block_qual.clear();
	block_qual.storage			= TStorageQualifier::EvqBuffer;
	block_qual.layoutMatrix		= TLayoutMatrix::ElmColumnMajor;
	block_qual.layoutPacking	= TLayoutPacking::ElpStd430;
	block_qual.layoutBinding	= 0;
	block_qual.layoutSet		= setIndex;

	TIntermSymbol*	storage_buf	= new TIntermSymbol{ 0x10000001, "dbg_ShaderTrace", TType{type_list, "dbg_ShaderTraceStorage", block_qual} };

	posOffset  = position->getQualifier().layoutOffset;
	dataOffset = data_arr->getQualifier().layoutOffset;
	
	dbgInfo.SetDebugStorage( storage_buf );
}

/*
=================================================
	CreateShaderBuiltinSymbols
=================================================
*/
void  CreateShaderBuiltinSymbols (TIntermNode* root, DebugInfo &dbgInfo)
{
	const auto	shader				= dbgInfo.GetShaderType();
	const bool	is_compute			= (shader == EShLangCompute or shader == EShLangTaskNV or shader == EShLangMeshNV);
	const bool	need_invocation_id	= (shader == EShLangGeometry or shader == EShLangTessControl);
	const bool	need_primitive_id	= (shader == EShLangFragment or shader == EShLangTessControl or shader == EShLangTessEvaluation);
	const bool	need_launch_id		= (shader == EShLangRayGenNV or shader == EShLangIntersectNV or shader == EShLangAnyHitNV or
									   shader == EShLangClosestHitNV or shader == EShLangMissNV or shader == EShLangCallableNV);
	
	TSourceLoc	loc	{};

	if ( shader == EShLangFragment and not dbgInfo.GetCachedSymbolNode( "gl_FragCoord" ))
	{
		TPublicType		vec4_type;	vec4_type.init({});
		vec4_type.basicType			= TBasicType::EbtFloat;
		vec4_type.vectorSize		= 4;
		vec4_type.qualifier.storage	= TStorageQualifier::EvqFragCoord;
		vec4_type.qualifier.builtIn	= TBuiltInVariable::EbvFragCoord;
		
		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_FragCoord", TType{vec4_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}
	
	// "Any static use of this variable in a fragment shader causes the entire shader to be evaluated per-sample rather than per-fragment."
	// so don't add 'gl_SampleID' and 'gl_SamplePosition' if it doesn't exists.
	
	if ( need_primitive_id and not dbgInfo.GetCachedSymbolNode( "gl_PrimitiveID" ))
	{
		TPublicType		int_type;	int_type.init({});
		int_type.basicType			= TBasicType::EbtInt;
		int_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		int_type.qualifier.builtIn	= TBuiltInVariable::EbvPrimitiveId;
		int_type.qualifier.flat		= true;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_PrimitiveID", TType{int_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}

	if ( is_compute and not dbgInfo.GetCachedSymbolNode( "gl_GlobalInvocationID" ))
	{
		TPublicType		uint_type;	uint_type.init({});
		uint_type.basicType			= TBasicType::EbtUint;
		uint_type.vectorSize		= 3;
		uint_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		uint_type.qualifier.builtIn	= TBuiltInVariable::EbvGlobalInvocationId;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_GlobalInvocationID", TType{uint_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}

	if ( is_compute and not dbgInfo.GetCachedSymbolNode( "gl_LocalInvocationID" ))
	{
		TPublicType		uint_type;	uint_type.init({});
		uint_type.basicType			= TBasicType::EbtUint;
		uint_type.vectorSize		= 3;
		uint_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		uint_type.qualifier.builtIn	= TBuiltInVariable::EbvLocalInvocationId;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_LocalInvocationID", TType{uint_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}

	if ( is_compute and not dbgInfo.GetCachedSymbolNode( "gl_WorkGroupID" ))
	{
		TPublicType		uint_type;	uint_type.init({});
		uint_type.basicType			= TBasicType::EbtUint;
		uint_type.vectorSize		= 3;
		uint_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		uint_type.qualifier.builtIn	= TBuiltInVariable::EbvWorkGroupId;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_WorkGroupID", TType{uint_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}
	
	if ( need_invocation_id and not dbgInfo.GetCachedSymbolNode( "gl_InvocationID" ))
	{
		TPublicType		int_type;	int_type.init({});
		int_type.basicType			= TBasicType::EbtInt;
		int_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		int_type.qualifier.builtIn	= TBuiltInVariable::EbvInvocationId;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_InvocationID", TType{int_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}
	
	if ( shader == EShLangGeometry and not dbgInfo.GetCachedSymbolNode( "gl_PrimitiveIDIn" ))
	{
		TPublicType		int_type;	int_type.init({});
		int_type.basicType			= TBasicType::EbtInt;
		int_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		int_type.qualifier.builtIn	= TBuiltInVariable::EbvPrimitiveId;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_PrimitiveIDIn", TType{int_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}
	
	if ( shader == EShLangTessEvaluation and not dbgInfo.GetCachedSymbolNode( "gl_TessCoord" ))
	{
		TPublicType		float_type;		float_type.init({});
		float_type.basicType			= TBasicType::EbtFloat;
		float_type.vectorSize			= 3;
		float_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		float_type.qualifier.builtIn	= TBuiltInVariable::EbvTessCoord;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_TessCoord", TType{float_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}
	
	if ( shader == EShLangTessEvaluation and not dbgInfo.GetCachedSymbolNode( "gl_TessLevelInner" ))
	{
		TPublicType		float_type;		float_type.init({});
		float_type.basicType			= TBasicType::EbtFloat;
		float_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		float_type.qualifier.builtIn	= TBuiltInVariable::EbvTessLevelInner;
		float_type.arraySizes			= new TArraySizes{};
		float_type.arraySizes->addInnerSize( 2 );

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_TessLevelInner", TType{float_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}
	
	if ( shader == EShLangTessEvaluation and not dbgInfo.GetCachedSymbolNode( "gl_TessLevelOuter" ))
	{
		TPublicType		float_type;		float_type.init({});
		float_type.basicType			= TBasicType::EbtFloat;
		float_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		float_type.qualifier.builtIn	= TBuiltInVariable::EbvTessLevelOuter;
		float_type.arraySizes			= new TArraySizes{};
		float_type.arraySizes->addInnerSize( 4 );

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_TessLevelOuter", TType{float_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}

	if ( need_launch_id and not dbgInfo.GetCachedSymbolNode( "gl_LaunchIDNV" ))
	{
		TPublicType		uint_type;	uint_type.init({});
		uint_type.basicType			= TBasicType::EbtUint;
		uint_type.vectorSize		= 3;
		uint_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		uint_type.qualifier.builtIn	= TBuiltInVariable::EbvLaunchIdNV;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_LaunchIDNV", TType{uint_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}

	if ( shader == EShLangVertex and not dbgInfo.GetCachedSymbolNode( "gl_VertexIndex" ))
	{
		TPublicType		int_type;	int_type.init({});
		int_type.basicType			= TBasicType::EbtInt;
		int_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		int_type.qualifier.builtIn	= TBuiltInVariable::EbvVertexIndex;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_VertexIndex", TType{int_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}
	
	if ( shader == EShLangVertex and not dbgInfo.GetCachedSymbolNode( "gl_InstanceIndex" ))
	{
		TPublicType		int_type;	int_type.init({});
		int_type.basicType			= TBasicType::EbtInt;
		int_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		int_type.qualifier.builtIn	= TBuiltInVariable::EbvInstanceIndex;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_InstanceIndex", TType{int_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb, true );
	}
}

/*
=================================================
	CreateAppendToTraceBody2
----
	also see 'CreateAppendToTrace2()'
=================================================
*/
TIntermAggregate*  CreateAppendToTraceBody2 (DebugInfo &dbgInfo)
{
	TPublicType		uint_type;	uint_type.init({});
	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage = TStorageQualifier::EvqConstReadOnly;

	// last_pos, location, size, value
	const uint32_t		dbg_data_size = 3;

	TIntermAggregate*	fn_node		= new TIntermAggregate{ TOperator::EOpFunction };
	TIntermAggregate*	fn_args		= new TIntermAggregate{ TOperator::EOpParameters };
	TIntermAggregate*	fn_body		= new TIntermAggregate{ TOperator::EOpSequence };
	TIntermAggregate*	branch_body = new TIntermAggregate{ TOperator::EOpSequence };

	// build function body
	{
		fn_node->setType( TType{ TBasicType::EbtVoid, TStorageQualifier::EvqGlobal } );
		fn_body->setType( TType{ TBasicType::EbtVoid } );
		fn_node->setName( "dbg_AppendToTrace(u1;" );
		fn_node->getSequence().push_back( fn_args );
		fn_node->getSequence().push_back( fn_body );
	}
	
	// build function argument sequence
	{
		TIntermSymbol*	arg0 = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "sourceLocation", TType{uint_type} };
		fn_args->setType( TType{EbtVoid} );
		fn_args->getSequence().push_back( arg0 );
	}

	// "pos" variable
	uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
	TIntermSymbol*	var_pos		= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "pos", TType{uint_type} };
	
	// "uint pos = atomicAdd( dbg_ShaderTrace.position, x );"
	{
		TIntermAggregate*	move_pos	= new TIntermAggregate{ TOperator::EOpSequence };
		TIntermBinary*		assign_op	= new TIntermBinary{ TOperator::EOpAssign };			// pos = ...
		
		branch_body->setType( TType{EbtVoid} );
		branch_body->getSequence().push_back( move_pos );

		move_pos->setType( TType{EbtVoid} );
		move_pos->getSequence().push_back( assign_op );
		
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		TConstUnionArray		data_size_value(1);	data_size_value[0].setUConst( dbg_data_size );
		TIntermConstantUnion*	data_size	= new TIntermConstantUnion{ data_size_value, TType{uint_type} };
		
		TIntermAggregate*		add_op	= new TIntermAggregate{ TOperator::EOpAtomicAdd };		// atomicAdd
		uint_type.qualifier.storage = TStorageQualifier::EvqGlobal;
		add_op->setType( TType{uint_type} );
		add_op->setOperationPrecision( TPrecisionQualifier::EpqHigh );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqInOut );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqIn );
		add_op->getSequence().push_back( dbgInfo.GetDebugStorageField("position") );
		add_op->getSequence().push_back( data_size );

		assign_op->setType( TType{uint_type} );
		assign_op->setLeft( var_pos );
		assign_op->setRight( add_op );
	}

	// "dbg_ShaderTrace.outData[pos++] = ..."
	{
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		TConstUnionArray		type_value(1);	type_value[0].setUConst( (uint32_t(TBasicType::EbtVoid) & 0xFF) );
		TIntermConstantUnion*	type_id			= new TIntermConstantUnion{ type_value, TType{uint_type} };
		TConstUnionArray		const_value(1);	const_value[0].setUConst( 1 );
		TIntermConstantUnion*	const_one		= new TIntermConstantUnion{ const_value, TType{uint_type} };
		
		uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
		TIntermBinary*			assign_data0	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data1	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data2	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			indexed_access	= new TIntermBinary{ TOperator::EOpIndexIndirect };
		TIntermUnary*			inc_pos			= new TIntermUnary{ TOperator::EOpPostIncrement };
		TIntermSymbol*			last_pos		= dbgInfo.GetCachedSymbolNode( "dbg_LastPosition" );
		TIntermBinary*			new_last_pos	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			prev_pos		= new TIntermBinary{ TOperator::EOpSub };

		// "pos++"
		inc_pos->setType( TType{uint_type} );
		inc_pos->setOperand( var_pos );

		// "dbg_ShaderTrace.outData[pos++]"
		indexed_access->setType( TType{uint_type} );
		indexed_access->getWritableType().setFieldName( "outData" );
		indexed_access->setLeft( dbgInfo.GetDebugStorageField("outData") );
		indexed_access->setRight( inc_pos );
		
		// "dbg_ShaderTrace.outData[pos++] = dbg_LastPosition"
		assign_data0->setType( TType{uint_type} );
		assign_data0->setLeft( indexed_access );
		assign_data0->setRight( last_pos );
		branch_body->getSequence().push_back( assign_data0 );
		
		// "pos - 1"
		prev_pos->setType( TType{uint_type} );
		prev_pos->setLeft( var_pos );
		prev_pos->setRight( const_one );

		// "dbg_LastPosition = pos - 1"
		new_last_pos->setType( TType{uint_type} );
		new_last_pos->setLeft( last_pos );
		new_last_pos->setRight( prev_pos );
		branch_body->getSequence().push_back( new_last_pos );

		// "dbg_ShaderTrace.outData[pos++] = sourceLocation"
		assign_data1->setType( TType{uint_type} );
		assign_data1->setLeft( indexed_access );
		assign_data1->setRight( fn_args->getSequence()[0]->getAsTyped() );
		branch_body->getSequence().push_back( assign_data1 );
		
		// "dbg_ShaderTrace.outData[pos++] = typeid"
		assign_data2->setType( TType{uint_type} );
		assign_data2->setLeft( indexed_access );
		assign_data2->setRight( type_id );
		branch_body->getSequence().push_back( assign_data2 );
	}
	
	// "if ( dbg_IsEnabled )"
	{
		TIntermSymbol*		condition	= dbgInfo.GetCachedSymbolNode( "dbg_IsEnabled" );
		TIntermSelection*	selection	= new TIntermSelection{ condition, branch_body, nullptr };
		selection->setType( TType{EbtVoid} );

		fn_body->getSequence().push_back( selection );
	}

	return fn_node;
}

/*
=================================================
	CreateAppendToTraceBody
----
	also see 'CreateAppendToTrace()'
=================================================
*/
TIntermAggregate*  CreateAppendToTraceBody (const TString &fnName, DebugInfo &dbgInfo)
{
	TPublicType		value_type;	value_type.init({});
	TPublicType		uint_type;	uint_type.init({});
	TPublicType		index_type;	index_type.init({});

	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage = TStorageQualifier::EvqConstReadOnly;
	
	index_type.basicType		= TBasicType::EbtInt;
	index_type.qualifier.storage= TStorageQualifier::EvqConst;

	// extract type
	int	scale = 1;
	{
		size_t	pos  = fnName.find( '(' );
		size_t	pos1 = fnName.find( ';' );
		CHECK_ERR( pos != TString::npos and pos1 != TString::npos and pos < pos1 );
		++pos;

		const bool	is_vector	= (fnName[pos] == 'v');
		const bool	is_matrix	= (fnName[pos] == 'm');

		if ( is_vector or is_matrix )	++pos;

		const bool	is_64	= (pos+2 < fnName.size() and fnName[pos+1] == '6' and fnName[pos+2] == '4');
		//const bool	is_16	= (pos+2 < fnName.size() and fnName[pos+1] == '1' and fnName[pos+2] == '6');

		switch ( fnName[pos] )
		{
			case 'f' :	value_type.basicType = TBasicType::EbtFloat;	break;
			case 'd' :	value_type.basicType = TBasicType::EbtDouble;	scale = 2;	break;
			case 'b' :	value_type.basicType = TBasicType::EbtBool;		break;
			case 'i' :	if ( is_64 ) { value_type.basicType = TBasicType::EbtInt64;  scale = 2; } else value_type.basicType = TBasicType::EbtInt;	break;
			case 'u' :	if ( is_64 ) { value_type.basicType = TBasicType::EbtUint64; scale = 2; } else value_type.basicType = TBasicType::EbtUint;	break;
			default  :	RETURN_ERR( "unknown type" );
		}
		++pos;

		if ( is_matrix ) {
			value_type.setMatrix( fnName[pos] - '0', fnName[pos+1] - '0' );
			CHECK_ERR(	value_type.matrixCols > 0 and value_type.matrixCols <= 4 and
						value_type.matrixRows > 0 and value_type.matrixRows <= 4 );
		}
		else
		if ( is_vector ) {
			value_type.setVector( fnName[pos] - '0' );
			CHECK_ERR( value_type.vectorSize > 0 and value_type.vectorSize <= 4 );
		}
		else {
			// scalar
			value_type.vectorSize = 1;
		}
	}

	// last_pos, location, size, value
	const uint32_t		dbg_data_size = (value_type.matrixCols * value_type.matrixRows + value_type.vectorSize) * scale + 3;

	TIntermAggregate*	fn_node		= new TIntermAggregate{ TOperator::EOpFunction };
	TIntermAggregate*	fn_args		= new TIntermAggregate{ TOperator::EOpParameters };
	TIntermAggregate*	fn_body		= new TIntermAggregate{ TOperator::EOpSequence };
	TIntermAggregate*	branch_body = new TIntermAggregate{ TOperator::EOpSequence };

	// build function body
	{
		value_type.qualifier.storage = TStorageQualifier::EvqGlobal;
		fn_node->setType( TType{value_type} );
		fn_node->setName( fnName );
		fn_node->getSequence().push_back( fn_args );
		fn_node->getSequence().push_back( fn_body );
	}
	
	// build function argument sequence
	{
		value_type.qualifier.storage = TStorageQualifier::EvqConstReadOnly;
		TIntermSymbol*		arg0	 = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "value", TType{value_type} };
		TIntermSymbol*		arg1	 = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "sourceLocation", TType{uint_type} };

		fn_args->setType( TType{EbtVoid} );
		fn_args->getSequence().push_back( arg0 );
		fn_args->getSequence().push_back( arg1 );
	}

	// build function body
	{
		value_type.qualifier.storage = TStorageQualifier::EvqTemporary;
		fn_body->setType( TType{value_type} );
	}

	// "pos" variable
	uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
	TIntermSymbol*	var_pos		= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "pos", TType{uint_type} };
	
	// "uint pos = atomicAdd( dbg_ShaderTrace.position, x );"
	{
		TIntermAggregate*	move_pos	= new TIntermAggregate{ TOperator::EOpSequence };
		TIntermBinary*		assign_op	= new TIntermBinary{ TOperator::EOpAssign };			// pos = ...
		
		branch_body->setType( TType{EbtVoid} );
		branch_body->getSequence().push_back( move_pos );

		move_pos->setType( TType{EbtVoid} );
		move_pos->getSequence().push_back( assign_op );
		
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		TConstUnionArray		data_size_value(1);	data_size_value[0].setUConst( dbg_data_size );
		TIntermConstantUnion*	data_size	= new TIntermConstantUnion{ data_size_value, TType{uint_type} };
		
		TIntermAggregate*		add_op	= new TIntermAggregate{ TOperator::EOpAtomicAdd };		// atomicAdd
		uint_type.qualifier.storage = TStorageQualifier::EvqGlobal;
		add_op->setType( TType{uint_type} );
		add_op->setOperationPrecision( TPrecisionQualifier::EpqHigh );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqInOut );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqIn );
		add_op->getSequence().push_back( dbgInfo.GetDebugStorageField("position") );
		add_op->getSequence().push_back( data_size );

		assign_op->setType( TType{uint_type} );
		assign_op->setLeft( var_pos );
		assign_op->setRight( add_op );
	}

	// "dbg_ShaderTrace.outData[pos++] = ..."
	{
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		TConstUnionArray		type_value(1);	type_value[0].setUConst( (uint32_t(value_type.basicType) & 0xFF) | ((value_type.vectorSize & 0xF) << 8) |
																		 ((value_type.matrixRows & 0xF) << 8) | ((value_type.matrixCols & 0xF) << 12) );
		TIntermConstantUnion*	type_id			= new TIntermConstantUnion{ type_value, TType{uint_type} };
		TConstUnionArray		const_value(1);	const_value[0].setUConst( 1 );
		TIntermConstantUnion*	const_one		= new TIntermConstantUnion{ const_value, TType{uint_type} };
		
		uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
		TIntermBinary*			assign_data0	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data1	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data2	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			indexed_access	= new TIntermBinary{ TOperator::EOpIndexIndirect };
		TIntermUnary*			inc_pos			= new TIntermUnary{ TOperator::EOpPostIncrement };
		TIntermSymbol*			last_pos		= dbgInfo.GetCachedSymbolNode( "dbg_LastPosition" );
		TIntermBinary*			new_last_pos	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			prev_pos		= new TIntermBinary{ TOperator::EOpSub };

		// "pos++"
		inc_pos->setType( TType{uint_type} );
		inc_pos->setOperand( var_pos );

		// "dbg_ShaderTrace.outData[pos++]"
		indexed_access->setType( TType{uint_type} );
		indexed_access->getWritableType().setFieldName( "outData" );
		indexed_access->setLeft( dbgInfo.GetDebugStorageField("outData") );
		indexed_access->setRight( inc_pos );
		
		// "dbg_ShaderTrace.outData[pos++] = dbg_LastPosition"
		assign_data0->setType( TType{uint_type} );
		assign_data0->setLeft( indexed_access );
		assign_data0->setRight( last_pos );
		branch_body->getSequence().push_back( assign_data0 );
		
		// "pos - 1"
		prev_pos->setType( TType{uint_type} );
		prev_pos->setLeft( var_pos );
		prev_pos->setRight( const_one );

		// "dbg_LastPosition = pos - 1"
		new_last_pos->setType( TType{uint_type} );
		new_last_pos->setLeft( last_pos );
		new_last_pos->setRight( prev_pos );
		branch_body->getSequence().push_back( new_last_pos );

		// "dbg_ShaderTrace.outData[pos++] = sourceLocation"
		assign_data1->setType( TType{uint_type} );
		assign_data1->setLeft( indexed_access );
		assign_data1->setRight( fn_args->getSequence()[1]->getAsTyped() );
		branch_body->getSequence().push_back( assign_data1 );
		
		// "dbg_ShaderTrace.outData[pos++] = typeid"
		assign_data2->setType( TType{uint_type} );
		assign_data2->setLeft( indexed_access );
		assign_data2->setRight( type_id );
		branch_body->getSequence().push_back( assign_data2 );

		// "ToUint(...)"
		const auto	TypeToUint	= [] (TIntermTyped* operand, int index) -> TIntermTyped*
		{
			TPublicType		utype;	utype.init({});
			utype.basicType			= TBasicType::EbtUint;
			utype.qualifier.storage = TStorageQualifier::EvqGlobal;

			switch ( operand->getType().getBasicType() )
			{
				case TBasicType::EbtFloat : {
					TIntermUnary*	as_uint = new TIntermUnary{ TOperator::EOpFloatBitsToUint };
					as_uint->setType( TType{utype} );
					as_uint->setOperand( operand );
					as_uint->setOperationPrecision( TPrecisionQualifier::EpqHigh );
					return as_uint;
				}
				case TBasicType::EbtInt : {
					TIntermUnary*	to_uint = new TIntermUnary{ TOperator::EOpConvIntToUint };
					to_uint->setType( TType{utype} );
					to_uint->setOperand( operand );
					return to_uint;
				}
				case TBasicType::EbtUint : {
					return operand;
				}
				case TBasicType::EbtBool : {
					TIntermUnary*	to_uint = new TIntermUnary{ TOperator::EOpConvBoolToUint };
					to_uint->setType( TType{utype} );
					to_uint->setOperand( operand );
					return to_uint;
				}
				case TBasicType::EbtDouble : {
					// "doubleBitsToUint64(value)"
					TIntermUnary*	as_uint64 = new TIntermUnary{ TOperator::EOpDoubleBitsToUint64 };
					utype.basicType = TBasicType::EbtUint64;
					as_uint64->setType( TType{utype} );
					as_uint64->setOperand( operand );
					as_uint64->setOperationPrecision( TPrecisionQualifier::EpqHigh );
					
					// "doubleBitsToUint64(value) >> x"
					utype.qualifier.storage = TStorageQualifier::EvqConst;
					TConstUnionArray		shift_value(1);	shift_value[0].setU64Const( (index&1)*32 );
					TIntermConstantUnion*	const_shift		= new TIntermConstantUnion{ shift_value, TType{utype} };
					TIntermBinary*			shift			= new TIntermBinary{ TOperator::EOpRightShift };
					utype.qualifier.storage	= TStorageQualifier::EvqTemporary;
					shift->setType( TType{utype} );
					shift->setLeft( as_uint64 );
					shift->setRight( const_shift );
					
					// "uint(doubleBitsToUint64(value) >> x)"
					TIntermUnary*			to_uint = new TIntermUnary{ TOperator::EOpConvUint64ToUint };
					utype.basicType			= TBasicType::EbtUint;
					utype.qualifier.storage	= TStorageQualifier::EvqGlobal;
					to_uint->setType( TType{utype} );
					to_uint->setOperand( shift );
					return to_uint;
				}
				case TBasicType::EbtInt64 : {
					// "value >> x"
					utype.basicType			= TBasicType::EbtInt64;
					utype.qualifier.storage = TStorageQualifier::EvqConst;
					TConstUnionArray		shift_value(1);	shift_value[0].setI64Const( (index&1)*32 );
					TIntermConstantUnion*	const_shift		= new TIntermConstantUnion{ shift_value, TType{utype} };
					TIntermBinary*			shift			= new TIntermBinary{ TOperator::EOpRightShift };
					utype.qualifier.storage	= TStorageQualifier::EvqTemporary;
					shift->setType( TType{utype} );
					shift->setLeft( operand );
					shift->setRight( const_shift );
					
					// "uint(value >> x)"
					TIntermUnary*			to_uint = new TIntermUnary{ TOperator::EOpConvInt64ToUint };
					utype.basicType			= TBasicType::EbtUint;
					utype.qualifier.storage	= TStorageQualifier::EvqGlobal;
					to_uint->setType( TType{utype} );
					to_uint->setOperand( shift );
					return to_uint;
				}
				case TBasicType::EbtUint64 : {
					// "value >> x"
					utype.basicType			= TBasicType::EbtUint64;
					utype.qualifier.storage = TStorageQualifier::EvqConst;
					TConstUnionArray		shift_value(1);	shift_value[0].setU64Const( (index&1)*32 );
					TIntermConstantUnion*	const_shift		= new TIntermConstantUnion{ shift_value, TType{utype} };
					TIntermBinary*			shift			= new TIntermBinary{ TOperator::EOpRightShift };
					utype.qualifier.storage	= TStorageQualifier::EvqTemporary;
					shift->setType( TType{utype} );
					shift->setLeft( operand );
					shift->setRight( const_shift );
					
					// "uint(value >> x)"
					TIntermUnary*			to_uint = new TIntermUnary{ TOperator::EOpConvUint64ToUint };
					utype.basicType			= TBasicType::EbtUint;
					utype.qualifier.storage	= TStorageQualifier::EvqGlobal;
					to_uint->setType( TType{utype} );
					to_uint->setOperand( shift );
					return to_uint;
				}
			}
			RETURN_ERR( "not supported" );
		};

		// "dbg_ShaderTrace.outData[pos++] = ToUint(value)"
		if ( value_type.isScalar() )
		{
			for (int i = 0; i < scale; ++i)
			{
				TIntermBinary*	assign_data3	= new TIntermBinary{ TOperator::EOpAssign };
				TIntermTyped*	scalar			= fn_args->getSequence()[0]->getAsTyped();
			
				assign_data3->setType( TType{uint_type} );
				assign_data3->setLeft( indexed_access );
				assign_data3->setRight( TypeToUint( scalar, i ));
				branch_body->getSequence().push_back( assign_data3 );
			}
		}
		else
		if ( value_type.matrixCols and value_type.matrixRows )
		{
			TIntermTyped*	mat			= fn_args->getSequence()[0]->getAsTyped();
			TPublicType		pub_type;	pub_type.init({});

			pub_type.basicType			= mat->getType().getBasicType();
			pub_type.qualifier.storage	= mat->getType().getQualifier().storage;

			for (int c = 0; c < value_type.matrixCols; ++c)
			{
				TConstUnionArray		col_index(1);	col_index[0].setIConst( c );
				TIntermConstantUnion*	col_field		= new TIntermConstantUnion{ col_index, TType{index_type} };
				TIntermBinary*			col_access		= new TIntermBinary{ TOperator::EOpIndexDirect };
				
				// "matrix[c]"
				pub_type.setVector( value_type.matrixRows );
				col_access->setType( TType{pub_type} );
				col_access->setLeft( mat );
				col_access->setRight( col_field );

				for (int r = 0; r < value_type.matrixRows * scale; ++r)
				{
					TConstUnionArray		row_index(1);	col_index[0].setIConst( r / scale );
					TIntermConstantUnion*	row_field		= new TIntermConstantUnion{ row_index, TType{index_type} };
					TIntermBinary*			row_access		= new TIntermBinary{ TOperator::EOpIndexDirect };
					TIntermBinary*			assign_data3	= new TIntermBinary{ TOperator::EOpAssign };
					
					// "matrix[c][r]"
					pub_type.setVector( 1 );
					row_access->setType( TType{pub_type} );
					row_access->setLeft( col_access );
					row_access->setRight( row_field );
					
					// "dbg_ShaderTrace.outData[pos++] = ToUint(value.x)"
					assign_data3->setType( TType{uint_type} );
					assign_data3->setLeft( indexed_access );
					assign_data3->setRight( TypeToUint( row_access, r ));
					branch_body->getSequence().push_back( assign_data3 );
				}
			}
		}
		else
		for (int i = 0; i < value_type.vectorSize * scale; ++i)
		{
			TIntermBinary*			assign_data3	= new TIntermBinary{ TOperator::EOpAssign };
			TConstUnionArray		field_index(1);	field_index[0].setIConst( i / scale );
			TIntermConstantUnion*	vec_field		= new TIntermConstantUnion{ field_index, TType{index_type} };
			TIntermBinary*			field_access	= new TIntermBinary{ TOperator::EOpIndexDirect };
			TIntermTyped*			vec				= fn_args->getSequence()[0]->getAsTyped();
			
			TPublicType		pub_type;	pub_type.init({});
			pub_type.basicType			= vec->getType().getBasicType();
			pub_type.qualifier.storage	= vec->getType().getQualifier().storage;

			// "value.x"
			field_access->setType( TType{pub_type} );
			field_access->setLeft( vec );
			field_access->setRight( vec_field );
			
			// "dbg_ShaderTrace.outData[pos++] = ToUint(value.x)"
			assign_data3->setType( TType{uint_type} );
			assign_data3->setLeft( indexed_access );
			assign_data3->setRight( TypeToUint( field_access, i ));
			branch_body->getSequence().push_back( assign_data3 );
		}
	}
	
	// "if ( dbg_IsEnabled )"
	{
		TIntermSymbol*		condition	= dbgInfo.GetCachedSymbolNode( "dbg_IsEnabled" );
		TIntermSelection*	selection	= new TIntermSelection{ condition, branch_body, nullptr };
		selection->setType( TType{EbtVoid} );

		fn_body->getSequence().push_back( selection );
	}

	// "return value"
	{
		TIntermBranch*		fn_return	= new TIntermBranch{ TOperator::EOpReturn, fn_args->getSequence()[0]->getAsTyped() };
		fn_body->getSequence().push_back( fn_return );
	}

	return fn_node;
}

/*
=================================================
	CreateAddTimeToTraceBody2
----
	also see 'CreateAddTimeToTrace2()'
=================================================
*/
TIntermAggregate*  CreateAddTimeToTraceBody2 (DebugInfo &dbgInfo)
{
	TPublicType		uint_type;	uint_type.init({});
	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage = TStorageQualifier::EvqConstReadOnly;

	// last_pos, location, size, startTime, endTime
	const uint32_t		dbg_data_size = 3 + 4 + 4;

	TIntermAggregate*	fn_node		= new TIntermAggregate{ TOperator::EOpFunction };
	TIntermAggregate*	fn_args		= new TIntermAggregate{ TOperator::EOpParameters };
	TIntermAggregate*	fn_body		= new TIntermAggregate{ TOperator::EOpSequence };
	TIntermAggregate*	branch_body = new TIntermAggregate{ TOperator::EOpSequence };

	// build function body
	{
		fn_node->setType( TType{ TBasicType::EbtVoid, TStorageQualifier::EvqGlobal } );
		fn_body->setType( TType{ TBasicType::EbtVoid } );
		fn_node->setName( "dbg_AddTimeToTrace(vu4;u1;" );
		fn_node->getSequence().push_back( fn_args );
		fn_node->getSequence().push_back( fn_body );
	}
	
	// build function argument sequence
	{
		uint_type.vectorSize	= 4;
		TIntermSymbol*	arg0	 = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "startTime", TType{uint_type} };
		
		uint_type.vectorSize	= 1;
		TIntermSymbol*	arg1	= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "sourceLocation", TType{uint_type} };

		fn_args->setType( TType{EbtVoid} );
		fn_args->getSequence().push_back( arg0 );
		fn_args->getSequence().push_back( arg1 );
	}

	// build branch body
	branch_body->setType( TType{EbtVoid} );

	// "endTime = vec4( clock(), clock() );"
	uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
	uint_type.vectorSize		= 4;
	TIntermSymbol*	end_time	= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "endTime", TType{uint_type} };
	
	TIntermBinary*  end_time_assign = AssignClock( end_time, {}, dbgInfo );
	branch_body->getSequence().push_back( end_time_assign );

	// "pos" variable
	uint_type.vectorSize		= 1;
	TIntermSymbol*	var_pos		= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "pos", TType{uint_type} };
	
	// "uint pos = atomicAdd( dbg_ShaderTrace.position, x );"
	{
		TIntermAggregate*	move_pos	= new TIntermAggregate{ TOperator::EOpSequence };
		TIntermBinary*		assign_op	= new TIntermBinary{ TOperator::EOpAssign };			// pos = ...
		
		branch_body->getSequence().push_back( move_pos );

		move_pos->setType( TType{EbtVoid} );
		move_pos->getSequence().push_back( assign_op );
		
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		TConstUnionArray		data_size_value(1);	data_size_value[0].setUConst( dbg_data_size );
		TIntermConstantUnion*	data_size	= new TIntermConstantUnion{ data_size_value, TType{uint_type} };
		
		TIntermAggregate*		add_op	= new TIntermAggregate{ TOperator::EOpAtomicAdd };		// atomicAdd
		uint_type.qualifier.storage = TStorageQualifier::EvqGlobal;
		add_op->setType( TType{uint_type} );
		add_op->setOperationPrecision( TPrecisionQualifier::EpqHigh );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqInOut );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqIn );
		add_op->getSequence().push_back( dbgInfo.GetDebugStorageField("position") );
		add_op->getSequence().push_back( data_size );

		assign_op->setType( TType{uint_type} );
		assign_op->setLeft( var_pos );
		assign_op->setRight( add_op );
	}

	// "dbg_ShaderTrace.outData[pos++] = ..."
	{
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		TConstUnionArray		type_value(1);	type_value[0].setUConst( (ShaderTrace::TBasicType_Clock & 0xFF) | (4 << 8) );
		TIntermConstantUnion*	type_id			= new TIntermConstantUnion{ type_value, TType{uint_type} };
		TConstUnionArray		const_value(1);	const_value[0].setUConst( 1 );
		TIntermConstantUnion*	const_one		= new TIntermConstantUnion{ const_value, TType{uint_type} };
		
		uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
		TIntermBinary*			assign_data0	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data1	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data2	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			indexed_access	= new TIntermBinary{ TOperator::EOpIndexIndirect };
		TIntermUnary*			inc_pos			= new TIntermUnary{ TOperator::EOpPostIncrement };
		TIntermSymbol*			last_pos		= dbgInfo.GetCachedSymbolNode( "dbg_LastPosition" );
		TIntermBinary*			new_last_pos	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			prev_pos		= new TIntermBinary{ TOperator::EOpSub };

		// "pos++"
		inc_pos->setType( TType{uint_type} );
		inc_pos->setOperand( var_pos );

		// "dbg_ShaderTrace.outData[pos++]"
		indexed_access->setType( TType{uint_type} );
		indexed_access->getWritableType().setFieldName( "outData" );
		indexed_access->setLeft( dbgInfo.GetDebugStorageField("outData") );
		indexed_access->setRight( inc_pos );
		
		// "dbg_ShaderTrace.outData[pos++] = dbg_LastPosition"
		assign_data0->setType( TType{uint_type} );
		assign_data0->setLeft( indexed_access );
		assign_data0->setRight( last_pos );
		branch_body->getSequence().push_back( assign_data0 );
		
		// "pos - 1"
		prev_pos->setType( TType{uint_type} );
		prev_pos->setLeft( var_pos );
		prev_pos->setRight( const_one );

		// "dbg_LastPosition = pos - 1"
		new_last_pos->setType( TType{uint_type} );
		new_last_pos->setLeft( last_pos );
		new_last_pos->setRight( prev_pos );
		branch_body->getSequence().push_back( new_last_pos );

		// "dbg_ShaderTrace.outData[pos++] = sourceLocation"
		assign_data1->setType( TType{uint_type} );
		assign_data1->setLeft( indexed_access );
		assign_data1->setRight( fn_args->getSequence()[1]->getAsTyped() );
		branch_body->getSequence().push_back( assign_data1 );
		
		// "dbg_ShaderTrace.outData[pos++] = typeid"
		assign_data2->setType( TType{uint_type} );
		assign_data2->setLeft( indexed_access );
		assign_data2->setRight( type_id );
		branch_body->getSequence().push_back( assign_data2 );

		const auto	WriteUVec4 = [&branch_body, &indexed_access, &uint_type] (TIntermTyped* vec)
		{
			TPublicType		index_type;	index_type.init({});
			index_type.basicType		= TBasicType::EbtInt;
			index_type.qualifier.storage= TStorageQualifier::EvqConst;

			for (int i = 0; i < vec->getType().getVectorSize(); ++i)
			{
				TIntermBinary*			assign_data		= new TIntermBinary{ TOperator::EOpAssign };
				TConstUnionArray		field_index(1);	field_index[0].setIConst( i );
				TIntermConstantUnion*	vec_field		= new TIntermConstantUnion{ field_index, TType{index_type} };
				TIntermBinary*			field_access	= new TIntermBinary{ TOperator::EOpIndexDirect };
			
				TPublicType		pub_type;	pub_type.init({});
				pub_type.basicType			= vec->getType().getBasicType();
				pub_type.qualifier.storage	= vec->getType().getQualifier().storage;

				// "value.x"
				field_access->setType( TType{pub_type} );
				field_access->setLeft( vec );
				field_access->setRight( vec_field );
			
				// "dbg_ShaderTrace.outData[pos++] = value.x"
				assign_data->setType( TType{uint_type} );
				assign_data->setLeft( indexed_access );
				assign_data->setRight( field_access );
				branch_body->getSequence().push_back( assign_data );
			}
		};

		// "dbg_ShaderTrace.outData[pos++] = startTime"
		WriteUVec4( fn_args->getSequence()[0]->getAsTyped() );

		// "dbg_ShaderTrace.outData[pos++] = endTime"
		WriteUVec4( end_time );
	}
	
	// "if ( dbg_IsEnabled )"
	{
		TIntermSymbol*		condition	= dbgInfo.GetCachedSymbolNode( "dbg_IsEnabled" );
		TIntermSelection*	selection	= new TIntermSelection{ condition, branch_body, nullptr };
		selection->setType( TType{EbtVoid} );

		fn_body->getSequence().push_back( selection );
	}

	return fn_node;
}

/*
=================================================
	CreateAddTimeToTraceBody
----
	also see 'CreateAddTimeToTrace()'
=================================================
*/
TIntermAggregate*  CreateAddTimeToTraceBody (const TString &fnName, DebugInfo &dbgInfo)
{
	TPublicType		value_type;	value_type.init({});
	TPublicType		uint_type;	uint_type.init({});
	TPublicType		index_type;	index_type.init({});

	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage = TStorageQualifier::EvqConstReadOnly;
	
	index_type.basicType		= TBasicType::EbtInt;
	index_type.qualifier.storage= TStorageQualifier::EvqConst;

	// extract type
	{
		size_t	pos  = fnName.find( ';' );
		size_t	pos1 = fnName.find( ';', pos+1 );
		CHECK_ERR( pos != TString::npos and pos1 != TString::npos and pos < pos1 );
		++pos;

		const bool	is_vector	= (fnName[pos] == 'v');
		const bool	is_matrix	= (fnName[pos] == 'm');

		if ( is_vector or is_matrix )	++pos;

		const bool	is_64	= (pos+2 < fnName.size() and fnName[pos+1] == '6' and fnName[pos+2] == '4');
		//const bool	is_16	= (pos+2 < fnName.size() and fnName[pos+1] == '1' and fnName[pos+2] == '6');

		switch ( fnName[pos] )
		{
			case 'f' :	value_type.basicType = TBasicType::EbtFloat;	break;
			case 'd' :	value_type.basicType = TBasicType::EbtDouble;	break;
			case 'b' :	value_type.basicType = TBasicType::EbtBool;		break;
			case 'i' :	if ( is_64 ) { value_type.basicType = TBasicType::EbtInt64;  } else value_type.basicType = TBasicType::EbtInt;	break;
			case 'u' :	if ( is_64 ) { value_type.basicType = TBasicType::EbtUint64; } else value_type.basicType = TBasicType::EbtUint;	break;
			default  :	RETURN_ERR( "unknown type" );
		}
		++pos;

		if ( is_matrix ) {
			value_type.setMatrix( fnName[pos] - '0', fnName[pos+1] - '0' );
			CHECK_ERR(	value_type.matrixCols > 0 and value_type.matrixCols <= 4 and
						value_type.matrixRows > 0 and value_type.matrixRows <= 4 );
		}
		else
		if ( is_vector ) {
			value_type.setVector( fnName[pos] - '0' );
			CHECK_ERR( value_type.vectorSize > 0 and value_type.vectorSize <= 4 );
		}
		else {
			// scalar
			value_type.vectorSize = 1;
		}
	}

	// last_pos, location, size, startTime, endTime
	const uint32_t		dbg_data_size = 3 + 4 + 4;

	TIntermAggregate*	fn_node		= new TIntermAggregate{ TOperator::EOpFunction };
	TIntermAggregate*	fn_args		= new TIntermAggregate{ TOperator::EOpParameters };
	TIntermAggregate*	fn_body		= new TIntermAggregate{ TOperator::EOpSequence };
	TIntermAggregate*	branch_body = new TIntermAggregate{ TOperator::EOpSequence };

	// build function body
	{
		value_type.qualifier.storage = TStorageQualifier::EvqGlobal;
		fn_node->setType( TType{value_type} );
		fn_node->setName( fnName );
		fn_node->getSequence().push_back( fn_args );
		fn_node->getSequence().push_back( fn_body );
	}
	
	// build function argument sequence
	{
		uint_type.vectorSize		= 4;
		TIntermSymbol*		arg0	 = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "startTime", TType{uint_type} };
		
		value_type.qualifier.storage = TStorageQualifier::EvqConstReadOnly;
		TIntermSymbol*		arg1	 = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "value", TType{value_type} };

		uint_type.vectorSize		= 1;
		TIntermSymbol*		arg2	 = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "sourceLocation", TType{uint_type} };

		fn_args->setType( TType{EbtVoid} );
		fn_args->getSequence().push_back( arg0 );
		fn_args->getSequence().push_back( arg1 );
		fn_args->getSequence().push_back( arg2 );
	}

	// build function body
	{
		value_type.qualifier.storage = TStorageQualifier::EvqTemporary;
		fn_body->setType( TType{value_type} );
	}
	
	// build branch body
	branch_body->setType( TType{EbtVoid} );

	// "endTime = vec4( clock(), clock() );"
	uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
	uint_type.vectorSize		= 4;
	TIntermSymbol*	end_time	= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "endTime", TType{uint_type} };
	
	TIntermBinary*  end_time_assign = AssignClock( end_time, {}, dbgInfo );
	branch_body->getSequence().push_back( end_time_assign );

	// "pos" variable
	uint_type.vectorSize		= 1;
	TIntermSymbol*	var_pos		= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "pos", TType{uint_type} };
	
	// "uint pos = atomicAdd( dbg_ShaderTrace.position, x );"
	{
		TIntermAggregate*	move_pos	= new TIntermAggregate{ TOperator::EOpSequence };
		TIntermBinary*		assign_op	= new TIntermBinary{ TOperator::EOpAssign };			// pos = ...
		
		branch_body->getSequence().push_back( move_pos );

		move_pos->setType( TType{EbtVoid} );
		move_pos->getSequence().push_back( assign_op );
		
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		TConstUnionArray		data_size_value(1);	data_size_value[0].setUConst( dbg_data_size );
		TIntermConstantUnion*	data_size	= new TIntermConstantUnion{ data_size_value, TType{uint_type} };
		
		TIntermAggregate*		add_op	= new TIntermAggregate{ TOperator::EOpAtomicAdd };		// atomicAdd
		uint_type.qualifier.storage = TStorageQualifier::EvqGlobal;
		add_op->setType( TType{uint_type} );
		add_op->setOperationPrecision( TPrecisionQualifier::EpqHigh );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqInOut );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqIn );
		add_op->getSequence().push_back( dbgInfo.GetDebugStorageField("position") );
		add_op->getSequence().push_back( data_size );

		assign_op->setType( TType{uint_type} );
		assign_op->setLeft( var_pos );
		assign_op->setRight( add_op );
	}

	// "dbg_ShaderTrace.outData[pos++] = ..."
	{
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		TConstUnionArray		type_value(1);	type_value[0].setUConst( (ShaderTrace::TBasicType_Clock & 0xFF) | (4 << 8) );
		TIntermConstantUnion*	type_id			= new TIntermConstantUnion{ type_value, TType{uint_type} };
		TConstUnionArray		const_value(1);	const_value[0].setUConst( 1 );
		TIntermConstantUnion*	const_one		= new TIntermConstantUnion{ const_value, TType{uint_type} };
		
		uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
		TIntermBinary*			assign_data0	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data1	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data2	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			indexed_access	= new TIntermBinary{ TOperator::EOpIndexIndirect };
		TIntermUnary*			inc_pos			= new TIntermUnary{ TOperator::EOpPostIncrement };
		TIntermSymbol*			last_pos		= dbgInfo.GetCachedSymbolNode( "dbg_LastPosition" );
		TIntermBinary*			new_last_pos	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			prev_pos		= new TIntermBinary{ TOperator::EOpSub };

		// "pos++"
		inc_pos->setType( TType{uint_type} );
		inc_pos->setOperand( var_pos );

		// "dbg_ShaderTrace.outData[pos++]"
		indexed_access->setType( TType{uint_type} );
		indexed_access->getWritableType().setFieldName( "outData" );
		indexed_access->setLeft( dbgInfo.GetDebugStorageField("outData") );
		indexed_access->setRight( inc_pos );
		
		// "dbg_ShaderTrace.outData[pos++] = dbg_LastPosition"
		assign_data0->setType( TType{uint_type} );
		assign_data0->setLeft( indexed_access );
		assign_data0->setRight( last_pos );
		branch_body->getSequence().push_back( assign_data0 );
		
		// "pos - 1"
		prev_pos->setType( TType{uint_type} );
		prev_pos->setLeft( var_pos );
		prev_pos->setRight( const_one );

		// "dbg_LastPosition = pos - 1"
		new_last_pos->setType( TType{uint_type} );
		new_last_pos->setLeft( last_pos );
		new_last_pos->setRight( prev_pos );
		branch_body->getSequence().push_back( new_last_pos );

		// "dbg_ShaderTrace.outData[pos++] = sourceLocation"
		assign_data1->setType( TType{uint_type} );
		assign_data1->setLeft( indexed_access );
		assign_data1->setRight( fn_args->getSequence()[2]->getAsTyped() );
		branch_body->getSequence().push_back( assign_data1 );
		
		// "dbg_ShaderTrace.outData[pos++] = typeid"
		assign_data2->setType( TType{uint_type} );
		assign_data2->setLeft( indexed_access );
		assign_data2->setRight( type_id );
		branch_body->getSequence().push_back( assign_data2 );
		
		const auto	WriteUVec4 = [&branch_body, &indexed_access, &uint_type] (TIntermTyped* vec)
		{
			TPublicType		index_type;	index_type.init({});
			index_type.basicType		= TBasicType::EbtInt;
			index_type.qualifier.storage= TStorageQualifier::EvqConst;

			for (int i = 0; i < vec->getType().getVectorSize(); ++i)
			{
				TIntermBinary*			assign_data		= new TIntermBinary{ TOperator::EOpAssign };
				TConstUnionArray		field_index(1);	field_index[0].setIConst( i );
				TIntermConstantUnion*	vec_field		= new TIntermConstantUnion{ field_index, TType{index_type} };
				TIntermBinary*			field_access	= new TIntermBinary{ TOperator::EOpIndexDirect };
			
				TPublicType		pub_type;	pub_type.init({});
				pub_type.basicType			= vec->getType().getBasicType();
				pub_type.qualifier.storage	= vec->getType().getQualifier().storage;

				// "value.x"
				field_access->setType( TType{pub_type} );
				field_access->setLeft( vec );
				field_access->setRight( vec_field );
			
				// "dbg_ShaderTrace.outData[pos++] = value.x"
				assign_data->setType( TType{uint_type} );
				assign_data->setLeft( indexed_access );
				assign_data->setRight( field_access );
				branch_body->getSequence().push_back( assign_data );
			}
		};

		// "dbg_ShaderTrace.outData[pos++] = startTime"
		WriteUVec4( fn_args->getSequence()[0]->getAsTyped() );

		// "dbg_ShaderTrace.outData[pos++] = endTime"
		WriteUVec4( end_time );
	}
	
	// "if ( dbg_IsEnabled )"
	{
		TIntermSymbol*		condition	= dbgInfo.GetCachedSymbolNode( "dbg_IsEnabled" );
		TIntermSelection*	selection	= new TIntermSelection{ condition, branch_body, nullptr };
		selection->setType( TType{EbtVoid} );

		fn_body->getSequence().push_back( selection );
	}

	// "return value"
	{
		TIntermBranch*		fn_return	= new TIntermBranch{ TOperator::EOpReturn, fn_args->getSequence()[1]->getAsTyped() };
		fn_body->getSequence().push_back( fn_return );
	}

	return fn_node;
}

/*
=================================================
	CreateGetCurrentTimeBody
----
	also see 'AssignClock()'
=================================================
*/
TIntermAggregate*  CreateGetCurrentTimeBody (DebugInfo &dbgInfo)
{
	TIntermAggregate*	fn_node		= new TIntermAggregate{ TOperator::EOpFunction };
	TIntermAggregate*	fn_args		= new TIntermAggregate{ TOperator::EOpParameters };
	TIntermAggregate*	fn_body		= new TIntermAggregate{ TOperator::EOpSequence };
	TIntermAggregate*	branch_true = new TIntermAggregate{ TOperator::EOpSequence };
	TIntermSymbol*		curr_time	= nullptr;
	TPublicType			uint_type;	uint_type.init({});

	// build function body
	{
		uint_type.basicType				= TBasicType::EbtUint;
		uint_type.vectorSize			= 4;
		uint_type.qualifier.storage		= TStorageQualifier::EvqGlobal;
		uint_type.qualifier.precision	= TPrecisionQualifier::EpqHigh;

		fn_node->setType( TType{uint_type} );
		fn_node->setName( "dbg_GetCurrentTime(" );
		fn_node->getSequence().push_back( fn_args );
		fn_node->getSequence().push_back( fn_body );

		uint_type.qualifier.storage	= TStorageQualifier::EvqTemporary;
		curr_time					= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "curr_time", TType{uint_type} };
		
		TConstUnionArray		zero_value{ 4 };
		zero_value[0].setUConst( 0 );
		zero_value[1].setUConst( 0 );
		zero_value[2].setUConst( 0 );
		zero_value[3].setUConst( 0 );
		uint_type.qualifier.storage			= TStorageQualifier::EvqConst;
		TIntermConstantUnion*	zero_const	= new TIntermConstantUnion{ zero_value, TType{uint_type} };

		TIntermBinary*		init_time	= new TIntermBinary{ TOperator::EOpAssign };
		uint_type.qualifier.storage		= TStorageQualifier::EvqTemporary;
		init_time->setType( TType{uint_type} );
		init_time->setLeft( curr_time );
		init_time->setRight( zero_const );

		fn_body->getSequence().push_back( init_time );
	}

	// build 'true' branch body
	{
		branch_true->setType( TType{EbtVoid} );
		
		TPublicType		int_type;	int_type.init({});
		int_type.basicType			= TBasicType::EbtInt;
		int_type.vectorSize			= 1;
		int_type.qualifier.storage	= TStorageQualifier::EvqConst;
		
		uint_type.vectorSize		= 2;
		uint_type.qualifier.storage	= TStorageQualifier::EvqTemporary;

		if ( dbgInfo._shaderSubgroupClock )
		{
			TConstUnionArray		x_value{1};	x_value[0].setIConst( 0 );
			TConstUnionArray		y_value{1};	y_value[0].setIConst( 1 );
			TIntermConstantUnion*	x_index		= new TIntermConstantUnion{ x_value, TType{int_type} };
			TIntermConstantUnion*	y_index		= new TIntermConstantUnion{ y_value, TType{int_type} };
			TIntermAggregate*		xy_index	= new TIntermAggregate{ TOperator::EOpSequence };

			xy_index->getSequence().push_back( x_index );
			xy_index->getSequence().push_back( y_index );

			TIntermBinary*			assign		= new TIntermBinary{ TOperator::EOpAssign };
			TIntermBinary*			swizzle		= new TIntermBinary{ TOperator::EOpVectorSwizzle };
			TIntermAggregate*		time_call	= new TIntermAggregate( TOperator::EOpReadClockSubgroupKHR );
			
			swizzle->setType( TType{uint_type} );
			swizzle->setLeft( curr_time );
			swizzle->setRight( xy_index );

			assign->setType( TType{uint_type} );
			assign->setLeft( swizzle );
			assign->setRight( time_call );
			
			time_call->setType( TType{uint_type} );

			branch_true->getSequence().push_back( assign );
		}

		if ( dbgInfo._shaderDeviceClock )
		{
			TConstUnionArray		z_value{1};	z_value[0].setIConst( 2 );
			TConstUnionArray		w_value{1};	w_value[0].setIConst( 3 );
			TIntermConstantUnion*	z_index		= new TIntermConstantUnion{ z_value, TType{int_type} };
			TIntermConstantUnion*	w_index		= new TIntermConstantUnion{ w_value, TType{int_type} };
			TIntermAggregate*		zw_index	= new TIntermAggregate{ TOperator::EOpSequence };

			zw_index->getSequence().push_back( z_index );
			zw_index->getSequence().push_back( w_index );

			TIntermBinary*			assign		= new TIntermBinary{ TOperator::EOpAssign };
			TIntermBinary*			swizzle		= new TIntermBinary{ TOperator::EOpVectorSwizzle };
			TIntermAggregate*		time_call	= new TIntermAggregate( TOperator::EOpReadClockDeviceKHR );

			swizzle->setType( TType{uint_type} );
			swizzle->setLeft( curr_time );
			swizzle->setRight( zw_index );
			
			assign->setType( TType{uint_type} );
			assign->setLeft( swizzle );
			assign->setRight( time_call );
			
			time_call->setType( TType{uint_type} );

			branch_true->getSequence().push_back( assign );
		}
	}

	// "if ( dbg_IsEnabled )"
	{
		TIntermSymbol*		condition	= dbgInfo.GetCachedSymbolNode( "dbg_IsEnabled" );
		TIntermSelection*	selection	= new TIntermSelection{ condition, branch_true, nullptr };
		selection->setType( TType{EbtVoid} );

		fn_body->getSequence().push_back( selection );
	}

	// "return curr_time;"
	{
		TIntermBranch*		fn_return	= new TIntermBranch{ TOperator::EOpReturn, curr_time };
		fn_body->getSequence().push_back( fn_return );
	}

	return fn_node;
}

/*
=================================================
	AppendShaderInputVaryings
=================================================
*/
bool  AppendShaderInputVaryings (TIntermAggregate* body, DebugInfo &dbgInfo)
{
	CHECK_ERR( dbgInfo.GetCallStack().size() );

	TIntermAggregate*	root		= dbgInfo.GetCallStack().front().node->getAsAggregate();
	TIntermAggregate*	linker_objs	= nullptr;
	CHECK_ERR( root );

	for (auto* node : root->getSequence()) {
		if ( auto* aggr = node->getAsAggregate(); aggr and aggr->getOp() == TOperator::EOpLinkerObjects ) {
			linker_objs = aggr;
			break;
		}
	}
	CHECK_ERR( linker_objs );

	for (auto* node : linker_objs->getSequence())
	{
		TIntermSymbol*	symb = node->getAsSymbolNode();

		if ( not symb or symb->getQualifier().storage != TStorageQualifier::EvqVaryingIn )
			continue;

		if ( symb->isStruct() )
		{
			auto&	struct_fields = *symb->getType().getStruct();
			
			TPublicType		index_type;		index_type.init({});
			index_type.basicType			= TBasicType::EbtInt;
			index_type.qualifier.storage	= TStorageQualifier::EvqConst;

			for (auto& field : struct_fields)
			{
				ASSERT( not field.type->isStruct() );	// TODO

				const size_t			index			= std::distance( struct_fields.data(), &field );
				TConstUnionArray		index_Value(1);	index_Value[0].setIConst( int(index) );
				TIntermConstantUnion*	field_index		= new TIntermConstantUnion{ index_Value, TType{index_type} };
				TIntermBinary*			field_access	= new TIntermBinary{ TOperator::EOpIndexDirectStruct };
				field_access->setType( *field.type );
				field_access->setLeft( symb );
				field_access->setRight( field_index );

				const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( field_access, symb->getLoc()/*field.loc*/ );

				if ( auto* fncall = CreateAppendToTrace( field_access, loc_id, dbgInfo ))
					body->getSequence().push_back( fncall );
			}
		}
		else
		if ( symb->isScalar() or symb->isVector() or symb->isMatrix() )
		{
			const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( symb, symb->getLoc() );

			if ( auto* fncall = CreateAppendToTrace( symb, loc_id, dbgInfo ))
				body->getSequence().push_back( fncall );
		}
	}

	return true;
}

/*
=================================================
	RecordVertexShaderInfo
=================================================
*/
TIntermAggregate*  RecordVertexShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );
	
	// "dbg_AppendToTrace( gl_VertexIndex, location )"
	{
		TIntermSymbol*	vert_index	= dbgInfo.GetCachedSymbolNode( "gl_VertexIndex" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( vert_index, loc );

		body->getSequence().push_back( CreateAppendToTrace( vert_index, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_InstanceIndex, location )"
	{
		TIntermSymbol*	instance	= dbgInfo.GetCachedSymbolNode( "gl_InstanceIndex" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( instance, loc );

		body->getSequence().push_back( CreateAppendToTrace( instance, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_DrawID, location )"
	if ( auto* draw_id = dbgInfo.GetCachedSymbolNode( "gl_DrawID" ) )
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( draw_id, loc );
		body->getSequence().push_back( CreateAppendToTrace( draw_id, loc_id, dbgInfo ));
	}

	return body;
}

/*
=================================================
	RecordTessControlShaderInfo
=================================================
*/
TIntermAggregate*  RecordTessControlShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );
	
	// "dbg_AppendToTrace( gl_InvocationID, location )"
	{
		TIntermSymbol*	invocation	= dbgInfo.GetCachedSymbolNode( "gl_InvocationID" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( invocation, loc );

		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_PrimitiveID, location )"
	{
		TIntermSymbol*	primitive	= dbgInfo.GetCachedSymbolNode( "gl_PrimitiveID" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( primitive, loc );

		body->getSequence().push_back( CreateAppendToTrace( primitive, loc_id, dbgInfo ));
	}

	return body;
}

/*
=================================================
	RecordTessEvaluationShaderInfo
=================================================
*/
TIntermAggregate*  RecordTessEvaluationShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );
	
	// "dbg_AppendToTrace( gl_PrimitiveID, location )"
	{
		TIntermSymbol*	primitive	= dbgInfo.GetCachedSymbolNode( "gl_PrimitiveID" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( primitive, loc );

		body->getSequence().push_back( CreateAppendToTrace( primitive, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_TessCoord, location )"
	{
		TIntermSymbol*	tess_coord	= dbgInfo.GetCachedSymbolNode( "gl_TessCoord" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( tess_coord, loc );

		body->getSequence().push_back( CreateAppendToTrace( tess_coord, loc_id, dbgInfo ));
	}
	
	TPublicType		index_type;		index_type.init({});
	index_type.basicType			= TBasicType::EbtInt;
	index_type.qualifier.storage	= TStorageQualifier::EvqConst;

	// "dbg_AppendToTrace( gl_TessLevelInner, location )"
	{
		TIntermSymbol*		inner_level	= dbgInfo.GetCachedSymbolNode( "gl_TessLevelInner" );
		const uint32_t		loc_id		= dbgInfo.GetCustomSourceLocation( inner_level, loc );
		TIntermAggregate*	vec2_ctor	= new TIntermAggregate{ TOperator::EOpConstructVec2 };
		TPublicType			float_type;	float_type.init({});
		float_type.basicType			= TBasicType::EbtFloat;
		float_type.qualifier.storage	= TStorageQualifier::EvqTemporary;
		float_type.qualifier.builtIn	= TBuiltInVariable::EbvTessLevelInner;
		float_type.qualifier.precision	= TPrecisionQualifier::EpqHigh;

		for (int i = 0; i < 2; ++i) {
			TIntermBinary*			elem_access		= new TIntermBinary{ TOperator::EOpIndexDirect };
			TConstUnionArray		elem_Value(1);	elem_Value[0].setIConst( i );
			TIntermConstantUnion*	elem_index		= new TIntermConstantUnion{ elem_Value, TType{index_type} };
			elem_access->setType( TType{float_type} );
			elem_access->setLeft( inner_level );
			elem_access->setRight( elem_index );
			vec2_ctor->getSequence().push_back( elem_access );
		}

		float_type.vectorSize			= 2;
		float_type.qualifier.builtIn	= TBuiltInVariable::EbvNone;
		float_type.qualifier.precision	= TPrecisionQualifier::EpqNone;

		vec2_ctor->setType( TType{float_type} );
		body->getSequence().push_back( CreateAppendToTrace( vec2_ctor, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_TessLevelOuter, location )"
	{
		TIntermSymbol*		outer_level	= dbgInfo.GetCachedSymbolNode( "gl_TessLevelOuter" );
		const uint32_t		loc_id		= dbgInfo.GetCustomSourceLocation( outer_level, loc );
		TIntermAggregate*	vec4_ctor	= new TIntermAggregate{ TOperator::EOpConstructVec4 };
		TPublicType			float_type;	float_type.init({});
		float_type.basicType			= TBasicType::EbtFloat;
		float_type.qualifier.storage	= TStorageQualifier::EvqTemporary;
		float_type.qualifier.builtIn	= TBuiltInVariable::EbvTessLevelOuter;
		float_type.qualifier.precision	= TPrecisionQualifier::EpqHigh;

		for (int i = 0; i < 4; ++i) {
			TIntermBinary*			elem_access		= new TIntermBinary{ TOperator::EOpIndexDirect };
			TConstUnionArray		elem_Value(1);	elem_Value[0].setIConst( i );
			TIntermConstantUnion*	elem_index		= new TIntermConstantUnion{ elem_Value, TType{index_type} };
			elem_access->setType( TType{float_type} );
			elem_access->setLeft( outer_level );
			elem_access->setRight( elem_index );
			vec4_ctor->getSequence().push_back( elem_access );
		}
		
		float_type.vectorSize			= 4;
		float_type.qualifier.builtIn	= TBuiltInVariable::EbvNone;
		float_type.qualifier.precision	= TPrecisionQualifier::EpqNone;

		vec4_ctor->setType( TType{float_type} );
		body->getSequence().push_back( CreateAppendToTrace( vec4_ctor, loc_id, dbgInfo ));
	}

	return body;
}

/*
=================================================
	RecordGeometryShaderInfo
=================================================
*/
TIntermAggregate*  RecordGeometryShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );
	
	// "dbg_AppendToTrace( gl_InvocationID, location )"
	{
		TIntermSymbol*	invocation	= dbgInfo.GetCachedSymbolNode( "gl_InvocationID" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( invocation, loc );

		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_PrimitiveIDIn, location )"
	{
		TIntermSymbol*	primitive	= dbgInfo.GetCachedSymbolNode( "gl_PrimitiveIDIn" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( primitive, loc );

		body->getSequence().push_back( CreateAppendToTrace( primitive, loc_id, dbgInfo ));
	}

	return body;
}

/*
=================================================
	RecordFragmentShaderInfo
=================================================
*/
TIntermAggregate*  RecordFragmentShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );

	// "dbg_AppendToTrace( gl_FragCoord, location )"
	{
		TIntermSymbol*	fragcoord	= dbgInfo.GetCachedSymbolNode( "gl_FragCoord" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( fragcoord, loc );

		body->getSequence().push_back( CreateAppendToTrace( fragcoord, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_SampleID, location )"
	if ( TIntermSymbol*  sample_id = dbgInfo.GetCachedSymbolNode( "gl_SampleID" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( sample_id, loc );
		body->getSequence().push_back( CreateAppendToTrace( sample_id, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_SamplePosition, location )"
	if ( TIntermSymbol*  sample_pos = dbgInfo.GetCachedSymbolNode( "gl_SamplePosition" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( sample_pos, loc );
		body->getSequence().push_back( CreateAppendToTrace( sample_pos, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_PrimitiveID, location )"
	{
		TIntermSymbol*	primitive_id	= dbgInfo.GetCachedSymbolNode( "gl_PrimitiveID" );
		const uint32_t	loc_id			= dbgInfo.GetCustomSourceLocation( primitive_id, loc );

		body->getSequence().push_back( CreateAppendToTrace( primitive_id, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_SamplePosition, location )"
	if ( TIntermSymbol*  layer = dbgInfo.GetCachedSymbolNode( "gl_Layer" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( layer, loc );
		body->getSequence().push_back( CreateAppendToTrace( layer, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_ViewportIndex, location )"
	if ( TIntermSymbol*  viewport_idx = dbgInfo.GetCachedSymbolNode( "gl_ViewportIndex" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( viewport_idx, loc );
		body->getSequence().push_back( CreateAppendToTrace( viewport_idx, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_SubgroupInvocationID, location )"
	if ( auto* invocation = dbgInfo.GetCachedSymbolNode( "gl_SubgroupInvocationID" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( invocation, loc );
		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}

	AppendShaderInputVaryings( body, dbgInfo );

	return body;
}

/*
=================================================
	RecordComputeShaderInfo
=================================================
*/
TIntermAggregate*  RecordComputeShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );
	
	// "dbg_AppendToTrace( gl_GlobalInvocationID, location )"
	{
		TIntermSymbol*	invocation	= dbgInfo.GetCachedSymbolNode( "gl_GlobalInvocationID" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( invocation, loc );

		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_LocalInvocationID, location )"
	{
		TIntermSymbol*	invocation	= dbgInfo.GetCachedSymbolNode( "gl_LocalInvocationID" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( invocation, loc );

		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_WorkGroupID, location )"
	{
		TIntermSymbol*	invocation	= dbgInfo.GetCachedSymbolNode( "gl_WorkGroupID" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( invocation, loc );

		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_SubgroupID, location )"
	if ( auto* invocation = dbgInfo.GetCachedSymbolNode( "gl_SubgroupID" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( invocation, loc );
		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_SubgroupInvocationID, location )"
	if ( auto* invocation = dbgInfo.GetCachedSymbolNode( "gl_SubgroupInvocationID" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( invocation, loc );
		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}

	return body;
}

/*
=================================================
	RecordRayGenShaderInfo
=================================================
*/
TIntermAggregate*  RecordRayGenShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );
	
	// "dbg_AppendToTrace( gl_LaunchIDNV, location )"
	{
		TIntermSymbol*	invocation	= dbgInfo.GetCachedSymbolNode( "gl_LaunchIDNV" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( invocation, loc );

		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}
	return body;
}

/*
=================================================
	RecordHitShaderInfo
=================================================
*/
TIntermAggregate*  RecordHitShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );
	
	// "dbg_AppendToTrace( gl_LaunchIDNV, location )"
	{
		TIntermSymbol*	invocation	= dbgInfo.GetCachedSymbolNode( "gl_LaunchIDNV" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( invocation, loc );

		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_PrimitiveID, location )"
	if ( auto* primitive = dbgInfo.GetCachedSymbolNode( "gl_PrimitiveID" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( primitive, loc );
		body->getSequence().push_back( CreateAppendToTrace( primitive, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_InstanceID, location )"
	if ( auto* instance = dbgInfo.GetCachedSymbolNode( "gl_InstanceID" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( instance, loc );
		body->getSequence().push_back( CreateAppendToTrace( instance, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_InstanceCustomIndexNV, location )"
	if ( auto* instance_index = dbgInfo.GetCachedSymbolNode( "gl_InstanceCustomIndexNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( instance_index, loc );
		body->getSequence().push_back( CreateAppendToTrace( instance_index, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_WorldRayOriginNV, location )"
	if ( auto* world_ray_origin = dbgInfo.GetCachedSymbolNode( "gl_WorldRayOriginNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( world_ray_origin, loc );
		body->getSequence().push_back( CreateAppendToTrace( world_ray_origin, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_WorldRayDirectionNV, location )"
	if ( auto* world_ray_direction = dbgInfo.GetCachedSymbolNode( "gl_WorldRayDirectionNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( world_ray_direction, loc );
		body->getSequence().push_back( CreateAppendToTrace( world_ray_direction, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_ObjectRayOriginNV, location )"
	if ( auto* object_ray_origin = dbgInfo.GetCachedSymbolNode( "gl_ObjectRayOriginNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( object_ray_origin, loc );
		body->getSequence().push_back( CreateAppendToTrace( object_ray_origin, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_ObjectRayDirectionNV, location )"
	if ( auto* object_ray_direction = dbgInfo.GetCachedSymbolNode( "gl_ObjectRayDirectionNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( object_ray_direction, loc );
		body->getSequence().push_back( CreateAppendToTrace( object_ray_direction, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_RayTminNV, location )"
	if ( auto* ray_tmin = dbgInfo.GetCachedSymbolNode( "gl_RayTminNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( ray_tmin, loc );
		body->getSequence().push_back( CreateAppendToTrace( ray_tmin, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_RayTmaxNV, location )"
	if ( auto* ray_tmax = dbgInfo.GetCachedSymbolNode( "gl_RayTmaxNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( ray_tmax, loc );
		body->getSequence().push_back( CreateAppendToTrace( ray_tmax, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_IncomingRayFlagsNV, location )"
	if ( auto* incoming_ray_flags = dbgInfo.GetCachedSymbolNode( "gl_IncomingRayFlagsNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( incoming_ray_flags, loc );
		body->getSequence().push_back( CreateAppendToTrace( incoming_ray_flags, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_HitTNV, location )"
	if ( auto* hit_t = dbgInfo.GetCachedSymbolNode( "gl_HitTNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( hit_t, loc );
		body->getSequence().push_back( CreateAppendToTrace( hit_t, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_HitKindNV, location )"
	if ( auto* hit_kind = dbgInfo.GetCachedSymbolNode( "gl_HitKindNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( hit_kind, loc );
		body->getSequence().push_back( CreateAppendToTrace( hit_kind, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_ObjectToWorldNV, location )"
	if ( auto* object_to_world = dbgInfo.GetCachedSymbolNode( "gl_ObjectToWorldNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( object_to_world, loc );
		body->getSequence().push_back( CreateAppendToTrace( object_to_world, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_WorldToObjectNV, location )"
	if ( auto* world_to_object = dbgInfo.GetCachedSymbolNode( "gl_WorldToObjectNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( world_to_object, loc );
		body->getSequence().push_back( CreateAppendToTrace( world_to_object, loc_id, dbgInfo ));
	}
	return body;
}

/*
=================================================
	RecordIntersectionShaderInfo
=================================================
*/
TIntermAggregate*  RecordIntersectionShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );
	
	// "dbg_AppendToTrace( gl_LaunchIDNV, location )"
	{
		TIntermSymbol*	invocation	= dbgInfo.GetCachedSymbolNode( "gl_LaunchIDNV" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( invocation, loc );

		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_PrimitiveID, location )"
	if ( auto* primitive = dbgInfo.GetCachedSymbolNode( "gl_PrimitiveID" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( primitive, loc );
		body->getSequence().push_back( CreateAppendToTrace( primitive, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_InstanceID, location )"
	if ( auto* instance = dbgInfo.GetCachedSymbolNode( "gl_InstanceID" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( instance, loc );
		body->getSequence().push_back( CreateAppendToTrace( instance, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_InstanceCustomIndexNV, location )"
	if ( auto* instance_index = dbgInfo.GetCachedSymbolNode( "gl_InstanceCustomIndexNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( instance_index, loc );
		body->getSequence().push_back( CreateAppendToTrace( instance_index, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_WorldRayOriginNV, location )"
	if ( auto* world_ray_origin = dbgInfo.GetCachedSymbolNode( "gl_WorldRayOriginNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( world_ray_origin, loc );
		body->getSequence().push_back( CreateAppendToTrace( world_ray_origin, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_WorldRayDirectionNV, location )"
	if ( auto* world_ray_dir = dbgInfo.GetCachedSymbolNode( "gl_WorldRayDirectionNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( world_ray_dir, loc );
		body->getSequence().push_back( CreateAppendToTrace( world_ray_dir, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_ObjectRayOriginNV, location )"
	if ( auto* object_ray_origin = dbgInfo.GetCachedSymbolNode( "gl_ObjectRayOriginNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( object_ray_origin, loc );
		body->getSequence().push_back( CreateAppendToTrace( object_ray_origin, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_ObjectRayDirectionNV, location )"
	if ( auto* object_ray_dir = dbgInfo.GetCachedSymbolNode( "gl_ObjectRayDirectionNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( object_ray_dir, loc );
		body->getSequence().push_back( CreateAppendToTrace( object_ray_dir, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_RayTminNV, location )"
	if ( auto* ray_tmin = dbgInfo.GetCachedSymbolNode( "gl_RayTminNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( ray_tmin, loc );
		body->getSequence().push_back( CreateAppendToTrace( ray_tmin, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_RayTmaxNV, location )"
	if ( auto* ray_tmax = dbgInfo.GetCachedSymbolNode( "gl_RayTmaxNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( ray_tmax, loc );
		body->getSequence().push_back( CreateAppendToTrace( ray_tmax, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_IncomingRayFlagsNV, location )"
	if ( auto* incoming_ray_flags = dbgInfo.GetCachedSymbolNode( "gl_IncomingRayFlagsNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( incoming_ray_flags, loc );
		body->getSequence().push_back( CreateAppendToTrace( incoming_ray_flags, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_ObjectToWorldNV, location )"
	if ( auto* object_to_world = dbgInfo.GetCachedSymbolNode( "gl_ObjectToWorldNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( object_to_world, loc );
		body->getSequence().push_back( CreateAppendToTrace( object_to_world, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_WorldToObjectNV, location )"
	if ( auto* world_to_object = dbgInfo.GetCachedSymbolNode( "gl_WorldToObjectNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( world_to_object, loc );
		body->getSequence().push_back( CreateAppendToTrace( world_to_object, loc_id, dbgInfo ));
	}
	return body;
}

/*
=================================================
	RecordMissShaderInfo
=================================================
*/
TIntermAggregate*  RecordMissShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );
	
	// "dbg_AppendToTrace( gl_LaunchIDNV, location )"
	{
		TIntermSymbol*	invocation	= dbgInfo.GetCachedSymbolNode( "gl_LaunchIDNV" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( invocation, loc );

		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}
	
	// "dbg_AppendToTrace( gl_WorldRayOriginNV, location )"
	if ( auto* world_ray_origin = dbgInfo.GetCachedSymbolNode( "gl_WorldRayOriginNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( world_ray_origin, loc );
		body->getSequence().push_back( CreateAppendToTrace( world_ray_origin, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_WorldRayDirectionNV, location )"
	if ( auto* world_ray_direction = dbgInfo.GetCachedSymbolNode( "gl_WorldRayDirectionNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( world_ray_direction, loc );
		body->getSequence().push_back( CreateAppendToTrace( world_ray_direction, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_ObjectRayOriginNV, location )"
	if ( auto* obj_ray_origin = dbgInfo.GetCachedSymbolNode( "gl_ObjectRayOriginNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( obj_ray_origin, loc );
		body->getSequence().push_back( CreateAppendToTrace( obj_ray_origin, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_ObjectRayDirectionNV, location )"
	if ( auto* obj_ray_direction = dbgInfo.GetCachedSymbolNode( "gl_ObjectRayDirectionNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( obj_ray_direction, loc );
		body->getSequence().push_back( CreateAppendToTrace( obj_ray_direction, loc_id, dbgInfo ));
	}
	return body;
}

/*
=================================================
	RecordCallableShaderInfo
=================================================
*/
TIntermAggregate*  RecordCallableShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	body = new TIntermAggregate{ TOperator::EOpSequence };
	body->setType( TType{EbtVoid} );
	
	// "dbg_AppendToTrace( gl_LaunchIDNV, location )"
	{
		TIntermSymbol*	invocation	= dbgInfo.GetCachedSymbolNode( "gl_LaunchIDNV" );
		const uint32_t	loc_id		= dbgInfo.GetCustomSourceLocation( invocation, loc );

		body->getSequence().push_back( CreateAppendToTrace( invocation, loc_id, dbgInfo ));
	}

	// "dbg_AppendToTrace( gl_IncomingRayFlagsNV, location )"
	if ( auto* incoming_ray_flags = dbgInfo.GetCachedSymbolNode( "gl_IncomingRayFlagsNV" ))
	{
		const uint32_t	loc_id = dbgInfo.GetCustomSourceLocation( incoming_ray_flags, loc );
		body->getSequence().push_back( CreateAppendToTrace( incoming_ray_flags, loc_id, dbgInfo ));
	}
	return body;
}

/*
=================================================
	RecordShaderInfo
=================================================
*/
TIntermAggregate*  RecordShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	switch ( dbgInfo.GetShaderType() )
	{
		case EShLangVertex :			return RecordVertexShaderInfo( loc, dbgInfo );
		case EShLangTessControl :		return RecordTessControlShaderInfo( loc, dbgInfo );
		case EShLangTessEvaluation :	return RecordTessEvaluationShaderInfo( loc, dbgInfo );
		case EShLangGeometry :			return RecordGeometryShaderInfo( loc, dbgInfo );
		case EShLangFragment :			return RecordFragmentShaderInfo( loc, dbgInfo );
		case EShLangTaskNV :
		case EShLangMeshNV :
		case EShLangCompute :			return RecordComputeShaderInfo( loc, dbgInfo );
		case EShLangRayGenNV :			return RecordRayGenShaderInfo( loc, dbgInfo );
		case EShLangAnyHitNV :
		case EShLangClosestHitNV :		return RecordHitShaderInfo( loc, dbgInfo );
		case EShLangIntersectNV :		return RecordIntersectionShaderInfo( loc, dbgInfo );
		case EShLangMissNV :			return RecordMissShaderInfo( loc, dbgInfo );
		case EShLangCallableNV :		return RecordCallableShaderInfo( loc, dbgInfo );
	}
	return nullptr;
}

/*
=================================================
	CreateFragmentShaderIsDebugInvocation
=================================================
*/
TIntermOperator*  CreateFragmentShaderIsDebugInvocation (DebugInfo &dbgInfo)
{
	TPublicType		bool_type;	bool_type.init({});
	bool_type.basicType			= TBasicType::EbtBool;
	bool_type.qualifier.storage	= TStorageQualifier::EvqTemporary;

	TPublicType		int_type;	int_type.init({});
	int_type.basicType			= TBasicType::EbtInt;
	int_type.qualifier.storage	= TStorageQualifier::EvqTemporary;
	
	TPublicType		float_type;	 float_type.init({});
	float_type.basicType		 = TBasicType::EbtFloat;
	float_type.qualifier.storage = TStorageQualifier::EvqTemporary;
	
	TPublicType		index_type;	 index_type.init({});
	index_type.basicType		 = TBasicType::EbtInt;
	index_type.qualifier.storage = TStorageQualifier::EvqConst;
	
	TIntermBinary*	eq1 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.fragCoordX
		TIntermBinary*			fscoord_x	= dbgInfo.GetDebugStorageField( "fragCoordX" );
		CHECK_ERR( fscoord_x );

		// gl_FragCoord.x
		TConstUnionArray		x_index(1);	x_index[0].setIConst( 0 );
		TIntermConstantUnion*	x_field		= new TIntermConstantUnion{ x_index, TType{index_type} };
		TIntermBinary*			frag_x		= new TIntermBinary{ TOperator::EOpIndexDirect };
		frag_x->setType( TType{float_type} );
		frag_x->setLeft( dbgInfo.GetCachedSymbolNode( "gl_FragCoord" ));
		frag_x->setRight( x_field );

		// int(gl_FragCoord.x)
		TIntermUnary*			uint_fc_x	= new TIntermUnary{ TOperator::EOpConvFloatToInt };
		uint_fc_x->setType( TType{int_type} );
		uint_fc_x->setOperand( frag_x );

		// ... == ...
		eq1->setType( TType{bool_type} );
		eq1->setLeft( uint_fc_x );
		eq1->setRight( fscoord_x );
	}
	
	TIntermBinary*	eq2 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.fragCoordY
		TIntermBinary*			fscoord_y	= dbgInfo.GetDebugStorageField( "fragCoordY" );
		CHECK_ERR( fscoord_y );

		// gl_FragCoord.y
		TConstUnionArray		y_index(1);	y_index[0].setIConst( 1 );
		TIntermConstantUnion*	y_field		= new TIntermConstantUnion{ y_index, TType{index_type} };
		TIntermBinary*			frag_y		= new TIntermBinary{ TOperator::EOpIndexDirect };
		frag_y->setType( TType{float_type} );
		frag_y->setLeft( dbgInfo.GetCachedSymbolNode( "gl_FragCoord" ));
		frag_y->setRight( y_field );

		// int(gl_FragCoord.y)
		TIntermUnary*			uint_fc_y	= new TIntermUnary{ TOperator::EOpConvFloatToInt };
		uint_fc_y->setType( TType{int_type} );
		uint_fc_y->setOperand( frag_y );
	
		// ... == ...
		eq2->setType( TType{bool_type} );
		eq2->setLeft( uint_fc_y );
		eq2->setRight( fscoord_y );
	}

	// ... && ...
	TIntermBinary*	cmp1		= new TIntermBinary{ TOperator::EOpLogicalAnd };
	cmp1->setType( TType{bool_type} );
	cmp1->setLeft( eq1 );
	cmp1->setRight( eq2 );

	return cmp1;
}

/*
=================================================
	CreateComputeShaderIsDebugInvocation
=================================================
*/
TIntermOperator*  CreateComputeShaderIsDebugInvocation (DebugInfo &dbgInfo)
{
	TPublicType		bool_type;	bool_type.init({});
	bool_type.basicType			= TBasicType::EbtBool;
	bool_type.qualifier.storage	= TStorageQualifier::EvqTemporary;

	TPublicType		uint_type;	uint_type.init({});
	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage	= TStorageQualifier::EvqTemporary;

	TPublicType		index_type;	 index_type.init({});
	index_type.basicType		 = TBasicType::EbtInt;
	index_type.qualifier.storage = TStorageQualifier::EvqConst;

	TIntermBinary*	eq1 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.globalInvocationX
		TIntermBinary*			thread_id_x	= dbgInfo.GetDebugStorageField( "globalInvocationX" );
		CHECK_ERR( thread_id_x );

		// gl_GlobalInvocationID.x
		TConstUnionArray		x_index(1);	x_index[0].setIConst( 0 );
		TIntermConstantUnion*	x_field		= new TIntermConstantUnion{ x_index, TType{index_type} };
		TIntermBinary*			ginvoc_x	= new TIntermBinary{ TOperator::EOpIndexDirect };
		ginvoc_x->setType( TType{uint_type} );
		ginvoc_x->setLeft( dbgInfo.GetCachedSymbolNode( "gl_GlobalInvocationID" ));
		ginvoc_x->setRight( x_field );
	
		// ... == ...
		eq1->setType( TType{bool_type} );
		eq1->setLeft( thread_id_x );
		eq1->setRight( ginvoc_x );
	}
	
	TIntermBinary*	eq2 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.globalInvocationY
		TIntermBinary*			thread_id_y	= dbgInfo.GetDebugStorageField( "globalInvocationY" );
		CHECK_ERR( thread_id_y );

		// gl_GlobalInvocationID.y
		TConstUnionArray		y_index(1);	y_index[0].setIConst( 1 );
		TIntermConstantUnion*	y_field		= new TIntermConstantUnion{ y_index, TType{index_type} };
		TIntermBinary*			ginvoc_y	= new TIntermBinary{ TOperator::EOpIndexDirect };
		ginvoc_y->setType( TType{uint_type} );
		ginvoc_y->setLeft( dbgInfo.GetCachedSymbolNode( "gl_GlobalInvocationID" ));
		ginvoc_y->setRight( y_field );
	
		// ... == ...
		eq2->setType( TType{bool_type} );
		eq2->setLeft( thread_id_y );
		eq2->setRight( ginvoc_y );
	}
	
	TIntermBinary*	eq3 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.globalInvocationZ
		TIntermBinary*			thread_id_z	= dbgInfo.GetDebugStorageField( "globalInvocationZ" );
		CHECK_ERR( thread_id_z );

		// gl_GlobalInvocationID.z
		TConstUnionArray		z_index(1);	z_index[0].setIConst( 2 );
		TIntermConstantUnion*	z_field		= new TIntermConstantUnion{ z_index, TType{index_type} };
		TIntermBinary*			ginvoc_z	= new TIntermBinary{ TOperator::EOpIndexDirect };
		ginvoc_z->setType( TType{uint_type} );
		ginvoc_z->setLeft( dbgInfo.GetCachedSymbolNode( "gl_GlobalInvocationID" ));
		ginvoc_z->setRight( z_field );
	
		// ... == ...
		eq3->setType( TType{bool_type} );
		eq3->setLeft( thread_id_z );
		eq3->setRight( ginvoc_z );
	}
	
	// ... && ...
	TIntermBinary*		cmp1		= new TIntermBinary{ TOperator::EOpLogicalAnd };
	cmp1->setType( TType{bool_type} );
	cmp1->setLeft( eq1 );
	cmp1->setRight( eq2 );
	
	// ... && ...
	TIntermBinary*		cmp2		= new TIntermBinary{ TOperator::EOpLogicalAnd };
	cmp2->setType( TType{bool_type} );
	cmp2->setLeft( cmp1 );
	cmp2->setRight( eq3 );

	return cmp2;
}

/*
=================================================
	CreateRayTracingShaderIsDebugInvocation
=================================================
*/
TIntermOperator*  CreateRayTracingShaderIsDebugInvocation (DebugInfo &dbgInfo)
{	
	TPublicType		bool_type;	bool_type.init({});
	bool_type.basicType			= TBasicType::EbtBool;
	bool_type.qualifier.storage	= TStorageQualifier::EvqTemporary;

	TPublicType		uint_type;	uint_type.init({});
	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage	= TStorageQualifier::EvqTemporary;

	TPublicType		index_type;	 index_type.init({});
	index_type.basicType		 = TBasicType::EbtInt;
	index_type.qualifier.storage = TStorageQualifier::EvqConst;

	TIntermBinary*	eq1 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.launchID_x
		TIntermBinary*			thread_id_x	= dbgInfo.GetDebugStorageField( "launchID_x" );
		CHECK_ERR( thread_id_x );

		// gl_LaunchIDNV.x
		TConstUnionArray		x_index(1);	x_index[0].setIConst( 0 );
		TIntermConstantUnion*	x_field		= new TIntermConstantUnion{ x_index, TType{index_type} };
		TIntermBinary*			launch_x	= new TIntermBinary{ TOperator::EOpIndexDirect };
		launch_x->setType( TType{uint_type} );
		launch_x->setLeft( dbgInfo.GetCachedSymbolNode( "gl_LaunchIDNV" ));
		launch_x->setRight( x_field );
	
		// ... == ...
		eq1->setType( TType{bool_type} );
		eq1->setLeft( thread_id_x );
		eq1->setRight( launch_x );
	}
	
	TIntermBinary*	eq2 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.launchID_y
		TIntermBinary*			thread_id_y	= dbgInfo.GetDebugStorageField( "launchID_y" );
		CHECK_ERR( thread_id_y );

		// gl_LaunchIDNV.y
		TConstUnionArray		y_index(1);	y_index[0].setIConst( 1 );
		TIntermConstantUnion*	y_field		= new TIntermConstantUnion{ y_index, TType{index_type} };
		TIntermBinary*			launch_y	= new TIntermBinary{ TOperator::EOpIndexDirect };
		launch_y->setType( TType{uint_type} );
		launch_y->setLeft( dbgInfo.GetCachedSymbolNode( "gl_LaunchIDNV" ));
		launch_y->setRight( y_field );
	
		// ... == ...
		eq2->setType( TType{bool_type} );
		eq2->setLeft( thread_id_y );
		eq2->setRight( launch_y );
	}
	
	TIntermBinary*	eq3 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.launchID_z
		TIntermBinary*			thread_id_z	= dbgInfo.GetDebugStorageField( "launchID_z" );
		CHECK_ERR( thread_id_z );

		// gl_LaunchIDNV.z
		TConstUnionArray		z_index(1);	z_index[0].setIConst( 2 );
		TIntermConstantUnion*	z_field		= new TIntermConstantUnion{ z_index, TType{index_type} };
		TIntermBinary*			launch_z	= new TIntermBinary{ TOperator::EOpIndexDirect };
		launch_z->setType( TType{uint_type} );
		launch_z->setLeft( dbgInfo.GetCachedSymbolNode( "gl_LaunchIDNV" ));
		launch_z->setRight( z_field );
	
		// ... == ...
		eq3->setType( TType{bool_type} );
		eq3->setLeft( thread_id_z );
		eq3->setRight( launch_z );
	}
	
	// ... && ...
	TIntermBinary*		cmp1		= new TIntermBinary{ TOperator::EOpLogicalAnd };
	cmp1->setType( TType{bool_type} );
	cmp1->setLeft( eq1 );
	cmp1->setRight( eq2 );
	
	// ... && ...
	TIntermBinary*		cmp2		= new TIntermBinary{ TOperator::EOpLogicalAnd };
	cmp2->setType( TType{bool_type} );
	cmp2->setLeft( cmp1 );
	cmp2->setRight( eq3 );

	return cmp2;
}

/*
=================================================
	InsertGlobalVariablesAndBuffers
=================================================
*/
bool  InsertGlobalVariablesAndBuffers (TIntermAggregate* linkerObjs, TIntermAggregate* globalVars, uint32_t initialPosition, DebugInfo &dbgInfo)
{
	// "bool dbg_IsEnabled"
	TPublicType		type;	type.init({});
	type.basicType			= TBasicType::EbtBool;
	type.qualifier.storage	= TStorageQualifier::EvqGlobal;

	TIntermSymbol*			is_debug_enabled = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "dbg_IsEnabled", TType{type} };
	dbgInfo.CacheSymbolNode( is_debug_enabled, true );
	linkerObjs->getSequence().insert( linkerObjs->getSequence().begin(), is_debug_enabled );
	
	// "dbg_IsEnabled = ..."
	TIntermBinary*			init_debug_enabled = new TIntermBinary{ TOperator::EOpAssign };
	type.qualifier.storage = TStorageQualifier::EvqTemporary;
	init_debug_enabled->setType( TType{type} );
	init_debug_enabled->setLeft( is_debug_enabled );
	globalVars->getSequence().insert( globalVars->getSequence().begin(), init_debug_enabled );
	
	switch ( dbgInfo.GetShaderType() )
	{
		case EShLangVertex :
		case EShLangTessControl :
		case EShLangTessEvaluation :
		case EShLangGeometry :
		case EShLangTaskNV :
		case EShLangMeshNV :
		{
			type.qualifier.storage	= TStorageQualifier::EvqConst;
			TConstUnionArray		false_value(1);	false_value[0].setBConst( false );
			TIntermConstantUnion*	const_false		= new TIntermConstantUnion{ false_value, TType{type} };
			
			init_debug_enabled->setRight( const_false );
			break;
		}

		case EShLangFragment :		init_debug_enabled->setRight( CreateFragmentShaderIsDebugInvocation( dbgInfo ));	break;
		case EShLangCompute :		init_debug_enabled->setRight( CreateComputeShaderIsDebugInvocation( dbgInfo ));		break;
		
		case EShLangRayGenNV :
		case EShLangIntersectNV :
		case EShLangAnyHitNV :
		case EShLangClosestHitNV :
		case EShLangMissNV :
		case EShLangCallableNV :	init_debug_enabled->setRight( CreateRayTracingShaderIsDebugInvocation( dbgInfo ));		break;

		default :					RETURN_ERR( "not supported" );
	}


	// "uint dbg_LastPosition"
	type.basicType			= TBasicType::EbtUint;
	type.qualifier.storage	= TStorageQualifier::EvqGlobal;

	TIntermSymbol*			last_pos		= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "dbg_LastPosition", TType{type} };
	dbgInfo.CacheSymbolNode( last_pos, true );
	linkerObjs->getSequence().insert( linkerObjs->getSequence().begin(), last_pos );
	
	// "dbg_LastPosition = ~0u"
	type.qualifier.storage = TStorageQualifier::EvqConst;
	TConstUnionArray		init_value(1);	init_value[0].setUConst( initialPosition );
	TIntermConstantUnion*	init_const		= new TIntermConstantUnion{ init_value, TType{type} };
	TIntermBinary*			init_pos		= new TIntermBinary{ TOperator::EOpAssign };
		
	type.qualifier.storage = TStorageQualifier::EvqTemporary;
	init_pos->setType( TType{type} );
	init_pos->setLeft( last_pos );
	init_pos->setRight( init_const );
	globalVars->getSequence().insert( globalVars->getSequence().begin(), init_pos );
	

	linkerObjs->getSequence().insert( linkerObjs->getSequence().begin(), dbgInfo.GetDebugStorage() );
	return true;
}

/*
=================================================
	CreateEnableIfBody
=================================================
*/
bool  CreateEnableIfBody (TIntermAggregate* fnDecl, DebugInfo &dbgInfo)
{
	CHECK_ERR( fnDecl->getName() == "dbg_EnableProfiling(b1;" );
	CHECK_ERR( fnDecl->getSequence().size() >= 1 );
	
	auto*	params = fnDecl->getSequence()[0]->getAsAggregate();
	CHECK_ERR( params and params->getOp() == TOperator::EOpParameters );

	TIntermTyped*		arg		= params->getSequence()[0]->getAsTyped();
	CHECK_ERR( arg );

	TIntermAggregate*	body	= nullptr;

	if ( fnDecl->getSequence().size() == 1 )
	{
		body = new TIntermAggregate{ TOperator::EOpSequence };
		fnDecl->getSequence().push_back( body );
	}
	else
	{
		body = fnDecl->getSequence()[1]->getAsAggregate();
		CHECK_ERR( body );
	}
	
	TIntermSymbol*		is_debug_enabled = dbgInfo.GetCachedSymbolNode( "dbg_IsEnabled" );
	TIntermAggregate*	branch_body		 = RecordShaderInfo( TSourceLoc{}, dbgInfo );
	CHECK_ERR( is_debug_enabled and branch_body );

	TPublicType		type;	type.init({});
	type.basicType			= TBasicType::EbtBool;
	type.qualifier.storage	= TStorageQualifier::EvqTemporary;

	// "not dbg_IsEnabled"
	TIntermUnary*		condition	= new TIntermUnary{ TOperator::EOpLogicalNot };
	condition->setType( TType{type} );
	condition->setOperand( is_debug_enabled );

	// "if ( not dbg_IsEnabled ) {...}"
	TIntermSelection*	selection	= new TIntermSelection{ arg, branch_body, nullptr };
	selection->setType( TType{EbtVoid} );
	body->getSequence().push_back( selection );

	// "dbg_IsEnabled = arg"
	TIntermBinary*	assign = new TIntermBinary{ TOperator::EOpAssign };
	assign->setType( TType{type} );
	assign->setLeft( is_debug_enabled );
	assign->setRight( arg );
	branch_body->getSequence().insert( branch_body->getSequence().begin(), assign );

	return true;
}

/*
=================================================
	CreateDebugTraceFunctions
=================================================
*/
bool  CreateDebugTraceFunctions (TIntermNode* root, uint32_t initialPosition, DebugInfo &dbgInfo)
{
	TIntermAggregate*	aggr = root->getAsAggregate();
	CHECK_ERR( aggr );

	TIntermAggregate*	linker_objs	= nullptr;
	TIntermAggregate*	global_vars	= nullptr;
	
	for (auto& entry : aggr->getSequence())
	{
		if ( auto*  aggr2 = entry->getAsAggregate() )
		{
			if ( aggr2->getOp() == TOperator::EOpLinkerObjects )
				linker_objs = aggr2;

			if ( aggr2->getOp() == TOperator::EOpSequence )
				global_vars = aggr2;
		}
	}
	
	if ( not linker_objs ) {
		linker_objs = new TIntermAggregate{ TOperator::EOpLinkerObjects };
		aggr->getSequence().push_back( linker_objs );
	}

	if ( not global_vars ) {
		global_vars = new TIntermAggregate{ TOperator::EOpSequence };
		aggr->getSequence().push_back( global_vars );
	}

	CHECK_ERR( InsertGlobalVariablesAndBuffers( linker_objs, global_vars, initialPosition, dbgInfo ));

	for (auto& entry : aggr->getSequence())
	{
		auto*	aggr2 = entry->getAsAggregate();
		if ( not (aggr2 and aggr2->getOp() == TOperator::EOpFunction) )
			continue;
			
		if ( aggr2->getName() == "main("	and
			 aggr2->getSequence().size() >= 2 )
		{
			auto*	body = aggr2->getSequence()[1]->getAsAggregate();
			CHECK_ERR( body );

			auto*	shader_info = RecordShaderInfo( TSourceLoc{}, dbgInfo );
			CHECK_ERR( shader_info );

			body->getSequence().insert( body->getSequence().begin(), shader_info );
		}
		else
		if ( aggr2->getName().rfind( "dbg_EnableProfiling(", 0 ) == 0 )
		{
			CHECK_ERR( CreateEnableIfBody( INOUT aggr2, dbgInfo ));
		}
	}

	for (auto& fn : dbgInfo.GetRequiredFunctions())
	{
		if ( fn == "dbg_AppendToTrace(u1;" )
		{
			auto*	body = CreateAppendToTraceBody2( dbgInfo );
			CHECK_ERR( body );

			aggr->getSequence().push_back( body );
		}
		else
		if ( fn.rfind( "dbg_AppendToTrace(", 0 ) == 0 )
		{
			auto*	body = CreateAppendToTraceBody( fn, dbgInfo );
			CHECK_ERR( body );

			aggr->getSequence().push_back( body );
		}
		else
		if ( fn == "dbg_AddTimeToTrace(vu4;u1;" )
		{
			auto*	body = CreateAddTimeToTraceBody2( dbgInfo );
			CHECK_ERR( body );

			aggr->getSequence().push_back( body );
		}
		else
		if ( fn.rfind( "dbg_AddTimeToTrace(", 0 ) == 0 )
		{
			auto*	body = CreateAddTimeToTraceBody( fn, dbgInfo );
			CHECK_ERR( body );

			aggr->getSequence().push_back( body );
		}
		else
		if ( fn == "dbg_GetCurrentTime(" )
		{
			auto*	body = CreateGetCurrentTimeBody( dbgInfo );
			CHECK_ERR( body );

			aggr->getSequence().push_back( body );
		}
		else
			RETURN_ERR( "unknown function" );
	}
	return true;
}

/*
=================================================
	CreateAppendToTrace2
----
	'sourceLoc' - location index returned by DebugInfo::GetSourceLocation or DebugInfo::GetCustomSourceLocation.
	also see 'CreateAppendToTraceBody2()'
=================================================
*/
TIntermAggregate*  CreateAppendToTrace2 (uint32_t sourceLoc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	fcall = new TIntermAggregate( TOperator::EOpFunctionCall );

	fcall->setUserDefined();
	fcall->setName( "dbg_AppendToTrace(u1;" );
	fcall->setType( TType{ TBasicType::EbtVoid, TStorageQualifier::EvqGlobal });
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	
	TPublicType		uint_type;		uint_type.init({});
	uint_type.basicType				= TBasicType::EbtUint;
	uint_type.qualifier.storage		= TStorageQualifier::EvqConst;

	TConstUnionArray		loc_value(1);	loc_value[0].setUConst( sourceLoc );
	TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };
	
	loc_const->setLoc({});
	fcall->getSequence().push_back( loc_const );

	dbgInfo.RequestFunc( fcall->getName() );

	return fcall;
}

/*
=================================================
	CreateAppendToTrace
----
	returns new node instead of 'exprNode'.
	'exprNode' - any operator.
	'sourceLoc' - location index returned by DebugInfo::GetSourceLocation or DebugInfo::GetCustomSourceLocation.
	also see 'CreateAppendToTraceBody()'
=================================================
*/
TIntermAggregate*  CreateAppendToTrace (TIntermTyped* exprNode, uint32_t sourceLoc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	fcall		= new TIntermAggregate( TOperator::EOpFunctionCall );
	std::string			type_name;
	const TType &		type		= exprNode->getType();

	switch ( type.getBasicType() )
	{
		case TBasicType::EbtFloat :		type_name = "f";	break;
		case TBasicType::EbtInt :		type_name = "i";	break;
		case TBasicType::EbtUint :		type_name = "u";	break;
		case TBasicType::EbtBool :		type_name = "b";	break;
		case TBasicType::EbtDouble :	type_name = "d";	break;
		case TBasicType::EbtInt64 :		type_name = "i64";	break;
		case TBasicType::EbtUint64 :	type_name = "u64";	break;
			
		case TBasicType::EbtSampler :	return nullptr;
		case TBasicType::EbtStruct :	return nullptr;

		case TBasicType::EbtVoid :
		case TBasicType::EbtFloat16 :
		case TBasicType::EbtInt8 :
		case TBasicType::EbtUint8 :
		case TBasicType::EbtInt16 :
		case TBasicType::EbtUint16 :
		case TBasicType::EbtAtomicUint :
		case TBasicType::EbtBlock :
		case TBasicType::EbtAccStructNV :
		case TBasicType::EbtString :
		case TBasicType::EbtNumTypes :
		default :						RETURN_ERR( "not supported" );
	}
	
	if ( type.isArray() )
		RETURN_ERR( "arrays is not supported yet" )
	else
	if ( type.isMatrix() )
		type_name = "m" + type_name + std::to_string(type.getMatrixCols()) + std::to_string(type.getMatrixRows());
	else
	if ( type.isVector() )
		type_name = "v" + type_name + std::to_string(type.getVectorSize());
	else
	if ( type.isScalarOrVec1() )
		type_name += "1";
	else
		RETURN_ERR( "unknown type" );


	fcall->setLoc( exprNode->getLoc() );
	fcall->setUserDefined();
	fcall->setName( TString{"dbg_AppendToTrace("} + TString{type_name.c_str()} + ";u1;" );
	fcall->setType( exprNode->getType() );
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	fcall->getSequence().push_back( exprNode );
	
	TPublicType		uint_type;		uint_type.init( exprNode->getLoc() );
	uint_type.basicType				= TBasicType::EbtUint;
	uint_type.qualifier.storage		= TStorageQualifier::EvqConst;

	TConstUnionArray		loc_value(1);	loc_value[0].setUConst( sourceLoc );
	TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };
	
	loc_const->setLoc( exprNode->getLoc() );
	fcall->getSequence().push_back( loc_const );

	dbgInfo.RequestFunc( fcall->getName() );

	return fcall;
}

/*
=================================================
	CreateAddTimeToTrace2
----
	also see 'CreateAddTimeToTraceBody2()'
=================================================
*/
TIntermAggregate*  CreateAddTimeToTrace2 (TIntermSymbol* startTimeNode, DebugInfo &dbgInfo)
{
	TIntermAggregate*	fcall = new TIntermAggregate( TOperator::EOpFunctionCall );

	fcall->setUserDefined();
	fcall->setName( TString{"dbg_AddTimeToTrace(vu4;u1;"} );
	fcall->setType( TType{ TBasicType::EbtVoid, TStorageQualifier::EvqGlobal });
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	
	TPublicType		uint_type;		uint_type.init({});
	uint_type.basicType				= TBasicType::EbtUint;
	uint_type.qualifier.storage		= TStorageQualifier::EvqConst;

	auto	end_loc = startTimeNode->getLoc();
	end_loc.column = 0;

	const uint32_t			source_loc		= dbgInfo.GetCustomSourceLocation2( startTimeNode, startTimeNode->getLoc(), end_loc );
	TConstUnionArray		loc_value(1);	loc_value[0].setUConst( source_loc );
	TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };
	
	loc_const->setLoc({});
	fcall->getSequence().push_back( startTimeNode );
	fcall->getSequence().push_back( loc_const );

	dbgInfo.RequestFunc( fcall->getName() );

	return fcall;
}

/*
=================================================
	CreateAddTimeToTrace
----
	returns new node instead of 'exprNode'.
	'exprNode' - any operator.
	also see 'CreateAddTimeToTraceBody()'
=================================================
*/
TIntermAggregate*  CreateAddTimeToTrace (TIntermTyped* exprNode, TIntermSymbol* startTimeNode, DebugInfo &dbgInfo)
{
	TIntermAggregate*	fcall		= new TIntermAggregate( TOperator::EOpFunctionCall );
	std::string			type_name;
	const TType &		type		= exprNode->getType();

	switch ( type.getBasicType() )
	{
		case TBasicType::EbtFloat :		type_name = "f";	break;
		case TBasicType::EbtInt :		type_name = "i";	break;
		case TBasicType::EbtUint :		type_name = "u";	break;
		case TBasicType::EbtBool :		type_name = "b";	break;
		case TBasicType::EbtDouble :	type_name = "d";	break;
		case TBasicType::EbtInt64 :		type_name = "i64";	break;
		case TBasicType::EbtUint64 :	type_name = "u64";	break;
			
		case TBasicType::EbtSampler :	return nullptr;
		case TBasicType::EbtStruct :	return nullptr;

		case TBasicType::EbtVoid :
		case TBasicType::EbtFloat16 :
		case TBasicType::EbtInt8 :
		case TBasicType::EbtUint8 :
		case TBasicType::EbtInt16 :
		case TBasicType::EbtUint16 :
		case TBasicType::EbtAtomicUint :
		case TBasicType::EbtBlock :
		case TBasicType::EbtAccStructNV :
		case TBasicType::EbtString :
		case TBasicType::EbtNumTypes :
		default :						RETURN_ERR( "not supported" );
	}
	
	if ( type.isArray() )
		RETURN_ERR( "arrays is not supported yet" )
	else
	if ( type.isMatrix() )
		type_name = "m" + type_name + std::to_string(type.getMatrixCols()) + std::to_string(type.getMatrixRows());
	else
	if ( type.isVector() )
		type_name = "v" + type_name + std::to_string(type.getVectorSize());
	else
	if ( type.isScalarOrVec1() )
		type_name += "1";
	else
		RETURN_ERR( "unknown type" );
	
	fcall->setLoc( exprNode->getLoc() );
	fcall->setUserDefined();
	fcall->setName( TString{"dbg_AddTimeToTrace(vu4;"} + TString{type_name.c_str()} + ";u1;" );
	fcall->setType( exprNode->getType() );
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	fcall->getSequence().push_back( startTimeNode );
	fcall->getSequence().push_back( exprNode );
	
	TPublicType		uint_type;		uint_type.init( exprNode->getLoc() );
	uint_type.basicType				= TBasicType::EbtUint;
	uint_type.qualifier.storage		= TStorageQualifier::EvqConst;
	
	auto	end_loc = startTimeNode->getLoc();
	end_loc.column = 0;

	const uint32_t			source_loc		= dbgInfo.GetCustomSourceLocation2( startTimeNode, startTimeNode->getLoc(), end_loc );
	TConstUnionArray		loc_value(1);	loc_value[0].setUConst( source_loc );
	TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };
	
	loc_const->setLoc( exprNode->getLoc() );
	fcall->getSequence().push_back( loc_const );

	dbgInfo.RequestFunc( fcall->getName() );

	return fcall;
}

/*
=================================================
	AssignClock
----
	also see 'CreateGetCurrentTimeBody'
=================================================
*/
TIntermBinary*  AssignClock (TIntermSymbol* dst, const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	fcall		= new TIntermAggregate( TOperator::EOpFunctionCall );
	TPublicType			uint_type;	uint_type.init({});
	uint_type.basicType				= TBasicType::EbtUint;
	uint_type.vectorSize			= 4;
	uint_type.qualifier.storage		= TStorageQualifier::EvqTemporary;
	uint_type.qualifier.precision	= TPrecisionQualifier::EpqHigh;

	fcall->setLoc( loc );
	fcall->setUserDefined();
	fcall->setName( "dbg_GetCurrentTime(" );
	fcall->setType( TType{uint_type} );
		
	TIntermBinary*		assign_time	= new TIntermBinary{ TOperator::EOpAssign };
	assign_time->setLoc( loc );
	assign_time->setType( TType{uint_type} );
	assign_time->setLeft( dst );
	assign_time->setRight( fcall );

	dbgInfo.RequestFunc( fcall->getName() );

	return assign_time;
}

/*
=================================================
	ProcessFunctionDefinition
=================================================
*/
bool  ProcessFunctionDefinition (TIntermAggregate* node, DebugInfo &dbgInfo)
{
	CHECK_ERR( node->getSequence().size() >= 2 );
	CHECK_ERR( node->getSequence()[0]->getAsOperator()->getOp() == TOperator::EOpParameters );
	CHECK_ERR( node->getSequence()[1]->getAsOperator()->getOp() == TOperator::EOpSequence );

	TIntermAggregate*	body = node->getSequence()[1]->getAsAggregate();

	TIntermSymbol*		start_time_node	= dbgInfo.CreateStartTimeSymbolNode();
	start_time_node->setLoc( node->getLoc() );

	TIntermBinary*		assign_time	= AssignClock( start_time_node, body->getLoc(), dbgInfo );

	body->getSequence().insert( body->getSequence().begin(), assign_time );

	CHECK_ERR( dbgInfo.PushStartTime( start_time_node ));
	return true;
}

/*
=================================================
	ProcessFunctionDefinition2
=================================================
*/
bool  ProcessFunctionDefinition2 (TIntermAggregate* node, DebugInfo &dbgInfo)
{
	CHECK_ERR( node->getSequence().size() >= 2 );
	CHECK_ERR( node->getSequence()[0]->getAsOperator()->getOp() == TOperator::EOpParameters );
	CHECK_ERR( node->getSequence()[1]->getAsOperator()->getOp() == TOperator::EOpSequence );

	TIntermAggregate*	body		= node->getSequence()[1]->getAsAggregate();
	TIntermNode*		last_node	= body->getSequence().back();

	if ( auto* branch = last_node->getAsBranchNode(); branch and branch->getFlowOp() == TOperator::EOpReturn )
		return true;

	TIntermSymbol*		start_time	= dbgInfo.GetStartTime();
	CHECK_ERR( start_time );
	
	const uint32_t		loc_id		= dbgInfo.GetSourceLocation( last_node, last_node->getLoc() );

	body->getSequence().push_back( CreateAddTimeToTrace2( start_time, dbgInfo ));

	CHECK_ERR( dbgInfo.PopStartTime() );
	return true;
}

/*
=================================================
	RecursiveProcessSymbolNode
=================================================
*/
bool  RecursiveProcessSymbolNode (TIntermSymbol* node, DebugInfo &dbgInfo)
{
	//dbgInfo.AddSymbol( node, false );

	if ( // fragment shader
		 node->getName() == "gl_FragCoord"				or
		 node->getName() == "gl_SampleID"				or
		 node->getName() == "gl_PrimitiveID"			or
		 node->getName() == "gl_SamplePosition"			or
		 node->getName() == "gl_Layer"					or
		 node->getName() == "gl_ViewportIndex"			or
		 node->getName() == "gl_FrontFacing"			or
		 node->getName() == "gl_HelperInvocation"		or
		 // vertex shader
		 node->getName() == "gl_VertexIndex"			or
		 node->getName() == "gl_InstanceIndex"			or
		 node->getName() == "gl_DrawIDARB"				or		// requires GL_ARB_shader_draw_parameters
		 node->getName() == "gl_DrawID"					or		// requires version 460
		 // geometry shader
		 node->getName() == "gl_InvocationID"			or
		 node->getName() == "gl_PrimitiveIDIn"			or
		 // tessellation
		 //		reuse 'gl_InvocationID'
		//		reuse 'gl_PrimitiveID'
		 node->getName() == "gl_PatchVerticesIn"		or
		 node->getName() == "gl_TessCoord"				or
		 node->getName() == "gl_TessLevelInner"			or
		 node->getName() == "gl_TessLevelOuter"			or
		 // compute shader
		 node->getName() == "gl_GlobalInvocationID"		or
		 node->getName() == "gl_LocalInvocationID"		or
		 node->getName() == "gl_LocalInvocationIndex"	or
		 node->getName() == "gl_WorkGroupID"			or
		 node->getName() == "gl_NumWorkGroups"			or
		 node->getName() == "gl_WorkGroupSize"			or
		 node->getName() == "gl_SubgroupID"				or
		 // task & mesh shader
		 //		reuse 'gl_GlobalInvocationID'
		 //		reuse 'gl_LocalInvocationID'
		 //		reuse 'gl_LocalInvocationIndex'
		 //		reuse 'gl_WorkGroupID'
		 //		reuse 'gl_NumWorkGroups'
		 //		reuse 'gl_WorkGroupSize'
		 node->getName() == "gl_MeshViewCountNV"		or
		 // ray generation shader
		 node->getName() == "gl_LaunchIDNV"				or
		 node->getName() == "gl_LaunchSizeNV"			or
		 // ray intersection & any-hit & closest-hit & miss shaders
		 //		reuse 'gl_LaunchIDNV'
		 //		reuse 'gl_LaunchSizeNV'
		 //		reuse 'gl_PrimitiveID'
		 node->getName() == "gl_InstanceCustomIndexNV"	or
		 node->getName() == "gl_WorldRayOriginNV"		or
		 node->getName() == "gl_WorldRayDirectionNV"	or
		 node->getName() == "gl_ObjectRayOriginNV"		or
		 node->getName() == "gl_ObjectRayDirectionNV"	or
		 node->getName() == "gl_RayTminNV"				or
		 node->getName() == "gl_RayTmaxNV"				or
		 node->getName() == "gl_IncomingRayFlagsNV"		or
		 node->getName() == "gl_ObjectToWorldNV"		or
		 node->getName() == "gl_WorldToObjectNV"		or
		 // ray intersection & any-hit & closest-hit shaders
		 node->getName() == "gl_HitTNV"					or
		 node->getName() == "gl_HitKindNV"				or
		 node->getName() == "gl_InstanceID"				or
		 // all shaders
		 node->getName() == "gl_SubgroupInvocationID"	)
	{
		dbgInfo.CacheSymbolNode( node, false );
		return true;
	}

	// do nothing
	return true;
}
}	// namespace
//-----------------------------------------------------------------------------



/*
=================================================
	InsertFunctionProfiler
=================================================
*/
bool  ShaderTrace::InsertFunctionProfiler (TIntermediate &intermediate, uint32_t setIndex, bool shaderSubgroupClock, bool shaderDeviceClock)
{
	//CHECK_ERR( shaderSubgroupClock or shaderDeviceClock );

	if ( shaderSubgroupClock )
		intermediate.addRequestedExtension( "GL_ARB_shader_clock" );

	if ( shaderDeviceClock )
		intermediate.addRequestedExtension( "GL_EXT_shader_realtime_clock" );

	DebugInfo		dbg_info{ intermediate.getStage(), OUT _exprLocations, _fileMap, shaderSubgroupClock, shaderDeviceClock };

	TIntermNode*	root = intermediate.getTreeRoot();
	CHECK_ERR( root );

	_initialPosition = uint32_t(CombineHash( std::hash<void*>{}( this ), setIndex+1 ));
	ASSERT( _initialPosition != 0 );
	_initialPosition |= 0x80000000u;

	CreateShaderDebugStorage( setIndex, dbg_info, OUT _posOffset, OUT _dataOffset );

	dbg_info.Enter( root );
	{
		CHECK_ERR( RecursiveProcessNode( root, dbg_info ));

		CreateShaderBuiltinSymbols( root, dbg_info );

		CHECK_ERR( CreateDebugTraceFunctions( root, _initialPosition, dbg_info ));
	}
	dbg_info.Leave( root );
	
	CHECK_ERR( dbg_info.UpdateSymbolIDs() );
	CHECK_ERR( dbg_info.PostProcess( OUT _varNames ));
	return true;
}
