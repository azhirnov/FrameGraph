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
	VCmdBatch::VCmdBatch (VResourceManager &resMngr, VDeviceQueueInfoPtr queue, EQueueType type, ArrayView<CommandBuffer> dependsOn) :
		_state{ EState::Initial },	_resMngr{ resMngr },
		_queue{ queue },			_queueType{ type }
	{
		STATIC_ASSERT( decltype(_state)::is_always_lock_free );
		
		_shaderDebugger.bufferAlign = BytesU{resMngr.GetDevice().GetDeviceLimits().minStorageBufferOffsetAlignment};
			
		if ( FG_EnableShaderDebugging ) {
			CHECK( resMngr.GetDevice().GetDeviceLimits().maxBoundDescriptorSets > FG_DebugDescriptorSet );
		}

		for (auto& dep : dependsOn)
		{
			if ( auto* batch = Cast<VCmdBatch>(dep.GetBatch()) )
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
	OnBegin
=================================================
*/
	bool  VCmdBatch::OnBegin (const CommandBufferDesc &desc)
	{
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
	bool  VCmdBatch::OnComplete (VDebugger &debugger, const ShaderDebugCallback_t &shaderDbgCallback)
	{
		ASSERT( _submitted );
		_SetState( EState::Complete );

		_ParseDebugOutput( shaderDbgCallback );
		
		_FinalizeStagingBuffers();
		_ReleaseResources();

		debugger.AddBatchDump( _debugDump );
		_debugDump.clear();

		_submitted = null;
		return true;
	}
	
/*
=================================================
	_FinalizeStagingBuffers
=================================================
*/
	void VCmdBatch::_FinalizeStagingBuffers ()
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
			for (auto& sb : _staging.hostToDevice) {
				_resMngr.ReleaseResource( sb.bufferId.Release() );
			}
			_staging.hostToDevice.clear();

			for (auto& sb : _staging.deviceToHost) {
				_resMngr.ReleaseResource( sb.bufferId.Release() );
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

		auto&	dev = _resMngr.GetDevice();
		VK_CHECK( dev.vkDeviceWaitIdle( dev.GetVkDevice() ), void());

		// release descriptor sets
		for (auto& ds : _shaderDebugger.descCache) {
			_resMngr.GetDescriptorManager().DeallocDescriptorSet( ds.second );
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
			_resMngr.ReleaseResource( sb.shaderTraceBuffer.Release() );
			_resMngr.ReleaseResource( sb.readBackBuffer.Release() );
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
		
		auto	read_back_buf	= _shaderDebugger.buffers[ dbg.sbIndex ].readBackBuffer.Get();

		VBuffer const*		buf	= _resMngr.GetResource( read_back_buf );
		CHECK_ERR( buf );

		VMemoryObj const*	mem = _resMngr.GetResource( buf->GetMemoryID() );
		CHECK_ERR( mem );

		VMemoryObj::MemoryInfo	info;
		CHECK_ERR( mem->GetInfo( _resMngr.GetMemoryManager(), OUT info ));
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
		if ( _resMngr.GetResource( buf.memoryId )->GetInfo( _resMngr.GetMemoryManager(),OUT info ) )
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
		for (auto& res : _resourcesToRelease)
		{
			switch ( res.GetUID() )
			{
				case RawBufferID::GetUID() :			_resMngr.ReleaseResource( RawBufferID{ res.Index(), res.InstanceID() });				break;
				case RawImageID::GetUID() :				_resMngr.ReleaseResource( RawImageID{ res.Index(), res.InstanceID() });					break;
				case RawGPipelineID::GetUID() :			_resMngr.ReleaseResource( RawGPipelineID{ res.Index(), res.InstanceID() });				break;
				case RawMPipelineID::GetUID() :			_resMngr.ReleaseResource( RawMPipelineID{ res.Index(), res.InstanceID() });				break;
				case RawCPipelineID::GetUID() :			_resMngr.ReleaseResource( RawCPipelineID{ res.Index(), res.InstanceID() });				break;
				case RawRTPipelineID::GetUID() :		_resMngr.ReleaseResource( RawRTPipelineID{ res.Index(), res.InstanceID() });			break;
				case RawSamplerID::GetUID() :			_resMngr.ReleaseResource( RawSamplerID{ res.Index(), res.InstanceID() });				break;
				case RawDescriptorSetLayoutID::GetUID():_resMngr.ReleaseResource( RawDescriptorSetLayoutID{ res.Index(), res.InstanceID() });	break;
				case RawPipelineResourcesID::GetUID() :	_resMngr.ReleaseResource( RawPipelineResourcesID{ res.Index(), res.InstanceID() });		break;
				case RawRTSceneID::GetUID() :			_resMngr.ReleaseResource( RawRTSceneID{ res.Index(), res.InstanceID() });				break;
				case RawRTGeometryID::GetUID() :		_resMngr.ReleaseResource( RawRTGeometryID{ res.Index(), res.InstanceID() });			break;
				case RawRTShaderTableID::GetUID() :		_resMngr.ReleaseResource( RawRTShaderTableID{ res.Index(), res.InstanceID() });			break;
				case RawSwapchainID::GetUID() :			_resMngr.ReleaseResource( RawSwapchainID{ res.Index(), res.InstanceID() });				break;
				case RawMemoryID::GetUID() :			_resMngr.ReleaseResource( RawMemoryID{ res.Index(), res.InstanceID() });				break;
				case RawPipelineLayoutID::GetUID() :	_resMngr.ReleaseResource( RawPipelineLayoutID{ res.Index(), res.InstanceID() });		break;
				case RawRenderPassID::GetUID() :		_resMngr.ReleaseResource( RawRenderPassID{ res.Index(), res.InstanceID() });			break;
				case RawFramebufferID::GetUID() :		_resMngr.ReleaseResource( RawFramebufferID{ res.Index(), res.InstanceID() });			break;
				default :								CHECK( !"not supported" );																break;
			}
		}
		_resourcesToRelease.clear();
	}
	
/*
=================================================
_GetWritable
=================================================
*/
	bool VCmdBatch::GetWritable (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
								 OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &outSize, OUT void* &mappedPtr)
	{
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

			BufferID	buf_id{ _resMngr.CreateBuffer( BufferDesc{ _staging.hostWritableBufferSize, _staging.hostWritebleBufferUsage }, 
													   MemoryDesc{ EMemoryType::HostWrite }, EQueueFamilyMask(0) | _queue->familyIndex,
													   "HostWriteBuffer" )};
			CHECK_ERR( buf_id );

			RawMemoryID	mem_id = _resMngr.GetResource( buf_id.Get() )->GetMemoryID();
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

			BufferID	buf_id{ _resMngr.CreateBuffer( BufferDesc{ _staging.hostReadableBufferSize, EBufferUsage::TransferDst },
													   MemoryDesc{ EMemoryType::HostRead }, EQueueFamilyMask(0) | _queue->familyIndex,
													   "HostReadBuffer" )};
			CHECK_ERR( buf_id );
			
			RawMemoryID	mem_id = _resMngr.GetResource( buf_id.Get() )->GetMemoryID();
			CHECK_ERR( mem_id );

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
		CHECK( GetState() == EState::Recording );

		if ( _shaderDebugger.buffers.empty() )
			return;
		
		auto&	dev = _resMngr.GetDevice();

		// copy data
		for (auto& dbg : _shaderDebugger.modes)
		{
			auto	buf		= _resMngr.GetResource( _shaderDebugger.buffers[dbg.sbIndex].shaderTraceBuffer.Get() )->Handle();
			BytesU	size	= _resMngr.GetDebugShaderStorageSize( dbg.shaderStages ) + SizeOf<uint>;	// per shader data + position
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
			barrier.buffer			= _resMngr.GetResource( sb.shaderTraceBuffer.Get() )->Handle();
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
		CHECK( GetState() == EState::Recording );

		if ( _shaderDebugger.buffers.empty() )
			return;
		
		auto&	dev = _resMngr.GetDevice();
		
		// copy to staging buffer
		for (auto& sb : _shaderDebugger.buffers)
		{
			VkBuffer	src_buf	= _resMngr.GetResource( sb.shaderTraceBuffer.Get() )->Handle();
			VkBuffer	dst_buf	= _resMngr.GetResource( sb.readBackBuffer.Get() )->Handle();

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
			sb.capacity				= _shaderDebugger.bufferSize * (1 + _shaderDebugger.buffers.size() / 2);
			sb.shaderTraceBuffer	= BufferID{_resMngr.CreateBuffer( BufferDesc{ sb.capacity, EBufferUsage::Storage | EBufferUsage::Transfer },
																	  Default, EQueueFamilyMask(0) | _queue->familyIndex,
																	  "DebugOutputStorage" )};
			sb.readBackBuffer		= BufferID{_resMngr.CreateBuffer( BufferDesc{ sb.capacity, EBufferUsage::TransferDst },
																	  MemoryDesc{EMemoryType::HostRead}, EQueueFamilyMask(0) | _queue->familyIndex,
																	  "ReadBackDebugOutput" )};
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
		auto&	dev			= _resMngr.GetDevice();
		auto	layout_id	= _resMngr.GetDescriptorSetLayout( debugMode, stages );
		auto*	layout		= _resMngr.GetResource( layout_id );
		auto*	buffer		= _resMngr.GetResource( storageBuffer );
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
			CHECK_ERR( _resMngr.GetDescriptorManager().AllocDescriptorSet( layout->Handle(), OUT ds ));

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
