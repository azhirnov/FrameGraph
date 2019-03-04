// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphThread.h"
#include "VMemoryManager.h"
#include "VSwapchainKHR.h"
#include "VStagingBufferManager.h"
#include "Shared/PipelineResourcesHelper.h"
#include "VFrameGraphDebugger.h"
#include "VShaderDebugger.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VFrameGraphThread::VFrameGraphThread (VFrameGraphInstance &fg, EThreadUsage usage, StringView name) :
		_state{ EState::Initial },		_frameId{ 0 },
		_debugName{ name },				_instance{ fg },
		_resourceMngr{ _mainAllocator, _statistic.resources, fg.GetResourceMngr() }
	{
		EXLOCK( _rcCheck );

		_mainAllocator.SetBlockSize( 16_Mb );

		if ( EnumEq( usage, EThreadUsage::MemAllocation ) )
		{
			_memoryMngr.reset( new VMemoryManager{ fg.GetDevice() });
		}

		if ( EnumEq( usage, EThreadUsage::Transfer ) )
		{
			_stagingMngr.reset( new VStagingBufferManager( *this ));
		}
	}
	
/*
=================================================
	destructor
=================================================
*/
	VFrameGraphThread::~VFrameGraphThread ()
	{
		EXLOCK( _rcCheck );
		CHECK( _GetState() == EState::Destroyed );
	}
	
/*
=================================================
	_GetState / _SetState
=================================================
*/
	inline void  VFrameGraphThread::_SetState (EState newState)
	{
		_state.store( newState, memory_order_release );
	}

	inline bool  VFrameGraphThread::_SetState (EState expected, EState newState)
	{
		return _state.compare_exchange_strong( INOUT expected, newState, memory_order_release, memory_order_relaxed );
	}
	
	inline bool  VFrameGraphThread::_IsInitialized () const
	{
		const EState	state = _GetState();
		return state >= EState::Idle and state < EState::Failed;
	}
	
	inline bool  VFrameGraphThread::_IsInitialOrIdleState () const
	{
		const EState	state = _GetState();
		return state == EState::Initial or state == EState::Idle;
	}

/*
=================================================
	CreatePipeline
=================================================
*/
	MPipelineID  VFrameGraphThread::CreatePipeline (INOUT MeshPipelineDesc &desc, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return MPipelineID{ _resourceMngr.CreatePipeline( INOUT desc, dbgName, IsInSeparateThread() )};
	}
	
	RTPipelineID  VFrameGraphThread::CreatePipeline (INOUT RayTracingPipelineDesc &desc)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return RTPipelineID{ _resourceMngr.CreatePipeline( INOUT desc, IsInSeparateThread() )};
	}
	
	GPipelineID  VFrameGraphThread::CreatePipeline (INOUT GraphicsPipelineDesc &desc, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return GPipelineID{ _resourceMngr.CreatePipeline( INOUT desc, dbgName, IsInSeparateThread() )};
	}
	
	CPipelineID  VFrameGraphThread::CreatePipeline (INOUT ComputePipelineDesc &desc, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return CPipelineID{ _resourceMngr.CreatePipeline( INOUT desc, dbgName, IsInSeparateThread() )};
	}
	
