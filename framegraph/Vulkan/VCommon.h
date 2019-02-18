// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/MipmapLevel.h"
#include "framegraph/Public/MultiSamples.h"
#include "framegraph/Public/ImageLayer.h"
#include "framegraph/Public/IDs.h"
#include "framegraph/Public/VulkanTypes.h"
#include "framegraph/Public/FGEnums.h"
#include "framegraph/Shared/LocalResourceID.h"

#include "extensions/vulkan_loader/VulkanLoader.h"
#include "extensions/vulkan_loader/VulkanCheckError.h"

#include "stl/ThreadSafe/RaceConditionCheck.h"
#include "stl/ThreadSafe/SpinLock.h"
#include "stl/Containers/Appendable.h"
#include "stl/Containers/InPlace.h"
#include "stl/Memory/LinearAllocator.h"
#include "Utils/VEnums.h"

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
											 RawRTGeometryID, RawRTSceneID, RawRTShaderTableID >;
	using AppendableResourceIDs_t	= Appendable< UntypedResourceID_t >;
	
	using VkDescriptorSets_t		= FixedArray< VkDescriptorSet, FG_MaxDescriptorSets >;
	
	using VDeviceQueueInfoPtr		= Ptr< const struct VDeviceQueueInfo >;
	

	enum BLASHandle_t : uint64_t {};
	
	struct VkGeometryInstance
	{
		// 4x3 row-major matrix
		float4			transformRow0;
		float4			transformRow1;
		float4			transformRow2;

		uint			customIndex		: 24;
		uint			mask			: 8;
		uint			instanceOffset	: 24;
		uint			flags			: 8;
		BLASHandle_t	blasHandle;
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
	class VDescriptorManager;
	class VLocalBuffer;
	class VLocalImage;
	class VLocalRTGeometry;
	class VLocalRTScene;
	class VRenderPassCache;
	class VBaseDrawVerticesTask;
	class VBaseDrawMeshes;
	class VComputePipeline;
	class VGraphicsPipeline;
	class VMeshPipeline;
	class VRayTracingPipeline;
	class VRayTracingShaderTable;
	class VPipelineLayout;
	class VShaderDebugger;
	class VPipelineResources;
	class VFrameGraphInstance;


	struct VPipelineResourceSet
	{
		struct Item {
			DescriptorSetID				descSetId;
			VPipelineResources const*	pplnRes		= null;
			uint						offsetIndex;		// in 'dynamicOffsets'
			uint						offsetCount	= 0;
		};

		FixedArray< Item, FG_MaxDescriptorSets >					resources;
		mutable FixedArray< uint, FG_MaxBufferDynamicOffsets >		dynamicOffsets;
	};


}	// FG
