// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VImage.h"
#include "Shared/EnumUtils.h"
#include "VEnumCast.h"
#include "VFrameGraph.h"
#include "VBarrierManager.h"
#include "Shared/EnumUtils.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VImage::VImage (const VFrameGraphWeak &fg) :
		_imageId{ VK_NULL_HANDLE },		_memoryId{ Default },
		_aspectMask{ 0 },				_currentLayout{ VK_IMAGE_LAYOUT_MAX_ENUM },
		_frameGraph{ fg }
	{}

/*
=================================================
	destructor
=================================================
*/	
	VImage::~VImage ()
	{
		_Destroy();
	}
	
/*
=================================================
	GetImageType
=================================================
*/
	ND_ static VkImageType  GetImageType (EImage type)
	{
		ENABLE_ENUM_CHECKS();
		switch ( type )
		{
			case EImage::Tex1D :
			case EImage::Tex1DArray :
				return VK_IMAGE_TYPE_1D;

			case EImage::Tex2D :
			case EImage::Tex2DMS :
			case EImage::TexCube :
			case EImage::Tex2DArray :
			case EImage::Tex2DMSArray :
			case EImage::TexCubeArray :
				return VK_IMAGE_TYPE_2D;

			case EImage::Tex3D :
				return VK_IMAGE_TYPE_3D;

			case EImage::Buffer :
				ASSERT(false);
				break;		// TODO

			case EImage::Unknown :
				break;		// to shutup warnings
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "not supported", VK_IMAGE_TYPE_MAX_ENUM );
	}
	
