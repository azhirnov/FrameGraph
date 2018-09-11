// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "framegraph/Public/LowLevel/Pipeline.h"

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

		virtual bool IsSupported (const GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat) const = 0;
		virtual bool IsSupported (const ComputePipelineDesc &ppln, EShaderLangFormat dstFormat) const = 0;
		
		virtual bool Compile (INOUT GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat) = 0;
		virtual bool Compile (INOUT ComputePipelineDesc &ppln, EShaderLangFormat dstFormat) = 0;
	};

}	// FG
