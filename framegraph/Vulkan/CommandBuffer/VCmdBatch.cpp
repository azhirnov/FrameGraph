// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VCmdBatch.h"
#include "VFrameGraph.h"

namespace FG
{
	
/*
=================================================
	constructor
=================================================
*/
	VCmdBatch::VCmdBatch (VFrameGraph &fg, uint indexInPool) :
		_state{ EState::Initial },		_frameGraph{ fg },
		_indexInPool{ indexInPool }
	{
		STATIC_ASSERT( decltype(_state)::is_always_lock_free );
		
		_counter.store( 0 );
		_shaderDebugger.bufferAlign = BytesU{fg.GetDevice().GetDeviceLimits().minStorageBufferOffsetAlignment};
			
		if ( FG_EnableShaderDebugging ) {
			CHECK( fg.GetDevice().GetDeviceLimits().maxBoundDescriptorSets > FG_DebugDescriptorSet );
		}
	}

/*
=================================================
	destructor
=================================================
*/
	VCmdBatch::~VCmdBatch ()
	{
		EXLOCK( _drCheck );
		CHECK( _counter.load( memory_order_relaxed ) == 0 );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	void  VCmdBatch::Initialize (EQueueType type, ArrayView<CommandBuffer> dependsOn)
	{
		EXLOCK( _drCheck );

		ASSERT( _dependencies.empty() );
		ASSERT( _batch.commands.empty() );
		ASSERT( _batch.signalSemaphores.empty() );
		ASSERT( _batch.waitSemaphores.empty() );
		ASSERT( _staging.hostToDevice.empty() );
		ASSERT( _staging.deviceToHost.empty() );
		ASSERT( _staging.onBufferLoadedEvents.empty() );
		ASSERT( _staging.onImageLoadedEvents.empty() );
		ASSERT( _resourcesToRelease.empty() );
		ASSERT( _swapchains.empty() );
		ASSERT( _shaderDebugger.buffers.empty() );
		ASSERT( _shaderDebugger.modes.empty() );
		ASSERT( _submitted == null );
		ASSERT( _counter.load( memory_order_relaxed ) == 0 );

		_queueType = type;

		_state.store( EState::Initial, memory_order_relaxed );
		
		for (auto& dep : dependsOn)
		{
			if ( auto* batch = Cast<VCmdBatch>(dep.GetBatch()) )
				_dependencies.push_back( batch );
		}
	}

/*
=================================================
	Release
=================================================
*/
	void  VCmdBatch::Release ()
	{
		EXLOCK( _drCheck );
		CHECK( GetState() == EState::Complete );
		ASSERT( _counter.load( memory_order_relaxed ) == 0 );

		_frameGraph.RecycleBatch( this );
	}
	
/*
=================================================
	SignalSemaphore
=================================================
*/
	void  VCmdBatch::SignalSemaphore (VkSemaphore sem)
	{
		EXLOCK( _drCheck );
		ASSERT( GetState() < EState::Submitted );
		CHECK_ERR( _batch.signalSemaphores.size() < _batch.signalSemaphores.capacity(), void());

		_batch.signalSemaphores.push_back( sem );
	}
	
/*
=================================================
	WaitSemaphore
=================================================
*/
	void  VCmdBatch::WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage)
	{
		EXLOCK( _drCheck );
		ASSERT( GetState() < EState::Submitted );
		CHECK_ERR( _batch.waitSemaphores.size() < _batch.waitSemaphores.capacity(), void());

		_batch.waitSemaphores.push_back( sem, stage );
	}
	
/*
=================================================
	PushFrontCommandBuffer / PushBackCommandBuffer
=================================================
*/
	void  VCmdBatch::PushFrontCommandBuffer (VkCommandBuffer cmd, const VCommandPool *pool)
	{
		EXLOCK( _drCheck );
		ASSERT( GetState() < EState::Submitted );
		CHECK_ERR( _batch.commands.size() < _batch.commands.capacity(), void());

		_batch.commands.insert( 0, cmd, pool );
	}

