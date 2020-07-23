// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraph.h"
#include "VResourceManager.h"
#include "VDevice.h"
#include "VCmdBatch.h"
#include "VDebugger.h"
#include "stl/ThreadSafe/LfIndexedPool.h"

namespace FG
{

	//
	// Vulkan Frame Graph instance
	//

	class VFrameGraph final : public IFrameGraph
	{
	// types
	private:
		enum class EState : uint
		{
			Initial,
			Initialization,
			Idle,
			Destroyed,
		};
		
		using EBatchState		= VCmdBatch::EState;
		using PerQueueSem_t		= StaticArray< VkSemaphore, uint(EQueueType::_Count) >;

		struct QueueData
		{
		// immutable data
			VDeviceQueueInfoPtr			ptr;			// pointer to the physical queue
			EQueueType					type			= Default;

		// mutable data
			Array<VCmdBatchPtr>			pending;		// TODO: circular queue
			Array<VSubmitted *>			submitted;
			PerQueueSem_t				semaphores		{};

			VCommandPool				cmdPool;
			Array<VkImageMemoryBarrier>	imageBarriers;
		};

		using CmdBufferPool_t	= LfIndexedPool< VCommandBuffer, uint, 32, 4 >;
		using CmdBatchPool_t	= LfIndexedPool< VCmdBatch, uint, 32, 16 >;
		using SubmittedPool_t	= LfIndexedPool< VSubmitted, uint, 32, 8 >;
		using QueueMap_t		= StaticArray< QueueData, uint(EQueueType::_Count) >;
		using Fences_t			= Array< VkFence >;
		using Semaphores_t		= Array< VkSemaphore >;


	// variables
	private:
		Atomic<EState>			_state;

		VDevice					_device;

		Mutex					_queueGuard;		// TODO: remove global lock
		QueueMap_t				_queueMap;
		EQueueUsage				_queueUsage;

		CmdBufferPool_t			_cmdBufferPool;
		CmdBatchPool_t			_cmdBatchPool;
		SubmittedPool_t			_submittedPool;

		VResourceManager		_resourceMngr;
		VDebugger				_debugger;
		VkQueryPool				_queryPool;			// for time measurements

		ShaderDebugCallback_t	_shaderDebugCallback;

		mutable Mutex			_statisticGuard;
		mutable Statistics		_lastStatistic;

		mutable Atomic<uint64_t>   _submitingTime {0};
		mutable Atomic<uint64_t>   _waitingTime   {0};


	// methods
	public:
		explicit VFrameGraph (const VulkanDeviceInfo &);
		~VFrameGraph ();

		// initialization //
		bool			Initialize ();
		void			Deinitialize () override;
		bool			AddPipelineCompiler (const PipelineCompiler &comp) override;
		bool			SetShaderDebugCallback (ShaderDebugCallback_t &&) override;
		DeviceInfo_t	GetDeviceInfo () const override;
		EQueueUsage		GetAvilableQueues () const override		{ return _queueUsage; }


