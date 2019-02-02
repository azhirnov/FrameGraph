// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphThread.h"
#include "VMemoryManager.h"
#include "VSwapchainKHR.h"
#include "VStagingBufferManager.h"
#include "Shared/PipelineResourcesInitializer.h"
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
		_threadUsage{ usage },		_state{ EState::Initial },
		_frameId{ 0 },				_debugName{ name },
		_instance{ fg },
		_resourceMngr{ _mainAllocator, _statistic.resources, fg.GetResourceMngr() }
	{
		SCOPELOCK( _rcCheck );

		_mainAllocator.SetBlockSize( 16_Mb );

		if ( EnumEq( _threadUsage, EThreadUsage::MemAllocation ) )
		{
			_memoryMngr.reset( new VMemoryManager{ fg.GetDevice() });
		}

		if ( EnumEq( _threadUsage, EThreadUsage::Transfer ) )
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
		SCOPELOCK( _rcCheck );
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
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return MPipelineID{ _resourceMngr.CreatePipeline( INOUT desc, dbgName, IsInSeparateThread() )};
	}
	
	RTPipelineID  VFrameGraphThread::CreatePipeline (INOUT RayTracingPipelineDesc &desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return RTPipelineID{ _resourceMngr.CreatePipeline( INOUT desc, dbgName, IsInSeparateThread() )};
	}
	
	GPipelineID  VFrameGraphThread::CreatePipeline (INOUT GraphicsPipelineDesc &desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return GPipelineID{ _resourceMngr.CreatePipeline( INOUT desc, dbgName, IsInSeparateThread() )};
	}
	
	CPipelineID  VFrameGraphThread::CreatePipeline (INOUT ComputePipelineDesc &desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return CPipelineID{ _resourceMngr.CreatePipeline( INOUT desc, dbgName, IsInSeparateThread() )};
	}

