// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/RayTracingGeometryDesc.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Ray Tracing Geometry immutable data
	//

	class VRayTracingGeometry final
	{
	// variables
	private:
		VkAccelerationStructureNV		_bottomLevelAS		= VK_NULL_HANDLE;


	// methods
	public:
		VRayTracingGeometry () {}
		~VRayTracingGeometry ();

		bool Create (const VDevice &dev, const RayTracingGeometryDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
					 EQueueFamily queueFamily, StringView dbgName);

		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
	};


}	// FG