/*
=================================================
	TransitImageLayoutToDefault
=================================================
*/
	void  VFrameGraphThread::TransitImageLayoutToDefault (RawImageID imageId, VkImageLayout initialLayout, uint queueFamily,
														  VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
	{
		const auto&	image = *_resourceMngr.GetResource( imageId );
		
		if ( image.DefaultLayout() == initialLayout )
			return;

		VkImageMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask		= 0;
		barrier.dstAccessMask		= 0;
		barrier.oldLayout			= initialLayout;
		barrier.newLayout			= image.DefaultLayout();
		barrier.image				= image.Handle();
		barrier.subresourceRange	= { image.AspectMask(), 0, image.MipmapLevels(), 0, image.ArrayLayers() };

		// error will be generated by validation layer if current queue family
		// doesn't match with queue family in the command buffer
		barrier.srcQueueFamilyIndex	= queueFamily;
		barrier.dstQueueFamilyIndex	= queueFamily;

		_barrierMngr.AddImageBarrier( srcStage, dstStage, 0, barrier );
	}

/*
=================================================
	CreateImage
=================================================
*/
	ImageID  VFrameGraphThread::CreateImage (const ImageDesc &desc, const MemoryDesc &mem, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		CHECK_ERR( _memoryMngr );

		RawImageID	result = _resourceMngr.CreateImage( desc, mem, *_memoryMngr, _instance.GetQueuesMask( desc.queues ), dbgName, IsInSeparateThread() );
		
		// add first image layout transition
		if ( result )
		{
			TransitImageLayoutToDefault( result, VK_IMAGE_LAYOUT_UNDEFINED, VK_QUEUE_FAMILY_IGNORED,
										 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
		}
		return ImageID{ result };
	}
	
/*
=================================================
	CreateLogicalImage
=================================================
*
	ImageID  VFrameGraphThread::CreateLogicalImage (EMemoryType memType, const ImageDesc &desc)
	{
		RETURN_ERR( "not supported" );
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	BufferID  VFrameGraphThread::CreateBuffer (const BufferDesc &desc, const MemoryDesc &mem, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		CHECK_ERR( _memoryMngr );

		return BufferID{ _resourceMngr.CreateBuffer( desc, mem, *_memoryMngr, _instance.GetQueuesMask( desc.queues ), dbgName, IsInSeparateThread() )};
	}
	
/*
=================================================
	CreateLogicalBuffer
=================================================
*
	BufferID  VFrameGraphThread::CreateLogicalBuffer (EMemoryType memType, const BufferDesc &desc)
	{
		RETURN_ERR( "not supported" );
	}
	
/*
=================================================
	CreateImage
=================================================
*/
	ImageID  VFrameGraphThread::CreateImage (const ExternalImageDesc_t &desc, OnExternalImageReleased_t &&onRelease, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		auto*	img_desc = UnionGetIf<VulkanImageDesc>( &desc );
		CHECK_ERR( img_desc );

		RawImageID	result = _resourceMngr.CreateImage( *img_desc, std::move(onRelease), dbgName );
		
		// add first image layout transition
		if ( result )
		{
			VkImageLayout	initial_layout	= BitCast<VkImageLayout>( img_desc->layout );
			
			TransitImageLayoutToDefault( result, initial_layout, img_desc->queueFamily, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
		}
		return ImageID{ result };
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	BufferID  VFrameGraphThread::CreateBuffer (const ExternalBufferDesc_t &desc, OnExternalBufferReleased_t &&onRelease, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		
		auto*	buf_desc = UnionGetIf<VulkanBufferDesc>( &desc );
		CHECK_ERR( buf_desc );

		return BufferID{ _resourceMngr.CreateBuffer( *buf_desc, std::move(onRelease), dbgName )};
	}

/*
=================================================
	CreateSampler
=================================================
*/
	SamplerID  VFrameGraphThread::CreateSampler (const SamplerDesc &desc, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return SamplerID{ _resourceMngr.CreateSampler( desc, dbgName, IsInSeparateThread() )};
	}
	
/*
=================================================
	CreateRayTracingShaderTable
=================================================
*/
	RTShaderTableID  VFrameGraphThread::CreateRayTracingShaderTable (StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return RTShaderTableID{ _resourceMngr.CreateRayTracingShaderTable( dbgName, IsInSeparateThread() )};
	}

/*
=================================================
	_InitPipelineResources
=================================================
*/
	template <typename PplnID>
	bool  VFrameGraphThread::_InitPipelineResources (const PplnID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		
		auto const *	ppln = _resourceMngr.GetResource( pplnId );
		CHECK_ERR( ppln );

		auto const *	ppln_layout = _resourceMngr.GetResource( ppln->GetLayoutID() );
		CHECK_ERR( ppln_layout );

		RawDescriptorSetLayoutID	layout_id;
		uint						binding;

		if ( not ppln_layout->GetDescriptorSetLayout( id, OUT layout_id, OUT binding ) )
			return false;

		VDescriptorSetLayout const*	ds_layout = _resourceMngr.GetResource( layout_id );
		CHECK_ERR( ds_layout );

		CHECK_ERR( PipelineResourcesHelper::Initialize( OUT resources, layout_id, ds_layout->GetResources() ));
		return true;
	}
	
/*
=================================================
	InitPipelineResources
=================================================
*/
	bool  VFrameGraphThread::InitPipelineResources (RawGPipelineID pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const
	{
		return _InitPipelineResources( pplnId, id, OUT resources );
	}

	bool  VFrameGraphThread::InitPipelineResources (RawCPipelineID pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const
	{
		return _InitPipelineResources( pplnId, id, OUT resources );
	}

	bool  VFrameGraphThread::InitPipelineResources (RawMPipelineID pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const
	{
		return _InitPipelineResources( pplnId, id, OUT resources );
	}

	bool  VFrameGraphThread::InitPipelineResources (RawRTPipelineID pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const
	{
		return _InitPipelineResources( pplnId, id, OUT resources );
	}
	
/*
=================================================
	CachePipelineResources
=================================================
*/
	bool  VFrameGraphThread::CachePipelineResources (INOUT PipelineResources &resources)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return _resourceMngr.CacheDescriptorSet( INOUT resources, IsInSeparateThread() );
	}

/*
=================================================
	CreateRayTracingGeometry
=================================================
*/
	RTGeometryID  VFrameGraphThread::CreateRayTracingGeometry (const RayTracingGeometryDesc &desc, const MemoryDesc &mem, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		
		return RTGeometryID{ _resourceMngr.CreateRayTracingGeometry( desc, mem, *_memoryMngr, dbgName, IsInSeparateThread() )};
	}
	
/*
=================================================
	CreateRayTracingScene
=================================================
*/
	RTSceneID  VFrameGraphThread::CreateRayTracingScene (const RayTracingSceneDesc &desc, const MemoryDesc &mem, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		
		return RTSceneID{ _resourceMngr.CreateRayTracingScene( desc, mem, *_memoryMngr, dbgName, IsInSeparateThread() )};
	}

/*
=================================================
	_ReleaseResource
=================================================
*/
	template <typename ID>
	inline void VFrameGraphThread::_ReleaseResource (INOUT ID &id)
	{
		if ( not id )
			return;

		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return _resourceMngr.ReleaseResource( id.Release(), IsInSeparateThread() );
	}
	
/*
=================================================
	ReleaseResource
=================================================
*/
	void VFrameGraphThread::ReleaseResource (INOUT GPipelineID &id)		{ _ReleaseResource( INOUT id ); }
	void VFrameGraphThread::ReleaseResource (INOUT CPipelineID &id)		{ _ReleaseResource( INOUT id ); }
	void VFrameGraphThread::ReleaseResource (INOUT MPipelineID &id)		{ _ReleaseResource( INOUT id ); }
	void VFrameGraphThread::ReleaseResource (INOUT RTPipelineID &id)	{ _ReleaseResource( INOUT id ); }
	void VFrameGraphThread::ReleaseResource (INOUT ImageID &id)			{ _ReleaseResource( INOUT id ); }
	void VFrameGraphThread::ReleaseResource (INOUT BufferID &id)		{ _ReleaseResource( INOUT id ); }
	void VFrameGraphThread::ReleaseResource (INOUT SamplerID &id)		{ _ReleaseResource( INOUT id ); }
	void VFrameGraphThread::ReleaseResource (INOUT RTGeometryID &id)	{ _ReleaseResource( INOUT id ); }
	void VFrameGraphThread::ReleaseResource (INOUT RTSceneID &id)		{ _ReleaseResource( INOUT id ); }
	void VFrameGraphThread::ReleaseResource (INOUT RTShaderTableID &id)	{ _ReleaseResource( INOUT id ); }
	
/*
=================================================
	ReleaseResource
=================================================
*/
	void VFrameGraphThread::ReleaseResource (INOUT PipelineResources &resources)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return _resourceMngr.ReleaseResource( INOUT resources, IsInSeparateThread() );
	}

/*
=================================================
	_GetDescription
=================================================
*/
	template <typename Desc, typename ID>
	inline Desc const&  VFrameGraphThread::_GetDescription (const ID &id) const
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		// read access available without synchronizations
		return _resourceMngr.GetDescription( id );
	}
	
/*
=================================================
	GetDescription
=================================================
*/
	BufferDesc const&  VFrameGraphThread::GetDescription (RawBufferID id) const
	{
		return _GetDescription<BufferDesc>( id );
	}

	ImageDesc const&  VFrameGraphThread::GetDescription (RawImageID id) const
	{
		return _GetDescription<ImageDesc>( id );
	}
	
	/*SamplerDesc const&  VFrameGraphThread::GetDescription (RawSamplerID id) const
	{
		return _GetDescription<SamplerDesc>( id );
	}*/
	
/*
=================================================
	Initialize
=================================================
*/
	bool VFrameGraphThread::Initialize (const SwapchainCreateInfo_t *swapchainCI)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Initial, EState::Initializing ));

		// create swapchain
		if ( swapchainCI )
		{
			CHECK_ERR( Visit( *swapchainCI,
					[this] (const VulkanSwapchainCreateInfo &info)	{ return _RecreateSwapchain( info ); },
					[] (const auto &)								{ RETURN_ERR( "unsupported swapchain create info!", false ); }
				));
		}

		// create query pool
		{
			VkQueryPoolCreateInfo	info = {};
			info.sType		= VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			info.queryType	= VK_QUERY_TYPE_TIMESTAMP;
			info.queryCount	= uint(_queues.capacity() * GetRingBufferSize() * 2);
		
			VDevice const&	dev = GetDevice();
			VK_CHECK( dev.vkCreateQueryPool( dev.GetVkDevice(), &info, null, OUT &_query.pool ));

			_query.capacity = info.queryCount;
			_query.size		= 0;
		}

		if ( _memoryMngr )
			CHECK_ERR( _memoryMngr->Initialize() );

		if ( _stagingMngr )
			CHECK_ERR( _stagingMngr->Initialize() );

		CHECK_ERR( _resourceMngr.Initialize() );
		CHECK_ERR( _SetState( EState::Initializing, EState::Idle ));
		return true;
	}

/*
=================================================
	SignalSemaphore
=================================================
*/
	void VFrameGraphThread::SignalSemaphore (VkSemaphore sem)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _submissionGraph, void());
		CHECK( _submissionGraph->SignalSemaphore( _cmdBatchId, sem ));
	}

