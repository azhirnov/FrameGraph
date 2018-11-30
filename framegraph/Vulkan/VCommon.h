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


namespace FG
{

	//template <typename T>
	//using TempArray					= std::vector< T, foonathan::memory::std_allocator< T, > >;

	using DebugName_t				= StaticString<64>;

	using UntypedVkResource_t		= Pair< VkObjectType, uint64_t >;
	using AppendableVkResources_t	= Appendable< UntypedVkResource_t >;
	
	using UntypedResourceID_t		= Union< RawImageID, RawSamplerID, RawBufferID, RawGPipelineID, RawCPipelineID,
											 RawMPipelineID, RawRTPipelineID, RawMemoryID, RawDescriptorSetLayoutID,
											 RawPipelineLayoutID, RawRenderPassID, RawFramebufferID, RawPipelineResourcesID,
											 RawRTGeometryID, RawRTSceneID >;
	using AppendableResourceIDs_t	= Appendable< UntypedResourceID_t >;
	
	using VkDescriptorSets_t		= FixedArray< VkDescriptorSet, FG_MaxDescriptorSets >;
	
	using VDeviceQueueInfoPtr		= Ptr< const struct VDeviceQueueInfo >;


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
		Unknown		= ~0u,
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
	class VBaseDrawVerticesTask;
	class VBaseDrawMeshes;
	class VComputePipeline;
	class VGraphicsPipeline;
	class VMeshPipeline;


}	// FG
