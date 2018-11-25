// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VTaskGraph.h"
#include "VFrameGraphThread.h"

namespace FG
{

/*
=================================================
	Add
=================================================
*/
	template <typename VisitorT>
	template <typename T>
	inline Task  VTaskGraph<VisitorT>::Add (VFrameGraphThread *fg, const T &task)
	{
		void *	ptr  = fg->GetAllocator().Alloc< VFgTask<T> >();
		auto	iter = *_nodes->insert( PlacementNew< VFgTask<T> >( ptr, fg, task, &_Visitor<T> )).first;

		if ( iter->Inputs().empty() )
			_entries->push_back( iter );

		for (auto in_node : iter->Inputs())
		{
			ASSERT( !!_nodes->count( in_node ) );

			in_node->Attach( iter );
		}
		return iter;
	}
	
/*
=================================================
	OnStart
=================================================
*/
	template <typename VisitorT>
	inline void  VTaskGraph<VisitorT>::OnStart (VFrameGraphThread *fg)
	{
		_nodes.Create( fg->GetAllocator() );
		_entries.Create( fg->GetAllocator() );
		_entries->reserve( 64 );
	}
	
/*
=================================================
	OnDiscardMemory
=================================================
*/
	template <typename VisitorT>
	inline void  VTaskGraph<VisitorT>::OnDiscardMemory ()
	{
		// TODO: skip call for trivial destructors
		for (auto& node : *_nodes)
		{
			node->~IFrameGraphTask();

			// TODO: clear memory
		}

		_nodes.Destroy();
		_entries.Destroy();
	}
//-----------------------------------------------------------------------------

	
	
/*
=================================================
	VFgTask< SubmitRenderPass >
=================================================
*/
	inline VFgTask<SubmitRenderPass>::VFgTask (VFrameGraphThread *fg, const SubmitRenderPass &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_logicalPass{ fg->GetResourceManager()->GetState( task.renderPassId )}
	{
		CHECK( _logicalPass->Submit() );
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	CopyDescriptorSets
=================================================
*/
	inline void CopyDescriptorSets (VFrameGraphThread *fg, const PipelineResourceSet &inResourceSet, OUT VPipelineResourceSet &resourceSet)
	{
		for (auto& res : inResourceSet)
		{
			if ( not res )
				return;

			resourceSet.resources.push_back( fg->GetResourceManager()->CreateDescriptorSet( *res, true ));
			resourceSet.dynamicOffsets.append( res->GetDynamicOffsets() );
		}
	}
	
/*
=================================================
	RemapVertexBuffers
=================================================
*/
	inline void RemapVertexBuffers (VFrameGraphThread *fg, const DrawTask::Buffers_t &inBuffers, const VertexInputState &vertexInput,
									OUT VFgDrawTask<DrawTask>::VertexBuffers_t &vertexBuffers,
									OUT VFgDrawTask<DrawTask>::VertexOffsets_t &vertexOffsets,
									OUT VFgDrawTask<DrawTask>::VertexStrides_t &vertexStrides)
	{
		for (auto& vb : inBuffers)
		{
			auto				iter	= vertexInput.BufferBindings().find( vb.first );
			VLocalBuffer const*	buffer	= fg->GetResourceManager()->GetState( vb.second.buffer );
			
			CHECK_ERR( iter != vertexInput.BufferBindings().end(), void());
			
			vertexBuffers[ iter->second.index ]	= buffer;
			vertexOffsets[ iter->second.index ]	= VkDeviceSize( vb.second.offset );
			vertexStrides[ iter->second.index ] = iter->second.stride;
		}
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	VFgDrawTask< DrawTask >
=================================================
*/
	inline VFgDrawTask<DrawTask>::VFgDrawTask (VFrameGraphThread *fg, const DrawTask &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		IDrawTask{ task, pass1, pass2 },		_vbCount{ uint(task.vertexBuffers.size()) },
		pipeline{ task.pipeline },				renderState{ task.renderState },
		dynamicStates{ task.dynamicStates },	vertexInput{ task.vertexInput },
		drawCmd{ task.drawCmd },				scissors{ task.scissors }
	{
		CopyDescriptorSets( fg, task.resources, OUT _resources );
		RemapVertexBuffers( fg, task.vertexBuffers, task.vertexInput, OUT _vertexBuffers, OUT _vbOffsets, OUT _vbStrides );
	}

/*
=================================================
	VFgDrawTask< DrawIndexedTask >
=================================================
*/
	inline VFgDrawTask<DrawIndexedTask>::VFgDrawTask (VFrameGraphThread *fg, const DrawIndexedTask &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		IDrawTask{ task, pass1, pass2 },				_vbCount{ uint(task.vertexBuffers.size()) },
		pipeline{ task.pipeline },						renderState{ task.renderState },
		dynamicStates{ task.dynamicStates },			indexBuffer{ fg->GetResourceManager()->GetState( task.indexBuffer )},
		indexBufferOffset{ task.indexBufferOffset },	indexType{ task.indexType },
		vertexInput{ task.vertexInput },				drawCmd{ task.drawCmd },
		scissors{ task.scissors }
	{
		CopyDescriptorSets( fg, task.resources, OUT _resources );
		RemapVertexBuffers( fg, task.vertexBuffers, task.vertexInput, OUT _vertexBuffers, OUT _vbOffsets, OUT _vbStrides );
	}

/*
=================================================
	VFgDrawTask< DrawMeshTask >
=================================================
*/
	inline VFgDrawTask<DrawMeshTask>::VFgDrawTask (VFrameGraphThread *fg, const DrawMeshTask &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
		IDrawTask{ task, pass1, pass2 },		pipeline{ task.pipeline },
		renderState{ task.renderState },		dynamicStates{ task.dynamicStates },
		drawCmd{ task.drawCmd },				scissors{ task.scissors }
	{
		CopyDescriptorSets( fg, task.resources, OUT _resources );
	}
//-----------------------------------------------------------------------------


/*
=================================================
	VFgTask< DispatchCompute >
=================================================
*/
	inline VFgTask<DispatchCompute>::VFgTask (VFrameGraphThread *fg, const DispatchCompute &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		pipeline{ task.pipeline },
		groupCount{ Max( task.groupCount, 1u ) },
		localGroupSize{ task.localGroupSize }
	{
		CopyDescriptorSets( fg, task.resources, OUT _resources );
	}

/*
=================================================
	VFgTask< DispatchIndirectCompute >
=================================================
*/
	inline VFgTask<DispatchIndirectCompute>::VFgTask (VFrameGraphThread *fg, const DispatchIndirectCompute &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		pipeline{ task.pipeline },
		indirectBuffer{ fg->GetResourceManager()->GetState( task.indirectBuffer )},
		indirectBufferOffset{ VkDeviceSize(task.indirectBufferOffset) },
		localGroupSize{ task.localGroupSize }
	{
		CopyDescriptorSets( fg, task.resources, OUT _resources );
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< CopyBuffer >
=================================================
*/
	inline VFgTask<CopyBuffer>::VFgTask (VFrameGraphThread *fg, const CopyBuffer &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcBuffer{ fg->GetResourceManager()->GetState( task.srcBuffer )},
		dstBuffer{ fg->GetResourceManager()->GetState( task.dstBuffer )},
		regions{ task.regions }
	{
	}

/*
=================================================
	VFgTask< CopyImage >
=================================================
*/
	inline VFgTask<CopyImage>::VFgTask (VFrameGraphThread *fg, const CopyImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcImage{ fg->GetResourceManager()->GetState( task.srcImage )},
		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstImage{ fg->GetResourceManager()->GetState( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		regions{ task.regions }
	{
		//ASSERT( not _dstImage->IsImmutable() );
	}

/*
=================================================
	VFgTask< CopyBufferToImage >
=================================================
*/
	inline VFgTask<CopyBufferToImage>::VFgTask (VFrameGraphThread *fg, const CopyBufferToImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcBuffer{ fg->GetResourceManager()->GetState( task.srcBuffer )},
		dstImage{ fg->GetResourceManager()->GetState( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		regions{ task.regions }
	{
		//ASSERT( not _dstImage->IsImmutable() );
	}

/*
=================================================
	VFgTask< CopyImageToBuffer >
=================================================
*/
	inline VFgTask<CopyImageToBuffer>::VFgTask (VFrameGraphThread *fg, const CopyImageToBuffer &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcImage{ fg->GetResourceManager()->GetState( task.srcImage )},
		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstBuffer{ fg->GetResourceManager()->GetState( task.dstBuffer )},
		regions{ task.regions }
	{
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< BlitImage >
=================================================
*/
	inline VFgTask<BlitImage>::VFgTask (VFrameGraphThread *fg, const BlitImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcImage{ fg->GetResourceManager()->GetState( task.srcImage )},
		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstImage{ fg->GetResourceManager()->GetState( task.dstImage )},
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
	VFgTask< ResolveImage >
=================================================
*/
	inline VFgTask<ResolveImage>::VFgTask (VFrameGraphThread *fg, const ResolveImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		srcImage{ fg->GetResourceManager()->GetState( task.srcImage )},
		srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		dstImage{ fg->GetResourceManager()->GetState( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		regions{ task.regions }
	{
		//ASSERT( not _dstImage->IsImmutable() );
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< FillBuffer >
=================================================
*/
	inline VFgTask<FillBuffer>::VFgTask (VFrameGraphThread *fg, const FillBuffer &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		dstBuffer{ fg->GetResourceManager()->GetState( task.dstBuffer )},
		dstOffset{ VkDeviceSize(task.dstOffset) },
		size{ VkDeviceSize(task.size) },
		pattern{ task.pattern }
	{}

/*
=================================================
	VFgTask< ClearColorImage >
=================================================
*/
	inline VFgTask<ClearColorImage>::VFgTask (VFrameGraphThread *fg, const ClearColorImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		dstImage{ fg->GetResourceManager()->GetState( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		ranges{ task.ranges },
		_clearValue{}
	{
		//ASSERT( not _dstImage->IsImmutable() );

		if ( auto* fval = std::get_if<float4>( &task.clearValue ) )
			::memcpy( _clearValue.float32, fval, sizeof(_clearValue.float32) );
		else
		if ( auto* ival = std::get_if<int4>( &task.clearValue ) )
			::memcpy( _clearValue.int32, ival, sizeof(_clearValue.int32) );
		else
		if ( auto* uval = std::get_if<uint4>( &task.clearValue ) )
			::memcpy( _clearValue.uint32, uval, sizeof(_clearValue.uint32) );
	}

/*
=================================================
	VFgTask< ClearDepthStencilImage >
=================================================
*/
	inline VFgTask<ClearDepthStencilImage>::VFgTask (VFrameGraphThread *fg, const ClearDepthStencilImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		dstImage{ fg->GetResourceManager()->GetState( task.dstImage )},
		dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		clearValue{ task.clearValue.depth, task.clearValue.stencil }
	{
		//ASSERT( not _dstImage->IsImmutable() );
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< UpdateBuffer >
=================================================
*/
	inline VFgTask<UpdateBuffer>::VFgTask (VFrameGraphThread *fg, const UpdateBuffer &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		dstBuffer{ fg->GetResourceManager()->GetState( task.dstBuffer )},
		dstOffset{ VkDeviceSize(task.offset) },
		_data{ task.data.begin(), task.data.end(), fg->GetAllocator() }
	{}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< Present >
=================================================
*/
	inline VFgTask<Present>::VFgTask (VFrameGraphThread *fg, const Present &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		image{ fg->GetResourceManager()->GetState( task.srcImage )},
		layer{ task.layer }
	{}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< PresentVR >
=================================================
*/
	inline VFgTask<PresentVR>::VFgTask (VFrameGraphThread *fg, const PresentVR &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		leftEyeImage{ fg->GetResourceManager()->GetState( task.leftEye )},
		leftEyeLayer{ task.leftEyeLayer },
		rightEyeImage{ fg->GetResourceManager()->GetState( task.rightEye )},
		rightEyeLayer{ task.rightEyeLayer }
	{}


}	// FG