/*
=================================================
	IsCompatibleWith
=================================================
*/
	bool  VFrameGraphThread::IsCompatibleWith (const FGThreadPtr &thread, EThreadUsage usage) const
	{
		ASSERT( EnumAny( usage, EThreadUsage::_QueueMask ));
		SCOPELOCK( _rcCheck );

		const auto*	other = DynCast<VFrameGraphThread>( thread.operator->() );
		
		if ( not other or not (other->_threadUsage & _threadUsage & usage) )
			return false;

		for (auto& queue : _queues)
		{
			if ( !!usage and EnumEq( queue.usage, usage ) )
			{
				bool	found = false;

				// find same queue in another thread
				for (auto& q : other->_queues)
				{
					if ( q.ptr == queue.ptr and EnumEq( q.usage, usage ) ) {
						found = true;
						break;
					}
				}

				if ( not found )
					return false;
				
				usage &= ~queue.usage;
			}
		}

		return not usage;
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
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		CHECK_ERR( _memoryMngr );

		RawImageID	result = _resourceMngr.CreateImage( desc, mem, *_memoryMngr, _queueFamilyMask, dbgName, IsInSeparateThread() );
		
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
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		CHECK_ERR( _memoryMngr );

		return BufferID{ _resourceMngr.CreateBuffer( desc, mem, *_memoryMngr, _queueFamilyMask, dbgName, IsInSeparateThread() )};
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
	ImageID  VFrameGraphThread::CreateImage (const ExternalImageDesc &desc, OnExternalImageReleased_t &&onRelease, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
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
	BufferID  VFrameGraphThread::CreateBuffer (const ExternalBufferDesc &desc, OnExternalBufferReleased_t &&onRelease, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
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
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return SamplerID{ _resourceMngr.CreateSampler( desc, dbgName, IsInSeparateThread() )};
	}
	
/*
=================================================
	_InitPipelineResources
=================================================
*/
	template <typename PplnID>
	bool  VFrameGraphThread::_InitPipelineResources (const PplnID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		
		auto const *	ppln = _resourceMngr.GetResource( pplnId );
		CHECK_ERR( ppln );

		auto const *	ppln_layout = _resourceMngr.GetResource( ppln->GetLayoutID() );
		CHECK_ERR( ppln_layout );

		RawDescriptorSetLayoutID	layout_id;
		uint						binding;
		CHECK_ERR( ppln_layout->GetDescriptorSetLayout( id, OUT layout_id, OUT binding ));

		VDescriptorSetLayout const*	ds_layout = _resourceMngr.GetResource( layout_id );
		CHECK_ERR( ds_layout );

		CHECK_ERR( PipelineResourcesInitializer::Initialize( OUT resources, layout_id, ds_layout->GetUniforms(), ds_layout->GetMaxIndex()+1 ));
		return true;
	}
	
/*
=================================================
	InitPipelineResources
=================================================
*/
	bool  VFrameGraphThread::InitPipelineResources (const RawGPipelineID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const
	{
		return _InitPipelineResources( pplnId, id, OUT resources );
	}

	bool  VFrameGraphThread::InitPipelineResources (const RawCPipelineID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const
	{
		return _InitPipelineResources( pplnId, id, OUT resources );
	}

	bool  VFrameGraphThread::InitPipelineResources (const RawMPipelineID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const
	{
		return _InitPipelineResources( pplnId, id, OUT resources );
	}

	bool  VFrameGraphThread::InitPipelineResources (const RawRTPipelineID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const
	{
		return _InitPipelineResources( pplnId, id, OUT resources );
	}

/*
=================================================
	CreateRayTracingGeometry
=================================================
*/
	RTGeometryID  VFrameGraphThread::CreateRayTracingGeometry (const RayTracingGeometryDesc &desc, const MemoryDesc &mem, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
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
		SCOPELOCK( _rcCheck );
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

		SCOPELOCK( _rcCheck );
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

/*
=================================================
	_GetDescription
=================================================
*/
	template <typename Desc, typename ID>
	inline Desc const&  VFrameGraphThread::_GetDescription (const ID &id) const
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		// read access available without synchronizations
		return _resourceMngr.GetDescription( id.Get() );
	}
	
/*
=================================================
	GetDescription
=================================================
*/
	BufferDesc const&  VFrameGraphThread::GetDescription (const BufferID &id) const
	{
		return _GetDescription<BufferDesc>( id );
	}

	ImageDesc const&  VFrameGraphThread::GetDescription (const ImageID &id) const
	{
		return _GetDescription<ImageDesc>( id );
	}
	
	/*SamplerDesc const&  VFrameGraphThread::GetDescription (SamplerID id) const
	{
		return _GetDescription<SamplerDesc>( id );
	}*/
	
/*
=================================================
	Initialize
=================================================
*/
	bool VFrameGraphThread::Initialize (const SwapchainCreateInfo *swapchainCI, ArrayView<FGThreadPtr> relativeThreads, ArrayView<FGThreadPtr> parallelThreads)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Initial, EState::Initializing ));
		ASSERT( parallelThreads.empty() );		// not supported yet
		ASSERT( relativeThreads.size() <= 1 );	// only 1 thread supported yet

		// create swapchain
		if ( swapchainCI )
		{
			CHECK_ERR( EnumEq( _threadUsage, EThreadUsage::Present ));

			CHECK_ERR( Visit( *swapchainCI,
					[this] (const VulkanSwapchainCreateInfo &info)	{ return _RecreateSwapchain( info ); },
					[] (const auto &)								{ RETURN_ERR( "unsupported swapchain create info!", false ); }
				));
		}
		
		CHECK_ERR( _SetupQueues( relativeThreads.empty() ? null : DynCast<VFrameGraphThread>(relativeThreads.front()) ));
		_queueFamilyMask = Default;

		for (auto& queue : _queues)
		{
			queue.frames.resize( GetRingBufferSize() );

			_queueFamilyMask |= queue.ptr->familyIndex;

			CHECK_ERR( _CreateCommandBuffers( INOUT queue ));
		}

		CHECK_ERR( _InitGpuQueries( INOUT _queues, INOUT _queryPool ));

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
	_CreateCommandBuffers
=================================================
*/
	bool VFrameGraphThread::_CreateCommandBuffers (INOUT PerQueue &queue) const
	{
		CHECK_ERR( not queue.cmdPoolId );

		VDevice const&	dev = GetDevice();
		
		VkCommandPoolCreateInfo		pool_info = {};
		pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex	= uint(queue.ptr->familyIndex);
		pool_info.flags				= 0; //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if ( EnumEq( queue.ptr->familyFlags, VK_QUEUE_PROTECTED_BIT ) )
			pool_info.flags |= VK_COMMAND_POOL_CREATE_PROTECTED_BIT;

		VK_CHECK( dev.vkCreateCommandPool( dev.GetVkDevice(), &pool_info, null, OUT &queue.cmdPoolId ));
		return true;
	}
	
/*
=================================================
	_InitGpuQueries
=================================================
*/
	bool VFrameGraphThread::_InitGpuQueries (INOUT PerQueueArray_t &queues, INOUT VkQueryPool &pool) const
	{
		CHECK_ERR( not pool );
		
		VkQueryPoolCreateInfo	info = {};
		info.sType		= VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		info.queryType	= VK_QUERY_TYPE_TIMESTAMP;
		info.queryCount	= uint(queues.size() * queues.front().frames.size()) * 2;
		
		VDevice const&	dev = GetDevice();
		VK_CHECK( dev.vkCreateQueryPool( dev.GetVkDevice(), &info, null, OUT &pool ));

		uint	index = 0;
		for (auto& queue : queues)
		{
			for (auto& frame : queue.frames)
			{
				frame.queryIndex = 2 * (index++);
			}
		}
		return true;
	}

/*
=================================================
	SignalSemaphore
=================================================
*/
	void VFrameGraphThread::SignalSemaphore (VkSemaphore sem)
	{
		SCOPELOCK( _rcCheck );
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
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _submissionGraph, void());
		CHECK( _submissionGraph->WaitSemaphore( _cmdBatchId, sem, stage ));
	}
	
/*
=================================================
	_GetQueue
=================================================
*/
	VFrameGraphThread::PerQueue*  VFrameGraphThread::_GetQueue (EThreadUsage usage)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		ASSERT( EnumAny( usage, _threadUsage ));
		ASSERT( EnumAny( usage, EThreadUsage::_QueueMask ));
		
		for (auto& queue : _queues)
		{
			if ( EnumEq( queue.usage, usage ) )
				return &queue;
		}
		return null;
	}
	
/*
=================================================
	_GetAnyGraphicsQueue
=================================================
*/
	VDeviceQueueInfoPtr  VFrameGraphThread::_GetAnyGraphicsQueue () const
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		const EThreadUsage		any_usage	= (_threadUsage & EThreadUsage::_QueueMask);
		VDeviceQueueInfoPtr		best_match;

		for (auto& queue : _queues)
		{
			if ( EnumEq( queue.usage, _currUsage ) )
				return queue.ptr;
			
			if ( EnumAny( queue.usage, any_usage ) )
				best_match = queue.ptr;
		}

		return best_match;
	}

/*
=================================================
	_CreateCommandBuffer
=================================================
*/
	VkCommandBuffer  VFrameGraphThread::_CreateCommandBuffer () const
	{
		CHECK_ERR( _currQueue );
		ASSERT( _currQueue->cmdPoolId );

		VkCommandBufferAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext				= null;
		info.commandPool		= _currQueue->cmdPoolId;
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
	_SetupQueues
=================================================
*/
	bool VFrameGraphThread::_SetupQueues (const SharedPtr<VFrameGraphThread> &relativeThread)
	{
		CHECK_ERR( _queues.empty() );

		const bool	graphics_present	= EnumEq( _threadUsage, EThreadUsage::Graphics ) and EnumEq( _threadUsage, EThreadUsage::Present );
		const bool	compute_present		= EnumEq( _threadUsage, EThreadUsage::AsyncCompute ) and EnumEq( _threadUsage, EThreadUsage::Present );

		// graphics
		if ( graphics_present )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::Graphics | EThreadUsage::Present, relativeThread ))
		else
		if ( EnumEq( _threadUsage, EThreadUsage::Graphics ) )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::Graphics, relativeThread ))
		else
		if ( EnumEq( _threadUsage, EThreadUsage::Transfer ) )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::Transfer, relativeThread ));


		// compute only
		if ( not graphics_present and compute_present )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::AsyncCompute | EThreadUsage::Present, relativeThread ))
		else
		if ( EnumEq( _threadUsage, EThreadUsage::AsyncCompute ) )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::AsyncCompute, relativeThread ));


		// transfer only
		if ( EnumEq( _threadUsage, EThreadUsage::AsyncStreaming ) )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::AsyncStreaming, relativeThread ));

		CHECK_ERR( not _queues.empty() );
		return true;
	}
	