/*
=================================================
	WaitSemaphore
=================================================
*/
	void VFrameGraphThread::WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _submissionGraph, void());
		CHECK( _submissionGraph->WaitSemaphore( _cmdBatchId, sem, stage ));
	}

/*
=================================================
	_CreateCommandBuffer
=================================================
*/
	VkCommandBuffer  VFrameGraphThread::_CreateCommandBuffer () const
	{
		CHECK_ERR( _currQueue and _currQueue->cmdPool );

		VkCommandBufferAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext				= null;
		info.commandPool		= _currQueue->cmdPool;
		info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandBufferCount	= 1;
				
		VDevice const&		dev	= GetDevice();
		VkCommandBuffer		cmd;
		VK_CHECK( dev.vkAllocateCommandBuffers( dev.GetVkDevice(), &info, OUT &cmd ));

		_currQueue->frames[_frameId].pending.push_back( cmd );
		return cmd;
	}

/*
=================================================
	Deinitialize
=================================================
*/
	void VFrameGraphThread::Deinitialize ()
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Idle, EState::BeforeDestroy ), void());
		
		VDevice const&	dev = GetDevice();

		for (auto& queue : _queues)
		{
			if ( queue.ptr )
				VK_CALL( dev.vkQueueWaitIdle( queue.ptr->handle ));
			
			if ( queue.cmdPool )
			{
				dev.vkDestroyCommandPool( dev.GetVkDevice(), queue.cmdPool, null );
				queue.cmdPool = VK_NULL_HANDLE;
			}
		}
		_queues.clear();

		if ( _query.pool ) {
			dev.vkDestroyQueryPool( dev.GetVkDevice(), _query.pool, null );
			_query.pool = VK_NULL_HANDLE;
		}

		if ( _stagingMngr )
			_stagingMngr->Deinitialize();

		if ( _swapchain )
			_swapchain->Deinitialize();
		
		if ( _shaderDebugger )
			_shaderDebugger.reset();

		_resourceMngr.Deinitialize();

		if ( _memoryMngr )
			_memoryMngr->Deinitialize();

		_swapchain.reset();
		_memoryMngr.reset();
		_stagingMngr.reset();
		_mainAllocator.Release();
		_shaderDebugCallback = {};
		
		CHECK_ERR( _SetState( EState::BeforeDestroy, EState::Destroyed ), void());
	}
	
