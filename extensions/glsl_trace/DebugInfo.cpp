// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "DebugInfo.h"

/*
=================================================
	GetSourceLocation
=================================================
*/
uint32_t  DebugInfo::GetSourceLocation (TIntermTyped* node, const TSourceLoc &curr)
{
	VariableID	id;
	_GetVariableID( node, OUT id );

	SrcPoint	point	{ uint32_t(curr.line), uint32_t(curr.column) };
	SrcLoc		range	= _callStack.back().loc;
	ASSERT( range.sourceId != ~0u );
	ASSERT( range.sourceId == uint32_t(curr.string) );

	_exprLocations.push_back({ id, range, point, {} });
	return uint32_t(_exprLocations.size()-1);
}

/*
=================================================
	GetCustomSourceLocation
=================================================
*/
uint32_t  DebugInfo::GetCustomSourceLocation (TIntermTyped* node, const TSourceLoc &curr)
{
	VariableID	id;
	_GetVariableID( node, OUT id );

	SrcLoc	range{ uint32_t(curr.string), uint32_t(curr.line), uint32_t(curr.column) };

	_exprLocations.push_back({ id, range, range.begin, {} });
	return uint32_t(_exprLocations.size()-1);
}

/*
=================================================
	AddLocation
=================================================
*/
void DebugInfo::AddLocation (const SrcLoc &src)
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
	dst.begin.value = Min( dst.begin.value,	src.begin.value );
	dst.end.value	= Max( dst.end.value,	src.end.value );
}

/*
=================================================
	SetDebugStorage
=================================================
*/
bool DebugInfo::SetDebugStorage (TIntermSymbol* symb)
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
			const size_t			index			= std::distance( _dbgStorage->getType().getStruct()->data(), &field );
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
void DebugInfo::AddSymbol (TIntermSymbol* node)
{
	ASSERT( node );
	_maxSymbolId = Max( _maxSymbolId, node->getId() );

	// register symbol
	VariableID	id;
	_GetVariableID( node, OUT id );
}

/*
=================================================
	TSourceLoc::operator ==
=================================================
*/
inline bool operator == (const TSourceLoc &lhs, const TSourceLoc &rhs)
{
	return	lhs.string	== rhs.string	and
			lhs.line	== rhs.line		and
			lhs.column	== rhs.column;
}

inline bool operator != (const TSourceLoc &lhs, const TSourceLoc &rhs)
{
	return not (lhs == rhs);
}

inline bool operator < (const TSourceLoc &lhs, const TSourceLoc &rhs)
{
	return	lhs.string	!= rhs.string	? lhs.string < rhs.string	:
			lhs.line	!= rhs.line		? lhs.line	 < rhs.line		:
										  lhs.column < rhs.column;
}

/*
=================================================
	_GetVariableID
=================================================
*/
void DebugInfo::_GetVariableID (TIntermTyped* node, OUT VariableID &id)
{
	id = VariableID(~0u);

	if ( auto* symb = node->getAsSymbolNode() )
	{
		auto	iter = _varInfos.find( VariableID(symb->getId()) );
		if ( iter == _varInfos.end() )
			iter = _varInfos.insert({ VariableID(symb->getId()), VariableInfo{symb->getName().c_str(), {}} }).first;

		auto&	locations = iter->second.locations;

		if ( locations.empty() or locations.back() != symb->getLoc() )
			locations.push_back( node->getLoc() );

		id = VariableID(symb->getId());
		return;
	}

	if ( auto* binary = node->getAsBinaryNode() )
	{
		if ( binary->getOp() == TOperator::EOpVectorSwizzle or
			(binary->getOp() == TOperator::EOpIndexDirect and not binary->getLeft()->isArray() and
			(binary->getLeft()->isScalar() or binary->getLeft()->isVector())) )
		{
			// vector swizzle
			return _GetVariableID( binary->getLeft(), OUT id );
		}
		else
		if ( binary->getOp() == TOperator::EOpIndexDirect )
		{
			// array element
			return _GetVariableID( binary->getLeft(), OUT id );
		}
		else
		if ( binary->getOp() == TOperator::EOpIndexIndirect )
		{}
		else
		if ( binary->getOp() == TOperator::EOpIndexDirectStruct )
		{}
	}
	 
	CHECK(false);
}
	
/*
=================================================
	PostProcess
=================================================
*/
bool  DebugInfo::PostProcess (OUT VarNames_t &varNames)
{
	for (auto& info : _varInfos)
	{
		std::sort(	info.second.locations.begin(), info.second.locations.end(),
					[] (auto& lhs, auto& rhs) { return lhs < rhs; }
				);

		bool	is_unused = true;
		
		for (auto& expr : _exprLocations)
		{
			if ( expr.varID == info.first ) {
				is_unused = false;
				continue;
			}

			for (auto& loc : info.second.locations)
			{
				uint32_t	loc_line	= uint32_t(loc.line);
				uint32_t	loc_column	= uint32_t(loc.column);

				if ( loc.string == 0 and loc_line == 0 and loc_column == 0 )
					continue;	// skip undefined location

				// check intersection
				if ( uint32_t(loc.string) != expr.range.sourceId )
					continue;

				if ( loc_line < expr.range.begin.Line() or loc_line > expr.range.end.Line() )
					continue;

				if ( loc_line == expr.range.begin.Line() and loc_column < expr.range.begin.Column() )
					continue;

				if ( loc_line == expr.range.end.Line() and loc_column > expr.range.end.Column() )
					continue;

				bool	exist = false;
				for (auto& v : expr.vars) {
					exist |= (v == info.first);
				}

				if ( not exist )
					expr.vars.push_back( info.first );

				is_unused = false;
			}
		}

		if ( not is_unused )
			varNames.insert_or_assign( info.first, std::move(info.second.name) );
	}

	return true;
}
