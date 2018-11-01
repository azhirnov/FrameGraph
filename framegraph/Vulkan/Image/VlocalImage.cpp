// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VLocalImage.h"
#include "VEnumCast.h"
#include "VTaskGraph.h"
#include "VBarrierManager.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VLocalImage::~VLocalImage ()
	{
		ASSERT( _imageData == null );
		ASSERT( _viewMap.empty() );
	}

/*
=================================================
	Create
=================================================
*/
	bool VLocalImage::Create (const VImage *imageData)
	{
		CHECK_ERR( GetState() == EState::Initial );
		CHECK_ERR( _imageData == null );
		CHECK_ERR( imageData and imageData->IsCreated() );

		_imageData		= imageData;
		_finalLayout	= _imageData->DefaultLayout();	// TODO
		
		// set initial state
		{
			ImageBarrier	barrier;
			barrier.isReadable	= true;
			barrier.isWritable	= true;
			barrier.stages		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			barrier.access		= 0;
			barrier.layout		= _imageData->InitialLayout();	// TODO
			barrier.index		= ExeOrderIndex::Initial;
			barrier.range		= SubRange{ 0, ArrayLayers() * MipmapLevels() };

			_readWriteBarriers.push_back( barrier );
		}

		_OnCreate();
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VLocalImage::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t)
	{
		_imageData->Merge( _viewMap, OUT readyToDelete );

		_imageData	= null;
		_viewMap.clear();
		
		_OnDestroy();
	}
	
/*
=================================================
	GetView
=================================================
*/
	VkImageView  VLocalImage::GetView (const VDevice &dev, const ImageViewDesc &viewDesc) const
	{
		ASSERT( IsCreated() );

		const HashedImageViewDesc	view_desc{ viewDesc, Description() };
		
		if ( auto view = _imageData->GetView( view_desc ) )
			return view;

		auto	inserted = _viewMap.insert({ view_desc, VkImageView(0) });

		// if exists
		if ( not inserted.second )
			return inserted.first->second;

		// create new view
		CHECK_ERR( _CreateView( dev, view_desc, OUT inserted.first->second ));

		return inserted.first->second;
	}
	
/*
=================================================
	GetView
=================================================
*/
	VkImageView  VLocalImage::GetView (const VDevice &dev, const Optional<ImageViewDesc> &viewDesc) const
	{
		return GetView( dev, viewDesc.value_or( ImageViewDesc{Description()} ));
	}

