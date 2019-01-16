// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VShaderDebugger.h"
#include "VFrameGraphThread.h"
#include "VEnumCast.h"
#include "VBuffer.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VShaderDebugger::VShaderDebugger (VFrameGraphThread &fg) :
		_frameGraph{ fg },
		_bufferAlign{ _frameGraph.GetDevice().GetDeviceLimits().minStorageBufferOffsetAlignment }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VShaderDebugger::~VShaderDebugger ()
	{
		auto&	dev = _frameGraph.GetDevice();

		if ( _descPool ) {
			dev.vkDestroyDescriptorPool( dev.GetVkDevice(), _descPool, null );
		}

		for (auto& ds_layout : _layoutCache) {
			_frameGraph.GetResourceManager()->ReleaseResource( ds_layout.second.Release(), false );
		}
		_layoutCache.clear();

		CHECK( _storageBuffers.empty() );
	}

/*
=================================================
	OnBeginRecording
=================================================
*/
	void VShaderDebugger::OnBeginRecording (VkCommandBuffer cmd)
	{
		if ( _storageBuffers.empty() )
			return;

		auto&	dev = _frameGraph.GetDevice();

		// copy data
		for (auto& dbg : _debugModes)
		{
			auto	buf		= _frameGraph.GetResourceManager()->GetResource( _storageBuffers[dbg.sbIndex].shaderTraceBuffer.Get() )->Handle();
			BytesU	size	= _GetBufferStaticSize( dbg.shaderType ) + SizeOf<uint>;	// per shader data + position
			ASSERT( size <= BytesU::SizeOf(dbg.data) );

			dev.vkCmdUpdateBuffer( cmd, buf, VkDeviceSize(dbg.offset), VkDeviceSize(size), dbg.data );
		}

		// add pipeline barriers
		FixedArray< VkBufferMemoryBarrier, 16 >		barriers;
		VkPipelineStageFlags						dst_stage_flags = 0;

		for (auto& sb : _storageBuffers)
		{
			VkBufferMemoryBarrier	barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask	= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			barrier.buffer			= _frameGraph.GetResourceManager()->GetResource( sb.shaderTraceBuffer.Get() )->Handle();
			barrier.offset			= 0;
			barrier.size			= VkDeviceSize(sb.size);
			barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

			dst_stage_flags |= sb.stages;
			barriers.push_back( barrier );
		}
		
		dev.vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, dst_stage_flags, 0, 0, null, uint(barriers.size()), barriers.data(), 0, null );
	}
	
