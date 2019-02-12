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
		if ( FG_EnableShaderDebugging )
		{
			CHECK( fg.GetDevice().GetDeviceLimits().maxBoundDescriptorSets > _descSetBinding );
		}
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
			BytesU	size	= _GetBufferStaticSize( dbg.shaderStages ) + SizeOf<uint>;	// per shader data + position
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
		CHECK_ERR( dbg.modules.size() );

		auto	read_back_buf	= _storageBuffers[ dbg.sbIndex ].readBackBuffer.Get();

		VBuffer const*		buf	= _frameGraph.GetResourceManager()->GetResource( read_back_buf );
		CHECK_ERR( buf );

		VMemoryObj const*	mem = _frameGraph.GetResourceManager()->GetResource( buf->GetMemoryID() );
		CHECK_ERR( mem );

		VMemoryObj::MemoryInfo	info;
		CHECK_ERR( mem->GetInfo( OUT info ));
		CHECK_ERR( info.mappedPtr );

		for (auto& shader : dbg.modules)
		{
			CHECK_ERR( shader->ParseDebugOutput( dbg.mode, ArrayView<uint8_t>{ Cast<uint8_t>(info.mappedPtr + dbg.offset), size_t(dbg.size) }, OUT _tempStrings ));
		
			cb( dbg.taskName, shader->GetDebugName(), dbg.shaderStages, _tempStrings );
		}
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

		dbg.modules.emplace_back( module );
		return true;
	}
	
/*
=================================================
	GetDebugModeInfo
=================================================
*/
	bool VShaderDebugger::GetDebugModeInfo (uint id, OUT EShaderDebugMode &mode, OUT EShaderStages &stages) const
	{
		CHECK_ERR( id < _debugModes.size() );

		auto&	dbg = _debugModes[id];

		mode	= dbg.mode;
		stages	= dbg.shaderStages;
		return true;
	}
	
/*
=================================================
	GetDescriptotSet
=================================================
*/
	bool VShaderDebugger::GetDescriptotSet (uint id, OUT uint &binding, OUT VkDescriptorSet &descSet, OUT uint &dynamicOffset) const
	{
		CHECK_ERR( id < _debugModes.size() );
		
		auto&	dbg = _debugModes[id];

		binding			= _descSetBinding;
		descSet			= dbg.descriptorSet;
		dynamicOffset	= uint(dbg.offset);
		return true;
	}
	
/*
=================================================
	_AllocDescriptorSet
=================================================
*/
	bool VShaderDebugger::_AllocDescriptorSet (EShaderDebugMode debugMode, EShaderStages stages, RawBufferID storageBuffer, BytesU size, OUT VkDescriptorSet &descSet)
	{
		auto&	dev			= _frameGraph.GetDevice();
		auto	layout_id	= _GetDescriptorSetLayout( debugMode, stages );
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
			sb.capacity				= _bufferSize * (1 + _storageBuffers.size() / 2);
			sb.shaderTraceBuffer	= _frameGraph.CreateBuffer( BufferDesc{ sb.capacity, EBufferUsage::Storage | EBufferUsage::Transfer }, Default, "DebugOutputStorage" );
			sb.readBackBuffer		= _frameGraph.CreateBuffer( BufferDesc{ sb.capacity, EBufferUsage::TransferDst }, MemoryDesc{EMemoryType::HostRead}, "ReadBackDebugOutput" );
			CHECK_ERR( sb.shaderTraceBuffer and sb.readBackBuffer );
			
			dbgMode.sbIndex	= uint(_storageBuffers.size());
			dbgMode.offset	= 0_b;
			sb.size			= dbgMode.offset + size;
			sb.stages		|= stage;
			
			_storageBuffers.push_back( std::move(sb) );
		}

		CHECK_ERR( _AllocDescriptorSet( dbgMode.mode, dbgMode.shaderStages, _storageBuffers[dbgMode.sbIndex].shaderTraceBuffer.Get(), dbgMode.size, OUT dbgMode.descriptorSet ));
		return true;
	}

