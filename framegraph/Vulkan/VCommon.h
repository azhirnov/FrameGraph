// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/MipmapLevel.h"
#include "framegraph/Public/LowLevel/MultiSamples.h"
#include "framegraph/Public/LowLevel/ImageLayer.h"
#include "framegraph/Public/LowLevel/IDs.h"
#include "framegraph/Public/LowLevel/VulkanTypes.h"
#include "VEnums.h"

#include "extensions/vulkan_loader/VulkanLoader.h"
#include "extensions/vulkan_loader/VulkanCheckError.h"


STATIC_ASSERT( sizeof(FG::VkInstance_t)			== sizeof(VkInstance) );
STATIC_ASSERT( sizeof(FG::VkPhysicalDevice_t)	== sizeof(VkPhysicalDevice) );
STATIC_ASSERT( sizeof(FG::VkDevice_t)			== sizeof(VkDevice) );
STATIC_ASSERT( sizeof(FG::VkQueue_t)			== sizeof(VkQueue) );
STATIC_ASSERT( sizeof(FG::VkCommandBuffer_t)	== sizeof(VkCommandBuffer) );
STATIC_ASSERT( sizeof(FG::VkSurface_t)			== sizeof(VkSurfaceKHR) );
STATIC_ASSERT( sizeof(FG::VkEvent_t)			== sizeof(VkEvent) );
STATIC_ASSERT( sizeof(FG::VkFence_t)			== sizeof(VkFence) );
STATIC_ASSERT( sizeof(FG::VkBuffer_t)			== sizeof(VkBuffer) );
STATIC_ASSERT( sizeof(FG::VkImage_t)			== sizeof(VkImage) );


namespace FG
{

	//
	// Memory Handle
	//
	struct VMemoryHandle final
	{
		struct _VMemory_t*	memory		= null;
		uint				allocator	= ~0u;

		VMemoryHandle () {}

		ND_ explicit operator const bool () const	{ return memory != null; }
	};
	

	enum class ExeOrderIndex : uint
	{
		Initial	= 0,
		First	= 1,
		Max		= std::numeric_limits<uint>::max() >> 1,
	};


	class VMemoryManager;
	class VDevice;
	class VBarrierManager;
	class VFrameGraphDebugger;
	class VCommandQueue;

	using VFrameGraphPtr			= SharedPtr< class VFrameGraph >;
	using VFrameGraphWeak			= WeakPtr< class VFrameGraph >;

	using VBufferPtr				= SharedPtr< class VBuffer >;
	using VImagePtr					= SharedPtr< class VImage >;
	using VSamplerPtr				= SharedPtr< class VSampler >;
	
	using VLogicalBufferPtr			= SharedPtr< class VLogicalBuffer >;
	using VLogicalImagePtr			= SharedPtr< class VLogicalImage >;

	using VMeshPipelinePtr			= SharedPtr< class VMeshPipeline >;
	using VRayTracingPipelinePtr	= SharedPtr< class VRayTracingPipeline >;
	using VGraphicsPipelinePtr		= SharedPtr< class VGraphicsPipeline >;
	using VComputePipelinePtr		= SharedPtr< class VComputePipeline >;
	using VPipelineLayoutPtr		= SharedPtr< class VPipelineLayout >;

	using VDescriptorSetLayoutPtr	= SharedPtr< class VDescriptorSetLayout >;
	using VDescriptorSetLayoutWeak	= WeakPtr< class VDescriptorSetLayout >;

	using IDescriptorPoolPtr		= SharedPtr< class IDescriptorPool >;

	using VPipelineResourcesPtr		= SharedPtr< class VPipelineResources >;
	using VPipelineResourcesWeak	= WeakPtr< class VPipelineResources >;

	using VPipelineResourceSet		= FixedMap< DescriptorSetID, VPipelineResourcesPtr, FG_MaxDescriptorSets >;
	
	class VLogicalRenderPass;
	using VRenderPassPtr			= SharedPtr< class VRenderPass >;
	using VFramebufferPtr			= SharedPtr< class VFramebuffer >;

}	// FG
