// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VCmdBatch.h"
#include "VDevice.h"
#include "VResourceManager.h"
#include "VDebugger.h"

namespace FG
{
	
/*
=================================================
	constructor
=================================================
*/
	VCmdBatch::VCmdBatch (VDeviceQueueInfoPtr queue, EQueueType type, ArrayView<CommandBuffer> dependsOn) :
		_state{ EState::Recording },	_queue{ queue },
		_queueType{ type }
	{
		STATIC_ASSERT( decltype(_state)::is_always_lock_free );

		for (auto& dep : dependsOn)
		{
			auto*	batch = Cast<VCmdBatch>(dep.GetBatch());

			if ( batch )
				_dependencies.push_back( batch );
		}
	}

/*
=================================================
	destructor
=================================================
*/
	VCmdBatch::~VCmdBatch ()
	{
	}
	
/*
=================================================
	Release
=================================================
*/
	void  VCmdBatch::Release ()
	{
		CHECK( GetState() == EState::Complete );

		delete this;
	}
	
/*
=================================================
	SignalSemaphore
=================================================
*/
	void  VCmdBatch::SignalSemaphore (VkSemaphore sem)
	{
		ASSERT( GetState() < EState::Submitted );

		_signalSemaphores.push_back( sem );
	}
	
/*
=================================================
	WaitSemaphore
=================================================
*/
	void  VCmdBatch::WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage)
	{
		ASSERT( GetState() < EState::Submitted );

		_waitSemaphores.push_back( sem );
		_waitDstStages.push_back( stage );
	}
	
/*
=================================================
	AddCommandBuffer
=================================================
*/
	void  VCmdBatch::AddCommandBuffer (VkCommandBuffer cmd)
	{
		ASSERT( GetState() < EState::Submitted );

		_commands.push_back( cmd );
	}
	
/*
=================================================
	AddDependency
=================================================
*/
	void  VCmdBatch::AddDependency (VCmdBatch *batch)
	{
		ASSERT( GetState() < EState::Backed );

		_dependencies.push_back( batch );
	}

/*
=================================================
	_SetState
=================================================
*/
	void  VCmdBatch::_SetState (EState newState)
	{
		ASSERT( uint(newState) > uint(GetState()) );

		_state.store( newState, memory_order_relaxed );
	}

/*
=================================================
	OnBacked
=================================================
*/
	bool  VCmdBatch::OnBacked (INOUT ResourceMap_t &resources)
	{
		_SetState( EState::Backed );

		std::swap( _resourcesToRelease, resources );
		return true;
	}
	
/*
=================================================
	OnReadyToSubmit
=================================================
*/
	bool  VCmdBatch::OnReadyToSubmit ()
	{
		_SetState( EState::Ready );
		return true;
	}

/*
=================================================
	OnSubmit
=================================================
*/
	bool  VCmdBatch::OnSubmit (OUT VkSubmitInfo &submitInfo, OUT Appendable<VSwapchain const*> swapchains, const VSubmittedPtr &ptr)
	{
		_SetState( EState::Submitted );
		
		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext				= null;
		submitInfo.pCommandBuffers		= _commands.data();
		submitInfo.commandBufferCount	= uint(_commands.size());
		submitInfo.pSignalSemaphores	= _signalSemaphores.data();
		submitInfo.signalSemaphoreCount	= uint(_signalSemaphores.size());
		submitInfo.pWaitSemaphores		= _waitSemaphores.data();
		submitInfo.pWaitDstStageMask	= _waitDstStages.data();
		submitInfo.waitSemaphoreCount	= uint(_waitSemaphores.size());

		for (auto& sw : _swapchains) {
			swapchains.push_back( sw );
		}

		_swapchains.clear();
		_dependencies.clear();
		_submitted = ptr;

		return true;
	}
	