/*
=================================================
	SetCompilationFlags
=================================================
*/
	bool VFrameGraphThread::SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsInitialOrIdleState() );

		_compilationFlags = flags;
		
		if ( EnumEq( _compilationFlags, ECompilationFlags::EnableDebugger ) )
		{
			if ( not _debugger )
			{
				CHECK_ERR( _instance.GetDebugger() );
				_debugger.reset( new VFrameGraphDebugger( *_instance.GetDebugger(), *this ));
			}

			_debugger->Setup( debugFlags );
		}
		else
			_debugger.reset();

		return true;
	}
	
/*
=================================================
	RecreateSwapchain
----
	The new surface must be compatible with any already created queues.
	I you have 2+ GPUs each of which is connected to its own monitor,
	you must recreate thread instead of recreating swapchain!
=================================================
*/
	bool VFrameGraphThread::RecreateSwapchain (const SwapchainCreateInfo_t &ci)
	{
		EXLOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		CHECK_ERR( _swapchain );

		CHECK_ERR( Visit( ci,
				[this] (const VulkanSwapchainCreateInfo &info)	{ return _RecreateSwapchain( info ); },
				[] (const auto &)								{ ASSERT(false);  return false; }
			));
		return true;
	}
	
/*
=================================================
	SetShaderDebugCallback
=================================================
*/
	bool VFrameGraphThread::SetShaderDebugCallback (ShaderDebugCallback_t &&cb)
	{
		_shaderDebugCallback = std::move(cb);
		return true;
	}