/*
=================================================
	_CreateView
=================================================
*/
	bool VLocalImage::_CreateView (const VDevice &dev, const HashedImageViewDesc &viewDesc, OUT VkImageView &outView) const
	{
		const VkComponentSwizzle	components[] = {
			VK_COMPONENT_SWIZZLE_IDENTITY,	// unknown
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
			VK_COMPONENT_SWIZZLE_ZERO,
			VK_COMPONENT_SWIZZLE_ONE
		};

		const uint4				swizzle		= Min( uint4(uint(CountOf(components)-1)), viewDesc.Get().swizzle.ToVec() );
		VkImageViewCreateInfo	view_info	= {};

		view_info.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.pNext			= null;
		view_info.viewType		= VEnumCast( viewDesc.Get().viewType );
		view_info.flags			= 0;
		view_info.image			= Handle();
		view_info.format		= VEnumCast( viewDesc.Get().format );
		view_info.components	= { components[swizzle.x], components[swizzle.y], components[swizzle.z], components[swizzle.w] };

		view_info.subresourceRange.aspectMask		= EPixelFormat_IsColor( viewDesc.Get().format ) ? VK_IMAGE_ASPECT_COLOR_BIT :
													  ((EPixelFormat_HasDepth( viewDesc.Get().format ) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
													   (EPixelFormat_HasStencil( viewDesc.Get().format ) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0));
		view_info.subresourceRange.baseMipLevel		= viewDesc.Get().baseLevel.Get();
		view_info.subresourceRange.levelCount		= viewDesc.Get().levelCount;
		view_info.subresourceRange.baseArrayLayer	= viewDesc.Get().baseLayer.Get();
		view_info.subresourceRange.layerCount		= viewDesc.Get().layerCount;
		
		VK_CHECK( dev.vkCreateImageView( dev.GetVkDevice(), &view_info, null, OUT &outView ));
		return true;
	}

/*
=================================================
	_FindFirstBarrier
=================================================
*/
	inline VLocalImage::BarrierArray_t::iterator
		VLocalImage::_FindFirstBarrier (BarrierArray_t &arr, const SubRange &otherRange)
	{
		auto iter = arr.begin();
		auto end  = arr.end();

		for (; iter != end and iter->range.end <= otherRange.begin; ++iter)
		{}
		return iter;
	}
	
/*
=================================================
	_ReplaceBarrier
=================================================
*/
	void VLocalImage::_ReplaceBarrier (BarrierArray_t &arr, BarrierArray_t::iterator iter, const ImageBarrier &barrier)
	{
		ASSERT( iter >= arr.begin() and iter <= arr.end() );

		bool replaced = false;

		for (; iter != arr.end();)
		{
			if ( iter->range.begin <  barrier.range.begin and
				 iter->range.end   <= barrier.range.end )
			{
				//	|1111111|22222|
				//     |bbbbb|			+
				//  |11|....			=
				iter->range.end = barrier.range.begin;
				++iter;
				continue;
			}

			if ( iter->range.begin < barrier.range.begin and
				 iter->range.end   > barrier.range.end )
			{
				//  |111111111111111|
				//      |bbbbb|			+
				//  |111|bbbbb|11111|	=

				const auto	src = *iter;

				iter->range.end = barrier.range.begin;

				iter = arr.insert( iter+1, barrier );
				replaced = true;

				iter = arr.insert( iter+1, src );

				iter->range.begin = barrier.range.end;
				break;
			}

			if ( iter->range.begin >= barrier.range.begin and
				 iter->range.begin <  barrier.range.end )
			{
				if ( iter->range.end > barrier.range.end )
				{
					//  ...|22222222222|
					//   |bbbbbbb|			+
					//  ...bbbbbb|22222|	=
					iter->range.begin = barrier.range.end;

					if ( not replaced )
					{
						arr.insert( iter, barrier );
						replaced = true;
					}
					break;
				}

				if ( replaced )
				{
					//  ...|22222|33333|
					//   |bbbbbbbbbbb|		+
					//  ...|bbbbbbbbb|...	=
					iter = arr.erase( iter );
				}
				else
				{
					*iter = barrier;
					++iter;
					replaced = true;
				}
				continue;
			}

			break;
		}
			
		if ( not replaced )
		{
			arr.insert( iter, barrier );
		}
	}

/*
=================================================
	AddPendingState
=================================================
*/
	void VLocalImage::AddPendingState (const ImageState &is) const
	{
		ASSERT( is.range.Mipmaps().begin < MipmapLevels() and is.range.Mipmaps().end <= MipmapLevels() );
		ASSERT( is.range.Layers().begin < ArrayLayers() and is.range.Layers().end <= ArrayLayers() );
		ASSERT( is.task );

		//if ( _isImmutable )
		//	return;

		ImageBarrier	barrier;
		barrier.isReadable	= EResourceState_IsReadable( is.state );
		barrier.isWritable	= EResourceState_IsWritable( is.state );
		barrier.stages		= EResourceState_ToPipelineStages( is.state );
		barrier.access		= EResourceState_ToAccess( is.state );
		barrier.layout		= is.layout;
		barrier.index		= is.task->ExecutionOrder();
		

		// extract sub ranges
		Array<SubRange>	sub_ranges;
		SubRange		layer_range	 { is.range.Layers().begin,  Min( is.range.Layers().end,  ArrayLayers() ) };
		SubRange		mipmap_range { is.range.Mipmaps().begin, Min( is.range.Mipmaps().end, MipmapLevels() ) };

		if ( is.range.IsWholeLayers() and is.range.IsWholeMipmaps() )
		{
			sub_ranges.push_back(SubRange{ 0, ArrayLayers() * MipmapLevels() });
		}
		else
		if ( is.range.IsWholeLayers() )
		{
			uint	begin = mipmap_range.begin   * ArrayLayers() + layer_range.begin;
			uint	end   = (mipmap_range.end-1) * ArrayLayers() + layer_range.end;
				
			sub_ranges.push_back(SubRange{ begin, end });
		}
		else
		for (uint mip = mipmap_range.begin; mip < mipmap_range.end; ++mip)
		{
			uint	begin = mip * ArrayLayers() + layer_range.begin;
			uint	end   = mip * ArrayLayers() + layer_range.end;

			sub_ranges.push_back(SubRange{ begin, end });
		}


		// merge with pending
		for (auto& range : sub_ranges)
		{
			auto	iter = _FindFirstBarrier( _pendingBarriers, range );

			if ( iter != _pendingBarriers.end() and iter->range.begin > range.begin )
			{
				barrier.range = { range.begin, iter->range.begin };

				iter = _pendingBarriers.insert( iter, barrier );
				++iter;

				range.begin = iter->range.begin;
			}

			for (; iter != _pendingBarriers.end() and iter->range.IsIntersects( range ); ++iter)
			{
				iter->range.begin	= Min( iter->range.begin, range.begin );
				range.begin			= iter->range.end;
				
				ASSERT( iter->index == barrier.index );
				ASSERT( iter->layout == barrier.layout );
								
				iter->stages		|= barrier.stages;
				iter->access		|= barrier.access;
				iter->isReadable	|= barrier.isReadable;
				iter->isWritable	|= barrier.isWritable;
			}

			if ( not range.IsEmpty() )
			{
				barrier.range = range;
				_pendingBarriers.insert( iter, barrier );
			}
		}
	}
	
/*
=================================================
	CommitBarrier
=================================================
*/
	void VLocalImage::CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const
	{
		for (const auto& pending : _pendingBarriers)
		{
			// barriers: write -> write, read -> write, write -> read, layout -> layout
			const auto	first = _FindFirstBarrier( _readWriteBarriers, pending.range );

			for (auto iter = first; iter != _readWriteBarriers.end() and iter->range.begin < pending.range.end; ++iter)
			{
				const SubRange	range		= iter->range.Intersect( pending.range );

				const bool		is_modified = iter->layout != pending.layout	or 
											  iter->isWritable					or
											  pending.isWritable;

				if ( not range.IsEmpty() and is_modified )
				{
					VkImageMemoryBarrier	barrier = {};
					barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.pNext				= null;
					barrier.image				= Handle();
					barrier.oldLayout			= iter->layout;
					barrier.newLayout			= pending.layout;
					barrier.srcAccessMask		= iter->access;
					barrier.dstAccessMask		= pending.access;
					barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

					barrier.subresourceRange.aspectMask		= AspectMask();
					barrier.subresourceRange.baseMipLevel	= (range.begin / ArrayLayers());
					barrier.subresourceRange.levelCount		= ((range.end - range.begin) / ArrayLayers());
					barrier.subresourceRange.baseArrayLayer	= (range.begin % ArrayLayers());
					barrier.subresourceRange.layerCount		= ((range.end - range.begin) % ArrayLayers() + 1);

					barrierMngr.AddImageBarrier( iter->stages, pending.stages, 0, barrier );

					//if ( debugger )
					//	debugger->AddImageBarrier( this, iter->index, pending.index, iter->stages, pending.stages, 0, barrier );
				}
			}

			_ReplaceBarrier( _readWriteBarriers, first, pending );
		}

		_pendingBarriers.clear();
	}


}	// FG