/*
=================================================
	OnComplete
=================================================
*/
	bool  VCmdBatch::OnComplete (VResourceManager &resMngr, VDebugger &debugger)
	{
		ASSERT( _submitted );
		_SetState( EState::Complete );
		
		using T = BufferView::value_type;
		
		// map device-to-host staging buffers
		for (auto& buf : _deviceToHost)
		{
			// buffer may be recreated on defragmentation pass, so we need to obtain actual pointer every frame
			CHECK( _MapMemory( resMngr, INOUT buf ));
		}

		// trigger buffer events
		for (auto& ev : _onBufferLoadedEvents)
		{
			FixedArray< ArrayView<T>, MaxBufferParts >	data_parts;
			BytesU										total_size;

			for (auto& part : ev.parts)
			{
				ArrayView<T>	view{ Cast<T>(part.buffer->mappedPtr + part.offset), size_t(part.size) };

				data_parts.push_back( view );
				total_size += part.size;
			}

			ASSERT( total_size == ev.totalSize );

			ev.callback( BufferView{data_parts} );
		}
		_onBufferLoadedEvents.clear();
		

		// trigger image events
		for (auto& ev : _onImageLoadedEvents)
		{
			FixedArray< ArrayView<T>, MaxImageParts >	data_parts;
			BytesU										total_size;

			for (auto& part : ev.parts)
			{
				ArrayView<T>	view{ Cast<T>(part.buffer->mappedPtr + part.offset), size_t(part.size) };

				data_parts.push_back( view );
				total_size += part.size;
			}

			ASSERT( total_size == ev.totalSize );

			ev.callback( ImageView{ data_parts, ev.imageSize, ev.rowPitch, ev.slicePitch, ev.format, ev.aspect });
		}
		_onImageLoadedEvents.clear();


		// release resources
		{
			for (auto& sb : _hostToDevice) {
				resMngr.ReleaseResource( sb.bufferId.Release() );
			}
			_hostToDevice.clear();

			for (auto& sb : _deviceToHost) {
				resMngr.ReleaseResource( sb.bufferId.Release() );
			}
			_deviceToHost.clear();

			_ReleaseResources( resMngr );
		}

		debugger.AddBatchDump( _debugDump );
		_debugDump.clear();

		_submitted = null;
		return true;
	}
	
/*
=================================================
	_MapMemory
=================================================
*/
	bool  VCmdBatch::_MapMemory (VResourceManager &resMngr, INOUT StagingBuffer &buf) const
	{
		VMemoryObj::MemoryInfo	info;
		if ( resMngr.GetResource( buf.memoryId )->GetInfo( resMngr.GetMemoryManager(),OUT info ) )
		{
			buf.mappedPtr	= info.mappedPtr;
			buf.memOffset	= info.offset;
			buf.mem			= info.mem;
			buf.isCoherent	= EnumEq( info.flags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
			return true;
		}
		return false;
	}

/*
=================================================
	_ReleaseResources
=================================================
*/
	void  VCmdBatch::_ReleaseResources (VResourceManager &resMngr)
	{
		for (auto& res : _resourcesToRelease)
		{
			switch ( res.GetUID() )
			{
				case RawBufferID::GetUID() :				resMngr.ReleaseResource( RawBufferID{ res.Index(), res.InstanceID() });					break;
				case RawImageID::GetUID() :					resMngr.ReleaseResource( RawImageID{ res.Index(), res.InstanceID() });					break;
				case RawGPipelineID::GetUID() :				resMngr.ReleaseResource( RawGPipelineID{ res.Index(), res.InstanceID() });				break;
				case RawMPipelineID::GetUID() :				resMngr.ReleaseResource( RawMPipelineID{ res.Index(), res.InstanceID() });				break;
				case RawCPipelineID::GetUID() :				resMngr.ReleaseResource( RawCPipelineID{ res.Index(), res.InstanceID() });				break;
				case RawRTPipelineID::GetUID() :			resMngr.ReleaseResource( RawRTPipelineID{ res.Index(), res.InstanceID() });				break;
				case RawSamplerID::GetUID() :				resMngr.ReleaseResource( RawSamplerID{ res.Index(), res.InstanceID() });				break;
				case RawDescriptorSetLayoutID::GetUID():	resMngr.ReleaseResource( RawDescriptorSetLayoutID{ res.Index(), res.InstanceID() });	break;
				case RawPipelineResourcesID::GetUID() :		resMngr.ReleaseResource( RawPipelineResourcesID{ res.Index(), res.InstanceID() });		break;
				case RawRTSceneID::GetUID() :				resMngr.ReleaseResource( RawRTSceneID{ res.Index(), res.InstanceID() });				break;
				case RawRTGeometryID::GetUID() :			resMngr.ReleaseResource( RawRTGeometryID{ res.Index(), res.InstanceID() });				break;
				case RawRTShaderTableID::GetUID() :			resMngr.ReleaseResource( RawRTShaderTableID{ res.Index(), res.InstanceID() });			break;
				case RawSwapchainID::GetUID() :				resMngr.ReleaseResource( RawSwapchainID{ res.Index(), res.InstanceID() });				break;
				case RawMemoryID::GetUID() :				resMngr.ReleaseResource( RawMemoryID{ res.Index(), res.InstanceID() });					break;
				case RawPipelineLayoutID::GetUID() :		resMngr.ReleaseResource( RawPipelineLayoutID{ res.Index(), res.InstanceID() });			break;
				case RawRenderPassID::GetUID() :			resMngr.ReleaseResource( RawRenderPassID{ res.Index(), res.InstanceID() });				break;
				case RawFramebufferID::GetUID() :			resMngr.ReleaseResource( RawFramebufferID{ res.Index(), res.InstanceID() });			break;
				default :									CHECK( !"not supported" );																break;
			}
		}
		_resourcesToRelease.clear();
	}


}	// FG