/*
=================================================
	_RecreateSwapchain
=================================================
*/
	bool VFrameGraphThread::_RecreateSwapchain (const VulkanSwapchainCreateInfo &info)
	{
		if ( _swapchain )
		{
			auto*	old_swapchain = dynamic_cast<VSwapchainKHR *>( _swapchain.get() );

			if ( old_swapchain and old_swapchain->Create( info, GetRingBufferSize() ))
				return true;

			_swapchain->Deinitialize();
			_swapchain.reset();
		}

		UniquePtr<VSwapchainKHR>	sc{ new VSwapchainKHR{ *this }};
		
		CHECK_ERR( sc->Create( info, GetRingBufferSize() ));

		_swapchain.reset( sc.release() );
		return true;
	}

/*
=================================================
	SyncOnBegin
=================================================
*/
	bool VFrameGraphThread::SyncOnBegin (const VSubmissionGraph *graph)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Idle, EState::BeforeStart ));
		ASSERT( graph );

		_frameId		 = (_frameId + 1) % GetRingBufferSize();
		_submissionGraph = graph;
		_statistic		 = Default;

		_resourceMngr.OnBeginFrame();

		if ( _debugger )
			_debugger->OnBeginFrame();
		
		CHECK_ERR( _SetState( EState::BeforeStart, EState::Ready ));
		return true;
	}

