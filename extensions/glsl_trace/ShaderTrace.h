// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace glslang {
	class TIntermediate;
}


struct ShaderTrace
{
public:
	enum VariableID : uint32_t { VariableID_Unknown = ~0u };

	struct SourcePoint
	{
		uint64_t	value;

		SourcePoint () : value{~0ull} {}
		SourcePoint (uint32_t line, uint32_t column) : value{(uint64_t(line) << 32) | column } {}

		uint32_t	Line ()		const	{ return uint32_t(value >> 32); }
		uint32_t	Column ()	const	{ return uint32_t(value & 0xFFFFFFFF); }
	};

	struct SourceLocation
	{
		uint32_t		sourceId	= ~0u;
		SourcePoint		begin;
		SourcePoint		end;

		SourceLocation () {}
		SourceLocation (uint32_t sourceId, uint32_t line, uint32_t column) : sourceId{sourceId}, begin{line, column}, end{line, column} {}

		bool operator == (const SourceLocation &rhs) const	{ return sourceId == rhs.sourceId && begin.value == rhs.begin.value && end.value == rhs.end.value; }
	};

	struct ExprInfo
	{
		VariableID				varID	= VariableID_Unknown;	// ID of output variable
		SourceLocation			range;							// begin and end location of expression
		SourcePoint				point;							// location of operator
		std::vector<VariableID>	vars;							// all variables IDs in this expression
	};

	struct SourceInfo
	{
		using LineRange = std::pair< size_t, size_t >;

		std::string				code;
		std::vector<LineRange>	lines;		// offset in bytes for each line in 'code'
	};

	using VarNames_t	= std::unordered_map< VariableID, std::string >;
	using ExprInfos_t	= std::vector<ExprInfo>;
	using Sources_t		= std::vector<SourceInfo>;


private:
	ExprInfos_t		_exprLocations;
	VarNames_t		_varNames;
	Sources_t		_sources;
	uint32_t		_shaderType		= ~0u;	// EShLanguage
	uint64_t		_posOffset		= 0;
	uint64_t		_dataOffset		= 0;


public:
	ShaderTrace () {}
	ShaderTrace (ShaderTrace &&) = default;
	ShaderTrace& operator = (ShaderTrace &&) = default;

	bool GenerateDebugInfo (glslang::TIntermediate &, uint32_t setIndex);

	bool GetDebugOutput (const void *ptr, uint64_t maxSize, std::vector<struct ShaderDebugTrace> &result) const;
	bool GetDebugOutput (const void *ptr, uint64_t maxSize, std::vector<std::string> &result) const;

	void SetSource (const char* const* sources, const size_t *lengths, size_t count);
	void SetSource (const char* source, size_t length);
	void GetSource (std::string &result) const;
};
