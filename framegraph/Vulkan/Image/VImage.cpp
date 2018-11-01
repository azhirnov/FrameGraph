// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VImage.h"
#include "VEnumCast.h"
#include "VDevice.h"
#include "VMemoryObj.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VImage::~VImage ()
	{
		ASSERT( _image == VK_NULL_HANDLE );
		ASSERT( _viewMap.empty() );
		//ASSERT( _memoryId );
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
	Create
=================================================
*/
	bool VImage::Create (const VDevice &dev, const ImageDesc &desc, RawMemoryID memId, INOUT VMemoryObj &memObj)
	{
		CHECK_ERR( GetState() == EState::Initial );
		CHECK_ERR( _image == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );
		
		const bool		opt_tiling	= not uint(memObj.MemoryType() & EMemoryTypeExt::HostVisible);

		_initialLayout	= opt_tiling ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PREINITIALIZED;
		_desc			= desc;		_desc.Validate();
		_memoryId		= MemoryID{ memId };
		
		// create image
		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext			= null;
		info.flags			= 0;
		info.imageType		= GetImageType( _desc.imageType );
		info.format			= VEnumCast( _desc.format );
		info.extent.width	= _desc.dimension.x;
		info.extent.height	= _desc.dimension.y;
		info.extent.depth	= _desc.dimension.z;
		info.mipLevels		= _desc.maxLevel.Get();
		info.arrayLayers	= _desc.arrayLayers.Get();
		info.samples		= VEnumCast( _desc.samples );
		info.tiling			= (opt_tiling ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR);
		info.usage			= VEnumCast( _desc.usage );
		info.initialLayout	= _initialLayout;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		if ( EImage_IsCube( _desc.imageType ) )
			info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		VK_CHECK( dev.vkCreateImage( dev.GetVkDevice(), &info, null, OUT &_image ));

		CHECK_ERR( memObj.AllocateForImage( _image ));
		
		if ( not _debugName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_image), _debugName, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
		}

		
		// set aspect mask
		{
			_aspectMask	= 0;

			if ( EPixelFormat_IsColor( _desc.format ) )
				_aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
			else
			{
				if ( EPixelFormat_HasDepth( _desc.format ) )
					_aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;

				if ( EPixelFormat_HasStencil( _desc.format ) )
					_aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}

		// set default layout
		{
			_defaultLayout = VK_IMAGE_LAYOUT_GENERAL;

			if ( BitSet<32>(uint( _desc.usage )).count() == 1 )
			{
				if ( EnumEq( _desc.usage, EImageUsage::Sampled ) )
					_defaultLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				else
				if ( EnumEq( _desc.usage, EImageUsage::ColorAttachment ) )
					_defaultLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				else
				if ( EnumEq( _desc.usage, EImageUsage::DepthStencilAttachment ) )
					_defaultLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
		}
		
		_OnCreate();
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VImage::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		if ( _image ) {
			readyToDelete.emplace_back( VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, uint64_t(_image) );
		}

		for (auto& view : _viewMap) {
			readyToDelete.emplace_back( VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, uint64_t(view.second) );
		}

		if ( _memoryId ) {
			unassignIDs.emplace_back( _memoryId.Release() );
		}
		
		_viewMap.clear();
		_image			= VK_NULL_HANDLE;
		_memoryId		= Default;
		_desc			= Default;
		_aspectMask		= 0;
		_initialLayout	= VK_IMAGE_LAYOUT_MAX_ENUM;
		_defaultLayout	= VK_IMAGE_LAYOUT_MAX_ENUM;
		
		_OnDestroy();
	}
	
/*
=================================================
	Merge
----
	this method must be externaly synchronized!
=================================================
*/
	void VImage::Merge (ImageViewMap_t &from, OUT AppendableVkResources_t readyToDelete) const
	{
		for (auto& view : from)
		{
			auto	iter = _viewMap.find( view.first );

			if ( iter == _viewMap.end() )
				_viewMap.insert( std::move(view) );
			else
				readyToDelete.emplace_back( VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, uint64_t(view.second) );
		}
	}
	
/*
=================================================
	GetView
=================================================
*/
	VkImageView  VImage::GetView (const HashedImageViewDesc &desc) const
	{
		auto	iter = _viewMap.find( desc );

		if ( iter != _viewMap.end() )
			return iter->second;

		return VK_NULL_HANDLE;
	}


}	// FG