		// resource manager //
		MPipelineID		CreatePipeline (INOUT MeshPipelineDesc &desc, StringView dbgName) override;
		RTPipelineID	CreatePipeline (INOUT RayTracingPipelineDesc &desc) override;
		GPipelineID		CreatePipeline (INOUT GraphicsPipelineDesc &desc, StringView dbgName) override;
		CPipelineID		CreatePipeline (INOUT ComputePipelineDesc &desc, StringView dbgName) override;
		ImageID			CreateImage (const ImageDesc &desc, const MemoryDesc &mem, StringView dbgName) override;
		ImageID			CreateImage (const ImageDesc &desc, const MemoryDesc &mem, EResourceState defaultState, StringView dbgName) override;
		BufferID		CreateBuffer (const BufferDesc &desc, const MemoryDesc &mem, StringView dbgName) override;
		ImageID			CreateImage (const ExternalImageDesc_t &desc, OnExternalImageReleased_t &&, StringView dbgName) override;
		BufferID		CreateBuffer (const ExternalBufferDesc_t &desc, OnExternalBufferReleased_t &&, StringView dbgName) override;
		SamplerID		CreateSampler (const SamplerDesc &desc, StringView dbgName) override;
		SwapchainID		CreateSwapchain (const SwapchainCreateInfo_t &, RawSwapchainID oldSwapchain, StringView dbgName) override;
		RTGeometryID	CreateRayTracingGeometry (const RayTracingGeometryDesc &desc, const MemoryDesc &mem, StringView dbgName) override;
		RTSceneID		CreateRayTracingScene (const RayTracingSceneDesc &desc, const MemoryDesc &mem, StringView dbgName) override;
		RTShaderTableID	CreateRayTracingShaderTable (StringView dbgName) override;
		bool			InitPipelineResources (RawGPipelineID pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const override;
		bool			InitPipelineResources (RawCPipelineID pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const override;
		bool			InitPipelineResources (RawMPipelineID pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const override;
		bool			InitPipelineResources (RawRTPipelineID pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const override;
		bool			CachePipelineResources (INOUT PipelineResources &resources) override;
		void			ReleaseResource (INOUT PipelineResources &resources) override;
		bool			ReleaseResource (INOUT GPipelineID &id) override;
		bool			ReleaseResource (INOUT CPipelineID &id) override;
		bool			ReleaseResource (INOUT MPipelineID &id) override;
		bool			ReleaseResource (INOUT RTPipelineID &id) override;
		bool			ReleaseResource (INOUT ImageID &id) override;
		bool			ReleaseResource (INOUT BufferID &id) override;
		bool			ReleaseResource (INOUT SamplerID &id) override;
		bool			ReleaseResource (INOUT SwapchainID &id) override;
		bool			ReleaseResource (INOUT RTGeometryID &id) override;
		bool			ReleaseResource (INOUT RTSceneID &id) override;
		bool			ReleaseResource (INOUT RTShaderTableID &id) override;
		
		bool			IsSupported (RawImageID image, const ImageViewDesc &desc) const override;
		bool			IsSupported (RawBufferID buffer, const BufferViewDesc &desc) const override;
		bool			IsSupported (const ImageDesc &desc, EMemoryType memType) const override;
		bool			IsSupported (const BufferDesc &desc, EMemoryType memType) const override;
		
		bool			IsResourceAlive (RawGPipelineID id) const override			{ return _resourceMngr.IsResourceAlive( id ); }
		bool			IsResourceAlive (RawCPipelineID id) const override			{ return _resourceMngr.IsResourceAlive( id ); }
		bool			IsResourceAlive (RawMPipelineID id) const override			{ return _resourceMngr.IsResourceAlive( id ); }
		bool			IsResourceAlive (RawRTPipelineID id) const override			{ return _resourceMngr.IsResourceAlive( id ); }
		bool			IsResourceAlive (RawImageID id) const override				{ return _resourceMngr.IsResourceAlive( id ); }
		bool			IsResourceAlive (RawBufferID id) const override				{ return _resourceMngr.IsResourceAlive( id ); }
		bool			IsResourceAlive (RawSwapchainID id) const override			{ return _resourceMngr.IsResourceAlive( id ); }
		bool			IsResourceAlive (RawRTGeometryID id) const override			{ return _resourceMngr.IsResourceAlive( id ); }
		bool			IsResourceAlive (RawRTSceneID id) const override			{ return _resourceMngr.IsResourceAlive( id ); }
		bool			IsResourceAlive (RawRTShaderTableID id) const override		{ return _resourceMngr.IsResourceAlive( id ); }
		
		GPipelineID		AcquireResource (RawGPipelineID id) override				{ return _resourceMngr.AcquireResource( id ) ? GPipelineID{id}		: Default; }
		CPipelineID		AcquireResource (RawCPipelineID id) override				{ return _resourceMngr.AcquireResource( id ) ? CPipelineID{id}		: Default; }
		MPipelineID		AcquireResource (RawMPipelineID id) override				{ return _resourceMngr.AcquireResource( id ) ? MPipelineID{id}		: Default; }
		RTPipelineID	AcquireResource (RawRTPipelineID id) override				{ return _resourceMngr.AcquireResource( id ) ? RTPipelineID{id}		: Default; }
		ImageID			AcquireResource (RawImageID id) override					{ return _resourceMngr.AcquireResource( id ) ? ImageID{id}			: Default; }
		BufferID		AcquireResource (RawBufferID id) override					{ return _resourceMngr.AcquireResource( id ) ? BufferID{id}			: Default; }
		SwapchainID		AcquireResource (RawSwapchainID id) override				{ return _resourceMngr.AcquireResource( id ) ? SwapchainID{id}		: Default; }
		RTGeometryID	AcquireResource (RawRTGeometryID id) override				{ return _resourceMngr.AcquireResource( id ) ? RTGeometryID{id}		: Default; }
		RTSceneID		AcquireResource (RawRTSceneID id) override					{ return _resourceMngr.AcquireResource( id ) ? RTSceneID{id}		: Default; }
		RTShaderTableID	AcquireResource (RawRTShaderTableID id) override			{ return _resourceMngr.AcquireResource( id ) ? RTShaderTableID{id}	: Default; }

		BufferDesc const&	GetDescription (RawBufferID id) const override;
		ImageDesc const&	GetDescription (RawImageID id) const override;
		ExternalBufferDesc_t GetApiSpecificDescription (RawBufferID id) const override;
		ExternalImageDesc_t  GetApiSpecificDescription (RawImageID id) const override;
		
		bool			UpdateHostBuffer (RawBufferID id, BytesU offset, BytesU size, const void *data) override;
		bool			MapBufferRange (RawBufferID id, BytesU offset, INOUT BytesU &size, OUT void* &data) override;


		// frame execution //
		CommandBuffer	Begin (const CommandBufferDesc &, ArrayView<CommandBuffer> dependsOn) override;
		bool			Execute (INOUT CommandBuffer &) override;
		bool			Wait (ArrayView<CommandBuffer> commands, Nanoseconds timeout) override;
		bool			Flush (EQueueUsage queues) override;
		bool			WaitIdle () override;


		// debugging //
		bool			GetStatistics (OUT Statistics &result) const override;
		bool			DumpToString (OUT String &result) const override;
		bool			DumpToGraphViz (OUT String &result) const override;


		// //
		void			RecycleBatch (const VCmdBatch *);

		
		ND_ VDeviceQueueInfoPtr	FindQueue (EQueueType type) const;
		ND_ VDevice const&		GetDevice ()				const	{ return _device; }
		ND_ VResourceManager &	GetResourceManager ()				{ return _resourceMngr; }
		ND_ VkQueryPool			GetQueryPool ()				const	{ return _queryPool; }


	private:
		// resource manager //
		template <typename PplnID>
		bool  _InitPipelineResources (const PplnID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const;

		template <typename ID>
		bool  _ReleaseResource (INOUT ID &id);
		
		void  _TransitImageLayoutToDefault (RawImageID imageId, VkImageLayout initialLayout, uint queueFamily);

		ND_ VkSemaphore	 _CreateSemaphore ();


		// queues //
		ND_ EQueueFamilyMask _GetQueuesMask (EQueueUsage types) const;
		ND_ QueueData&       _GetQueueData (EQueueType index);

			bool  _IsUnique (VDeviceQueueInfoPtr ptr) const;
			bool  _AddGraphicsQueue ();
			bool  _AddAsyncComputeQueue ();
			bool  _AddAsyncTransferQueue ();
			bool  _CreateQueue (EQueueType, VDeviceQueueInfoPtr);

			bool  _TryFlush (const VCmdBatchPtr &batch);
			bool  _FlushAll (EQueueUsage queues, uint maxIter);
			bool  _FlushQueue (EQueueType queue, uint maxIter);
			bool  _WaitQueue (EQueueType queue, Nanoseconds timeout);


		// states //
		ND_ bool	_IsInitialized () const;
		ND_ EState	_GetState () const;
		ND_ bool	_SetState (EState expected, EState newState);
	};


}	// FG
