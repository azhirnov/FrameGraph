// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Pipeline.h"

namespace FG
{

	//
	// Pipeline Compiler interface
	//

	class IPipelineCompiler : public std::enable_shared_from_this<IPipelineCompiler>
	{
	// interface
	public:
		virtual ~IPipelineCompiler () {}

		ND_ virtual bool IsSupported (const MeshPipelineDesc &ppln, EShaderLangFormat dstFormat) const = 0;
		ND_ virtual bool IsSupported (const RayTracingPipelineDesc &ppln, EShaderLangFormat dstFormat) const = 0;
		ND_ virtual bool IsSupported (const GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat) const = 0;
		ND_ virtual bool IsSupported (const ComputePipelineDesc &ppln, EShaderLangFormat dstFormat) const = 0;
		
		virtual bool Compile (INOUT MeshPipelineDesc &ppln, EShaderLangFormat dstFormat) = 0;
		virtual bool Compile (INOUT RayTracingPipelineDesc &ppln, EShaderLangFormat dstFormat) = 0;
		virtual bool Compile (INOUT GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat) = 0;
		virtual bool Compile (INOUT ComputePipelineDesc &ppln, EShaderLangFormat dstFormat) = 0;
	};

}	// FG