/*
=================================================
	OnEndRecording
=================================================
*/
	void VShaderDebugger::OnEndRecording (VkCommandBuffer cmd)
	{
		if ( _storageBuffers.empty() )
			return;

		auto&	dev = _frameGraph.GetDevice();
		
		// copy to staging buffer
		for (auto& sb : _storageBuffers)
		{
			VkBuffer	src_buf	= _frameGraph.GetResourceManager()->GetResource( sb.shaderTraceBuffer.Get() )->Handle();
			VkBuffer	dst_buf	= _frameGraph.GetResourceManager()->GetResource( sb.readBackBuffer.Get() )->Handle();

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
	OnEndFrame
=================================================
*/
	void VShaderDebugger::OnEndFrame (const ShaderDebugCallback_t &cb)
	{
		if ( _storageBuffers.empty() )
			return;

		ASSERT( cb );

		auto&	dev = _frameGraph.GetDevice();

		VK_CHECK( dev.vkDeviceWaitIdle( dev.GetVkDevice() ), void());

		// release descriptor sets
		VK_CALL( dev.vkResetDescriptorPool( dev.GetVkDevice(), _descPool, 0 ));
		_descCache.clear();

		// process shader debug output
		if ( cb ) {
			for (auto& dbg : _debugModes) {
				_ParseDebugOutput( cb, dbg );
			}
		}
		_debugModes.clear();

		// release storage buffers
		for (auto& sb : _storageBuffers)
		{
			_frameGraph.ReleaseResource( sb.shaderTraceBuffer );
			_frameGraph.ReleaseResource( sb.readBackBuffer );
		}
		_storageBuffers.clear();
	}
	
/*
=================================================
	_ParseDebugOutput
=================================================
*/
	bool VShaderDebugger::_ParseDebugOutput (const ShaderDebugCallback_t &cb, const DebugMode &dbg) const
	{
		CHECK_ERR( dbg.module );

		auto	read_back_buf	= _storageBuffers[ dbg.sbIndex ].readBackBuffer.Get();

		VBuffer const*		buf	= _frameGraph.GetResourceManager()->GetResource( read_back_buf );
		CHECK_ERR( buf );

		VMemoryObj const*	mem = _frameGraph.GetResourceManager()->GetResource( buf->GetMemoryID() );
		CHECK_ERR( mem );

		VMemoryObj::MemoryInfo	info;
		CHECK_ERR( mem->GetInfo( OUT info ));
		CHECK_ERR( info.mappedPtr );

		CHECK_ERR( dbg.module->ParseDebugOutput( dbg.mode, ArrayView<uint8_t>{ Cast<uint8_t>(info.mappedPtr + dbg.offset), size_t(dbg.size) }, OUT _tempStrings ));
		
		cb( dbg.taskName, dbg.shaderType, _tempStrings );
		return true;
	}

/*
=================================================
	SetShaderModule
=================================================
*/
	bool VShaderDebugger::SetShaderModule (uint id, const SharedShaderPtr &module)
	{
		CHECK_ERR( id < _debugModes.size() );
		
		auto&	dbg = _debugModes[id];

		CHECK_ERR( dbg.module == null );
		dbg.module = module;

		return true;
	}
	
/*
=================================================
	GetDebugModeInfo
=================================================
*/
	bool VShaderDebugger::GetDebugModeInfo (uint id, OUT EShaderDebugMode &mode, OUT EShader &shader) const
	{
		CHECK_ERR( id < _debugModes.size() );

		auto&	dbg = _debugModes[id];

		mode	= dbg.mode;
		shader	= dbg.shaderType;
		return true;
	}
	
/*
=================================================
	GetDescriptotSet
=================================================
*/
	bool VShaderDebugger::GetDescriptotSet (uint id, OUT VkDescriptorSet &descSet, OUT uint &dynamicOffset) const
	{
		CHECK_ERR( id < _debugModes.size() );
		
		auto&	dbg = _debugModes[id];

		descSet			= dbg.descriptorSet;
		dynamicOffset	= uint(dbg.offset);
		return true;
	}
	
/*
=================================================
	_AllocDescriptorSet
=================================================
*/
	bool VShaderDebugger::_AllocDescriptorSet (EShaderDebugMode debugMode, EShader shader, RawBufferID storageBuffer, BytesU size, OUT VkDescriptorSet &descSet)
	{
		auto&	dev			= _frameGraph.GetDevice();
		auto	layout_id	= GetDescriptorSetLayout( debugMode, shader );
		auto*	layout		= _frameGraph.GetResourceManager()->GetResource( layout_id );
		auto*	buffer		= _frameGraph.GetResourceManager()->GetResource( storageBuffer );
		CHECK_ERR( layout and buffer );

		// create descriptor pool
		if ( not _descPool )
		{
			const VkDescriptorPoolSize	pool_sizes[] = {
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100}
			};

			VkDescriptorPoolCreateInfo	info = {};
			info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			info.maxSets		= 100;
			info.poolSizeCount	= uint(CountOf( pool_sizes ));
			info.pPoolSizes		= pool_sizes;

			VK_CHECK( dev.vkCreateDescriptorPool( dev.GetVkDevice(), &info, null, OUT &_descPool ));
		}

		// find descriptor set in cache
		auto	iter = _descCache.find({ storageBuffer, layout_id });

		if ( iter != _descCache.end() )
		{
			descSet = iter->second;
			return true;
		}

		// allocate descriptor set
		{
			VkDescriptorSetAllocateInfo	info		= {};
			VkDescriptorSetLayout		vk_layouts[]= { layout->Handle() };

			info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			info.descriptorSetCount	= uint(CountOf( vk_layouts ));
			info.pSetLayouts		= vk_layouts;
			info.descriptorPool		= _descPool;

			VK_CHECK( dev.vkAllocateDescriptorSets( dev.GetVkDevice(), &info, OUT &descSet ));
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

		_descCache.insert_or_assign( {storageBuffer, layout_id}, descSet );
		return true;
	}

/*
=================================================
	_AllocStorage
=================================================
*/
	bool VShaderDebugger::_AllocStorage (INOUT DebugMode &dbgMode, const BytesU size)
	{
		const VkPipelineStageFlags	stage	= VEnumCast( EShaderStages(1 << uint(dbgMode.shaderType) ));

		dbgMode.size = Min( size, _bufferSize );

		// find place in existing storage buffers
		for (auto& sb : _storageBuffers)
		{
			dbgMode.offset = AlignToLarger( sb.size, _bufferAlign );

			if ( dbgMode.size <= (sb.capacity - dbgMode.offset) )
			{
				dbgMode.sbIndex	= uint(Distance( _storageBuffers.data(), &sb ));
				sb.size			= dbgMode.offset + size;
				sb.stages		|= stage;
				break;
			}
		}

		// create new storage buffer
		if ( dbgMode.sbIndex == UMax )
		{
			StorageBuffer	sb;
			sb.shaderTraceBuffer	= _frameGraph.CreateBuffer( BufferDesc{ _bufferSize, EBufferUsage::Storage | EBufferUsage::Transfer }, Default, "DebugOutputStorage" );
			sb.readBackBuffer		= _frameGraph.CreateBuffer( BufferDesc{ _bufferSize, EBufferUsage::TransferDst }, MemoryDesc{EMemoryType::HostRead}, "ReadBackDebugOutput" );
			sb.capacity				= _bufferSize;
			CHECK_ERR( sb.shaderTraceBuffer and sb.readBackBuffer );
			
			dbgMode.sbIndex	= uint(_storageBuffers.size());
			dbgMode.offset	= 0_b;
			sb.size			= dbgMode.offset + size;
			sb.stages		|= stage;
			
			_storageBuffers.push_back( std::move(sb) );
		}

		CHECK_ERR( _AllocDescriptorSet( dbgMode.mode, dbgMode.shaderType, _storageBuffers[dbgMode.sbIndex].shaderTraceBuffer.Get(), dbgMode.size, OUT dbgMode.descriptorSet ));
		return true;
	}

/*
=================================================
	Append
=================================================
*/
	uint VShaderDebugger::Append (INOUT ArrayView<RectI> &, const TaskName_t &name, const _fg_hidden_::GraphicsShaderDebugMode &mode, BytesU size)
	{
		DebugMode	dbg_mode;
		dbg_mode.taskName	= name;
		dbg_mode.mode		= mode.mode;
		dbg_mode.shaderType	= mode.shader;
		
		CHECK_ERR( _AllocStorage( INOUT dbg_mode, size ), UMax );

		switch ( mode.shader ) {
			case EShader::Vertex :			MemCopy( OUT dbg_mode.data, mode.vert );		break;
			case EShader::TessControl :		MemCopy( OUT dbg_mode.data, mode.tessCont );	break;
			case EShader::TessEvaluation :	MemCopy( OUT dbg_mode.data, mode.tessEval );	break;
			case EShader::Geometry :		MemCopy( OUT dbg_mode.data, mode.geom );		break;
			case EShader::Fragment :		MemCopy( OUT dbg_mode.data, mode.frag );		break;
			case EShader::MeshTask :		MemCopy( OUT dbg_mode.data, mode.task );		break;
			case EShader::Mesh :			MemCopy( OUT dbg_mode.data, mode.mesh );		break;
		}
		
		_debugModes.push_back( std::move(dbg_mode) );
		return uint(_debugModes.size() - 1);
	}
	
/*
=================================================
	Append
=================================================
*/
	uint VShaderDebugger::Append (const TaskName_t &name, const _fg_hidden_::ComputeShaderDebugMode &mode, BytesU size)
	{
		DebugMode	dbg_mode;
		dbg_mode.taskName	= name;
		dbg_mode.mode		= mode.mode;
		dbg_mode.shaderType	= EShader::Compute;

		CHECK_ERR( _AllocStorage( INOUT dbg_mode, size ), UMax );

		MemCopy( OUT dbg_mode.data, mode.globalID );
		
		_debugModes.push_back( std::move(dbg_mode) );
		return uint(_debugModes.size() - 1);
	}
	
/*
=================================================
	Append
=================================================
*
	uint VShaderDebugger::Append (INOUT VPipelineResourceSet &resources, const TaskName_t &name,
								  const _fg_hidden_::RayTracingShaderDebugMode &mode, BytesU size)
	{
		return UMax;
	}
	
/*
=================================================
	GetDescriptorSetLayout
=================================================
*/
	RawDescriptorSetLayoutID  VShaderDebugger::GetDescriptorSetLayout (EShaderDebugMode debugMode, EShader debuggableShader)
	{
		const uint	key  = (uint(debuggableShader) & 0xFF) | (uint(debugMode) << 8);
		auto		iter = _layoutCache.find( key );

		if ( iter != _layoutCache.end() )
			return iter->second.Get();

		PipelineDescription::UniformMap_t	uniforms;
		PipelineDescription::Uniform		sb_uniform;
		PipelineDescription::StorageBuffer	sb_desc;
		const EShaderStages					stage	= EShaderStages(1 << uint(debuggableShader) );

		sb_desc.state				= EResourceState_FromShaders( stage ) | EResourceState::ShaderReadWrite | EResourceState::_BufferDynamicOffset;
		sb_desc.arrayStride			= SizeOf<uint>;
		sb_desc.staticSize			= _GetBufferStaticSize( debuggableShader ) + SizeOf<uint>;	// per shader data + position
		sb_desc.dynamicOffsetIndex	= 0;

		sb_uniform.index			= BindingIndex{ UMax, 0 };
		sb_uniform.stageFlags		= stage;
		sb_uniform.data				= sb_desc;

		uniforms.insert({ UniformID{"dbg_ShaderTrace"}, sb_uniform });

		RawDescriptorSetLayoutID	layout = _frameGraph.GetResourceManager()
				->CreateDescriptorSetLayout( MakeShared<const PipelineDescription::UniformMap_t>( std::move(uniforms) ), true );
		CHECK_ERR( layout );

		_layoutCache.insert({ key, DescriptorSetLayoutID{layout} });
		return layout;
	}
	
/*
=================================================
	_GetBufferStaticSize
=================================================
*/
	BytesU  VShaderDebugger::_GetBufferStaticSize (EShader type) const
	{
		ENABLE_ENUM_CHECKS();
		switch ( type )
		{
			case EShader::Vertex :
			case EShader::TessControl :
			case EShader::TessEvaluation :
			case EShader::Geometry :	break;
			case EShader::Fragment :	return SizeOf<uint> * 4;
			case EShader::Compute :		return SizeOf<uint> * 3;
			case EShader::MeshTask :
			case EShader::Mesh :
			case EShader::RayGen :
			case EShader::RayAnyHit :
			case EShader::RayClosestHit :
			case EShader::RayMiss :
			case EShader::RayIntersection :
			case EShader::RayCallable :
			case EShader::_Count :
			case EShader::Unknown :		break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unsupported shader type" );
	}


}	// FG
