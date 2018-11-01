// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphThread.h"
#include "VMemoryManager.h"
#include "VSwapchainKHR.h"
#include "VStagingBufferManager.h"
#include "Shared/PipelineResourcesInitializer.h"
#include "VTaskGraph.hpp"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VFrameGraphThread::VFrameGraphThread (VFrameGraph &fg, EThreadUsage usage) :
		_usage{ usage },				_state{ EState::Initial },
		_compilationFlags{ Default },	_instance{ fg },
		_resourceMngr{ _mainAllocator, fg.GetResourceMngr() }
	{
		if ( EnumEq( _usage, EThreadUsage::MemAllocation ) )
		{
			_memoryMngr.reset( new VMemoryManager{ fg.GetDevice() });
		}

		if ( EnumEq( _usage, EThreadUsage::Transfer ) )
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
		CHECK( _GetState() == EState::Destroyed );
	}
	
/*
=================================================
	_GetState / _SetState
=================================================
*/
	inline void  VFrameGraphThread::_SetState (EState newState)
	{
		_state.store( newState, std::memory_order_release );
	}

	inline bool  VFrameGraphThread::_SetState (EState expected, EState newState)
	{
		return _state.compare_exchange_strong( INOUT expected, newState, std::memory_order_release, std::memory_order_relaxed );
	}
	
	inline bool  VFrameGraphThread::_IsInitialized () const
	{
		const EState	state = _GetState();
		return state > EState::Initial and state < EState::Failed;
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
	MPipelineID  VFrameGraphThread::CreatePipeline (MeshPipelineDesc &&desc)
	{
		ASSERT( _IsInitialized() );

		return MPipelineID{ _resourceMngr.CreatePipeline( std::move(desc), _IsInSeparateThread() )};
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	RTPipelineID  VFrameGraphThread::CreatePipeline (RayTracingPipelineDesc &&desc)
	{
		ASSERT( _IsInitialized() );

		return RTPipelineID{ _resourceMngr.CreatePipeline( std::move(desc), _IsInSeparateThread() )};
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	GPipelineID  VFrameGraphThread::CreatePipeline (GraphicsPipelineDesc &&desc)
	{
		ASSERT( _IsInitialized() );

		return GPipelineID{ _resourceMngr.CreatePipeline( std::move(desc), _IsInSeparateThread() )};
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	CPipelineID  VFrameGraphThread::CreatePipeline (ComputePipelineDesc &&desc)
	{
		ASSERT( _IsInitialized() );

		return CPipelineID{ _resourceMngr.CreatePipeline( std::move(desc), _IsInSeparateThread() )};
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
		ASSERT( _IsInitialized() );
		return _GetDescriptorSet( pplnId, id, OUT layout, OUT binding );
	}

	bool  VFrameGraphThread::GetDescriptorSet (const CPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const
	{
		ASSERT( _IsInitialized() );
		return _GetDescriptorSet( pplnId, id, OUT layout, OUT binding );
	}

	bool  VFrameGraphThread::GetDescriptorSet (const MPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const
	{
		ASSERT( _IsInitialized() );
		return _GetDescriptorSet( pplnId, id, OUT layout, OUT binding );
	}

	bool  VFrameGraphThread::GetDescriptorSet (const RTPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const
	{
		ASSERT( _IsInitialized() );
		return _GetDescriptorSet( pplnId, id, OUT layout, OUT binding );
	}

/*
=================================================
	CreateImage
=================================================
*/
	ImageID  VFrameGraphThread::CreateImage (const MemoryDesc &mem, const ImageDesc &desc)
	{
		ASSERT( _IsInitialized() );
		CHECK_ERR( _memoryMngr );
		
		return ImageID{ _resourceMngr.CreateImage( mem, desc, *_memoryMngr, _IsInSeparateThread() )};
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
	BufferID  VFrameGraphThread::CreateBuffer (const MemoryDesc &mem, const BufferDesc &desc)
	{
		ASSERT( _IsInitialized() );
		CHECK_ERR( _memoryMngr );
		
		return BufferID{ _resourceMngr.CreateBuffer( mem, desc, *_memoryMngr, _IsInSeparateThread() )};
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
	CreateSampler
=================================================
*/
	SamplerID  VFrameGraphThread::CreateSampler (const SamplerDesc &desc)
	{
		ASSERT( _IsInitialized() );

		return SamplerID{ _resourceMngr.CreateSampler( desc, _IsInSeparateThread() )};
	}
	
/*
=================================================
	InitPipelineResources
=================================================
*/
	bool  VFrameGraphThread::InitPipelineResources (RawDescriptorSetLayoutID layoutId, OUT PipelineResources &resources) const
	{
		ASSERT( layoutId );
		ASSERT( _IsInitialized() );

		VDescriptorSetLayout const*	ds_layout = _resourceMngr.GetResource( layoutId );
		CHECK_ERR( ds_layout );

		CHECK_ERR( PipelineResourcesInitializer::Initialize( OUT resources, layoutId, ds_layout->GetUniforms(), ds_layout->GetMaxIndex()+1 ));
		return true;
	}

/*
=================================================
	_DestroyResource
=================================================
*/
	template <typename ID>
	inline void VFrameGraphThread::_DestroyResource (INOUT ID &id)
	{
		ASSERT( _IsInitialized() );

		return _resourceMngr.DestroyResource( id.Release(), _IsInSeparateThread() );
	}
	
/*
=================================================
	DestroyResource
=================================================
*/
	void VFrameGraphThread::DestroyResource (INOUT GPipelineID &id)		{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT CPipelineID &id)		{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT RTPipelineID &id)	{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT ImageID &id)			{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT BufferID &id)		{ _DestroyResource( INOUT id ); }
	void VFrameGraphThread::DestroyResource (INOUT SamplerID &id)		{ _DestroyResource( INOUT id ); }

/*
=================================================
	_GetDescription
=================================================
*/
	template <typename Desc, typename ID>
	inline Desc const&  VFrameGraphThread::_GetDescription (const ID &id) const
	{
		ASSERT( _IsInitialized() );
		ASSERT( _ownThread.IsCurrent() );

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
	bool VFrameGraphThread::Initialize ()
	{
		CHECK_ERR( _SetState( EState::Initial, EState::Idle ));

		// swapchain must be created before initializing
		if ( EnumEq( _usage, EThreadUsage::Present ) )
			CHECK_ERR( _swapchain );
		
		CHECK_ERR( _SetupQueues() );

		for (auto& queue : _queues)
		{
			queue.frames.resize( _instance.GetRingBufferSize() );

			CHECK_ERR( _CreateCommandBuffers( INOUT queue ));
		}

		if ( _memoryMngr )
			CHECK_ERR( _memoryMngr->Initialize() );

		if ( _stagingMngr )
			CHECK_ERR( _stagingMngr->Initialize() );

		CHECK_ERR( _resourceMngr.Initialize() );
		return true;
	}
	
/*
=================================================
	_CreateCommandBuffers
=================================================
*/
	bool VFrameGraphThread::_CreateCommandBuffers (INOUT PerQueue &queue) const
	{
		ASSERT( _IsInitialized() );
		CHECK_ERR( not queue.cmdPoolId );

		VDevice const&	dev = GetDevice();
		
		VkCommandPoolCreateInfo		pool_info = {};
		pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex	= queue.familyIndex;
		pool_info.flags				= 0;

		if ( EnumEq( queue.queueFlags, VK_QUEUE_PROTECTED_BIT ) )
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
		auto&	queue = _queues.front();

		queue.signalSemaphores.push_back( sem );
	}

/*
=================================================
	WaitSemaphore
=================================================
*/
	void VFrameGraphThread::WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage)
	{
		auto&	queue = _queues.front();

		queue.waitSemaphores.push_back( sem );
		queue.waitDstStageMasks.push_back( stage );
	}
	
/*
=================================================
	CreateCommandBuffer
=================================================
*/
	VkCommandBuffer  VFrameGraphThread::CreateCommandBuffer ()
	{
		ASSERT( _IsInitialized() );

		auto&			queue	= _queues.front();
		VDevice const&	dev		= GetDevice();

		CHECK_ERR( queue.cmdPoolId );

		VkCommandBufferAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext				= null;
		info.commandPool		= queue.cmdPoolId;
		info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandBufferCount	= 1;

		VkCommandBuffer		cmd;
		VK_CHECK( dev.vkAllocateCommandBuffers( dev.GetVkDevice(), &info, OUT &cmd ));

		queue.frames[_currFrame].commands.push_back( cmd );
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

		if ( EnumEq( _usage, EThreadUsage::Graphics ) and EnumEq( _usage, EThreadUsage::Present ) )
			CHECK_ERR( _AddGraphicsPresentQueue() )
		else
		if ( EnumEq( _usage, EThreadUsage::Graphics ) )
			CHECK_ERR( _AddGraphicsQueue() )
		else
		if ( EnumEq( _usage, EThreadUsage::Transfer ) )
			CHECK_ERR( _AddTransferQueue() );

		if ( EnumEq( _usage, EThreadUsage::AsyncCompute ) )
			CHECK_ERR( _AddAsyncComputeQueue() );

		if ( EnumEq( _usage, EThreadUsage::AsyncStreaming ) )
			CHECK_ERR( _AddAsyncStreamingQueue() );

		CHECK_ERR( not _queues.empty() );
		return true;
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
				auto&	dst		= _queues.back();

				dst.queueId		= queue.id;
				dst.familyIndex	= queue.familyIndex;
				dst.queueFlags	= queue.familyFlags;
				return true;
			}
		}
		return false;
	}
	
/*
=================================================
	_AddGraphicsPresentQueue
=================================================
*/
	bool VFrameGraphThread::_AddGraphicsPresentQueue ()
	{
		CHECK_ERR( _swapchain );

		for (auto& queue : GetDevice().GetVkQueues())
		{
			if ( EnumEq( queue.familyFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) and
				 _swapchain->IsCompatibleWithQueue( queue.familyIndex ) )
			{
				_queues.push_back( PerQueue{} );
				auto&	dst		= _queues.back();

				dst.queueId		= queue.id;
				dst.familyIndex	= queue.familyIndex;
				dst.queueFlags	= queue.familyFlags;

				CHECK_ERR( _swapchain->Initialize( dst.queueId ));
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
				auto&	dst		= _queues.back();

				dst.queueId		= queue.id;
				dst.familyIndex	= queue.familyIndex;
				dst.queueFlags	= queue.familyFlags;
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
		VDevice::QueueInfo const*	best_match = null;
		VDevice::QueueInfo const*	compatible = null;

		for (auto& queue : GetDevice().GetVkQueues())
		{
			if ( queue.familyFlags == VK_QUEUE_COMPUTE_BIT )
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
			auto&	dst		= _queues.back();

			dst.queueId		= best_match->id;
			dst.familyIndex	= best_match->familyIndex;
			dst.queueFlags	= best_match->familyFlags;
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
		VDevice::QueueInfo const*	best_match = null;
		VDevice::QueueInfo const*	compatible = null;

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
			auto&	dst		= _queues.back();

			dst.queueId		= best_match->id;
			dst.familyIndex	= best_match->familyIndex;
			dst.queueFlags	= best_match->familyFlags;
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
		CHECK_ERR( _SetState( EState::Idle, EState::BeforeDestroy ), void());
		
		VDevice const&	dev = GetDevice();

		for (auto& queue : _queues)
		{
			//VK_CALL( dev.vkQueueWaitIdle( queue.queueId ));
			
			for (auto& frame : queue.frames)
			{
				if ( frame.semaphore ) {
					dev.vkDestroySemaphore( dev.GetVkDevice(), frame.semaphore, null );
				}
			}
			
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
		ASSERT( _IsInitialOrIdleState() );

		_compilationFlags = flags;
		// TODO
	}
	
/*
=================================================
	CreateSwapchain
=================================================
*/
	bool VFrameGraphThread::CreateSwapchain (const SwapchainInfo_t &ci)
	{
		ASSERT( not _IsInitialized() );
		CHECK_ERR( EnumEq( _usage, EThreadUsage::Present ) );

		CHECK_ERR( Visit( ci,
				[this] (const VulkanSwapchainInfo &sci) -> bool
				{
					UniquePtr<VSwapchainKHR>	sc{ new VSwapchainKHR( GetDevice(), sci )};

					//sc->Recreate( ci );

					_swapchain.reset( sc.release() );
					return true;
				},
				/*[this] (const VulkanVREmulatorSwapchainInfo &sci) {
					
				},*/
				[] (const auto &)
				{
					ASSERT(false);
					return false;
				}
			));

		return true;
	}
	
/*
=================================================
	SyncOnBegin
=================================================
*/
	bool VFrameGraphThread::SyncOnBegin ()
	{
		CHECK_ERR( _SetState( EState::Idle, EState::BeforeStart ));

		_currFrame = (_currFrame + 1) % _queues.front().frames.size();

		_resourceMngr.OnBeginFrame();
		
		CHECK_ERR( _SetState( EState::BeforeStart, EState::Ready ));
		return true;
	}

/*
=================================================
	Begin
=================================================
*/
	bool VFrameGraphThread::Begin ()
	{
		CHECK_ERR( _SetState( EState::Ready, EState::BeforeRecording ));
		_ownThread.SetCurrent();
		
		VDevice const&	dev = _instance.GetDevice();

		for (auto& queue : _queues)
		{
			auto&	frame = queue.frames[_currFrame];

			// reset commands buffers
			if ( not frame.commands.empty() ) {
				dev.vkFreeCommandBuffers( dev.GetVkDevice(), queue.cmdPoolId, uint(frame.commands.size()), frame.commands.data() );
			}
			frame.commands.clear();
		}

		_taskGraph.OnStart( this );

		if ( _stagingMngr )
			_stagingMngr->OnBeginFrame( _currFrame );
		
		if ( _swapchain )
			CHECK( _swapchain->Acquire( *this ));

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
		CHECK_ERR( _SetState( EState::Recording, EState::Compiling ));
		ASSERT( _ownThread.IsCurrent() );
		
		if ( _stagingMngr )
			_stagingMngr->OnEndFrame();

		CHECK_ERR( _BuildCommandBuffers() );
		
		_taskGraph.OnDiscardMemory();

		CHECK_ERR( _SetState( EState::Compiling, EState::Pending ));
		return true;
	}
	
/*
=================================================
	SyncOnExecute
=================================================
*/
	bool VFrameGraphThread::SyncOnExecute (uint batchId, uint indexInBatch)
	{
		CHECK_ERR( _SetState( EState::Pending, EState::Execute ));
		
		_resourceMngr.OnEndFrame();
		
		for (auto& queue : _queues)
		{
			auto&	frame = queue.frames[_currFrame];

			CHECK( _instance.Submit( queue.queueId, batchId, indexInBatch,
									 frame.commands, queue.signalSemaphores,
									 queue.waitSemaphores, queue.waitDstStageMasks, queue.fenceRequired ));

			queue.signalSemaphores.clear();
			queue.waitDstStageMasks.clear();
			queue.waitSemaphores.clear();
		}

		_resourceMngr.OnDiscardMemory();
		_mainAllocator.Discard();
		
		CHECK_ERR( _SetState( EState::Execute, _swapchain ? EState::WaitForPresent : EState::Idle ));
		return true;
	}
	
/*
=================================================
	Present
=================================================
*/
	bool VFrameGraphThread::Present ()
	{
		CHECK_ERR( _SetState( EState::WaitForPresent, EState::Presenting ));

		_swapchain->Present( *this );
		
		CHECK_ERR( _SetState( EState::Presenting, EState::Idle ));
		return true;
	}
	
/*
=================================================
	OnWaitIdle
=================================================
*/
	bool VFrameGraphThread::OnWaitIdle ()
	{
		CHECK_ERR( _SetState( EState::Idle, EState::WaitIdle ));
		
		VDevice const&	dev = _instance.GetDevice();

		for (auto& queue : _queues)
		{
			auto&	frame = queue.frames[_currFrame];

			// reset commands buffers
			if ( not frame.commands.empty() ) {
				dev.vkFreeCommandBuffers( dev.GetVkDevice(), queue.cmdPoolId, uint(frame.commands.size()), frame.commands.data() );
			}
			frame.commands.clear();
		}

		// to generate 'on gpu data loaded' events
		if ( _stagingMngr )
		{
			_stagingMngr->OnBeginFrame( _currFrame );
			_stagingMngr->OnEndFrame();
		}

		CHECK_ERR( _SetState( EState::WaitIdle, EState::Idle ));
		return true;
	}

/*
=================================================
	_IsInSeparateThread
=================================================
*/
	bool VFrameGraphThread::_IsInSeparateThread () const
	{
		const EState	state = _GetState();

		if ( state == EState::Recording )
		{
			ASSERT( _ownThread.IsCurrent() );
			return true;
		}

		//ASSERT( _instance.GetThreadID().IsCurrent() );
		return false;
	}
	
/*
=================================================
	AddPipelineCompiler
=================================================
*/
	bool VFrameGraphThread::AddPipelineCompiler (const IPipelineCompilerPtr &comp)
	{
		CHECK_ERR( _IsInitialOrIdleState() );

		_resourceMngr.GetPipelineCache()->AddCompiler( comp );
		return true;
	}
	
/*
=================================================
	GetSwapchainImage
=================================================
*/
	ImageID  VFrameGraphThread::GetSwapchainImage (ESwapchainImage type)
	{
		CHECK_ERR( _swapchain );

		return ImageID{ _swapchain->GetImage( type )};
	}


}	// FG