	void  VCmdBatch::PushBackCommandBuffer (VkCommandBuffer cmd, const VCommandPool *pool)
	{
		EXLOCK( _drCheck );
		ASSERT( GetState() < EState::Submitted );
		CHECK_ERR( _batch.commands.size() < _batch.commands.capacity(), void());

		_batch.commands.push_back( cmd, pool );
	}
	
/*
=================================================
	AddDependency
=================================================
*/
	void  VCmdBatch::AddDependency (VCmdBatch *batch)
	{
		EXLOCK( _drCheck );
		ASSERT( GetState() < EState::Backed );
		CHECK_ERR( _dependencies.size() < _dependencies.capacity(), void());

		_dependencies.push_back( batch );
	}
	
/*
=================================================
	DestroyPostponed
=================================================
*/
	void  VCmdBatch::DestroyPostponed (VkObjectType type, uint64_t handle)
	{
		EXLOCK( _drCheck );
		ASSERT( GetState() < EState::Backed );

		_readyToDelete.push_back({ type, handle });
	}

/*
=================================================
	_SetState
=================================================
*/
	void  VCmdBatch::_SetState (EState newState)
	{
		EXLOCK( _drCheck );
		ASSERT( uint(newState) > uint(GetState()) );

		_state.store( newState, memory_order_relaxed );
	}
	
/*
=================================================
	OnBegin
=================================================
*/
	bool  VCmdBatch::OnBegin (const CommandBufferDesc &desc)
	{
		EXLOCK( _drCheck );
		_SetState( EState::Recording );

		//_submitImmediatly			= // TODO
		_staging.hostWritableBufferSize		= desc.hostWritableBufferSize;
		_staging.hostReadableBufferSize		= desc.hostWritableBufferSize;
		_staging.hostWritebleBufferUsage	= desc.hostWritebleBufferUsage | EBufferUsage::TransferSrc;

		return true;
	}

/*
=================================================
	OnBaked
=================================================
*/
	bool  VCmdBatch::OnBaked (INOUT ResourceMap_t &resources)
	{
		EXLOCK( _drCheck );
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
		EXLOCK( _drCheck );

		_SetState( EState::Ready );
		return true;
	}

/*
=================================================
	BeforeSubmit
=================================================
*/
	bool  VCmdBatch::BeforeSubmit (OUT VkSubmitInfo &submitInfo)
	{
		EXLOCK( _drCheck );
		
		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext				= null;
		submitInfo.pCommandBuffers		= _batch.commands.get<0>().data();
		submitInfo.commandBufferCount	= uint(_batch.commands.size());
		submitInfo.pSignalSemaphores	= _batch.signalSemaphores.data();
		submitInfo.signalSemaphoreCount	= uint(_batch.signalSemaphores.size());
		submitInfo.pWaitSemaphores		= _batch.waitSemaphores.get<0>().data();
		submitInfo.pWaitDstStageMask	= _batch.waitSemaphores.get<1>().data();
		submitInfo.waitSemaphoreCount	= uint(_batch.waitSemaphores.size());

		return true;
	}
	
/*
=================================================
	AfterSubmit
=================================================
*/
	bool  VCmdBatch::AfterSubmit (OUT Appendable<VSwapchain const*> swapchains, VSubmitted *ptr)
	{
		EXLOCK( _drCheck );
		_SetState( EState::Submitted );

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
	bool  VCmdBatch::OnComplete (VDebugger &debugger, const ShaderDebugCallback_t &shaderDbgCallback)
	{
		EXLOCK( _drCheck );
		ASSERT( _submitted );

		_SetState( EState::Complete );

		_FinalizeCommands();
		_ParseDebugOutput( shaderDbgCallback );
		_FinalizeStagingBuffers();
		_ReleaseResources();
		_ReleaseVkObjects();

		debugger.AddBatchDump( std::move(_debugDump) );
		debugger.AddBatchGraph( std::move(_debugGraph) );

		_debugDump.clear();
		_debugGraph	= Default;

		_submitted = null;
		return true;
	}
	
/*
=================================================
	_FinalizeCommands
=================================================
*/
	void  VCmdBatch::_FinalizeCommands ()
	{
		for (size_t i = 0; i < _batch.commands.size(); ++i)
		{
			if ( auto*  pool = _batch.commands.get<1>()[i] )
				pool->RecyclePrimary( _batch.commands.get<0>()[i] );
		}

		_batch.commands.clear();
		_batch.signalSemaphores.clear();
		_batch.waitSemaphores.clear();
	}

/*
=================================================
	_FinalizeStagingBuffers
=================================================
*/
	void  VCmdBatch::_FinalizeStagingBuffers ()
	{
		using T = BufferView::value_type;
		
		// map device-to-host staging buffers
		for (auto& buf : _staging.deviceToHost)
		{
			// buffer may be recreated on defragmentation pass, so we need to obtain actual pointer every frame
			CHECK( _MapMemory( INOUT buf ));
		}

		// trigger buffer events
		for (auto& ev : _staging.onBufferLoadedEvents)
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
		_staging.onBufferLoadedEvents.clear();
		

		// trigger image events
		for (auto& ev : _staging.onImageLoadedEvents)
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
		_staging.onImageLoadedEvents.clear();


		// release resources
		{
			auto&	rm = _frameGraph.GetResourceManager();

			for (auto& sb : _staging.hostToDevice) {
				rm.ReleaseResource( sb.bufferId.Release() );
			}
			_staging.hostToDevice.clear();

			for (auto& sb : _staging.deviceToHost) {
				rm.ReleaseResource( sb.bufferId.Release() );
			}
			_staging.deviceToHost.clear();
		}
	}

/*
=================================================
	_ParseDebugOutput
=================================================
*/
	void  VCmdBatch::_ParseDebugOutput (const ShaderDebugCallback_t &cb)
	{
		if ( _shaderDebugger.buffers.empty() )
			return;

		ASSERT( cb );

		auto&	dev = _frameGraph.GetDevice();
		auto&	rm  = _frameGraph.GetResourceManager();

		VK_CHECK( dev.vkDeviceWaitIdle( dev.GetVkDevice() ), void());

		// release descriptor sets
		for (auto& ds : _shaderDebugger.descCache) {
			rm.GetDescriptorManager().DeallocDescriptorSet( ds.second );
		}
		_shaderDebugger.descCache.clear();

		// process shader debug output
		if ( cb )
		{
			Array<String>	temp_strings;

			for (auto& dbg : _shaderDebugger.modes) {
				_ParseDebugOutput2( cb, dbg, temp_strings );
			}
		}
		_shaderDebugger.modes.clear();

		// release storage buffers
		for (auto& sb : _shaderDebugger.buffers)
		{
			rm.ReleaseResource( sb.shaderTraceBuffer.Release() );
			rm.ReleaseResource( sb.readBackBuffer.Release() );
		}
		_shaderDebugger.buffers.clear();
	}
	
/*
=================================================
	_ParseDebugOutput2
=================================================
*/
	bool  VCmdBatch::_ParseDebugOutput2 (const ShaderDebugCallback_t &cb, const DebugMode &dbg, Array<String> &tempStrings) const
	{
		CHECK_ERR( dbg.modules.size() );
		
		auto&	rm				= _frameGraph.GetResourceManager();
		auto	read_back_buf	= _shaderDebugger.buffers[ dbg.sbIndex ].readBackBuffer.Get();

		VBuffer const*		buf	= rm.GetResource( read_back_buf );
		CHECK_ERR( buf );

		VMemoryObj const*	mem = rm.GetResource( buf->GetMemoryID() );
		CHECK_ERR( mem );

		VMemoryObj::MemoryInfo	info;
		CHECK_ERR( mem->GetInfo( rm.GetMemoryManager(), OUT info ));
		CHECK_ERR( info.mappedPtr );

		for (auto& shader : dbg.modules)
		{
			CHECK_ERR( shader->ParseDebugOutput( dbg.mode, ArrayView<uint8_t>{ Cast<uint8_t>(info.mappedPtr + dbg.offset), size_t(dbg.size) }, OUT tempStrings ));
		
			cb( dbg.taskName, shader->GetDebugName(), dbg.shaderStages, tempStrings );
		}
		return true;
	}
//-----------------------------------------------------------------------------


/*
=================================================
	_MapMemory
=================================================
*/
	bool  VCmdBatch::_MapMemory (INOUT StagingBuffer &buf) const
	{
		VMemoryObj::MemoryInfo	info;
		auto&					rm = _frameGraph.GetResourceManager();

		if ( rm.GetResource( buf.memoryId )->GetInfo( rm.GetMemoryManager(),OUT info ) )
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
	void  VCmdBatch::_ReleaseResources ()
	{
		auto&	rm = _frameGraph.GetResourceManager();

		for (auto[res, count] : _resourcesToRelease)
		{
			switch ( res.GetUID() )
			{
				case RawBufferID::GetUID() :			rm.ReleaseResource( RawBufferID{ res.Index(), res.InstanceID() }, count );				break;
				case RawImageID::GetUID() :				rm.ReleaseResource( RawImageID{ res.Index(), res.InstanceID() }, count );				break;
				case RawGPipelineID::GetUID() :			rm.ReleaseResource( RawGPipelineID{ res.Index(), res.InstanceID() }, count );			break;
				case RawMPipelineID::GetUID() :			rm.ReleaseResource( RawMPipelineID{ res.Index(), res.InstanceID() }, count );			break;
				case RawCPipelineID::GetUID() :			rm.ReleaseResource( RawCPipelineID{ res.Index(), res.InstanceID() }, count );			break;
				case RawRTPipelineID::GetUID() :		rm.ReleaseResource( RawRTPipelineID{ res.Index(), res.InstanceID() }, count );			break;
				case RawSamplerID::GetUID() :			rm.ReleaseResource( RawSamplerID{ res.Index(), res.InstanceID() }, count );				break;
				case RawDescriptorSetLayoutID::GetUID():rm.ReleaseResource( RawDescriptorSetLayoutID{ res.Index(), res.InstanceID() }, count );	break;
				case RawPipelineResourcesID::GetUID() :	rm.ReleaseResource( RawPipelineResourcesID{ res.Index(), res.InstanceID() }, count );	break;
				case RawRTSceneID::GetUID() :			rm.ReleaseResource( RawRTSceneID{ res.Index(), res.InstanceID() }, count );				break;
				case RawRTGeometryID::GetUID() :		rm.ReleaseResource( RawRTGeometryID{ res.Index(), res.InstanceID() }, count );			break;
				case RawRTShaderTableID::GetUID() :		rm.ReleaseResource( RawRTShaderTableID{ res.Index(), res.InstanceID() }, count );		break;
				case RawSwapchainID::GetUID() :			rm.ReleaseResource( RawSwapchainID{ res.Index(), res.InstanceID() }, count );			break;
				case RawMemoryID::GetUID() :			rm.ReleaseResource( RawMemoryID{ res.Index(), res.InstanceID() }, count );				break;
				case RawPipelineLayoutID::GetUID() :	rm.ReleaseResource( RawPipelineLayoutID{ res.Index(), res.InstanceID() }, count );		break;
				case RawRenderPassID::GetUID() :		rm.ReleaseResource( RawRenderPassID{ res.Index(), res.InstanceID() }, count );			break;
				case RawFramebufferID::GetUID() :		rm.ReleaseResource( RawFramebufferID{ res.Index(), res.InstanceID() }, count );			break;
				default :								CHECK( !"not supported" );																break;
			}
		}
		_resourcesToRelease.clear();
	}
	
/*
=================================================
	_ReleaseVkObjects
=================================================
*/
	void  VCmdBatch::_ReleaseVkObjects ()
	{
		VDevice const&	dev  = _frameGraph.GetDevice();
		VkDevice		vdev = dev.GetVkDevice();
		
		for (auto& pair : _readyToDelete)
		{
			switch ( pair.first )
			{
				case VK_OBJECT_TYPE_SEMAPHORE :
					dev.vkDestroySemaphore( vdev, VkSemaphore(pair.second), null );
					break;

				case VK_OBJECT_TYPE_FENCE :
					dev.vkDestroyFence( vdev, VkFence(pair.second), null );
					break;

				case VK_OBJECT_TYPE_DEVICE_MEMORY :
					dev.vkFreeMemory( vdev, VkDeviceMemory(pair.second), null );
					break;

				case VK_OBJECT_TYPE_IMAGE :
					dev.vkDestroyImage( vdev, VkImage(pair.second), null );
					break;

				case VK_OBJECT_TYPE_EVENT :
					dev.vkDestroyEvent( vdev, VkEvent(pair.second), null );
					break;

				case VK_OBJECT_TYPE_QUERY_POOL :
					dev.vkDestroyQueryPool( vdev, VkQueryPool(pair.second), null );
					break;

				case VK_OBJECT_TYPE_BUFFER :
					dev.vkDestroyBuffer( vdev, VkBuffer(pair.second), null );
					break;

				case VK_OBJECT_TYPE_BUFFER_VIEW :
					dev.vkDestroyBufferView( vdev, VkBufferView(pair.second), null );
					break;

				case VK_OBJECT_TYPE_IMAGE_VIEW :
					dev.vkDestroyImageView( vdev, VkImageView(pair.second), null );
					break;

				case VK_OBJECT_TYPE_PIPELINE_LAYOUT :
					dev.vkDestroyPipelineLayout( vdev, VkPipelineLayout(pair.second), null );
					break;

				case VK_OBJECT_TYPE_RENDER_PASS :
					dev.vkDestroyRenderPass( vdev, VkRenderPass(pair.second), null );
					break;

				case VK_OBJECT_TYPE_PIPELINE :
					dev.vkDestroyPipeline( vdev, VkPipeline(pair.second), null );
					break;

				case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT :
					dev.vkDestroyDescriptorSetLayout( vdev, VkDescriptorSetLayout(pair.second), null );
					break;

				case VK_OBJECT_TYPE_SAMPLER :
					dev.vkDestroySampler( vdev, VkSampler(pair.second), null );
					break;

				case VK_OBJECT_TYPE_DESCRIPTOR_POOL :
					dev.vkDestroyDescriptorPool( vdev, VkDescriptorPool(pair.second), null );
					break;

				case VK_OBJECT_TYPE_FRAMEBUFFER :
					dev.vkDestroyFramebuffer( vdev, VkFramebuffer(pair.second), null );
					break;

				case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION :
					dev.vkDestroySamplerYcbcrConversion( vdev, VkSamplerYcbcrConversion(pair.second), null );
					break;

				case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE :
					dev.vkDestroyDescriptorUpdateTemplate( vdev, VkDescriptorUpdateTemplate(pair.second), null );
					break;

				case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV :
					dev.vkDestroyAccelerationStructureNV( vdev, VkAccelerationStructureNV(pair.second), null );
					break;

				default :
					FG_LOGE( "resource type is not supported" );
					break;
			}
		}
		_readyToDelete.clear();
	}

/*
=================================================
	_GetWritable
=================================================
*/
	bool VCmdBatch::GetWritable (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
								 OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &outSize, OUT void* &mappedPtr)
	{
		EXLOCK( _drCheck );
		ASSERT( blockAlign > 0_b and offsetAlign > 0_b );
		ASSERT( dstMinSize == AlignToSmaller( dstMinSize, blockAlign ));

		auto&	staging_buffers = _staging.hostToDevice;


		// search in existing
		StagingBuffer*	suitable		= null;
		StagingBuffer*	max_available	= null;
		BytesU			max_size;

		for (auto& buf : staging_buffers)
		{
			const BytesU	off	= AlignToLarger( buf.size, offsetAlign );
			const BytesU	av	= AlignToSmaller( buf.capacity - off, blockAlign );

			if ( av >= srcRequiredSize )
			{
				suitable = &buf;
				break;
			}

			if ( not max_available or av > max_size )
			{
				max_available	= &buf;
				max_size		= av;
			}
		}

		// no suitable space, try to use max available block
		if ( not suitable and max_available and max_size >= dstMinSize )
		{
			suitable = max_available;
		}

		// allocate new buffer
		if ( not suitable )
		{
			ASSERT( dstMinSize < _staging.hostWritableBufferSize );
			CHECK_ERR( staging_buffers.size() < staging_buffers.capacity() );

			BufferID	buf_id = _frameGraph.CreateBuffer( BufferDesc{ _staging.hostWritableBufferSize, _staging.hostWritebleBufferUsage }, 
															MemoryDesc{ EMemoryType::HostWrite }, "HostWriteBuffer" );
			CHECK_ERR( buf_id );

			RawMemoryID	mem_id = _frameGraph.GetResourceManager().GetResource( buf_id.Get() )->GetMemoryID();
			CHECK_ERR( mem_id );

			staging_buffers.push_back({ std::move(buf_id), mem_id, _staging.hostWritableBufferSize });

			suitable = &staging_buffers.back();
			CHECK( _MapMemory( *suitable ));
		}

		// write data to buffer
		dstOffset	= AlignToLarger( suitable->size, offsetAlign );
		outSize		= Min( AlignToSmaller( suitable->capacity - dstOffset, blockAlign ), srcRequiredSize );
		dstBuffer	= suitable->bufferId.Get();
		mappedPtr	= suitable->mappedPtr + dstOffset;

		suitable->size = dstOffset + outSize;
		return true;
	}
	
/*
=================================================
	_AddPendingLoad
=================================================
*/
	bool  VCmdBatch::_AddPendingLoad (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
									  OUT RawBufferID &dstBuffer, OUT OnBufferDataLoadedEvent::Range &range)
	{
		ASSERT( blockAlign > 0_b and offsetAlign > 0_b );
		ASSERT( dstMinSize == AlignToSmaller( dstMinSize, blockAlign ));

		auto&	staging_buffers = _staging.deviceToHost;
		

		// search in existing
		StagingBuffer*	suitable		= null;
		StagingBuffer*	max_available	= null;
		BytesU			max_size;

		for (auto& buf : staging_buffers)
		{
			const BytesU	off	= AlignToLarger( buf.size, offsetAlign );
			const BytesU	av	= AlignToSmaller( buf.capacity - off, blockAlign );

			if ( av >= srcRequiredSize )
			{
				suitable = &buf;
				break;
			}
			
			if ( not max_available or av > max_size )
			{
				max_available	= &buf;
				max_size		= av;
			}
		}

		// no suitable space, try to use max available block
		if ( not suitable and max_available and max_size >= dstMinSize )
		{
			suitable = max_available;
		}

		// allocate new buffer
		if ( not suitable )
		{
			ASSERT( dstMinSize < _staging.hostReadableBufferSize );
			CHECK_ERR( staging_buffers.size() < staging_buffers.capacity() );

			BufferID	buf_id = _frameGraph.CreateBuffer( BufferDesc{ _staging.hostReadableBufferSize, EBufferUsage::TransferDst },
															MemoryDesc{ EMemoryType::HostRead }, "HostReadBuffer" );
			CHECK_ERR( buf_id );
			
			RawMemoryID	mem_id = _frameGraph.GetResourceManager().GetResource( buf_id.Get() )->GetMemoryID();
			CHECK_ERR( mem_id );

			// TODO: make immutable because read after write happens after waiting for fences and it implicitly make changes visible to the host

			staging_buffers.push_back({ std::move(buf_id), mem_id, _staging.hostReadableBufferSize });

			suitable = &staging_buffers.back();
			CHECK( _MapMemory( *suitable ));
		}
		
		// write data to buffer
		range.buffer	= suitable;
		range.offset	= AlignToLarger( suitable->size, offsetAlign );
		range.size		= Min( AlignToSmaller( suitable->capacity - range.offset, blockAlign ), srcRequiredSize );
		dstBuffer		= suitable->bufferId.Get();

		suitable->size = range.offset + range.size;
		return true;
	}
	
/*
=================================================
	AddPendingLoad
=================================================
*/
	bool  VCmdBatch::AddPendingLoad (const BytesU srcOffset, const BytesU srcTotalSize,
									 OUT RawBufferID &dstBuffer, OUT OnBufferDataLoadedEvent::Range &range)
	{
		EXLOCK( _drCheck );

		// skip blocks less than 1/N of data size
		const BytesU	min_size = (srcTotalSize + MaxBufferParts-1) / MaxBufferParts;

		return _AddPendingLoad( srcTotalSize - srcOffset, 1_b, 16_b, min_size, OUT dstBuffer, OUT range );
	}

/*
=================================================
	AddDataLoadedEvent
=================================================
*/
	bool  VCmdBatch::AddDataLoadedEvent (OnBufferDataLoadedEvent &&ev)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( ev.callback and not ev.parts.empty() );

		_staging.onBufferLoadedEvents.push_back( std::move(ev) );
		return true;
	}
	
/*
=================================================
	AddPendingLoad
=================================================
*/
	bool  VCmdBatch::AddPendingLoad (const BytesU srcOffset, const BytesU srcTotalSize, const BytesU srcPitch,
									 OUT RawBufferID &dstBuffer, OUT OnImageDataLoadedEvent::Range &range)
	{
		EXLOCK( _drCheck );

		// skip blocks less than 1/N of total data size
		const BytesU	min_size = Max( (srcTotalSize + MaxImageParts-1) / MaxImageParts, srcPitch );

		return _AddPendingLoad( srcTotalSize - srcOffset, srcPitch, 16_b, min_size, OUT dstBuffer, OUT range );
	}

/*
=================================================
	AddDataLoadedEvent
=================================================
*/
	bool  VCmdBatch::AddDataLoadedEvent (OnImageDataLoadedEvent &&ev)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( ev.callback and not ev.parts.empty() );

		_staging.onImageLoadedEvents.push_back( std::move(ev) );
		return true;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	BeginShaderDebugger
=================================================
*/
	void  VCmdBatch::BeginShaderDebugger (VkCommandBuffer cmd)
	{
		EXLOCK( _drCheck );
		CHECK( GetState() == EState::Recording );

		if ( _shaderDebugger.buffers.empty() )
			return;
		
		auto&	dev = _frameGraph.GetDevice();
		auto&	rm  = _frameGraph.GetResourceManager();

		// copy data
		for (auto& dbg : _shaderDebugger.modes)
		{
			auto	buf		= rm.GetResource( _shaderDebugger.buffers[dbg.sbIndex].shaderTraceBuffer.Get() )->Handle();
			BytesU	size	= rm.GetDebugShaderStorageSize( dbg.shaderStages ) + SizeOf<uint>;	// per shader data + position
			ASSERT( size <= BytesU::SizeOf(dbg.data) );

			dev.vkCmdUpdateBuffer( cmd, buf, VkDeviceSize(dbg.offset), VkDeviceSize(size), dbg.data );
		}

		// add pipeline barriers
		FixedArray< VkBufferMemoryBarrier, 16 >		barriers;
		VkPipelineStageFlags						dst_stage_flags = 0;

		for (auto& sb : _shaderDebugger.buffers)
		{
			VkBufferMemoryBarrier	barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask	= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			barrier.buffer			= rm.GetResource( sb.shaderTraceBuffer.Get() )->Handle();
			barrier.offset			= 0;
			barrier.size			= VkDeviceSize(sb.size);
			barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

			dst_stage_flags |= sb.stages;
			barriers.push_back( barrier );

			if ( barriers.size() == barriers.capacity() )
			{
				dev.vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, dst_stage_flags, 0, 0, null, uint(barriers.size()), barriers.data(), 0, null );
				barriers.clear();
			}
		}
		
		if ( barriers.size() ) {
			dev.vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, dst_stage_flags, 0, 0, null, uint(barriers.size()), barriers.data(), 0, null );
		}
	}
	