/*
=================================================
	Append
=================================================
*/
	uint VShaderDebugger::Append (INOUT ArrayView<RectI> &, const TaskName_t &name, const _fg_hidden_::GraphicsShaderDebugMode &mode, BytesU size)
	{
		CHECK_ERR( FG_EnableShaderDebugging, UMax );

		DebugMode	dbg_mode;
		dbg_mode.taskName		= name;
		dbg_mode.mode			= mode.mode;
		dbg_mode.shaderStages	= mode.stages;
		
		CHECK_ERR( _AllocStorage( INOUT dbg_mode, size ), UMax );

		dbg_mode.data[0] = mode.fragCoord.x;
		dbg_mode.data[1] = mode.fragCoord.y;
		dbg_mode.data[2] = 0;
		dbg_mode.data[3] = 0;
		
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
		CHECK_ERR( FG_EnableShaderDebugging, UMax );

		DebugMode	dbg_mode;
		dbg_mode.taskName		= name;
		dbg_mode.mode			= mode.mode;
		dbg_mode.shaderStages	= EShaderStages::Compute;

		CHECK_ERR( _AllocStorage( INOUT dbg_mode, size ), UMax );

		MemCopy( OUT dbg_mode.data, mode.globalID );
		dbg_mode.data[3] = 0;
		
		_debugModes.push_back( std::move(dbg_mode) );
		return uint(_debugModes.size() - 1);
	}
	
/*
=================================================
	Append
=================================================
*/
	uint VShaderDebugger::Append (const TaskName_t &name, const _fg_hidden_::RayTracingShaderDebugMode &mode, BytesU size)
	{
		CHECK_ERR( FG_EnableShaderDebugging, UMax );

		DebugMode	dbg_mode;
		dbg_mode.taskName		= name;
		dbg_mode.mode			= mode.mode;
		dbg_mode.shaderStages	= EShaderStages::AllRayTracing;
		
		CHECK_ERR( _AllocStorage( INOUT dbg_mode, size ), UMax );

		MemCopy( OUT dbg_mode.data, mode.launchID );
		dbg_mode.data[3] = 0;
		
		_debugModes.push_back( std::move(dbg_mode) );
		return uint(_debugModes.size() - 1);
	}
	
/*
=================================================
	_GetDescriptorSetLayout
=================================================
*/
	RawDescriptorSetLayoutID  VShaderDebugger::_GetDescriptorSetLayout (EShaderDebugMode debugMode, EShaderStages debuggableShaders)
	{
		const uint	key  = (uint(debuggableShaders) & 0xFFFFFF) | (uint(debugMode) << 24);
		auto		iter = _layoutCache.find( key );

		if ( iter != _layoutCache.end() )
			return iter->second.Get();

		PipelineDescription::UniformMap_t	uniforms;
		PipelineDescription::Uniform		sb_uniform;
		PipelineDescription::StorageBuffer	sb_desc;

		sb_desc.state				= EResourceState_FromShaders( debuggableShaders ) | EResourceState::ShaderReadWrite | EResourceState::_BufferDynamicOffset;
		sb_desc.arrayStride			= SizeOf<uint>;
		sb_desc.staticSize			= _GetBufferStaticSize( debuggableShaders ) + SizeOf<uint>;	// per shader data + position
		sb_desc.dynamicOffsetIndex	= 0;

		sb_uniform.index			= BindingIndex{ UMax, 0 };
		sb_uniform.stageFlags		= debuggableShaders;
		sb_uniform.data				= sb_desc;
		sb_uniform.arraySize		= 1;

		uniforms.insert({ UniformID{"dbg_ShaderTrace"}, sb_uniform });

		RawDescriptorSetLayoutID	layout = _frameGraph.GetResourceManager()
				->CreateDescriptorSetLayout( MakeShared<const PipelineDescription::UniformMap_t>( std::move(uniforms) ), true );
		CHECK_ERR( layout );

		_layoutCache.insert({ key, DescriptorSetLayoutID{layout} });
		return layout;
	}
	
/*
=================================================
	GetDescriptorSetLayout
=================================================
*/
	bool VShaderDebugger::GetDescriptorSetLayout (EShaderDebugMode debugMode, EShaderStages debuggableShaders, OUT uint &binding, OUT RawDescriptorSetLayoutID &layout)
	{
		binding = _descSetBinding;
		layout  = _GetDescriptorSetLayout( debugMode, debuggableShaders );
		return layout.IsValid();
	}

/*
=================================================
	_GetBufferStaticSize
=================================================
*/
	BytesU  VShaderDebugger::_GetBufferStaticSize (EShaderStages stages) const
	{
		if ( EnumEq( EShaderStages::AllGraphics, stages ) )
			return SizeOf<uint> * 3;	// fragcoord
		
		if ( stages == EShaderStages::Compute )
			return SizeOf<uint> * 3;	// global invocation
		
		if ( EnumEq( EShaderStages::AllRayTracing, stages ) )
			return SizeOf<uint> * 3;	// launch

		RETURN_ERR( "unsupported shader type" );
	}


}	// FG
