// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/MipmapLevel.h"
#include "framegraph/Public/MultiSamples.h"
#include "framegraph/Public/ImageLayer.h"
#include "framegraph/Public/IDs.h"
#include "framegraph/Public/VulkanTypes.h"
#include "framegraph/Public/FGEnums.h"
#include "framegraph/Shared/LocalResourceID.h"

#include "stl/ThreadSafe/RaceConditionCheck.h"
#include "stl/ThreadSafe/SpinLock.h"
#include "stl/Containers/Appendable.h"
#include "stl/Containers/Storage.h"
#include "stl/Memory/LinearAllocator.h"
#include "Utils/VEnums.h"

#include "extensions/vulkan_loader/VulkanLoader.h"
#include "extensions/vulkan_loader/VulkanCheckError.h"

#if 0
#include <foonathan/memory/memory_pool.hpp>
#include <foonathan/memory/temporary_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>
#endif

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

	//template <typename T>
	//using TempArray					= std::vector< T, foonathan::memory::std_allocator< T, > >;

	using DebugName_t				= StaticString<64>;

	using UntypedVkResource_t		= Pair< VkDebugReportObjectTypeEXT, uint64_t >;
	using AppendableVkResources_t	= Appendable< UntypedVkResource_t >;
	
	using UntypedResourceID_t		= Union< RawImageID, RawSamplerID, RawBufferID, RawGPipelineID, RawCPipelineID,
											 RawMPipelineID, RawRTPipelineID, RawMemoryID, RawDescriptorSetLayoutID,
											 RawPipelineLayoutID, RawRenderPassID, RawFramebufferID, RawPipelineResourcesID >;
	using AppendableResourceIDs_t	= Appendable< UntypedResourceID_t >;
	
	using VkDescriptorSets_t		= FixedArray< VkDescriptorSet, FG_MaxDescriptorSets >;


	struct VPipelineResourceSet
	{
		FixedArray< RawPipelineResourcesID, FG_MaxDescriptorSets >	resources;
		FixedArray< uint, 64 >										dynamicOffsets;
	};


	enum class ExeOrderIndex : uint
	{
		Initial		= 0,
		First		= 1,
		Final		= 0x80000000,
	};

	enum class EQueueFamily : uint
	{
		External	= VK_QUEUE_FAMILY_EXTERNAL,
		Foreign		= VK_QUEUE_FAMILY_FOREIGN_EXT,
		Ignored		= VK_QUEUE_FAMILY_IGNORED,
		Unknown		= Ignored,
	};


	class VDevice;
	class VDebugger;
	class VSwapchain;
	class VMemoryObj;
	class VMemoryManager;
	class VStagingBufferManager;
	class VFrameGraphThread;
	class VSubmissionGraph;
	class VResourceManager;
	class VResourceManagerThread;
	class VBarrierManager;
	class VFrameGraphDebugger;
	class VRenderPass;
	class VLogicalRenderPass;
	class VPipelineCache;
	class VLocalBuffer;
	class VLocalImage;
	class VRenderPassCache;


}	// FG