/*
=================================================
	EndShaderDebugger
=================================================
*/
	void  VCmdBatch::EndShaderDebugger (VkCommandBuffer cmd)
	{
		EXLOCK( _drCheck );
		CHECK( GetState() == EState::Recording );

		if ( _shaderDebugger.buffers.empty() )
			return;
		
		auto&	dev = _frameGraph.GetDevice();
		auto&	rm  = _frameGraph.GetResourceManager();
		
		// copy to staging buffer
		for (auto& sb : _shaderDebugger.buffers)
		{
			VkBuffer	src_buf	= rm.GetResource( sb.shaderTraceBuffer.Get() )->Handle();
			VkBuffer	dst_buf	= rm.GetResource( sb.readBackBuffer.Get() )->Handle();

			VkBufferMemoryBarrier	barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.srcAccessMask	= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask	= VK_ACCESS_TRANSFER_READ_BIT;
			barrier.buffer			= src_buf;
			barrier.offset			= 0;
			barrier.size			= VkDeviceSize(sb.size);
			barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			
			dev.vkCmdPipelineBarrier( cmd, sb.stages, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 1, &barrier, 0, null );

			VkBufferCopy	region = {};
			region.size = VkDeviceSize(sb.size);

			dev.vkCmdCopyBuffer( cmd, src_buf, dst_buf, 1, &region );
		}
	}