/*
=================================================
	_AddGraphicsQueue
=================================================
*/
	bool VFrameGraphThread::_AddGpuQueue (const EThreadUsage usage, const SharedPtr<VFrameGraphThread> &relativeThread)
	{
		if ( relativeThread )
		{
			EThreadUsage	new_usage = (usage & ~EThreadUsage::Present);

			if ( auto* queue = relativeThread->_GetQueue( new_usage ) )
			{
				// TODO: check is swapchain can present on this queue

				_queues.push_back( PerQueue{} );
				auto&	q = _queues.back();

				q.ptr	= queue->ptr;
				q.usage	= usage;
				return true;
			}
		}

		switch ( usage )
		{
			case EThreadUsage::Graphics | EThreadUsage::Present :
				return _AddGraphicsAndPresentQueue();

			case EThreadUsage::Graphics :
				return _AddGraphicsQueue();

			case EThreadUsage::Transfer :		// TODO: remove?
				return _AddTransferQueue();

			case EThreadUsage::AsyncCompute | EThreadUsage::Present :
				return _AddAsyncComputeAndPresentQueue();
				
			case EThreadUsage::AsyncCompute :
				return _AddAsyncComputeQueue();

			case EThreadUsage::AsyncStreaming :
				return _AddAsyncStreamingQueue();
		}

		RETURN_ERR( "unsupported queue usage!" );
	}

