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
		for (auto& node : *_nodes) {
			node->~IFrameGraphTask();
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
		_renderPass{ fg->GetResourceManager()->GetState(LogicalRenderPassID{ uint(size_t(task.renderPass)), 0 }) }
	{}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< DispatchCompute >
=================================================
*/
	inline VFgTask<DispatchCompute>::VFgTask (VFrameGraphThread *fg, const DispatchCompute &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_pipeline{ task.pipeline },
		_groupCount{ Max( task.groupCount, 1u ) },
		_localGroupSize{ task.localGroupSize }
	{
		for (const auto& res : task.resources)
		{
			ASSERT( res.second );
			_resources.insert({ res.first, fg->GetResourceManager()->CreateDescriptorSet( *res.second, true ) });
		}
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< DispatchIndirectCompute >
=================================================
*/
	inline VFgTask<DispatchIndirectCompute>::VFgTask (VFrameGraphThread *fg, const DispatchIndirectCompute &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_pipeline{ task.pipeline },
		_indirectBuffer{ fg->GetResourceManager()->Remap( task.indirectBuffer )},
		_indirectBufferOffset{ VkDeviceSize(task.indirectBufferOffset) },
		_localGroupSize{ task.localGroupSize }
	{
		for (const auto& res : task.resources)
		{
			ASSERT( res.second );
			_resources.insert({ res.first, fg->GetResourceManager()->CreateDescriptorSet( *res.second, fg->GetPipelineCache() ) });
		}
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< CopyBuffer >
=================================================
*/
	inline VFgTask<CopyBuffer>::VFgTask (VFrameGraphThread *fg, const CopyBuffer &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_srcBuffer{ fg->GetResourceManager()->Remap( task.srcBuffer )},
		_dstBuffer{ fg->GetResourceManager()->Remap( task.dstBuffer )},
		_regions{ task.regions }
	{
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< CopyImage >
=================================================
*/
	inline VFgTask<CopyImage>::VFgTask (VFrameGraphThread *fg, const CopyImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_srcImage{ fg->GetResourceManager()->Remap( task.srcImage )},
		_srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		_dstImage{ fg->GetResourceManager()->Remap( task.dstImage )},
		_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		_regions{ task.regions }
	{
		//ASSERT( not _dstImage->IsImmutable() );
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< CopyBufferToImage >
=================================================
*/
	inline VFgTask<CopyBufferToImage>::VFgTask (VFrameGraphThread *fg, const CopyBufferToImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_srcBuffer{ fg->GetResourceManager()->Remap( task.srcBuffer )},
		_dstImage{ fg->GetResourceManager()->Remap( task.dstImage )},
		_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		_regions{ task.regions }
	{
		//ASSERT( not _dstImage->IsImmutable() );
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< CopyImageToBuffer >
=================================================
*/
	inline VFgTask<CopyImageToBuffer>::VFgTask (VFrameGraphThread *fg, const CopyImageToBuffer &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_srcImage{ fg->GetResourceManager()->Remap( task.srcImage )},
		_srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		_dstBuffer{ fg->GetResourceManager()->Remap( task.dstBuffer )},
		_regions{ task.regions }
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
		_srcImage{ fg->GetResourceManager()->Remap( task.srcImage )},
		_srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		_dstImage{ fg->GetResourceManager()->Remap( task.dstImage )},
		_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		_filter{ VEnumCast( task.filter ) },
		_regions{ task.regions }
	{
		//ASSERT( not _dstImage->IsImmutable() );

		//if ( EPixelFormat_HasDepthOrStencil( _srcImage->PixelFormat() ) )
		//{
	//		ASSERT( _filter != VK_FILTER_NEAREST );
	//		_filter = VK_FILTER_NEAREST;
	//	}
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< ResolveImage >
=================================================
*/
	inline VFgTask<ResolveImage>::VFgTask (VFrameGraphThread *fg, const ResolveImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_srcImage{ fg->GetResourceManager()->Remap( task.srcImage )},
		_srcLayout{ /*_srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :*/ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
		_dstImage{ fg->GetResourceManager()->Remap( task.dstImage )},
		_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		_regions{ task.regions }
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
		_dstBuffer{ fg->GetResourceManager()->Remap( task.dstBuffer )},
		_dstOffset{ VkDeviceSize(task.dstOffset) },
		_size{ VkDeviceSize(task.size) },
		_pattern{ task.pattern }
	{}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< ClearColorImage >
=================================================
*/
	inline VFgTask<ClearColorImage>::VFgTask (VFrameGraphThread *fg, const ClearColorImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_dstImage{ fg->GetResourceManager()->Remap( task.dstImage )},
		_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		_clearValue{},
		_ranges{ task.ranges }
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
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< ClearDepthStencilImage >
=================================================
*/
	inline VFgTask<ClearDepthStencilImage>::VFgTask (VFrameGraphThread *fg, const ClearDepthStencilImage &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_dstImage{ fg->GetResourceManager()->Remap( task.dstImage )},
		_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
		_clearValue{ task.clearValue.depth, task.clearValue.stencil }
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
		_buffer{ fg->GetResourceManager()->Remap( task.dstBuffer )},
		_offset{ VkDeviceSize(task.offset) },
		_data{ task.data.begin(), task.data.end() }
	{}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< Present >
=================================================
*/
	inline VFgTask<Present>::VFgTask (VFrameGraphThread *fg, const Present &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_image{ fg->GetResourceManager()->Remap( task.srcImage )},
		_layer{ task.layer }
	{}
//-----------------------------------------------------------------------------
	

/*
=================================================
	VFgTask< PresentVR >
=================================================
*/
	inline VFgTask<PresentVR>::VFgTask (VFrameGraphThread *fg, const PresentVR &task, ProcessFunc_t process) :
		IFrameGraphTask{ task, process },
		_leftEyeImage{ fg->GetResourceManager()->Remap( task.leftEye )},
		_leftEyeLayer{ task.leftEyeLayer },
		_rightEyeImage{ fg->GetResourceManager()->Remap( task.rightEye )},
		_rightEyeLayer{ task.rightEyeLayer }
	{}


}	// FG