/*
=================================================
	SetShaderModule
=================================================
*/
	bool  VCmdBatch::SetShaderModule (ShaderDbgIndex id, const SharedShaderPtr &module)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( uint(id) < _shaderDebugger.modes.size() );
		
		auto&	dbg = _shaderDebugger.modes[ uint(id) ];

		dbg.modules.emplace_back( module );
		return true;
	}
	
/*
=================================================
	GetDebugModeInfo
=================================================
*/
	bool  VCmdBatch::GetDebugModeInfo (ShaderDbgIndex id, OUT EShaderDebugMode &mode, OUT EShaderStages &stages) const
	{
		EXLOCK( _drCheck );
		CHECK_ERR( uint(id) < _shaderDebugger.modes.size() );

		auto&	dbg = _shaderDebugger.modes[ uint(id) ];

		mode	= dbg.mode;
		stages	= dbg.shaderStages;
		return true;
	}
	
/*
=================================================
	GetDescriptotSet
=================================================
*/
	bool  VCmdBatch::GetDescriptotSet (ShaderDbgIndex id, OUT uint &binding, OUT VkDescriptorSet &descSet, OUT uint &dynamicOffset) const
	{
		EXLOCK( _drCheck );
		CHECK_ERR( uint(id) < _shaderDebugger.modes.size() );
		
		auto&	dbg = _shaderDebugger.modes[ uint(id) ];

		binding			= FG_DebugDescriptorSet;
		descSet			= dbg.descriptorSet;
		dynamicOffset	= uint(dbg.offset);
		return true;
	}
	