/*
=================================================
	_AddGraphicsQueue
=================================================
*/
	bool VFrameGraphThread::_AddGraphicsQueue ()
	{
		for (auto& queue : GetDevice().GetVkQueues())
		{
			if ( EnumEq( queue.familyFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) )
			{
				_queues.push_back( PerQueue{} );
				auto&	q = _queues.back();

				q.ptr	= &queue;
				q.usage	= EThreadUsage::Graphics;
				return true;
			}
		}
		return false;
	}
	
/*
=================================================
	_AddGraphicsAndPresentQueue
=================================================
*/
	bool VFrameGraphThread::_AddGraphicsAndPresentQueue ()
	{
		CHECK_ERR( _swapchain );

		for (auto& queue : GetDevice().GetVkQueues())
		{
			if ( EnumEq( queue.familyFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) and
				 _swapchain->IsCompatibleWithQueue( queue.familyIndex ) )
			{
				_queues.push_back( PerQueue{} );
				auto&	q = _queues.back();

				q.ptr	= &queue;
				q.usage	= EThreadUsage::Graphics | EThreadUsage::Present;

				CHECK_ERR( _swapchain->Initialize( q.ptr ));
				return true;
			}
		}
		return false;
	}

/*
=================================================
	_AddTransferQueue
=================================================
*/
	bool VFrameGraphThread::_AddTransferQueue ()
	{
		for (auto& queue : GetDevice().GetVkQueues())
		{
			if ( EnumEq( queue.familyFlags, VK_QUEUE_TRANSFER_BIT ) )
			{
				_queues.push_back( PerQueue{} );
				auto&	q = _queues.back();

				q.ptr	= &queue;
				q.usage	= EThreadUsage::Transfer;
				return true;
			}
		}
		return false;
	}
	
/*
=================================================
	_AddAsyncComputeQueue
=================================================
*/
	bool VFrameGraphThread::_AddAsyncComputeQueue ()
	{
		VDeviceQueueInfoPtr		best_match;
		VDeviceQueueInfoPtr		compatible;

		for (auto& queue : GetDevice().GetVkQueues())
		{
			if ( EnumEq( queue.familyFlags, VK_QUEUE_COMPUTE_BIT ) and
				 not EnumEq( queue.familyFlags, VK_QUEUE_GRAPHICS_BIT ) )
			{
				best_match = &queue;
				continue;
			}
			
			if ( EnumEq( queue.familyFlags, VK_QUEUE_COMPUTE_BIT ) and 
				 (not compatible or BitSet<32>{compatible->familyFlags}.count() > BitSet<32>{queue.familyFlags}.count()) )
			{
				compatible =  &queue;
			}
		}

		if ( not best_match )
			best_match = compatible;

		if ( best_match )
		{
			_queues.push_back( PerQueue{} );
			auto&	q = _queues.back();

			q.ptr	= best_match;
			q.usage	= EThreadUsage::AsyncCompute;
			return true;
		}

		return false;
	}
	
