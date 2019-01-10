// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "Common.h"

// glslang includes
#include "glslang/glslang/Include/revision.h"
#include "glslang/glslang/OSDependent/osinclude.h"
#include "glslang/glslang/MachineIndependent/localintermediate.h"
#include "glslang/glslang/Include/intermediate.h"

using namespace glslang;
using VariableID = ShaderTrace::VariableID;


struct DebugInfo
{
private:
	using SrcLoc		= ShaderTrace::SourceLocation;
	using SrcPoint		= ShaderTrace::SourcePoint;
	using ExprInfos_t	= ShaderTrace::ExprInfos_t;
	using VarNames_t	= ShaderTrace::VarNames_t;

	struct StackFrame
	{
		TIntermNode*		node;
		SrcLoc				loc;

		StackFrame () {}
		explicit StackFrame (TIntermNode* node) : node{node}, loc{uint32_t(node->getLoc().string), uint32_t(node->getLoc().line), uint32_t(node->getLoc().column)} {}
	};

	struct VariableInfo
	{
		std::string				name;
		std::vector<TSourceLoc>	locations;
	};

	using SymbolLocations_t		= std::unordered_map< int, std::vector<TIntermSymbol *> >;
	using RequiredFunctions_t	= std::unordered_set< TString >;
	using CachedSymbols_t		= std::unordered_map< TString, TIntermSymbol *>;
	using VariableInfoMap_t		= std::unordered_map< VariableID, VariableInfo >;


public:
	std::vector<StackFrame>	_callStack;
	TIntermTyped *			_injection		= nullptr;

	RequiredFunctions_t		_requiredFunctions;
	CachedSymbols_t			_cachedSymbols;

	ExprInfos_t &			_exprLocations;
	VariableInfoMap_t		_varInfos;

	int						_maxSymbolId	= 0;

	TIntermSymbol *			_dbgStorage		= nullptr;

	const EShLanguage		_shLang;


public:
	DebugInfo (EShLanguage shLang, OUT ExprInfos_t &exprLoc) :
		_exprLocations{ exprLoc },
		_shLang{ shLang }
	{
		_requiredFunctions.insert( "dbg_IsDebugInvocation(" );
	}


	void Enter (TIntermNode* node)
	{
		_callStack.push_back(StackFrame{ node });
	}

	void Leave (TIntermNode* node)
	{
		CHECK( _callStack.back().node == node );
		_callStack.pop_back();
	}


	TIntermTyped*  GetInjection ()
	{
		auto	temp = _injection;
		_injection = nullptr;
		return temp;
	}

	void InjectNode (TIntermTyped *node)
	{
		if ( not node )
			return;

		CHECK( not _injection );
		_injection = node;
	}

	
	void _GetVariableID (TIntermTyped* node, OUT VariableID &id);

	uint32_t  GetSourceLocation (TIntermTyped* node, const TSourceLoc &curr);
	uint32_t  GetCustomSourceLocation (TIntermTyped* node, const TSourceLoc &curr);

	SrcLoc const&  GetCurrentLocation () const
	{
		return _callStack.back().loc;
	}
	
	void AddLocation (const TSourceLoc &loc)
	{
		return AddLocation({ uint32_t(loc.string), uint32_t(loc.line), uint32_t(loc.column) });
	}

	void AddLocation (const SrcLoc &src);


	void RequestFunc (const TString &fname)
	{
		_requiredFunctions.insert( fname );
	}

	RequiredFunctions_t const&  GetRequiredFunctions () const
	{
		return _requiredFunctions;
	}


	void AddSymbol (TIntermSymbol* node);

	int  GetUniqueSymbolID ()
	{
		return ++_maxSymbolId;
	}


	void CacheSymbolNode (TIntermSymbol* node)
	{
		_cachedSymbols.insert({ node->getName(), node });
		AddSymbol( node );
	}

	TIntermSymbol*  GetCachedSymbolNode (const TString &name)
	{
		auto	iter = _cachedSymbols.find( name );
		return	iter != _cachedSymbols.end() ? iter->second : nullptr;
	}


	TIntermSymbol*  GetDebugStorage () const
	{
		CHECK( _dbgStorage );
		return _dbgStorage;
	}

	bool SetDebugStorage (TIntermSymbol* symb);

	TIntermBinary*  GetDebugStorageField (const char* name) const;
	

	bool PostProcess (OUT VarNames_t &);

	EShLanguage  GetShaderType () const
	{
		return _shLang;
	}
};