/*
=================================================
	AppendShader
=================================================
*/
	ShaderDbgIndex  VCmdBatch::AppendShader (INOUT ArrayView<RectI> &, const TaskName_t &name, const _fg_hidden_::GraphicsShaderDebugMode &mode, BytesU size)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( FG_EnableShaderDebugging );

		DebugMode	dbg_mode;
		dbg_mode.taskName		= name;
		dbg_mode.mode			= mode.mode;
		dbg_mode.shaderStages	= mode.stages;
		
		CHECK_ERR( _AllocStorage( INOUT dbg_mode, size ));

		dbg_mode.data[0] = mode.fragCoord.x;
		dbg_mode.data[1] = mode.fragCoord.y;
		dbg_mode.data[2] = 0;
		dbg_mode.data[3] = 0;
		
		_shaderDebugger.modes.push_back( std::move(dbg_mode) );
		return ShaderDbgIndex(_shaderDebugger.modes.size() - 1);
	}
	
/*
=================================================
	AppendShader
=================================================
*/
	ShaderDbgIndex  VCmdBatch::AppendShader (const TaskName_t &name, const _fg_hidden_::ComputeShaderDebugMode &mode, BytesU size)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( FG_EnableShaderDebugging );

		DebugMode	dbg_mode;
		dbg_mode.taskName		= name;
		dbg_mode.mode			= mode.mode;
		dbg_mode.shaderStages	= EShaderStages::Compute;

		CHECK_ERR( _AllocStorage( INOUT dbg_mode, size ));

		MemCopy( OUT dbg_mode.data, mode.globalID );
		dbg_mode.data[3] = 0;
		
		_shaderDebugger.modes.push_back( std::move(dbg_mode) );
		return ShaderDbgIndex(_shaderDebugger.modes.size() - 1);
	}
	
