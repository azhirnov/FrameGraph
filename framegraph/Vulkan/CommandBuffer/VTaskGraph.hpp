// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VTaskGraph.h"
#include "VEnumCast.h"

namespace FG
{

/*
=================================================
	Add
=================================================
*/
	template <typename VisitorT>
	template <typename T>
	inline VFgTask<T>*  VTaskGraph<VisitorT>::Add (VCommandBuffer &cb, const T &task)
	{
		auto*	ptr  = cb.GetAllocator().Alloc< VFgTask<T> >();

		PlacementNew< VFgTask<T> >( OUT ptr, cb, task, &_Visitor<T> );
		CHECK_ERR( ptr->IsValid() );

		_nodes->insert( ptr );

		if ( ptr->Inputs().empty() )
			_entries->push_back( ptr );

		for (auto in_node : ptr->Inputs())
		{
			ASSERT( !!_nodes->count( in_node ) );

			in_node->Attach( ptr );
		}
		return ptr;
	}
//-----------------------------------------------------------------------------

	
	
/*
=================================================
	VFgTask< SubmitRenderPass >
=================================================
*/
	inline VFgTask<SubmitRenderPass>::VFgTask (VCommandBuffer &cb, const SubmitRenderPass &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		_logicalPass{ cb.ToLocal( task.renderPassId )}
	{
		if ( _logicalPass )
			CHECK( _logicalPass->Submit( cb, task.images, task.buffers ));
	}
	
/*
=================================================
	VFgTask< SubmitRenderPass >::IsValid
=================================================
*/
	inline bool  VFgTask<SubmitRenderPass>::IsValid () const
	{
		return _logicalPass;
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	CopyDescriptorSets
=================================================
*/
	inline void CopyDescriptorSets (VLogicalRenderPass *rp, VCommandBuffer &cb, const PipelineResourceSet &inResourceSet, OUT VPipelineResourceSet &outResourceSet)
	{
		size_t	offset_count = 0;

		for (auto& src : inResourceSet)
		{
			auto	offsets = src.second->GetDynamicOffsets();

			outResourceSet.resources.emplace_back( src.first, cb.CreateDescriptorSet( *src.second ),
												   uint(offset_count), uint(offsets.size()) );
			
			for (size_t i = 0; i < offsets.size(); ++i, ++offset_count) {
				outResourceSet.dynamicOffsets.push_back( offsets[i] );
			}
		}

		if ( not rp )
			return;

		const uint	base_offset	= uint(outResourceSet.dynamicOffsets.size());

		for (auto& src : rp->GetResources().resources)
		{
			ASSERT( inResourceSet.count( src.descSetId ) == 0 );
			
			outResourceSet.resources.emplace_back( src.descSetId, src.pplnRes, src.offsetIndex + base_offset, src.offsetCount );
		}

		for (auto& src : rp->GetResources().dynamicOffsets)
		{
			outResourceSet.dynamicOffsets.push_back( src );
		}
	}
	
/*
=================================================
	RemapVertexBuffers
=================================================
*/
	inline void RemapVertexBuffers (VCommandBuffer &cb, const DrawVertices::Buffers_t &inBuffers, const VertexInputState &vertexInput,
									OUT VFgDrawTask<DrawVertices>::VertexBuffers_t &vertexBuffers,
									OUT VFgDrawTask<DrawVertices>::VertexOffsets_t &vertexOffsets,
									OUT VFgDrawTask<DrawVertices>::VertexStrides_t &vertexStrides)
	{
		for (auto& vb : inBuffers)
		{
			auto				iter	= vertexInput.BufferBindings().find( vb.first );
			VLocalBuffer const*	buffer	= cb.ToLocal( vb.second.buffer );
			
			CHECK_ERR( iter != vertexInput.BufferBindings().end(), void());
			
			vertexBuffers[ iter->second.index ]	= buffer;
			vertexOffsets[ iter->second.index ]	= VkDeviceSize( vb.second.offset );
			vertexStrides[ iter->second.index ] = iter->second.stride;
		}
	}

/*
=================================================
	CopyScissors
=================================================
*/
	inline void CopyScissors (VCommandBuffer &cb, const _fg_hidden_::Scissors_t &inScissors, OUT ArrayView<RectI> &outScissors)
	{
		if ( inScissors.empty() )
			return;
		
		auto*	ptr = cb.GetAllocator().Alloc< RectI >( inScissors.size() );
		std::memcpy( OUT ptr, inScissors.data(), size_t(ArraySizeOf(inScissors)) );

		outScissors = { ptr, inScissors.size() };
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	VBaseDrawVerticesTask
=================================================
*/
	template <typename TaskType>
	VBaseDrawVerticesTask::VBaseDrawVerticesTask (VLogicalRenderPass &rp, VCommandBuffer &cb, const TaskType &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		IDrawTask{ task, pass1, pass2 },				_vbCount{ uint(task.vertexBuffers.size()) },
		pipeline{ cb.AcquireTemporary( task.pipeline )},
		pushConstants{ task.pushConstants },			vertexInput{ task.vertexInput },
		colorBuffers{ task.colorBuffers },				dynamicStates{ task.dynamicStates },
		topology{ task.topology },						primitiveRestart{ task.primitiveRestart }
	{
		CopyScissors( cb, task.scissors, OUT _scissors );
		CopyDescriptorSets( &rp, cb, task.resources, OUT _resources );
		RemapVertexBuffers( cb, task.vertexBuffers, task.vertexInput, OUT _vertexBuffers, OUT _vbOffsets, OUT _vbStrides );

		if ( task.debugMode.mode != Default )
			_debugModeIndex = cb.GetBatch().AppendShader( INOUT _scissors, task.taskName, task.debugMode );
	}

/*
=================================================
	VFgDrawTask< DrawVertices >
=================================================
*/
	inline VFgDrawTask<DrawVertices>::VFgDrawTask (VLogicalRenderPass &rp, VCommandBuffer &cb, const DrawVertices &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawVerticesTask{ rp, cb, task, pass1, pass2 },	commands{ task.commands }
	{}

/*
=================================================
	VFgDrawTask< DrawIndexed >
=================================================
*/
	inline VFgDrawTask<DrawIndexed>::VFgDrawTask (VLogicalRenderPass &rp, VCommandBuffer &cb, const DrawIndexed &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawVerticesTask{ rp, cb, task, pass1, pass2 },	commands{ task.commands },
		indexBuffer{ cb.ToLocal( task.indexBuffer )},
		indexBufferOffset{ task.indexBufferOffset },		indexType{ task.indexType }
	{}
	
/*
=================================================
	VFgDrawTask< DrawVerticesIndirect >
=================================================
*/
	inline VFgDrawTask<DrawVerticesIndirect>::VFgDrawTask (VLogicalRenderPass &rp, VCommandBuffer &cb, const DrawVerticesIndirect &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawVerticesTask{ rp, cb, task, pass1, pass2 },	commands{ task.commands },
		indirectBuffer{ cb.ToLocal( task.indirectBuffer )}
	{}
	
/*
=================================================
	VFgDrawTask< DrawIndexedIndirect >
=================================================
*/
	inline VFgDrawTask<DrawIndexedIndirect>::VFgDrawTask (VLogicalRenderPass &rp, VCommandBuffer &cb, const DrawIndexedIndirect &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawVerticesTask{ rp, cb, task, pass1, pass2 },	commands{ task.commands },
		indirectBuffer{ cb.ToLocal( task.indirectBuffer )},		indexBuffer{ cb.ToLocal( task.indexBuffer )},
		indexBufferOffset{ task.indexBufferOffset },			indexType{ task.indexType }
	{}
//-----------------------------------------------------------------------------

	
/*
=================================================
	VBaseDrawMeshes
=================================================
*/	
	template <typename TaskType>
	inline VBaseDrawMeshes::VBaseDrawMeshes (VLogicalRenderPass &rp, VCommandBuffer &cb, const TaskType &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		IDrawTask{ task, pass1, pass2 },		pipeline{ cb.AcquireTemporary( task.pipeline )},
		pushConstants{ task.pushConstants },	colorBuffers{ task.colorBuffers },
		dynamicStates{ task.dynamicStates }
	{
		CopyScissors( cb, task.scissors, OUT _scissors );
		CopyDescriptorSets( &rp, cb, task.resources, OUT _resources );
		
		if ( task.debugMode.mode != Default )
			_debugModeIndex = cb.GetBatch().AppendShader( INOUT _scissors, task.taskName, task.debugMode );
	}

/*
=================================================
	VFgDrawTask< DrawMeshes >
=================================================
*/
	inline VFgDrawTask<DrawMeshes>::VFgDrawTask (VLogicalRenderPass &rp, VCommandBuffer &cb, const DrawMeshes &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawMeshes{ rp, cb, task, pass1, pass2 },	commands{ task.commands }
	{}

/*
=================================================
	VFgDrawTask< DrawMeshesIndirect >
=================================================
*/
	inline VFgDrawTask<DrawMeshesIndirect>::VFgDrawTask (VLogicalRenderPass &rp, VCommandBuffer &cb, const DrawMeshesIndirect &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawMeshes{ rp, cb, task, pass1, pass2 },	commands{ task.commands },
		indirectBuffer{ cb.ToLocal( task.indirectBuffer )}
	{}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgDrawTask< CustomDraw >
=================================================
*/
	inline VFgDrawTask<CustomDraw>::VFgDrawTask (VLogicalRenderPass &, VCommandBuffer &cb, const CustomDraw &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		IDrawTask{ task, pass1, pass2 },	callback{ task.callback }
	{
		if ( task.images.size() )
		{
			auto*	img_ptr	= cb.GetAllocator().Alloc< Images_t::value_type >( task.images.size() );
			
			for (size_t i = 0; i < task.images.size(); ++i) {
				img_ptr[i] = { cb.ToLocal( task.images[i].first ), task.images[i].second };
			}
			_images = { img_ptr, task.images.size() };
		}

		if ( task.buffers.size() )
		{
			auto*	buf_ptr	= cb.GetAllocator().Alloc< Buffers_t::value_type >( task.buffers.size() );
			
			for (size_t i = 0; i < task.buffers.size(); ++i) {
				buf_ptr[i] = { cb.ToLocal( task.buffers[i].first ), task.buffers[i].second };
			}
			_buffers = { buf_ptr, task.buffers.size() };
		}
	}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< DispatchCompute >
=================================================
*/
	inline VFgTask<DispatchCompute>::VFgTask (VCommandBuffer &cb, const DispatchCompute &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },		pipeline{ cb.AcquireTemporary( task.pipeline )},
		pushConstants{ task.pushConstants },	commands{ task.commands },
		localGroupSize{ task.localGroupSize }
	{
		CopyDescriptorSets( null, cb, task.resources, OUT _resources );
		
		if ( task.debugMode.mode != Default )
			_debugModeIndex = cb.GetBatch().AppendShader( task.taskName, task.debugMode );
	}
	
/*
=================================================
	VFgTask< DispatchCompute >::IsValid
=================================================
*/
	inline bool  VFgTask<DispatchCompute>::IsValid () const
	{
		return pipeline;
	}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< DispatchComputeIndirect >
=================================================
*/
	inline VFgTask<DispatchComputeIndirect>::VFgTask (VCommandBuffer &cb, const DispatchComputeIndirect &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },					pipeline{ cb.AcquireTemporary( task.pipeline )},
		pushConstants{ task.pushConstants },				commands{ task.commands },
		indirectBuffer{ cb.ToLocal( task.indirectBuffer )},	localGroupSize{ task.localGroupSize }
	{
		CopyDescriptorSets( null, cb, task.resources, OUT _resources );
		
		if ( task.debugMode.mode != Default )
			_debugModeIndex = cb.GetBatch().AppendShader( task.taskName, task.debugMode );
	}
	
/*
=================================================
	VFgTask< DispatchComputeIndirect >::IsValid
=================================================
*/
	inline bool  VFgTask<DispatchComputeIndirect>::IsValid () const
	{
		return pipeline and indirectBuffer;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< CopyBuffer >
=================================================
*/
	inline VFgTask<CopyBuffer>::VFgTask (VCommandBuffer &cb, const CopyBuffer &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		srcBuffer{ cb.ToLocal( task.srcBuffer )},	dstBuffer{ cb.ToLocal( task.dstBuffer )},
		regions{ task.regions }
	{
	}
	
/*
=================================================
	VFgTask< CopyBuffer >::IsValid
=================================================
*/
	inline bool  VFgTask<CopyBuffer>::IsValid () const
	{
		return srcBuffer and dstBuffer and regions.size();
	}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< CopyImage >
=================================================
*/
	inline VFgTask<CopyImage>::VFgTask (VCommandBuffer &cb, const CopyImage &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		srcImage{ cb.ToLocal( task.srcImage )},		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstImage{ cb.ToLocal( task.dstImage )},		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		regions{ task.regions }
	{
		//ASSERT( not _dstImage->IsImmutable() );
	}
	
/*
=================================================
	VFgTask< CopyImage >::IsValid
=================================================
*/
	inline bool  VFgTask<CopyImage>::IsValid () const
	{
		return srcImage and dstImage and regions.size();
	}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< CopyBufferToImage >
=================================================
*/
	inline VFgTask<CopyBufferToImage>::VFgTask (VCommandBuffer &cb, const CopyBufferToImage &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },			srcBuffer{ cb.ToLocal( task.srcBuffer )},
		dstImage{ cb.ToLocal( task.dstImage )},		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		regions{ task.regions }
	{
		//ASSERT( not _dstImage->IsImmutable() );
	}
	
/*
=================================================
	VFgTask< CopyBufferToImage >::IsValid
=================================================
*/
	inline bool  VFgTask<CopyBufferToImage>::IsValid () const
	{
		return srcBuffer and dstImage and regions.size();
	}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< CopyImageToBuffer >
=================================================
*/
	inline VFgTask<CopyImageToBuffer>::VFgTask (VCommandBuffer &cb, const CopyImageToBuffer &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		srcImage{ cb.ToLocal( task.srcImage )},		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstBuffer{ cb.ToLocal( task.dstBuffer )},	regions{ task.regions }
	{
	}
	
/*
=================================================
	VFgTask< CopyImageToBuffer >::IsValid
=================================================
*/
	inline bool  VFgTask<CopyImageToBuffer>::IsValid () const
	{
		return srcImage and dstBuffer and regions.size();
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< BlitImage >
=================================================
*/
	inline VFgTask<BlitImage>::VFgTask (VCommandBuffer &cb, const BlitImage &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		srcImage{ cb.ToLocal( task.srcImage )},		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstImage{ cb.ToLocal( task.dstImage )},		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		filter{ VEnumCast( task.filter ) },			regions{ task.regions }
	{
		//ASSERT( not _dstImage->IsImmutable() );

		//if ( EPixelFormat_HasDepthOrStencil( _srcImage->PixelFormat() ) )
		//{
	//		ASSERT( _filter != VK_FILTER_NEAREST );
	//		_filter = VK_FILTER_NEAREST;
	//	}
	}
	
/*
=================================================
	VFgTask< BlitImage >::IsValid
=================================================
*/
	inline bool  VFgTask<BlitImage>::IsValid () const
	{
		return srcImage and dstImage and regions.size();
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	VFgTask< GenerateMipmaps >
=================================================
*/
	inline VFgTask<GenerateMipmaps>::VFgTask (VCommandBuffer &cb, const GenerateMipmaps &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },	image{ cb.ToLocal( task.image )},
		baseLevel{ task.baseLevel.Get() },	levelCount{ task.levelCount }
	{}
		
/*
=================================================
	VFgTask< GenerateMipmaps >::IsValid
=================================================
*/
	bool  VFgTask<GenerateMipmaps>::IsValid () const
	{
		return image and levelCount;
	}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< ResolveImage >
=================================================
*/
	inline VFgTask<ResolveImage>::VFgTask (VCommandBuffer &cb, const ResolveImage &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		srcImage{ cb.ToLocal( task.srcImage )},		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstImage{ cb.ToLocal( task.dstImage )},		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		regions{ task.regions }
	{
		//ASSERT( not _dstImage->IsImmutable() );
	}
	
/*
=================================================
	VFgTask< ResolveImage >::IsValid
=================================================
*/
	inline bool  VFgTask<ResolveImage>::IsValid () const
	{
		return srcImage and dstImage and regions.size();
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< FillBuffer >
=================================================
*/
	inline VFgTask<FillBuffer>::VFgTask (VCommandBuffer &cb, const FillBuffer &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		dstBuffer{ cb.ToLocal( task.dstBuffer )},	dstOffset{ VkDeviceSize(task.dstOffset) },
		size{ VkDeviceSize(task.size) },			pattern{ task.pattern }
	{}
	
/*
=================================================
	VFgTask< FillBuffer >::IsValid
=================================================
*/
	inline bool  VFgTask<FillBuffer>::IsValid () const
	{
		return dstBuffer and size > 0;
	}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< ClearColorImage >
=================================================
*/
	inline VFgTask<ClearColorImage>::VFgTask (VCommandBuffer &cb, const ClearColorImage &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		dstImage{ cb.ToLocal( task.dstImage )},		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		ranges{ task.ranges },						_clearValue{}
	{
		//ASSERT( not _dstImage->IsImmutable() );

		Visit(	task.clearValue,
				[&] (const RGBA32f &col)	{ std::memcpy( _clearValue.float32, &col, sizeof(_clearValue.float32) ); },
				[&] (const RGBA32u &col)	{ std::memcpy( _clearValue.uint32, &col, sizeof(_clearValue.uint32) ); },
				[&] (const RGBA32i &col)	{ std::memcpy( _clearValue.int32, &col, sizeof(_clearValue.int32) );} ,
				[&] (const NullUnion &)		{}
			);
	}
	
/*
=================================================
	VFgTask< ClearColorImage >::IsValid
=================================================
*/
	inline bool  VFgTask<ClearColorImage>::IsValid () const
	{
		return dstImage and ranges.size();
	}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< ClearDepthStencilImage >
=================================================
*/
	inline VFgTask<ClearDepthStencilImage>::VFgTask (VCommandBuffer &cb, const ClearDepthStencilImage &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		dstImage{ cb.ToLocal( task.dstImage )},							dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		clearValue{ task.clearValue.depth, task.clearValue.stencil },	ranges{ task.ranges }
	{
		//ASSERT( not _dstImage->IsImmutable() );
	}
	
/*
=================================================
	VFgTask< ClearDepthStencilImage >::IsValid
=================================================
*/
	inline bool  VFgTask<ClearDepthStencilImage>::IsValid () const
	{
		return dstImage and ranges.size();
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< UpdateBuffer >
=================================================
*/
	inline VFgTask<UpdateBuffer>::VFgTask (VCommandBuffer &cb, const UpdateBuffer &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		dstBuffer{ cb.ToLocal( task.dstBuffer )}
	{
		size_t	cnt = task.regions.size();
		Region*	dst = cb.GetAllocator().Alloc<Region>( cnt );

		for (size_t i = 0; i < cnt; ++i)
		{
			auto&	src = task.regions[i];

			dst[i].dataPtr		= cb.GetAllocator().Alloc( ArraySizeOf(src.data), AlignOf<uint8_t> );
			dst[i].dataSize		= VkDeviceSize(ArraySizeOf(src.data));
			dst[i].bufferOffset	= VkDeviceSize(src.offset);

			std::memcpy( dst[i].dataPtr, src.data.data(), size_t(dst[i].dataSize) );
		}

		_regions = ArrayView{ dst, cnt };
	}

/*
=================================================
	VFgTask< UpdateBuffer >::IsValid
=================================================
*/
	inline bool  VFgTask<UpdateBuffer>::IsValid () const
	{
		return dstBuffer and _regions.size();
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< Present >
=================================================
*/
	inline VFgTask<Present>::VFgTask (VCommandBuffer &cb, const Present &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },
		swapchain{ cb.AcquireTemporary( task.swapchain )},	srcImage{ cb.ToLocal( task.srcImage )},
		layer{ task.layer },								mipmap{ task.mipmap }
	{}
	
/*
=================================================
	VFgTask< Present >::IsValid
=================================================
*/
	inline bool  VFgTask<Present>::IsValid () const
	{
		return swapchain and srcImage;
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	VFgTask< UpdateRayTracingShaderTable >
=================================================
*/
	inline VFgTask<UpdateRayTracingShaderTable>::VFgTask (VCommandBuffer &cb, const UpdateRayTracingShaderTable &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },		pipeline{ task.pipeline },
		rtScene{ cb.ToLocal( task.rtScene )},
		shaderTable{const_cast<VRayTracingShaderTable*>( cb.AcquireTemporary( task.shaderTable ))},
		rayGenShader{ task.rayGenShader },		maxRecursionDepth{ task.maxRecursionDepth },
		_shaderGroupCount{ uint(task.shaderGroups.size()) }
	{
		_shaderGroups = cb.GetAllocator().Alloc<ShaderGroup>( _shaderGroupCount );
		std::memcpy( OUT _shaderGroups, task.shaderGroups.data(), size_t(ArraySizeOf(task.shaderGroups)) );
	}
	
/*
=================================================
	VFgTask< UpdateRayTracingShaderTable >::IsValid
=================================================
*/
	inline bool  VFgTask<UpdateRayTracingShaderTable>::IsValid () const
	{
		return pipeline and rtScene and shaderTable;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< BuildRayTracingGeometry >
=================================================
*/
	inline VFgTask<BuildRayTracingGeometry>::VFgTask (VCommandBuffer &cb, const BuildRayTracingGeometry &task, ProcessFunc_t process) :
		VFrameGraphTask{task, process},
		_usableBuffers{ cb.GetAllocator() }
	{}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< TraceRays >
=================================================
*/
	inline VFgTask<TraceRays>::VFgTask (VCommandBuffer &cb, const TraceRays &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },		shaderTable{ cb.AcquireTemporary( task.shaderTable )},
		pushConstants{ task.pushConstants },	groupCount{ Max( task.groupCount, 1u )}
	{
		CopyDescriptorSets( null, cb, task.resources, OUT _resources );

		if ( task.debugMode.mode != Default )
			_debugModeIndex = cb.GetBatch().AppendShader( task.taskName, task.debugMode );
	}
	
/*
=================================================
	VFgTask< TraceRays >::IsValid
=================================================
*/
	inline bool  VFgTask<TraceRays>::IsValid () const
	{
		return shaderTable;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	VFgTask< CustomTask >
=================================================
*/
	VFgTask<CustomTask>::VFgTask (VCommandBuffer &cb, const CustomTask &task, ProcessFunc_t process) :
		VFrameGraphTask{ task, process },	callback{ task.callback }
	{
		if ( task.images.size() )
		{
			auto*	img_ptr	= cb.GetAllocator().Alloc< Images_t::value_type >( task.images.size() );
			
			for (size_t i = 0; i < task.images.size(); ++i) {
				img_ptr[i] = { cb.ToLocal( task.images[i].first ), task.images[i].second };
			}
			_images = { img_ptr, task.images.size() };
		}

		if ( task.buffers.size() )
		{
			auto*	buf_ptr	= cb.GetAllocator().Alloc< Buffers_t::value_type >( task.buffers.size() );
			
			for (size_t i = 0; i < task.buffers.size(); ++i) {
				buf_ptr[i] = { cb.ToLocal( task.buffers[i].first ), task.buffers[i].second };
			}
			_buffers = { buf_ptr, task.buffers.size() };
		}
	}

/*
=================================================
	VFgTask< CustomTask >::IsValid
=================================================
*/
	inline bool  VFgTask<CustomTask>::IsValid () const
	{
		return true;
	}
//-----------------------------------------------------------------------------

}	// FG
