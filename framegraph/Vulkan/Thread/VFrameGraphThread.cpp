// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphThread.h"
#include "VMemoryManager.h"
#include "VSwapchainKHR.h"
#include "VStagingBufferManager.h"
#include "Shared/PipelineResourcesInitializer.h"
#include "VTaskGraph.hpp"
#include "VFrameGraphDebugger.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VFrameGraphThread::VFrameGraphThread (VFrameGraph &fg, EThreadUsage usage, const FGThreadPtr &relative, StringView name) :
		_threadUsage{ usage },			_state{ EState::Initial },
		_compilationFlags{ Default },	_relativeThread{ DynCast<VFrameGraphThread>(relative) },
		_debugName{ name },				_instance{ fg },
		_resourceMngr{ _mainAllocator, fg.GetResourceMngr() }
	{
		SCOPELOCK( _rcCheck );

		if ( relative ) {
			ASSERT( EnumAny( _threadUsage, relative->GetThreadUsage() & EThreadUsage::_QueueMask ));
		}

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
	MPipelineID  VFrameGraphThread::CreatePipeline (MeshPipelineDesc &&desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return MPipelineID{ _resourceMngr.CreatePipeline( std::move(desc), dbgName, IsInSeparateThread() )};
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	RTPipelineID  VFrameGraphThread::CreatePipeline (RayTracingPipelineDesc &&desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return RTPipelineID{ _resourceMngr.CreatePipeline( std::move(desc), dbgName, IsInSeparateThread() )};
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	GPipelineID  VFrameGraphThread::CreatePipeline (GraphicsPipelineDesc &&desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return GPipelineID{ _resourceMngr.CreatePipeline( std::move(desc), dbgName, IsInSeparateThread() )};
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	CPipelineID  VFrameGraphThread::CreatePipeline (ComputePipelineDesc &&desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return CPipelineID{ _resourceMngr.CreatePipeline( std::move(desc), dbgName, IsInSeparateThread() )};
	}

/*
=================================================
	GetDescriptorSet
=================================================
*/
	template <typename PplnID>
	inline bool  VFrameGraphThread::_GetDescriptorSet (const PplnID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const
	{
		auto const *	ppln = _resourceMngr.GetResource( pplnId.Get() );
		CHECK_ERR( ppln );

		auto const *	ppln_layout = _resourceMngr.GetResource( ppln->GetLayoutID() );
		CHECK_ERR( ppln_layout );
		
		CHECK_ERR( ppln_layout->GetDescriptorSetLayout( id, OUT layout, OUT binding ));
		return true;
	}

	bool  VFrameGraphThread::GetDescriptorSet (const GPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return _GetDescriptorSet( pplnId, id, OUT layout, OUT binding );
	}

	bool  VFrameGraphThread::GetDescriptorSet (const CPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return _GetDescriptorSet( pplnId, id, OUT layout, OUT binding );
	}

	bool  VFrameGraphThread::GetDescriptorSet (const MPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return _GetDescriptorSet( pplnId, id, OUT layout, OUT binding );
	}

	bool  VFrameGraphThread::GetDescriptorSet (const RTPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		return _GetDescriptorSet( pplnId, id, OUT layout, OUT binding );
	}
	
/*
=================================================
	IsCompatibleWith
=================================================
*/
	bool  VFrameGraphThread::IsCompatibleWith (const FGThreadPtr &thread, EThreadUsage usage) const
	{
		ASSERT( EnumAny( usage, EThreadUsage::_QueueMask ));
		CHECK_ERR( thread );
		SCOPELOCK( _rcCheck );

		const auto*	other = Cast<VFrameGraphThread>( thread.operator->() );

		if ( not (other->_threadUsage & _threadUsage & usage) )
			return false;

		for (auto& queue : _queues)
		{
			if ( EnumEq( queue.usage, usage ) )
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
	ImageID  VFrameGraphThread::CreateImage (const MemoryDesc &mem, const ImageDesc &desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		CHECK_ERR( _memoryMngr );

		VDeviceQueueInfoPtr  queue = _GetAnyGraphicsQueue();
		CHECK_ERR( queue );

		RawImageID	result = _resourceMngr.CreateImage( mem, desc, *_memoryMngr, queue->familyIndex, dbgName, IsInSeparateThread() );
		
		// add first image layout transition
		if ( result )
		{
			TransitImageLayoutToDefault( result, VK_IMAGE_LAYOUT_UNDEFINED, uint(queue->familyIndex),
										 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
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
	BufferID  VFrameGraphThread::CreateBuffer (const MemoryDesc &mem, const BufferDesc &desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		CHECK_ERR( _memoryMngr );
		
		VDeviceQueueInfoPtr  queue = _GetAnyGraphicsQueue();
		CHECK_ERR( queue );

		return BufferID{ _resourceMngr.CreateBuffer( mem, desc, *_memoryMngr, queue->familyIndex, dbgName, IsInSeparateThread() )};
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

		auto*	img_desc = std::get_if<VulkanImageDesc>( &desc );
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
		
		auto*	buf_desc = std::get_if<VulkanBufferDesc>( &desc );
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
	InitPipelineResources
=================================================
*/
	bool  VFrameGraphThread::InitPipelineResources (RawDescriptorSetLayoutID layoutId, OUT PipelineResources &resources) const
	{
		SCOPELOCK( _rcCheck );
		ASSERT( layoutId );
		ASSERT( _IsInitialized() );

		VDescriptorSetLayout const*	ds_layout = _resourceMngr.GetResource( layoutId );
		CHECK_ERR( ds_layout );

		CHECK_ERR( PipelineResourcesInitializer::Initialize( OUT resources, layoutId, ds_layout->GetUniforms(), ds_layout->GetMaxIndex()+1 ));
		return true;
	}
	
/*
=================================================
	CreateRayTracingGeometry
=================================================
*/
	RTGeometryID  VFrameGraphThread::CreateRayTracingGeometry (const RayTracingGeometryDesc &desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		
		VDeviceQueueInfoPtr  queue = _GetAnyGraphicsQueue();
		CHECK_ERR( queue );

		return RTGeometryID{ _resourceMngr.CreateRayTracingGeometry( desc, *_memoryMngr, queue->familyIndex, dbgName, IsInSeparateThread() )};
	}
	
/*
=================================================
	CreateRayTracingScene
=================================================
*/
	RTSceneID  VFrameGraphThread::CreateRayTracingScene (const RayTracingSceneDesc &desc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );
		
		VDeviceQueueInfoPtr  queue = _GetAnyGraphicsQueue();
		CHECK_ERR( queue );

		return RTSceneID{ _resourceMngr.CreateRayTracingScene( desc, *_memoryMngr, queue->familyIndex, dbgName, IsInSeparateThread() )};
	}

/*
=================================================
	_DestroyResource
=================================================
*/
	template <typename ID>
	inline void VFrameGraphThread::_DestroyResource (INOUT ID &id)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialized() );

		return _resourceMngr.DestroyResource( id.Release(), IsInSeparateThread() );
	}
	
/*
=================================================
	DestroyResource
=================================================
*/
	void VFrameGraphThread::DestroyResource (INOUT GPipelineID &id)		{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT CPipelineID &id)		{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT MPipelineID &id)		{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT RTPipelineID &id)	{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT ImageID &id)			{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT BufferID &id)		{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT SamplerID &id)		{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT RTGeometryID &id)	{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT RTSceneID &id)		{ _DestroyResource( INOUT id ); }

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
		return _resourceMngr.GetResource( id.Get() )->Description();
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
	bool VFrameGraphThread::Initialize (const SwapchainCreateInfo *swapchainCI)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Initial, EState::Initializing ));

		// create swapchain
		if ( swapchainCI )
		{
			CHECK_ERR( EnumEq( _threadUsage, EThreadUsage::Present ));

			CHECK_ERR( Visit( *swapchainCI,
					[this] (const VulkanSwapchainCreateInfo &info)	{ return _RecreateSwapchain( info ); },
					[] (const auto &)								{ RETURN_ERR( "unsupported swapchain create info!", false ); }
				));
		}
		
		CHECK_ERR( _SetupQueues() );

		for (auto& queue : _queues)
		{
			queue.frames.resize( GetRingBufferSize() );

			CHECK_ERR( _CreateCommandBuffers( INOUT queue ));
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
	VkCommandBuffer  VFrameGraphThread::_CreateCommandBuffer (EThreadUsage usage)
	{
		auto*	queue = _GetQueue( usage );
		CHECK_ERR( queue );
		ASSERT( queue->cmdPoolId );

		VkCommandBufferAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext				= null;
		info.commandPool		= queue->cmdPoolId;
		info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandBufferCount	= 1;
				
		VDevice const&		dev	= GetDevice();
		VkCommandBuffer		cmd;
		VK_CHECK( dev.vkAllocateCommandBuffers( dev.GetVkDevice(), &info, OUT &cmd ));

		queue->frames[_frameId].commands.push_back( cmd );
		return cmd;
	}

/*
=================================================
	_SetupQueues
=================================================
*/
	bool VFrameGraphThread::_SetupQueues ()
	{
		CHECK_ERR( _queues.empty() );

		const bool	graphics_present	= EnumEq( _threadUsage, EThreadUsage::Graphics ) and EnumEq( _threadUsage, EThreadUsage::Present );
		const bool	compute_present		= EnumEq( _threadUsage, EThreadUsage::AsyncCompute ) and EnumEq( _threadUsage, EThreadUsage::Present );

		// graphics
		if ( graphics_present )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::Graphics | EThreadUsage::Present ))
		else
		if ( EnumEq( _threadUsage, EThreadUsage::Graphics ) )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::Graphics ))
		else
		if ( EnumEq( _threadUsage, EThreadUsage::Transfer ) )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::Transfer ));


		// compute only
		if ( not graphics_present and compute_present )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::AsyncCompute | EThreadUsage::Present ))
		else
		if ( EnumEq( _threadUsage, EThreadUsage::AsyncCompute ) )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::AsyncCompute ));


		// transfer only
		if ( EnumEq( _threadUsage, EThreadUsage::AsyncStreaming ) )
			CHECK_ERR( _AddGpuQueue( EThreadUsage::AsyncStreaming ));

		CHECK_ERR( not _queues.empty() );
		return true;
	}
	