/*
=================================================
	_AddAsyncComputeAndPresentQueue
=================================================
*/
	bool VFrameGraphThread::_AddAsyncComputeAndPresentQueue ()
	{
		CHECK_ERR( _swapchain );

		VDeviceQueueInfoPtr		best_match;
		VDeviceQueueInfoPtr		compatible;

		for (auto& queue : GetDevice().GetVkQueues())
		{
			if ( EnumEq( queue.familyFlags, VK_QUEUE_COMPUTE_BIT )		and
				 not EnumEq( queue.familyFlags, VK_QUEUE_GRAPHICS_BIT )	and
				 _swapchain->IsCompatibleWithQueue( queue.familyIndex ) )
			{
				best_match = &queue;
				continue;
			}
			
			if ( EnumEq( queue.familyFlags, VK_QUEUE_COMPUTE_BIT ) and 
				 (not compatible or BitSet<32>{compatible->familyFlags}.count() > BitSet<32>{queue.familyFlags}.count()) and
				 _swapchain->IsCompatibleWithQueue( queue.familyIndex ) )
			{
				compatible =  &queue;
			}
		}

		if ( not best_match )
			best_match = compatible;

		if ( best_match )
		{
			_queues.push_back( PerQueue{} );
			auto&	q = _queues.back();

			q.ptr	= best_match;
			q.usage	= EThreadUsage::AsyncCompute | EThreadUsage::Present;

			CHECK_ERR( _swapchain->Initialize( q.ptr ));
			return true;
		}

		return false;
	}

/*
=================================================
	_AddAsyncStreamingQueue
=================================================
*/
	bool VFrameGraphThread::_AddAsyncStreamingQueue ()
	{
		VDeviceQueueInfoPtr		best_match;
		VDeviceQueueInfoPtr		compatible;

		for (auto& queue : GetDevice().GetVkQueues())
		{
			if ( queue.familyFlags == VK_QUEUE_TRANSFER_BIT )
			{
				best_match = &queue;
				continue;
			}
			
			if ( EnumEq( queue.familyFlags, VK_QUEUE_TRANSFER_BIT ) and 
				 (not compatible or BitSet<32>{compatible->familyFlags}.count() > BitSet<32>{queue.familyFlags}.count()) )
			{
				compatible =  &queue;
			}
		}

		if ( not best_match )
			best_match = compatible;

		if ( best_match )
		{
			_queues.push_back( PerQueue{} );
			auto&	q = _queues.back();

			q.ptr	= best_match;
			q.usage	= EThreadUsage::AsyncStreaming;
			return true;
		}

		return false;
	}