/*
=================================================
	AppendShader
=================================================
*/
	ShaderDbgIndex  VCmdBatch::AppendShader (const TaskName_t &name, const _fg_hidden_::RayTracingShaderDebugMode &mode, BytesU size)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( FG_EnableShaderDebugging );

		DebugMode	dbg_mode;
		dbg_mode.taskName		= name;
		dbg_mode.mode			= mode.mode;
		dbg_mode.shaderStages	= EShaderStages::AllRayTracing;
		
		CHECK_ERR( _AllocStorage( INOUT dbg_mode, size ));

		MemCopy( OUT dbg_mode.data, mode.launchID );
		dbg_mode.data[3] = 0;
		
		_shaderDebugger.modes.push_back( std::move(dbg_mode) );
		return ShaderDbgIndex(_shaderDebugger.modes.size() - 1);
	}
	
/*
=================================================
	_AllocStorage
=================================================
*/
	bool  VCmdBatch::_AllocStorage (INOUT DebugMode &dbgMode, const BytesU size)
	{
		VkPipelineStageFlags	stage = 0;

		for (EShaderStages s = EShaderStages(1); s <= dbgMode.shaderStages; s = EShaderStages(uint(s) << 1))
		{
			if ( not EnumEq( dbgMode.shaderStages, s ) )
				continue;

			ENABLE_ENUM_CHECKS();
			switch ( s )
			{
				case EShaderStages::Vertex :		stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;					break;
				case EShaderStages::TessControl :	stage = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;		break;
				case EShaderStages::TessEvaluation:	stage = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;	break;
				case EShaderStages::Geometry :		stage = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;					break;
				case EShaderStages::Fragment :		stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;					break;
				case EShaderStages::Compute :		stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;					break;
				case EShaderStages::MeshTask :		stage = VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV;					break;
				case EShaderStages::Mesh :			stage = VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;					break;
				case EShaderStages::RayGen :
				case EShaderStages::RayAnyHit :
				case EShaderStages::RayClosestHit :
				case EShaderStages::RayMiss :
				case EShaderStages::RayIntersection:
				case EShaderStages::RayCallable :	stage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV;			break;
				case EShaderStages::_Last :
				case EShaderStages::Unknown :
				case EShaderStages::AllGraphics :
				case EShaderStages::AllRayTracing :
				case EShaderStages::All :			// to shutup warnings	
				default :							RETURN_ERR( "unknown shader type" );
			}
			DISABLE_ENUM_CHECKS();
		}

		dbgMode.size = Min( size, _shaderDebugger.bufferSize );

		// find place in existing storage buffers
		for (auto& sb : _shaderDebugger.buffers)
		{
			dbgMode.offset = AlignToLarger( sb.size, _shaderDebugger.bufferAlign );

			if ( dbgMode.size <= (sb.capacity - dbgMode.offset) )
			{
				dbgMode.sbIndex	= uint(Distance( _shaderDebugger.buffers.data(), &sb ));
				sb.size			= dbgMode.offset + size;
				sb.stages		|= stage;
				break;
			}
		}

		// create new storage buffer
		if ( dbgMode.sbIndex == UMax )
		{
			StorageBuffer	sb;
			sb.capacity			 = _shaderDebugger.bufferSize * (1 + _shaderDebugger.buffers.size() / 2);
			sb.shaderTraceBuffer = _frameGraph.CreateBuffer( BufferDesc{ sb.capacity, EBufferUsage::Storage | EBufferUsage::Transfer },
															 Default, "DebugOutputStorage" );
			sb.readBackBuffer	 = _frameGraph.CreateBuffer( BufferDesc{ sb.capacity, EBufferUsage::TransferDst },
															 MemoryDesc{EMemoryType::HostRead}, "ReadBackDebugOutput" );
			CHECK_ERR( sb.shaderTraceBuffer and sb.readBackBuffer );
			
			dbgMode.sbIndex	= uint(_shaderDebugger.buffers.size());
			dbgMode.offset	= 0_b;
			sb.size			= dbgMode.offset + size;
			sb.stages		|= stage;
			
			_shaderDebugger.buffers.push_back( std::move(sb) );
		}

		CHECK_ERR( _AllocDescriptorSet( dbgMode.mode, dbgMode.shaderStages,
									    _shaderDebugger.buffers[dbgMode.sbIndex].shaderTraceBuffer.Get(),
									    dbgMode.size, OUT dbgMode.descriptorSet ));
		return true;
	}
	