/*
=================================================
	ValidateDimension
=================================================
*/
	static bool ValidateDimension (const EImage imageType, INOUT uint3 &dim, INOUT ImageLayer &layers)
	{
		ENABLE_ENUM_CHECKS();
		switch ( imageType )
		{
			case EImage::Buffer :
			case EImage::Tex1D :
			{
				ASSERT( dim.x > 0 );
				ASSERT( dim.y <= 1 and dim.z <= 1 and layers.Get() <= 1 );
				dim		= Max( uint3( dim.x, 0, 0 ), 1u );
				layers	= 1_layer;
				return true;
			}	
			case EImage::Tex2DMS :
			case EImage::Tex2D :
			{
				ASSERT( dim.x > 0 and dim.y > 0 );
				ASSERT( dim.z <= 1 and layers.Get() <= 1 );
				dim		= Max( uint3( dim.x, dim.y, 0 ), 1u );
				layers	= 1_layer;
				return true;
			}
			case EImage::TexCube :
			{
				ASSERT( dim.x > 0 and dim.y > 0 and dim.z <= 1 );
				ASSERT( layers.Get() == 6 );
				dim		= Max( uint3( dim.x, dim.y, 1 ), 1u );
				layers	= 6_layer;
				return true;
			}
			case EImage::Tex1DArray :
			{
				ASSERT( dim.x > 0 and layers.Get() > 0 );
			}
			case EImage::Tex2DMSArray :
			case EImage::Tex2DArray :
			{
				ASSERT( dim.x > 0 and dim.y > 0 and layers.Get() > 0 );
				ASSERT( dim.z <= 1 );
				dim		= Max( uint3( dim.x, dim.y, 0 ), 1u );
				layers	= Max( layers, 1_layer );
				return true;
			}
			case EImage::Tex3D :
			{
				ASSERT( dim.x > 0 and dim.y > 0 and dim.z > 0 );
				ASSERT( layers.Get() <= 1 );
				dim		= Max( uint3( dim.x, dim.y, dim.z ), 1u );
				layers	= 1_layer;
			}
			case EImage::TexCubeArray :
			{
				ASSERT( dim.x > 0 and dim.y > 0 and dim.z <= 1 );
				ASSERT( layers.Get() > 0 and layers.Get() % 6 == 0 );
				dim		= Max( uint3( dim.x, dim.y, 0 ), 1u );
				layers	= Max( layers, 6_layer );
				return true;
			}
			case EImage::Unknown :
				break;		// to shutup warnings
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid texture type" );
	}

/*
=================================================
	NumberOfMipmaps
=================================================
*/	
	ND_ static uint  NumberOfMipmaps (const EImage imageType, const uint3 &dim)
	{
		ENABLE_ENUM_CHECKS();
		switch ( imageType )
		{
			case EImage::Buffer :
			case EImage::Tex2DMS :
			case EImage::Tex2DMSArray :		return 1;

			case EImage::Tex2D :
			case EImage::Tex3D :			return std::ilogb( Max(Max( dim.x, dim.y ), dim.z ) ) + 1;
				
			case EImage::Tex1D :
			case EImage::Tex1DArray :		return std::ilogb( dim.x ) + 1;

			case EImage::TexCube :
			case EImage::TexCubeArray :
			case EImage::Tex2DArray :		return std::ilogb( Max( dim.x, dim.y ) ) + 1;

			case EImage::Unknown :			break;		// to shutup warnings
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid texture type", 1u );
	}

/*
=================================================
	ValidateDescription
=================================================
*/
	static void ValidateDescription (INOUT VImage::ImageDescription &desc)
	{
		ASSERT( desc.format != EPixelFormat::Unknown );
		ASSERT( desc.imageType != EImage::Unknown );

		ValidateDimension( desc.imageType, INOUT desc.dimension, INOUT desc.arrayLayers );

		if ( EImage_IsMultisampled( desc.imageType ) )
		{
			ASSERT( desc.samples > 1_samples );
			ASSERT( desc.maxLevel == 1_mipmap );
			desc.maxLevel = 1_mipmap;
		}
		else
		{
			ASSERT( desc.samples <= 1_samples );
			desc.samples = 1_samples;
			desc.maxLevel = MipmapLevel( Clamp( desc.maxLevel.Get(), 1u, NumberOfMipmaps( desc.imageType, desc.dimension ) ));
		}
	}

/*
=================================================
	_CreateImage
=================================================
*/
	bool VImage::_CreateImage (const Description_t &desc, EMemoryTypeExt memType, VMemoryManager &alloc)
	{
		CHECK_ERR( _imageId == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );
		
		VDevice const&	dev			= _frameGraph.lock()->GetDevice();
		const bool		opt_tiling	= not uint(memType & EMemoryTypeExt::HostVisible);

		_currentLayout		= opt_tiling ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PREINITIALIZED;
		_descr				= desc;
		
		ValidateDescription( INOUT _descr );


		// create image
		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext			= null;
		info.flags			= 0;
		info.imageType		= GetImageType( _descr.imageType );
		info.format			= VEnumCast( _descr.format );
		info.extent.width	= _descr.dimension.x;
		info.extent.height	= _descr.dimension.y;
		info.extent.depth	= _descr.dimension.z;
		info.mipLevels		= _descr.maxLevel.Get();
		info.arrayLayers	= _descr.arrayLayers.Get();
		info.samples		= VEnumCast( _descr.samples );
		info.tiling			= (opt_tiling ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR);
		info.usage			= VEnumCast( _descr.usage );
		info.initialLayout	= _currentLayout;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		if ( EImage_IsCube( _descr.imageType ) )
			info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		VK_CHECK( dev.vkCreateImage( dev.GetVkDevice(), &info, null, OUT &_imageId ) );

		CHECK_ERR( alloc.AllocForImage( _imageId, memType, OUT _memoryId ) );
		
		if ( not _debugName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_imageId), _debugName, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
		}

		
		// set aspect mask
		{
			_aspectMask	= 0;

			if ( EPixelFormat_IsColor( _descr.format ) )
				_aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
			else
			{
				if ( EPixelFormat_HasDepth( _descr.format ) )
					_aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;

				if ( EPixelFormat_HasStencil( _descr.format ) )
					_aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		

		// set initial state
		{
			ImageBarrier	barrier;
			barrier.isReadable	= true;
			barrier.isWritable	= true;
			barrier.stages		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			barrier.access		= 0;
			barrier.layout		= _currentLayout;
			barrier.index		= ExeOrderIndex::Initial;
			barrier.range		= SubRange{ 0, ArrayLayers() * MipmapLevels() };

			_readWriteBarriers.push_back( barrier );
		}
		return true;
	}

/*
=================================================
	_CreateImageView
=================================================
*/
	VkImageView  VImage::_CreateImageView (const ImageViewDesc &inDesc) const
	{
		CHECK_ERR( _imageId and _memoryId );

		VFrameGraphPtr	fg = _frameGraph.lock();
		CHECK_ERR( fg );

		VDevice const&	dev = fg->GetDevice();


		ImageViewDesc	desc{ inDesc };
		desc.Validate( _descr );

		const VkComponentSwizzle	components[] = {
			VK_COMPONENT_SWIZZLE_ZERO,	// unknown
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
			VK_COMPONENT_SWIZZLE_ZERO,
			VK_COMPONENT_SWIZZLE_ONE
		};
		const uint4		swizzle = Min( uint4(uint(CountOf(components)-1)), desc.swizzle.ToVec() );

		// create new image view
		VkImageViewCreateInfo	view_info	= {};

		view_info.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.pNext			= null;
		view_info.viewType		= VEnumCast( desc.viewType );
		view_info.flags			= 0;
		view_info.image			= _imageId;
		view_info.format		= VEnumCast( desc.format );
		view_info.components	= { components[swizzle.x], components[swizzle.y], components[swizzle.z], components[swizzle.w] };

		view_info.subresourceRange.aspectMask		= EPixelFormat_IsColor( desc.format ) ? VK_IMAGE_ASPECT_COLOR_BIT :
													  ((EPixelFormat_HasDepth( desc.format ) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
													   (EPixelFormat_HasStencil( desc.format ) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0));
		view_info.subresourceRange.baseMipLevel		= desc.baseLevel.Get();
		view_info.subresourceRange.levelCount		= desc.levelCount;
		view_info.subresourceRange.baseArrayLayer	= desc.baseLayer.Get();
		view_info.subresourceRange.layerCount		= desc.layerCount;
		
		VkImageView		img_view;
		VK_CHECK( dev.vkCreateImageView( dev.GetVkDevice(), &view_info, null, OUT &img_view ) );

		_imageViewMap.Add( desc, img_view );
		return img_view;
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void VImage::_Destroy ()
	{
		VFrameGraphPtr	fg = _frameGraph.lock();

		if ( fg )
		{
			for (auto& view : _imageViewMap)
			{
				fg->GetDevice().DeleteImageView( view.second );
			}

			if ( IsExternal() )
			{
				fg->GetDevice().FreeExternalImage( _imageId );
				ASSERT( not _memoryId );
			}
			else
			{
				fg->GetDevice().DeleteImage( _imageId );
				fg->GetMemoryManager().Dealloc( _memoryId );
			}
		}
		
		//_descr	= Default;	// keep it for debugging
		_imageId	= VK_NULL_HANDLE;
		_memoryId	= Default;
		_imageViewMap.Clear();
	}
	
/*
=================================================
	SetDebugName
=================================================
*/
	void VImage::SetDebugName (StringView name)
	{
		VFrameGraphPtr	fg = _frameGraph.lock();

		if ( _imageId and fg )
		{
			fg->GetDevice().SetObjectName( BitCast<uint64_t>(_imageId), name, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
		}

		_debugName = name;
	}

/*
=================================================
	MakeImmutable
=================================================
*/
	bool VImage::MakeImmutable (bool immutable)
	{
		if ( immutable == _isImmutable )
			return true;

		_isImmutable = immutable;

		if ( _isImmutable )
		{
			// TODO: add whole size barrier to '_pendingBarriers'
			ASSERT(false);
		}
		return true;
	}
	
/*
=================================================
	_FindFirstBarrier
=================================================
*/
	VImage::BarrierArray_t::iterator  VImage::_FindFirstBarrier (BarrierArray_t &arr, const SubRange &otherRange)
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
	void VImage::_ReplaceBarrier (BarrierArray_t &arr, BarrierArray_t::iterator iter, const ImageBarrier &barrier)
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
	void VImage::AddPendingState (const ImageState &is)
	{
		ASSERT( is.range.Mipmaps().begin < MipmapLevels() and is.range.Mipmaps().end <= MipmapLevels() );
		ASSERT( is.range.Layers().begin < ArrayLayers() and is.range.Layers().end <= ArrayLayers() );
		ASSERT( is.task );

		if ( _isImmutable )
			return;

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
	void VImage::CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger)
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
					barrier.image				= _imageId;
					barrier.oldLayout			= iter->layout;
					barrier.newLayout			= pending.layout;
					barrier.srcAccessMask		= iter->access;
					barrier.dstAccessMask		= pending.access;
					barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

					barrier.subresourceRange.aspectMask		= _aspectMask;
					barrier.subresourceRange.baseMipLevel	= (range.begin / ArrayLayers());
					barrier.subresourceRange.levelCount		= ((range.end - range.begin) / ArrayLayers());
					barrier.subresourceRange.baseArrayLayer	= (range.begin % ArrayLayers());
					barrier.subresourceRange.layerCount		= ((range.end - range.begin) % ArrayLayers() + 1);

					barrierMngr.AddImageBarrier( iter->stages, pending.stages, 0, barrier );

					if ( debugger )
						debugger->AddImageBarrier( this, iter->index, pending.index, iter->stages, pending.stages, 0, barrier );
				}
			}

			_ReplaceBarrier( _readWriteBarriers, first, pending );
		}

		_pendingBarriers.clear();
	}


}	// FG