/*
=================================================
	_AddGraphicsQueue
=================================================
*/
	bool VFrameGraphThread::_AddGpuQueue (const EThreadUsage usage)
	{
		// TODO:race condition protection for '_relativeThread'
		if ( auto thread = _relativeThread.lock() )
		{
			EThreadUsage	new_usage = (usage & ~EThreadUsage::Present);

			if ( auto* queue = thread->_GetQueue( new_usage ) )
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

		if ( _stagingMngr )
			_stagingMngr->Deinitialize();

		if ( _swapchain )
			_swapchain->Deinitialize();
		
		_resourceMngr.Deinitialize();

		if ( _memoryMngr )
			_memoryMngr->Deinitialize();

		_swapchain.reset();
		_memoryMngr.reset();
		_stagingMngr.reset();
		_mainAllocator.Release();
		
		CHECK_ERR( _SetState( EState::BeforeDestroy, EState::Destroyed ), void());
	}
	
/*
=================================================
	SetCompilationFlags
=================================================
*/
	void VFrameGraphThread::SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _IsInitialOrIdleState() );

		_compilationFlags = flags;
		
		if ( EnumEq( _compilationFlags, ECompilationFlags::EnableDebugger ) )
		{
			if ( not _debugger )
				_debugger.reset( new VFrameGraphDebugger( *_instance.GetDebugger() ));

			_debugger->Setup( debugFlags );
		}
		else
			_debugger.reset();
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

		_frameId = (_frameId + 1) % _queues.front().frames.size();
		_submissionGraph = graph;

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
		CHECK_ERR( _SetState( EState::Ready, EState::BeforeRecording ));

		ASSERT( EnumAny( usage, _threadUsage ));
		ASSERT( EnumAny( usage, EThreadUsage::_QueueMask ));

		VDevice const&	dev = _instance.GetDevice();

		for (auto& queue : _queues)
		{
			auto&	frame = queue.frames[_frameId];

			// recycle commands buffers
			if ( not frame.commands.empty() ) {
				dev.vkFreeCommandBuffers( dev.GetVkDevice(), queue.cmdPoolId, uint(frame.commands.size()), frame.commands.data() );
			}
			frame.commands.clear();
		}

		_taskGraph.OnStart( this );

		if ( _stagingMngr )
			_stagingMngr->OnBeginFrame( _frameId );

		_cmdBatchId		= id;
		_indexInBatch	= index;
		_currUsage		= (usage & _threadUsage);

		CHECK_ERR( _SetState( EState::BeforeRecording, EState::Recording ));
		return true;
	}
	
