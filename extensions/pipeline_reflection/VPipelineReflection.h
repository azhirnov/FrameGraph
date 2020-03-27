// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Pipeline.h"
#include "framegraph/Shared/HashCollisionCheck.h"
#include <mutex>

namespace FG
{

	//
	// Vulkan Pipeline Reflection
	//

	class VPipelineReflection final
	{
	// methods
	public:
		bool  Reflect (INOUT MeshPipelineDesc &ppln);
		bool  Reflect (INOUT RayTracingPipelineDesc &ppln);
		bool  Reflect (INOUT GraphicsPipelineDesc &ppln);
		bool  Reflect (INOUT ComputePipelineDesc &ppln);
	};


}	// FG
