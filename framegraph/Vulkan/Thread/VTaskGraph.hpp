// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VTaskGraph.h"
#include "VFrameGraphThread.h"
#include "VShaderDebugger.h"
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
	inline VFgTask<T>*  VTaskGraph<VisitorT>::Add (VFrameGraphThread *fg, const T &task)
	{
		auto*	ptr  = fg->GetAllocator().Alloc< VFgTask<T> >();

		PlacementNew< VFgTask<T> >( OUT ptr, fg, task, &_Visitor<T> );
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
	inline VFgTask<SubmitRenderPass>::VFgTask (VFrameGraphThread *fg, const SubmitRenderPass &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_logicalPass{ fg->GetResourceManager()->ToLocal( task.renderPassId )}
	{
		if ( _logicalPass )
			CHECK( _logicalPass->Submit() );
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
	inline void CopyDescriptorSets (VFrameGraphThread *fg, const PipelineResourceSet &inResourceSet, OUT VPipelineResourceSet &resourceSet)
	{
		size_t	offset_count = 0;

		for (auto& res : inResourceSet)
		{
			auto	offsets = res.second->GetDynamicOffsets();

			resourceSet.resources.push_back({ res.first, fg->GetResourceManager()->CreateDescriptorSet( *res.second, true ),
											  uint16_t(offset_count), uint16_t(offsets.size()) });
			
			for (size_t i = 0; i < offsets.size(); ++i, ++offset_count) {
				resourceSet.dynamicOffsets.push_back( offsets[i] );
			}
		}
	}
	
/*
=================================================
	RemapVertexBuffers
=================================================
*/
	inline void RemapVertexBuffers (VFrameGraphThread *fg, const DrawVertices::Buffers_t &inBuffers, const VertexInputState &vertexInput,
									OUT VFgDrawTask<DrawVertices>::VertexBuffers_t &vertexBuffers,
									OUT VFgDrawTask<DrawVertices>::VertexOffsets_t &vertexOffsets,
									OUT VFgDrawTask<DrawVertices>::VertexStrides_t &vertexStrides)
	{
		for (auto& vb : inBuffers)
		{
			auto				iter	= vertexInput.BufferBindings().find( vb.first );
			VLocalBuffer const*	buffer	= fg->GetResourceManager()->ToLocal( vb.second.buffer );
			
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
	inline void CopyScissors (VFrameGraphThread *fg, const _fg_hidden_::Scissors_t &inScissors, OUT ArrayView<RectI> &outScissors)
	{
		if ( inScissors.empty() )
			return;
		
		auto*	ptr = fg->GetAllocator().Alloc< RectI >( inScissors.size() );
		memcpy( OUT ptr, inScissors.data(), size_t(ArraySizeOf(inScissors)) );

		outScissors = { ptr, inScissors.size() };
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	VBaseDrawVerticesTask
=================================================
*/
	template <typename TaskType>
	VBaseDrawVerticesTask::VBaseDrawVerticesTask (VFrameGraphThread *fg, const TaskType &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		IDrawTask{ task, pass1, pass2 },				_vbCount{ uint(task.vertexBuffers.size()) },
		pipeline{ fg->GetResourceManager()->GetResource( task.pipeline )},
		pushConstants{ task.pushConstants },			vertexInput{ task.vertexInput },
		colorBuffers{ task.colorBuffers },				dynamicStates{ task.dynamicStates },
		topology{ task.topology },						primitiveRestart{ task.primitiveRestart }
	{
		CopyScissors( fg, task.scissors, OUT _scissors );
		CopyDescriptorSets( fg, task.resources, OUT _resources );
		RemapVertexBuffers( fg, task.vertexBuffers, task.vertexInput, OUT _vertexBuffers, OUT _vbOffsets, OUT _vbStrides );

		if ( task.debugMode.mode != Default )
			_debugModeIndex = fg->GetShaderDebugger()->Append( INOUT _scissors, task.taskName, task.debugMode );
	}

/*
=================================================
	VFgDrawTask< DrawVertices >
=================================================
*/
	inline VFgDrawTask<DrawVertices>::VFgDrawTask (VFrameGraphThread *fg, const DrawVertices &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawVerticesTask{ fg, task, pass1, pass2 },	commands{ task.commands }
	{}

/*
=================================================
	VFgDrawTask< DrawIndexed >
=================================================
*/
	inline VFgDrawTask<DrawIndexed>::VFgDrawTask (VFrameGraphThread *fg, const DrawIndexed &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawVerticesTask{ fg, task, pass1, pass2 },	commands{ task.commands },
		indexBuffer{ fg->GetResourceManager()->ToLocal( task.indexBuffer )},
		indexBufferOffset{ task.indexBufferOffset },		indexType{ task.indexType }
	{}
	
/*
=================================================
	VFgDrawTask< DrawVerticesIndirect >
=================================================
*/
	inline VFgDrawTask<DrawVerticesIndirect>::VFgDrawTask (VFrameGraphThread *fg, const DrawVerticesIndirect &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawVerticesTask{ fg, task, pass1, pass2 },	commands{ task.commands },
		indirectBuffer{ fg->GetResourceManager()->ToLocal( task.indirectBuffer )}
	{}
	
/*
=================================================
	VFgDrawTask< DrawIndexedIndirect >
=================================================
*/
	inline VFgDrawTask<DrawIndexedIndirect>::VFgDrawTask (VFrameGraphThread *fg, const DrawIndexedIndirect &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawVerticesTask{ fg, task, pass1, pass2 },	commands{ task.commands },
		indirectBuffer{ fg->GetResourceManager()->ToLocal( task.indirectBuffer )},
		indexBuffer{ fg->GetResourceManager()->ToLocal( task.indexBuffer )},
		indexBufferOffset{ task.indexBufferOffset },		indexType{ task.indexType }
	{}
//-----------------------------------------------------------------------------

	
/*
=================================================
	VBaseDrawMeshes
=================================================
*/	
	template <typename TaskType>
	inline VBaseDrawMeshes::VBaseDrawMeshes (VFrameGraphThread *fg, const TaskType &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		IDrawTask{ task, pass1, pass2 },		pipeline{ fg->GetResourceManager()->GetResource( task.pipeline )},
		pushConstants{ task.pushConstants },	colorBuffers{ task.colorBuffers },
		dynamicStates{ task.dynamicStates }
	{
		CopyScissors( fg, task.scissors, OUT _scissors );
		CopyDescriptorSets( fg, task.resources, OUT _resources );
		
		if ( task.debugMode.mode != Default )
			_debugModeIndex = fg->GetShaderDebugger()->Append( INOUT _scissors, task.taskName, task.debugMode );
	}

/*
=================================================
	VFgDrawTask< DrawMeshes >
=================================================
*/
	inline VFgDrawTask<DrawMeshes>::VFgDrawTask (VFrameGraphThread *fg, const DrawMeshes &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawMeshes{ fg, task, pass1, pass2 },	commands{ task.commands }
	{}

/*
=================================================
	VFgDrawTask< DrawMeshesIndirect >
=================================================
*/
	inline VFgDrawTask<DrawMeshesIndirect>::VFgDrawTask (VFrameGraphThread *fg, const DrawMeshesIndirect &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		VBaseDrawMeshes{ fg, task, pass1, pass2 },	commands{ task.commands },
		indirectBuffer{ fg->GetResourceManager()->ToLocal( task.indirectBuffer )}
	{}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< DispatchCompute >
=================================================
*/
	inline VFgTask<DispatchCompute>::VFgTask (VFrameGraphThread *fg, const DispatchCompute &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },		pipeline{ fg->GetResourceManager()->GetResource( task.pipeline )},
		pushConstants{ task.pushConstants },	commands{ task.commands },
		localGroupSize{ task.localGroupSize }
	{
		CopyDescriptorSets( fg, task.resources, OUT _resources );
		
		if ( task.debugMode.mode != Default )
			_debugModeIndex = fg->GetShaderDebugger()->Append( task.taskName, task.debugMode );
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
	inline VFgTask<DispatchComputeIndirect>::VFgTask (VFrameGraphThread *fg, const DispatchComputeIndirect &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },		pipeline{ fg->GetResourceManager()->GetResource( task.pipeline )},
		pushConstants{ task.pushConstants },
		commands{ task.commands },
		indirectBuffer{ fg->GetResourceManager()->ToLocal( task.indirectBuffer )},
		localGroupSize{ task.localGroupSize }
	{
		CopyDescriptorSets( fg, task.resources, OUT _resources );
		
		if ( task.debugMode.mode != Default )
			_debugModeIndex = fg->GetShaderDebugger()->Append( task.taskName, task.debugMode );
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
	inline VFgTask<CopyBuffer>::VFgTask (VFrameGraphThread *fg, const CopyBuffer &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcBuffer{ fg->GetResourceManager()->ToLocal( task.srcBuffer )},
		dstBuffer{ fg->GetResourceManager()->ToLocal( task.dstBuffer )},
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
	inline VFgTask<CopyImage>::VFgTask (VFrameGraphThread *fg, const CopyImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcImage{ fg->GetResourceManager()->ToLocal( task.srcImage )},
		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstImage{ fg->GetResourceManager()->ToLocal( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
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
	inline VFgTask<CopyBufferToImage>::VFgTask (VFrameGraphThread *fg, const CopyBufferToImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcBuffer{ fg->GetResourceManager()->ToLocal( task.srcBuffer )},
		dstImage{ fg->GetResourceManager()->ToLocal( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
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
	inline VFgTask<CopyImageToBuffer>::VFgTask (VFrameGraphThread *fg, const CopyImageToBuffer &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcImage{ fg->GetResourceManager()->ToLocal( task.srcImage )},
		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstBuffer{ fg->GetResourceManager()->ToLocal( task.dstBuffer )},
		regions{ task.regions }
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
	inline VFgTask<BlitImage>::VFgTask (VFrameGraphThread *fg, const BlitImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcImage{ fg->GetResourceManager()->ToLocal( task.srcImage )},
		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstImage{ fg->GetResourceManager()->ToLocal( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		filter{ VEnumCast( task.filter ) },
		regions{ task.regions }
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
	VFgTask< ResolveImage >
=================================================
*/
	inline VFgTask<ResolveImage>::VFgTask (VFrameGraphThread *fg, const ResolveImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcImage{ fg->GetResourceManager()->ToLocal( task.srcImage )},
		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstImage{ fg->GetResourceManager()->ToLocal( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
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
	inline VFgTask<FillBuffer>::VFgTask (VFrameGraphThread *fg, const FillBuffer &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		dstBuffer{ fg->GetResourceManager()->ToLocal( task.dstBuffer )},
		dstOffset{ VkDeviceSize(task.dstOffset) },
		size{ VkDeviceSize(task.size) },
		pattern{ task.pattern }
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
	inline VFgTask<ClearColorImage>::VFgTask (VFrameGraphThread *fg, const ClearColorImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		dstImage{ fg->GetResourceManager()->ToLocal( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		ranges{ task.ranges },
		_clearValue{}
	{
		//ASSERT( not _dstImage->IsImmutable() );

		Visit(	task.clearValue,
				[&] (const RGBA32f &col)		{ memcpy( _clearValue.float32, &col, sizeof(_clearValue.float32) ); },
				[&] (const RGBA32u &col)		{ memcpy( _clearValue.uint32, &col, sizeof(_clearValue.uint32) ); },
				[&] (const RGBA32i &col)		{ memcpy( _clearValue.int32, &col, sizeof(_clearValue.int32) );} ,
				[&] (const std::monostate &)	{}
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
	inline VFgTask<ClearDepthStencilImage>::VFgTask (VFrameGraphThread *fg, const ClearDepthStencilImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		dstImage{ fg->GetResourceManager()->ToLocal( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		clearValue{ task.clearValue.depth, task.clearValue.stencil },
		ranges{ task.ranges }
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
	inline VFgTask<UpdateBuffer>::VFgTask (VFrameGraphThread *fg, const UpdateBuffer &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		dstBuffer{ fg->GetResourceManager()->ToLocal( task.dstBuffer )}
	{
		size_t	cnt = task.regions.size();
		Region*	dst = fg->GetAllocator().Alloc<Region>( cnt );

		for (size_t i = 0; i < cnt; ++i)
		{
			auto&	src = task.regions[i];

			dst[i].dataPtr		= fg->GetAllocator().Alloc( ArraySizeOf(src.data), AlignOf<uint8_t> );
			dst[i].dataSize		= VkDeviceSize(ArraySizeOf(src.data));
			dst[i].bufferOffset	= VkDeviceSize(src.offset);

			memcpy( dst[i].dataPtr, src.data.data(), dst[i].dataSize );
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
	inline VFgTask<Present>::VFgTask (VFrameGraphThread *fg, const Present &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcImage{ fg->GetResourceManager()->ToLocal( task.srcImage )},
		layer{ task.layer }
	{}
	
/*
=================================================
	VFgTask< Present >::IsValid
=================================================
*/
	inline bool  VFgTask<Present>::IsValid () const
	{
		return srcImage;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< PresentVR >
=================================================
*/
	inline VFgTask<PresentVR>::VFgTask (VFrameGraphThread *fg, const PresentVR &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		leftEyeImage{ fg->GetResourceManager()->ToLocal( task.leftEye )},
		leftEyeLayer{ task.leftEyeLayer },
		rightEyeImage{ fg->GetResourceManager()->ToLocal( task.rightEye )},
		rightEyeLayer{ task.rightEyeLayer }
	{}
	
/*
=================================================
	VFgTask< PresentVR >::IsValid
=================================================
*/
	inline bool  VFgTask<PresentVR>::IsValid () const
	{
		return leftEyeImage and rightEyeImage;
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	VFgTask< UpdateRayTracingShaderTable >
=================================================
*/
	inline VFgTask<UpdateRayTracingShaderTable>::VFgTask (VFrameGraphThread *fg, const UpdateRayTracingShaderTable &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		pipeline{ fg->GetResourceManager()->GetResource( task.pipeline )},	rtScene{ fg->GetResourceManager()->ToLocal( task.rtScene )},
		dstBuffer{ fg->GetResourceManager()->ToLocal( task.dstBuffer )},	dstOffset{ VkDeviceSize( task.dstOffset )},
		rayGenShader{ task.rayGenShader },									missShaders{ task.missShaders },
		hitShaders{ task.hitShaders }
		//,	callableShaders{ task.callableShaders }
	{}
	
/*
=================================================
	VFgTask< UpdateRayTracingShaderTable >::IsValid
=================================================
*/
	inline bool  VFgTask<UpdateRayTracingShaderTable>::IsValid () const
	{
		return pipeline and rtScene and dstBuffer;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< BuildRayTracingGeometry >
=================================================
*/
	inline VFgTask<BuildRayTracingGeometry>::VFgTask (VFrameGraphThread *fg, const BuildRayTracingGeometry &task, ProcessFunc_t process) :
		IFrameGraphTask{task, process},
		_usableBuffers{ fg->GetAllocator() }
	{}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< TraceRays >
=================================================
*/
	inline VFgTask<TraceRays>::VFgTask (VFrameGraphThread *fg, const TraceRays &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },		pipeline{ fg->GetResourceManager()->GetResource( task.pipeline )},
		pushConstants{ task.pushConstants },	groupCount{ Max( task.groupCount, 1u )},
		sbtBuffer{ fg->GetResourceManager()->ToLocal( task.shaderTable.buffer )},
		rayGenOffset{ VkDeviceSize( task.shaderTable.rayGenOffset )},
		rayMissOffset{ VkDeviceSize( task.shaderTable.rayMissOffset )},
		rayHitOffset{ VkDeviceSize( task.shaderTable.rayHitOffset )},
		callableOffset{ VkDeviceSize( task.shaderTable.callableOffset )},
		rayMissStride{ uint16_t( task.shaderTable.rayMissStride )},
		rayHitStride{ uint16_t( task.shaderTable.rayHitStride )},
		callableStride{ uint16_t( task.shaderTable.callableStride )}
	{
		CopyDescriptorSets( fg, task.resources, OUT _resources );

		//if ( task.debugMode.mode != Default )
		//	_debugModeIndex = fg->GetShaderDebugger()->Append( INOUT _resources, task.debugMode );
	}
	
/*
=================================================
	VFgTask< TraceRays >::IsValid
=================================================
*/
	inline bool  VFgTask<TraceRays>::IsValid () const
	{
		return pipeline and sbtBuffer;
	}
//-----------------------------------------------------------------------------


}	// FG