/*
=================================================
	Begin
=================================================
*/
	bool VFrameGraphThread::Begin (const CommandBatchID &id, uint index, EQueueUsage usage)
	{
		EXLOCK( _rcCheck );
		
		if ( _SetState( EState::Ready, EState::BeforeRecording ) )
		{
			StaticArray<uint64_t, 2>	query_results;

			// recycle commands buffers
			for (auto& queue : _queues)
			{
				if ( queue.frames.empty() )
					continue;

				auto&	frame	= queue.frames[_frameId];
				auto&	dev		= _instance.GetDevice();

				if ( not frame.executed.empty() )
				{
					dev.vkFreeCommandBuffers( dev.GetVkDevice(), queue.cmdPool, uint(frame.executed.size()), frame.executed.data() );

					// read frame time
					VK_CALL( dev.vkGetQueryPoolResults( dev.GetVkDevice(), _query.pool, frame.queryIndex, uint(query_results.size()),
														sizeof(query_results), OUT query_results.data(),
														sizeof(query_results[0]), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT ));

					_statistic.renderer.gpuTime += Nanoseconds{query_results[1] - query_results[0]};
				}

				frame.executed.clear();
				ASSERT( frame.pending.empty() );
			}

			_isFirstUsage = true;
		}
		else
		{
			// if current thread used for another batch
			CHECK_ERR( _SetState( EState::Pending, EState::BeforeRecording ));

			_isFirstUsage = false;
		}

		_taskGraph.OnStart( GetAllocator() );

		if ( _stagingMngr )
			_stagingMngr->OnBeginFrame( _frameId, _isFirstUsage );


		auto	queue = _instance.FindQueue( usage );
		CHECK_ERR( queue );
		_queues.resize(Max( _queues.size(), uint(queue->familyIndex)+1 ));

		_currQueue		= &_queues[ uint(queue->familyIndex) ];
		_currUsage		= usage;
		_cmdBatchId		= id;
		_indexInBatch	= index;

		// initialize for queue
		if ( not _currQueue->cmdPool )
		{
			_currQueue->frames.resize( GetRingBufferSize() );
			_currQueue->ptr = queue;

			CHECK_ERR( _CreateCommandBuffers( *_currQueue ));

			for (auto& frame : _currQueue->frames)
			{
				if ( frame.queryIndex == UMax )
				{
					frame.queryIndex = _query.size;
					_query.size     += 2;
				}
			}
		}

		CHECK_ERR( _SetState( EState::BeforeRecording, EState::Recording ));
		return true;
	}
	
/*
=================================================
	_CreateCommandBuffers
=================================================
*/
	bool VFrameGraphThread::_CreateCommandBuffers (INOUT PerQueue &queue) const
	{
		CHECK_ERR( not queue.cmdPool );

		VDevice const&	dev = GetDevice();
		
		VkCommandPoolCreateInfo		pool_info = {};
		pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex	= uint(queue.ptr->familyIndex);
		pool_info.flags				= 0; //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if ( EnumEq( queue.ptr->familyFlags, VK_QUEUE_PROTECTED_BIT ) )
			pool_info.flags |= VK_COMMAND_POOL_CREATE_PROTECTED_BIT;

		VK_CHECK( dev.vkCreateCommandPool( dev.GetVkDevice(), &pool_info, null, OUT &queue.cmdPool ));
		return true;
	}
	