/*
=================================================
	_AllocDescriptorSet
=================================================
*/
	bool  VCmdBatch::_AllocDescriptorSet (EShaderDebugMode debugMode, EShaderStages stages, RawBufferID storageBuffer, BytesU size, OUT VkDescriptorSet &descSet)
	{
		auto&	dev			= _frameGraph.GetDevice();
		auto&	rm			= _frameGraph.GetResourceManager();
		auto	layout_id	= rm.GetDescriptorSetLayout( debugMode, stages );
		auto*	layout		= rm.GetResource( layout_id );
		auto*	buffer		= rm.GetResource( storageBuffer );
		CHECK_ERR( layout and buffer );

		// find descriptor set in cache
		auto	iter = _shaderDebugger.descCache.find({ storageBuffer, layout_id });

		if ( iter != _shaderDebugger.descCache.end() )
		{
			descSet = iter->second.first;
			return true;
		}

		// allocate descriptor set
		{
			VDescriptorSetLayout::DescriptorSet	ds;
			CHECK_ERR( rm.GetDescriptorManager().AllocDescriptorSet( layout->Handle(), OUT ds ));

			descSet = ds.first;
			_shaderDebugger.descCache.insert_or_assign( {storageBuffer, layout_id}, ds );
		}

		// update descriptor set
		{
			VkDescriptorBufferInfo	buffer_desc = {};
			buffer_desc.buffer		= buffer->Handle();
			buffer_desc.offset		= 0;
			buffer_desc.range		= VkDeviceSize(size);

			VkWriteDescriptorSet	write = {};
			write.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet			= descSet;
			write.dstBinding		= 0;
			write.descriptorCount	= 1;
			write.descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			write.pBufferInfo		= &buffer_desc;

			dev.vkUpdateDescriptorSets( dev.GetVkDevice(), 1, &write, 0, null );
		}
		return true;
	}


}	// FG