/*
=================================================
	Deinitialize
=================================================
*/
	void VFrameGraphThread::Deinitialize ()
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Idle, EState::BeforeDestroy ), void());
		
		VDevice const&	dev = GetDevice();

		for (auto& queue : _queues)
		{
			VK_CALL( dev.vkQueueWaitIdle( queue.ptr->handle ));
			
			if ( queue.cmdPoolId )
			{
				dev.vkDestroyCommandPool( dev.GetVkDevice(), queue.cmdPoolId, null );
				queue.cmdPoolId = VK_NULL_HANDLE;
			}
		}
		_queues.clear();

		if ( _queryPool ) {
			dev.vkDestroyQueryPool( dev.GetVkDevice(), _queryPool, null );
			_queryPool = VK_NULL_HANDLE;
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
		SCOPELOCK( _rcCheck );
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
	bool VFrameGraphThread::RecreateSwapchain (const SwapchainCreateInfo &ci)
	{
		SCOPELOCK( _rcCheck );
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

		// find suitable queue
		if ( _IsInitialized() )
		{
			bool	initialized = false;

			for (auto& queue : _queues)
			{
				if ( sc->IsCompatibleWithQueue( queue.ptr->familyIndex ) )
				{
					CHECK_ERR( sc->Initialize( queue.ptr ));
					initialized = true;
					break;
				}
			}
			CHECK_ERR( initialized );
		}

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
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Idle, EState::BeforeStart ));
		ASSERT( graph );

		_frameId		 = (_frameId + 1) % _queues.front().frames.size();
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
	bool VFrameGraphThread::Begin (const CommandBatchID &id, uint index, EThreadUsage usage)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( EnumAny( usage, _threadUsage ));
		ASSERT( EnumAny( usage, EThreadUsage::_QueueMask ));
		
		if ( _SetState( EState::Ready, EState::BeforeRecording ) )
		{
			StaticArray<uint64_t, 2>	query_results;

			// recycle commands buffers
			for (auto& queue : _queues)
			{
				auto&	frame	= queue.frames[_frameId];
				auto&	dev		= _instance.GetDevice();

				if ( not frame.executed.empty() )
				{
					dev.vkFreeCommandBuffers( dev.GetVkDevice(), queue.cmdPoolId, uint(frame.executed.size()), frame.executed.data() );

					// read frame time
					VK_CALL( dev.vkGetQueryPoolResults( dev.GetVkDevice(), _queryPool, frame.queryIndex, uint(query_results.size()),
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

		_cmdBatchId		= id;
		_indexInBatch	= index;
		_currUsage		= (usage & _threadUsage);
		_currQueue		= _GetQueue( _currUsage );

		CHECK_ERR( _SetState( EState::BeforeRecording, EState::Recording ));
		return true;
	}
	
/*
=================================================
	Execute
=================================================
*/
	bool VFrameGraphThread::Execute ()
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Recording, EState::Compiling ));
		ASSERT( _submissionGraph );
		
		const auto	start_time = TimePoint_t::clock::now();

		CHECK_ERR( _BuildCommandBuffers() );

		if ( _stagingMngr )
			_stagingMngr->OnEndFrame( _isFirstUsage );
		
		if ( _debugger )
			_debugger->OnEndFrame( _cmdBatchId, _indexInBatch );

		for (auto& queue : _queues)
		{
			if ( not EnumEq( queue.usage, _currUsage ) )
				continue;
			
			auto&	frame = queue.frames[_frameId];
			CHECK( _submissionGraph->Submit( queue.ptr, _cmdBatchId, _indexInBatch, frame.pending ));

			frame.executed.append( frame.pending );
			frame.pending.clear();
		}

		_taskGraph.OnDiscardMemory();
		_resourceMngr.AfterFrameCompilation();
		
		_cmdBatchId		= Default;
		_indexInBatch	= UMax;
		_currUsage		= Default;
		_currQueue		= null;
		
		const auto	dt = TimePoint_t::clock::now() - start_time;

		for (auto& queue : _queues)
		{
			auto&	frame = queue.frames[_frameId];

			_statistic.renderer.cpuTime += frame.executionTime;
			frame.executionTime = dt;
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
		SCOPELOCK( _rcCheck );

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
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Idle, EState::WaitIdle ));
		
		for (auto& queue : _queues)
		{
			auto&	frame	= queue.frames[_frameId];
			auto&	dev		= _instance.GetDevice();

			// recycle commands buffers
			if ( not frame.executed.empty() ) {
				dev.vkFreeCommandBuffers( dev.GetVkDevice(), queue.cmdPoolId, uint(frame.executed.size()), frame.executed.data() );
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
		SCOPELOCK( _rcCheck );
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
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _swapchain );

		return ImageID{ _swapchain->GetImage( type )};
	}
	*/
	
/*
=================================================
	GetShaderDebugger
=================================================
*/
	VShaderDebugger*  VFrameGraphThread::GetShaderDebugger ()
	{
		if ( not _shaderDebugger )
		{
			_shaderDebugger.reset( new VShaderDebugger{ *this });
		}
		return _shaderDebugger.get();
	}


}	// FG