/*
=================================================
	Execute
=================================================
*/
	bool VFrameGraphThread::Execute ()
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Recording, EState::Compiling ));
		ASSERT( _submissionGraph );
		
		const auto	start_time = TimePoint_t::clock::now();

		CHECK_ERR( _BuildCommandBuffers() );

		if ( _stagingMngr )
			_stagingMngr->OnEndFrame( _isFirstUsage );
		
		if ( _debugger )
			_debugger->OnEndFrame( _cmdBatchId, _indexInBatch );

		// submit
		{
			auto&	frame = _currQueue->frames[_frameId];
			CHECK( _submissionGraph->Submit( _currQueue->ptr, _cmdBatchId, _indexInBatch, frame.pending ));
			
			frame.executed.append( frame.pending );
			frame.pending.clear();
		}

		_taskGraph.OnDiscardMemory();
		_resourceMngr.AfterFrameCompilation();
		
		_cmdBatchId		= Default;
		_indexInBatch	= UMax;
		_currQueue		= null;
		
		const auto	dt = TimePoint_t::clock::now() - start_time;

		for (auto& queue : _queues)
		{
			if ( queue.frames.size() )
			{
				auto&	frame = queue.frames[_frameId];

				_statistic.renderer.cpuTime += frame.executionTime;
				frame.executionTime = dt;
			}
		}

		CHECK_ERR( _SetState( EState::Compiling, EState::Pending ));
		return true;
	}
	
/*
=================================================
	SyncOnExecute
=================================================
*/
	bool VFrameGraphThread::SyncOnExecute (INOUT Statistic_t &outStatistic)
	{
		EXLOCK( _rcCheck );

		if ( not _SetState( EState::Pending, EState::Execute ) )
		{
			// if thread is not used in current frame
			CHECK_ERR( _SetState( EState::Ready, EState::Execute ));
		}
		
		// check for uncommited barriers
		CHECK( _barrierMngr.Empty() );

		_resourceMngr.OnEndFrame();
		_resourceMngr.OnDiscardMemory();
		_mainAllocator.Discard();

		outStatistic.Merge( _statistic );
		_submissionGraph = null;
		
		if ( _swapchain )
			_swapchain->Present( RawImageID() );	// TODO: move to 'Execute' or VSubmissionGraph

		if ( _shaderDebugger )
			_shaderDebugger->OnEndFrame( _shaderDebugCallback );

		CHECK_ERR( _SetState( EState::Execute, EState::Idle ));
		return true;
	}
	
/*
=================================================
	OnWaitIdle
=================================================
*/
	bool VFrameGraphThread::OnWaitIdle ()
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Idle, EState::WaitIdle ));
		
		for (auto& queue : _queues)
		{
			if ( queue.frames.empty() )
				continue;

			auto&	frame	= queue.frames[_frameId];
			auto&	dev		= _instance.GetDevice();

			// recycle commands buffers
			if ( not frame.executed.empty() ) {
				dev.vkFreeCommandBuffers( dev.GetVkDevice(), queue.cmdPool, uint(frame.executed.size()), frame.executed.data() );
			}

			frame.executed.clear();
			ASSERT( frame.pending.empty() );
		}

		// to generate 'on gpu data loaded' events
		if ( _stagingMngr )
		{
			_stagingMngr->OnBeginFrame( _frameId, true );
			_stagingMngr->OnEndFrame( true );
		}

		CHECK_ERR( _SetState( EState::WaitIdle, EState::Idle ));
		return true;
	}

/*
=================================================
	IsInSeparateThread
=================================================
*/
	bool VFrameGraphThread::IsInSeparateThread () const
	{
		const EState	state = _GetState();

		if ( state == EState::Recording )
			return true;

		return false;
	}
	
/*
=================================================
	AddPipelineCompiler
=================================================
*/
	bool VFrameGraphThread::AddPipelineCompiler (const IPipelineCompilerPtr &comp)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsInitialOrIdleState() );

		_resourceMngr.GetPipelineCache()->AddCompiler( comp );
		return true;
	}
	
/*
=================================================
	GetSwapchainImage
=================================================
*
	ImageID  VFrameGraphThread::GetSwapchainImage (ESwapchainImage type)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _swapchain );

		return ImageID{ _swapchain->GetImage( type )};
	}
	*/
	
/*
=================================================
	CreateShaderDebugger
=================================================
*/
	Ptr<VShaderDebugger>  VFrameGraphThread::CreateShaderDebugger ()
	{
		if ( not _shaderDebugger )
		{
			_shaderDebugger.reset( new VShaderDebugger{ *this });
		}
		return _shaderDebugger.get();
	}


}	// FG
