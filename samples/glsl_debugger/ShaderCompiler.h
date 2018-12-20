// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Vulkan/VulkanDevice.h"
#include "stl/Algorithms/ArrayUtils.h"

// glslang includes
#include "glslang/glslang/Public/ShaderLang.h"
#include "glslang/glslang/Include/ResourceLimits.h"

using namespace FG;


class ShaderCompiler
{
public:
	struct SourcePoint
	{
		uint64_t	value	= UMax;

		SourcePoint () {}
		SourcePoint (uint line, uint column) : value{(uint64_t(line) << 32) | column } {}

		ND_ uint     Line ()		const	{ return uint(value >> 32); }
		ND_ uint     Column ()		const	{ return uint(value & 0xFFFFFFFF); }
		ND_ HashVal  CalcHash ()	const	{ return HashOf(value); }

		ND_ bool  operator == (const SourcePoint &rhs) const	{ return value == rhs.value; }
	};

	struct SourceLocation
	{
		uint			sourceId	= UMax;
		SourcePoint		begin;
		SourcePoint		end;

		SourceLocation () {}
		SourceLocation (uint sourceId, uint line, uint column) : sourceId{sourceId}, begin{line, column}, end{line, column} {}

		ND_ HashVal  CalcHash () const	{ return HashOf(sourceId) + begin.CalcHash() + end.CalcHash(); }

		ND_ bool  operator == (const SourceLocation &rhs) const	{ return sourceId == rhs.sourceId and begin == rhs.begin and end == rhs.end; }
	};

	struct ShaderDebugInfo
	{
		Array< SourceLocation >		srcLocations;
		Array< FG::String >			source;
		BytesU						posOffset;
		BytesU						dataOffset;
	};

	using Debuggable_t	= HashMap< VkShaderModule, ShaderDebugInfo >;


private:
	Array<uint>		_tempBuf;
	FG::String		_tempStr;
	Debuggable_t	_debuggableShaders;


public:
	ShaderCompiler ();
	~ShaderCompiler ();
	
	bool Compile (OUT VkShaderModule&		shaderModule,
				  const VulkanDevice&		device,
				  ArrayView<const char *>	source,
				  EShLanguage				shaderType,
				  bool						debuggable			= false,
				  glslang::EShTargetLanguageVersion	spvVersion	= glslang::EShTargetSpv_1_3);

	bool ParseDebugOutput (VkShaderModule shaderModule, const void *ptr, BytesU maxSize, BytesU posOffset) const;


private:
	bool _Compile (OUT Array<uint>&			spirvData,
				   OUT ShaderDebugInfo*		dbgInfo,
				   ArrayView<const char *>	source,
				   EShLanguage				shaderType,
				   glslang::EShTargetLanguageVersion	spvVersion);
	
	static bool _GenerateDebugInfo (const glslang::TIntermediate &, OUT ShaderDebugInfo &);

	static bool _GetSourceLine (const ShaderDebugInfo &, uint, INOUT FG::String &);

	static StringView  _DeclStorageBuffer ();
};