/*
=================================================
	Compile
=================================================
*/
	bool VFrameGraphThread::Compile ()
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Recording, EState::Compiling ));
		ASSERT( _submissionGraph );

		if ( _stagingMngr )
			_stagingMngr->OnEndFrame();

		CHECK_ERR( _BuildCommandBuffers() );
		
		if ( _debugger )
			_debugger->OnEndFrame( _cmdBatchId, _indexInBatch );

		for (auto& queue : _queues) {
			CHECK( _submissionGraph->Submit( queue.ptr, _cmdBatchId, _indexInBatch, queue.frames[_frameId].commands ));
		}
		_submissionGraph = null;

		_taskGraph.OnDiscardMemory();
		
		_cmdBatchId		= Default;
		_indexInBatch	= ~0u;
		_currUsage		= Default;

		CHECK_ERR( _SetState( EState::Compiling, EState::Pending ));
		return true;
	}
	
/*
=================================================
	SyncOnExecute
=================================================
*/
	bool VFrameGraphThread::SyncOnExecute ()
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
		
		if ( _swapchain )
			_swapchain->Present( RawImageID() );	// TODO

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
		
		VDevice const&	dev = _instance.GetDevice();

		for (auto& queue : _queues)
		{
			auto&	frame = queue.frames[_frameId];

			// recycle commands buffers
			if ( not frame.commands.empty() ) {
				dev.vkFreeCommandBuffers( dev.GetVkDevice(), queue.cmdPoolId, uint(frame.commands.size()), frame.commands.data() );
			}
			frame.commands.clear();
		}

		// to generate 'on gpu data loaded' events
		if ( _stagingMngr )
		{
			_stagingMngr->OnBeginFrame( _frameId );
			_stagingMngr->OnEndFrame();
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

}	// FG
