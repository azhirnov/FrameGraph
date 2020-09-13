// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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

		if ( auto queue = _frameGraph.FindQueue( type ))
		{
			_supportsQuery = AnyBits( queue->familyFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT );
		}

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

		// skip duplicates
		for (auto& dep : _dependencies)
		{
			if ( dep == batch )
				return;
		}

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
		
		_dbgQueueSync	= AllBits( desc.debugFlags, EDebugFlags::QueueSync );
		_statistic		= Default;
		_debugName		= desc.name;

		return true;
	}
	
/*
=================================================
	OnBeginRecording
=================================================
*/
	void  VCmdBatch::OnBeginRecording (VkCommandBuffer cmd)
	{
		EXLOCK( _drCheck );
		CHECK( GetState() == EState::Recording );

		if ( _supportsQuery )
		{
			VDevice const&	dev		= _frameGraph.GetDevice();
			VkQueryPool		pool	= _frameGraph.GetQueryPool();
		
			dev.vkCmdResetQueryPool( cmd, pool, _indexInPool*2, 2 );
			dev.vkCmdWriteTimestamp( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, pool, _indexInPool*2 );
		}

		_BeginShaderDebugger( cmd );
	}
	
/*
=================================================
	OnEndRecording
=================================================
*/
	void  VCmdBatch::OnEndRecording (VkCommandBuffer cmd)
	{
		EXLOCK( _drCheck );
		CHECK( GetState() == EState::Recording );
		
		_EndShaderDebugger( cmd );
		
		if ( _supportsQuery )
		{
			VDevice const&	dev		= _frameGraph.GetDevice();
			VkQueryPool		pool	= _frameGraph.GetQueryPool();

			dev.vkCmdWriteTimestamp( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, pool, _indexInPool*2 + 1 );
		}
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


		// flush mapped memory before submitting
		FixedArray<VkMappedMemoryRange, 32>		regions;
		VDevice const&							dev = _frameGraph.GetDevice();
		
		for (auto& buf : _staging.hostToDevice)
		{
			if ( buf.isCoherent )
				continue;

			if ( regions.size() == regions.capacity() )
			{
				VK_CALL( dev.vkFlushMappedMemoryRanges( dev.GetVkDevice(), uint(regions.size()), regions.data() ));
				regions.clear();
			}

			auto&	reg = regions.emplace_back();
			reg.sType	= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			reg.pNext	= null;
			reg.memory	= buf.mem;
			reg.offset	= VkDeviceSize(buf.memOffset);
			reg.size	= VkDeviceSize(buf.size);
		}
		
		if ( regions.size() )
			VK_CALL( dev.vkFlushMappedMemoryRanges( dev.GetVkDevice(), uint(regions.size()), regions.data() ));

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
	bool  VCmdBatch::OnComplete (VDebugger &debugger, const ShaderDebugCallback_t &shaderDbgCallback, INOUT Statistic_t &outStatistic)
	{
		EXLOCK( _drCheck );
		ASSERT( _submitted );

		_SetState( EState::Complete );

		_FinalizeCommands();
		_ParseDebugOutput( shaderDbgCallback );
		_FinalizeStagingBuffers( _frameGraph.GetDevice() );
		_ReleaseResources();
		_ReleaseVkObjects();

		debugger.AddBatchDump( _debugName, std::move(_debugDump) );
		debugger.AddBatchGraph( std::move(_debugGraph) );

		_debugDump  = {};
		_debugGraph	= Default;
		
		// read frame time
		if ( _supportsQuery )
		{
			VDevice const&	dev		= _frameGraph.GetDevice();
			VkQueryPool		pool	= _frameGraph.GetQueryPool();
			uint64_t		query_results[2];

			VK_CALL( dev.vkGetQueryPoolResults( dev.GetVkDevice(), pool, _indexInPool*2, uint(CountOf(query_results)),
												sizeof(query_results), OUT query_results,
												sizeof(query_results[0]), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT ));

			_statistic.renderer.gpuTime += Nanoseconds{query_results[1] - query_results[0]};
		}
		outStatistic.Merge( _statistic );

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
	void  VCmdBatch::_FinalizeStagingBuffers (const VDevice &dev)
	{
		using T = BufferView::value_type;
		
		FixedArray<VkMappedMemoryRange, 32>		regions;

		// map device-to-host staging buffers
		for (auto& buf : _staging.deviceToHost)
		{
			// buffer may be recreated on defragmentation pass, so we need to obtain actual pointer every frame
			CHECK( _MapMemory( INOUT buf ));
			
			if ( buf.isCoherent )
				continue;

			// invalidate non-cocherent memory before reading
			if ( regions.size() == regions.capacity() )
			{
				VK_CALL( dev.vkInvalidateMappedMemoryRanges( dev.GetVkDevice(), uint(regions.size()), regions.data() ));
				regions.clear();
			}

			auto&	reg = regions.emplace_back();
			reg.sType	= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			reg.pNext	= null;
			reg.memory	= buf.mem;
			reg.offset	= VkDeviceSize(buf.memOffset);
			reg.size	= VkDeviceSize(buf.size);
		}

		if ( regions.size() )
			VK_CALL( dev.vkInvalidateMappedMemoryRanges( dev.GetVkDevice(), uint(regions.size()), regions.data() ));


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
				rm.ReleaseStagingBuffer( sb.index );
			}
			_staging.hostToDevice.clear();

			for (auto& sb : _staging.deviceToHost) {
				rm.ReleaseStagingBuffer( sb.index );
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

		auto&	rm  = _frameGraph.GetResourceManager();

		//VK_CHECK( dev.vkDeviceWaitIdle( dev.GetVkDevice() ), void());

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
		ASSERT( cb );

		if ( (not cb) or (dbg.mode == EShaderDebugMode::Timemap) )
			return true;

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

		if ( rm.GetResource( buf.memoryId )->GetInfo( rm.GetMemoryManager(),OUT info ))
		{
			buf.mappedPtr	= info.mappedPtr;
			buf.memOffset	= info.offset;
			buf.mem			= info.mem;
			buf.isCoherent	= AllBits( info.flags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
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
				case RawCPipelineID::GetUID() :			rm.ReleaseResource( RawCPipelineID{ res.Index(), res.InstanceID() }, count );			break;
				case RawSamplerID::GetUID() :			rm.ReleaseResource( RawSamplerID{ res.Index(), res.InstanceID() }, count );				break;
				case RawDescriptorSetLayoutID::GetUID():rm.ReleaseResource( RawDescriptorSetLayoutID{ res.Index(), res.InstanceID() }, count );	break;
				case RawPipelineResourcesID::GetUID() :	rm.ReleaseResource( RawPipelineResourcesID{ res.Index(), res.InstanceID() }, count );	break;

				#ifdef VK_NV_mesh_shader
				case RawMPipelineID::GetUID() :			rm.ReleaseResource( RawMPipelineID{ res.Index(), res.InstanceID() }, count );			break;
				#endif

				#ifdef VK_NV_ray_tracing
				case RawRTPipelineID::GetUID() :		rm.ReleaseResource( RawRTPipelineID{ res.Index(), res.InstanceID() }, count );			break;
				case RawRTSceneID::GetUID() :			rm.ReleaseResource( RawRTSceneID{ res.Index(), res.InstanceID() }, count );				break;
				case RawRTGeometryID::GetUID() :		rm.ReleaseResource( RawRTGeometryID{ res.Index(), res.InstanceID() }, count );			break;
				case RawRTShaderTableID::GetUID() :		rm.ReleaseResource( RawRTShaderTableID{ res.Index(), res.InstanceID() }, count );		break;
				#endif

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
			BEGIN_ENUM_CHECKS();
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

				#ifdef VK_KHR_sampler_ycbcr_conversion
				case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION :
					dev.vkDestroySamplerYcbcrConversionKHR( vdev, VkSamplerYcbcrConversion(pair.second), null );
					break;
				#endif

				case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE :
					dev.vkDestroyDescriptorUpdateTemplateKHR( vdev, VkDescriptorUpdateTemplate(pair.second), null );
					break;

				#ifdef VK_NV_ray_tracing
				case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV :
					dev.vkDestroyAccelerationStructureNV( vdev, VkAccelerationStructureNV(pair.second), null );
					break;
				#endif

				case VK_OBJECT_TYPE_UNKNOWN :
				case VK_OBJECT_TYPE_INSTANCE :
				case VK_OBJECT_TYPE_PHYSICAL_DEVICE :
				case VK_OBJECT_TYPE_DEVICE :
				case VK_OBJECT_TYPE_QUEUE :
				case VK_OBJECT_TYPE_COMMAND_BUFFER :
				case VK_OBJECT_TYPE_SHADER_MODULE :
				case VK_OBJECT_TYPE_PIPELINE_CACHE :
				case VK_OBJECT_TYPE_DESCRIPTOR_SET :
				case VK_OBJECT_TYPE_COMMAND_POOL :
				case VK_OBJECT_TYPE_SURFACE_KHR :
				case VK_OBJECT_TYPE_SWAPCHAIN_KHR :
				case VK_OBJECT_TYPE_DISPLAY_KHR :
				case VK_OBJECT_TYPE_DISPLAY_MODE_KHR :
				case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT :
				case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT :
				case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT :
					
				#ifdef VK_KHR_deferred_host_operations
				case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR :
				#endif

				#ifdef VK_EXT_private_data
				case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT :
				#endif

				#ifdef VK_NV_device_generated_commands
				case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV :
				#elif defined(VK_NVX_device_generated_commands)
				case VK_OBJECT_TYPE_OBJECT_TABLE_NVX :
				case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX :
				#endif

				#ifdef VK_INTEL_performance_query
				case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL :
				#endif

				#ifndef VK_VERSION_1_2
				case VK_OBJECT_TYPE_RANGE_SIZE :
				#endif

				case VK_OBJECT_TYPE_MAX_ENUM :
				default :
					FG_LOGE( "resource type is not supported" );
					break;
			}
			END_ENUM_CHECKS();
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

		auto&			staging_buffers = _staging.hostToDevice;
		const BytesU	stagingbuf_size	= _frameGraph.GetResourceManager().GetHostWriteBufferSize();

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
			ASSERT( dstMinSize < stagingbuf_size );
			CHECK_ERR( staging_buffers.size() < staging_buffers.capacity() );

			VResourceManager&	rm = _frameGraph.GetResourceManager();
			
			RawBufferID			buf_id;
			StagingBufferIdx	buf_idx;
			CHECK_ERR( rm.CreateStagingBuffer( EBufferUsage::TransferSrc, OUT buf_id, OUT buf_idx ));

			RawMemoryID		mem_id = rm.GetResource( buf_id )->GetMemoryID();
			CHECK_ERR( mem_id );

			staging_buffers.push_back({ buf_idx, buf_id, mem_id, stagingbuf_size });

			suitable = &staging_buffers.back();
			CHECK( _MapMemory( *suitable ));
		}

		// write data to buffer
		dstOffset	= AlignToLarger( suitable->size, offsetAlign );
		outSize		= Min( AlignToSmaller( suitable->capacity - dstOffset, blockAlign ), srcRequiredSize );
		dstBuffer	= suitable->bufferId;
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

		auto&			staging_buffers = _staging.deviceToHost;
		const BytesU	stagingbuf_size	= _frameGraph.GetResourceManager().GetHostReadBufferSize();
		

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
			ASSERT( dstMinSize < stagingbuf_size );
			CHECK_ERR( staging_buffers.size() < staging_buffers.capacity() );
			
			VResourceManager&	rm = _frameGraph.GetResourceManager();
			
			RawBufferID			buf_id;
			StagingBufferIdx	buf_idx;
			CHECK_ERR( rm.CreateStagingBuffer( EBufferUsage::TransferDst, OUT buf_id, OUT buf_idx ));
			
			RawMemoryID		mem_id = rm.GetResource( buf_id )->GetMemoryID();
			CHECK_ERR( mem_id );

			// TODO: make immutable because read after write happens after waiting for fences and it implicitly make changes visible to the host

			staging_buffers.push_back({ buf_idx, buf_id, mem_id, stagingbuf_size });

			suitable = &staging_buffers.back();
			CHECK( _MapMemory( *suitable ));
		}
		
		// write data to buffer
		range.buffer	= suitable;
		range.offset	= AlignToLarger( suitable->size, offsetAlign );
		range.size		= Min( AlignToSmaller( suitable->capacity - range.offset, blockAlign ), srcRequiredSize );
		dstBuffer		= suitable->bufferId;

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
	_BeginShaderDebugger
=================================================
*/
	void  VCmdBatch::_BeginShaderDebugger (VkCommandBuffer cmd)
	{
		if ( _shaderDebugger.buffers.empty() )
			return;
		
		auto&	dev = _frameGraph.GetDevice();
		auto&	rm  = _frameGraph.GetResourceManager();
		
		auto	AddBarriers = [&dev, &rm, cmd, this] (VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags dstStage)
		{
			FixedArray< VkBufferMemoryBarrier, 16 >		barriers;
			VkPipelineStageFlags						dst_stage_flags = 0;

			for (auto& sb : _shaderDebugger.buffers)
			{
				VkBufferMemoryBarrier	barrier = {};
				barrier.sType			= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				barrier.srcAccessMask	= srcAccess;
				barrier.dstAccessMask	= dstAccess;
				barrier.buffer			= rm.GetResource( sb.shaderTraceBuffer.Get() )->Handle();
				barrier.offset			= 0;
				barrier.size			= VkDeviceSize(sb.size);
				barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

				dst_stage_flags |= sb.stages;
				barriers.push_back( barrier );

				if ( barriers.size() == barriers.capacity() )
				{
					dev.vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, dst_stage_flags | dstStage, 0, 0, null, uint(barriers.size()), barriers.data(), 0, null );
					barriers.clear();
					dst_stage_flags = 0;
				}
			}
		
			if ( barriers.size() )
				dev.vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, dst_stage_flags | dstStage, 0, 0, null, uint(barriers.size()), barriers.data(), 0, null );
		};
		
		AddBarriers( 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

		// clear
		for (auto& sb : _shaderDebugger.buffers)
		{
			VkBuffer	buf = rm.GetResource( sb.shaderTraceBuffer.Get() )->Handle();

			dev.vkCmdFillBuffer( cmd, buf, 0, VkDeviceSize(sb.size), 0 );
		}

		AddBarriers( VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

		// copy data
		for (auto& dbg : _shaderDebugger.modes)
		{
			VkBuffer	buf		= rm.GetResource( _shaderDebugger.buffers[dbg.sbIndex].shaderTraceBuffer.Get() )->Handle();
			BytesU		size	= rm.GetDebugShaderStorageSize( dbg.shaderStages, dbg.mode );
			ASSERT( size <= BytesU::SizeOf(dbg.data) );

			dev.vkCmdUpdateBuffer( cmd, buf, VkDeviceSize(dbg.offset), VkDeviceSize(size), dbg.data );
		}

		AddBarriers( VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, 0 );
	}
	
/*
=================================================
	_EndShaderDebugger
=================================================
*/
	void  VCmdBatch::_EndShaderDebugger (VkCommandBuffer cmd)
	{
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
		dynamicOffset	= CheckCast<uint>(dbg.offset);
		return true;
	}
	
/*
=================================================
	GetShaderTimemap
=================================================
*/
	bool  VCmdBatch::GetShaderTimemap (ShaderDbgIndex id, RawBufferID &buf, OUT BytesU &offset, OUT BytesU &size, OUT uint2 &dim) const
	{
		EXLOCK( _drCheck );
		CHECK_ERR( uint(id) < _shaderDebugger.modes.size() );
		
		auto&	dbg  = _shaderDebugger.modes[ uint(id) ];
		
		buf		= _shaderDebugger.buffers[ dbg.sbIndex ].shaderTraceBuffer;
		offset	= dbg.offset;
		size	= dbg.size;
		dim		= uint2{ dbg.data[2], dbg.data[3] };

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
	AppendTimemap
=================================================
*/
	ShaderDbgIndex  VCmdBatch::AppendTimemap (const uint2 &dim, EShaderStages stages)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( FG_EnableShaderDebugging );

		DebugMode	dbg_mode;
		dbg_mode.mode			= EShaderDebugMode::Timemap;
		dbg_mode.shaderStages	= stages;

		BytesU		size =	(SizeOf<uint> * 4) +														// first 4 components
							(dim.x * dim.y * SizeOf<uint64_t>) +										// output pixels
							_frameGraph.GetDevice().GetDeviceLimits().minStorageBufferOffsetAlignment +	// align
							(dim.y * SizeOf<uint64_t>);													// temporary line
		
		CHECK_ERR( _AllocStorage( INOUT dbg_mode, size ));
		
		dbg_mode.data[0] = BitCast<uint>( 1.0f );
		dbg_mode.data[1] = BitCast<uint>( 1.0f );
		dbg_mode.data[2] = dim.x;
		dbg_mode.data[3] = dim.y;
		
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
			if ( not AllBits( dbgMode.shaderStages, s ))
				continue;

			BEGIN_ENUM_CHECKS();
			switch ( s )
			{
				case EShaderStages::Vertex :		stage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;					break;
				case EShaderStages::TessControl :	stage |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;		break;
				case EShaderStages::TessEvaluation:	stage |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;	break;
				case EShaderStages::Geometry :		stage |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;					break;
				case EShaderStages::Fragment :		stage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;					break;
				case EShaderStages::Compute :		stage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;					break;

				#ifdef VK_NV_mesh_shader
				case EShaderStages::MeshTask :		stage |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV;					break;
				case EShaderStages::Mesh :			stage |= VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;					break;
				#else
				case EShaderStages::MeshTask :
				case EShaderStages::Mesh :			break;
				#endif

				#ifdef VK_NV_ray_tracing
				case EShaderStages::RayGen :
				case EShaderStages::RayAnyHit :
				case EShaderStages::RayClosestHit :
				case EShaderStages::RayMiss :
				case EShaderStages::RayIntersection:
				case EShaderStages::RayCallable :	stage |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV;			break;
				#else
				case EShaderStages::RayGen :
				case EShaderStages::RayAnyHit :
				case EShaderStages::RayClosestHit :
				case EShaderStages::RayMiss :
				case EShaderStages::RayIntersection:
				case EShaderStages::RayCallable :	break;
				#endif

				case EShaderStages::_Last :
				case EShaderStages::Unknown :
				case EShaderStages::AllGraphics :
				case EShaderStages::AllRayTracing :
				case EShaderStages::All :			// to shutup warnings	
				default :							RETURN_ERR( "unknown shader type" );
			}
			END_ENUM_CHECKS();
		}

		dbgMode.size = Min( size, _shaderDebugger.bufferSize );

		// find place in existing storage buffers
		for (auto& sb : _shaderDebugger.buffers)
		{
			dbgMode.offset = AlignToLarger( sb.size, _shaderDebugger.bufferAlign );

			if ( dbgMode.size <= (sb.capacity - dbgMode.offset) )
			{
				dbgMode.sbIndex	= CheckCast<uint>( Distance( _shaderDebugger.buffers.data(), &sb ));
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
			
			dbgMode.sbIndex	= CheckCast<uint>(_shaderDebugger.buffers.size());
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
