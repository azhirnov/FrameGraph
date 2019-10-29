// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Vulkan/VulkanDevice.h"
#include "stl/Algorithms/ArrayUtils.h"

// glslang includes
#include "glslang/Public/ShaderLang.h"
#include "glslang/Include/ResourceLimits.h"

using namespace FGC;

struct ShaderTrace;


class ShaderCompiler
{
public:
	using Debuggable_t	= HashMap< VkShaderModule, ShaderTrace* >;


private:
	Array<uint>		_tempBuf;
	Debuggable_t	_debuggableShaders;


public:
	ShaderCompiler ();
	~ShaderCompiler ();
	
	bool Compile (OUT VkShaderModule&		shaderModule,
				  const VulkanDevice&		device,
				  ArrayView<const char *>	source,
				  EShLanguage				shaderType,
				  uint						dbgBufferSetIndex	= ~0u,
				  glslang::EShTargetLanguageVersion	spvVersion	= glslang::EShTargetSpv_1_3);

	bool GetDebugOutput (VkShaderModule shaderModule, const void *ptr, BytesU maxSize, OUT Array<FGC::String> &result) const;


private:
	bool _Compile (OUT Array<uint>&			spirvData,
				   OUT ShaderTrace*			dbgInfo,
				   uint						dbgBufferSetIndex,
				   ArrayView<const char *>	source,
				   EShLanguage				shaderType,
				   glslang::EShTargetLanguageVersion	spvVersion);
};
