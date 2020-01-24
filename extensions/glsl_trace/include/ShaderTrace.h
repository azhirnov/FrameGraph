// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
	enum class VariableID : uint32_t { Unknown = ~0u };

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
		VariableID				varID	= VariableID::Unknown;	// ID of output variable
		uint32_t				swizzle	= 0;
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
	using ExprInfos_t	= std::vector< ExprInfo >;
	using Sources_t		= std::vector< SourceInfo >;
	using FileMap_t		= std::unordered_map< std::string, uint32_t >;	// index in '_sources'

	static constexpr int	TBasicType_Clock = 0xcc;	// 4x uint64


private:
	ExprInfos_t		_exprLocations;
	VarNames_t		_varNames;
	Sources_t		_sources;
	FileMap_t		_fileMap;
	uint64_t		_posOffset		= 0;
	uint64_t		_dataOffset		= 0;
	uint32_t		_initialPosition;


public:
	ShaderTrace () {}

	ShaderTrace (ShaderTrace &&) = delete;
	ShaderTrace (const ShaderTrace &) = delete;
	ShaderTrace& operator = (ShaderTrace &&) = delete;
	ShaderTrace& operator = (const ShaderTrace &) = delete;

	bool InsertTraceRecording (glslang::TIntermediate &, uint32_t descSetIndex);
	bool InsertFunctionProfiler (glslang::TIntermediate &, uint32_t descSetIndex, bool shaderSubgroupClock, bool shaderDeviceClock);
	//bool InsertDebugAsserts (glslang::TIntermediate &, uint32_t descSetIndex);
	//bool InsertInstructionCounter (glslang::TIntermediate &, uint32_t descSetIndex);
	
	bool ParseShaderTrace (const void *ptr, uint64_t maxSize, std::vector<std::string> &result) const;

	void SetSource (const char* const* sources, const size_t *lengths, size_t count);
	void SetSource (const char* source, size_t length);
	void IncludeSource (const char* filename, const char* source, size_t length);	// if used '#include'
	void GetSource (std::string &result) const;

private:
	void _AppendSource (const char* source, size_t length);
};
