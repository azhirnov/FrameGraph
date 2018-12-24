// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VImage.h"
#include "VEnumCast.h"
#include "FGEnumCast.h"
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
		ASSERT( not _memoryId );
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
	_ChooseAspect
=================================================
*/
	VkImageAspectFlags  VImage::_ChooseAspect () const
	{
		VkImageAspectFlags	result = 0;

		if ( EPixelFormat_IsColor( _desc.format ) )
			result |= VK_IMAGE_ASPECT_COLOR_BIT;
		else
		{
			if ( EPixelFormat_HasDepth( _desc.format ) )
				result |= VK_IMAGE_ASPECT_DEPTH_BIT;

			if ( EPixelFormat_HasStencil( _desc.format ) )
				result |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return result;
	}
	
/*
=================================================
	_ChooseDefaultLayout
=================================================
*/
	VkImageLayout  VImage::_ChooseDefaultLayout () const
	{
		VkImageLayout	result = VK_IMAGE_LAYOUT_GENERAL;

		if ( EnumEq( _desc.usage, EImageUsage::Sampled ) )
			result = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		else
		if ( EnumEq( _desc.usage, EImageUsage::Storage ) )
			result = VK_IMAGE_LAYOUT_GENERAL;
		else
		if ( EnumEq( _desc.usage, EImageUsage::ColorAttachment ) )
			result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		else
		if ( EnumEq( _desc.usage, EImageUsage::DepthStencilAttachment ) )
			result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		return result;
	}

/*
=================================================
	Create
=================================================
*/
	bool VImage::Create (const VDevice &dev, const ImageDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
						 EQueueFamily queueFamily, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _image == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );
		CHECK_ERR( not desc.isExternal );
		
		const bool		opt_tiling	= not uint(memObj.MemoryType() & EMemoryTypeExt::HostVisible);

		_desc		= desc;		_desc.Validate();
		_memoryId	= MemoryID{ memId };
		
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
		info.initialLayout	= (opt_tiling ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PREINITIALIZED);
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		if ( EImage_IsCube( _desc.imageType ) )
			info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		// TODO: VK_IMAGE_CREATE_EXTENDED_USAGE_BIT
		// TODO: VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT for 3D image
		// TODO: VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT 
		// TODO: VK_IMAGE_CREATE_ALIAS_BIT 
		// TODO: VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT 

		VK_CHECK( dev.vkCreateImage( dev.GetVkDevice(), &info, null, OUT &_image ));

		CHECK_ERR( memObj.AllocateForImage( _image ));
		
		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_image), dbgName, VK_OBJECT_TYPE_IMAGE );
		}

		_aspectMask			= _ChooseAspect();
		_defaultLayout		= _ChooseDefaultLayout();
		_currQueueFamily	= queueFamily;
		_debugName			= dbgName;

		return true;
	}

/*
=================================================
	Create
=================================================
*/
	bool VImage::Create (const VDevice &dev, const VulkanImageDesc &desc, StringView dbgName, OnRelease_t &&onRelease)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _image == VK_NULL_HANDLE );
		
		_image				= BitCast<VkImage>( desc.image );
		_desc.imageType		= FGEnumCast( BitCast<VkImageType>( desc.imageType ), BitCast<ImageFlagsVk_t>( desc.flags ),
										  desc.arrayLayers, BitCast<VkSampleCountFlagBits>( desc.samples ) );
		_desc.dimension		= desc.dimension;
		_desc.format		= FGEnumCast( BitCast<VkFormat>( desc.format ));
		_desc.usage			= FGEnumCast( BitCast<VkImageUsageFlagBits>( desc.usage ));
		_desc.arrayLayers	= ImageLayer{ desc.arrayLayers };
		_desc.maxLevel		= MipmapLevel{ desc.maxLevels };
		_desc.samples		= FGEnumCast( BitCast<VkSampleCountFlagBits>( desc.samples ));
		_desc.isExternal	= true;

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_image), dbgName, VK_OBJECT_TYPE_IMAGE );
		}
		
		_aspectMask			= _ChooseAspect();
		_defaultLayout		= _ChooseDefaultLayout();
		_currQueueFamily	= BitCast<EQueueFamily>( desc.queueFamily );
		_debugName			= dbgName;
		_onRelease			= std::move(onRelease);

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VImage::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		SCOPELOCK( _rcCheck );

		for (auto& view : _viewMap) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_IMAGE_VIEW, uint64_t(view.second) );
		}
		
		if ( _desc.isExternal and _onRelease ) {
			_onRelease( BitCast<ImageVk_t>(_image) );
		}

		if ( not _desc.isExternal and _image ) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_IMAGE, uint64_t(_image) );
		}

		if ( _memoryId ) {
			unassignIDs.emplace_back( _memoryId.Release() );
		}
		
		_viewMap.clear();
		_debugName.clear();
		_image				= VK_NULL_HANDLE;
		_memoryId			= Default;
		_desc				= Default;
		_aspectMask			= 0;
		_defaultLayout		= VK_IMAGE_LAYOUT_MAX_ENUM;
		_currQueueFamily	= Default;
		_onRelease			= {};
	}
	
/*
=================================================
	GetView
=================================================
*/
	VkImageView  VImage::GetView (const VDevice &dev, const HashedImageViewDesc &desc) const
	{
		// find already created image view
		{
			SHAREDLOCK( _viewMapLock );

			auto	iter = _viewMap.find( desc );

			if ( iter != _viewMap.end() )
				return iter->second;
		}

		// create new image view
		SCOPELOCK( _viewMapLock );

		auto[iter, inserted] = _viewMap.insert({ desc, VK_NULL_HANDLE });

		if ( not inserted )
			return iter->second;	// other thread create view before
		
		CHECK_ERR( _CreateView( dev, desc, OUT iter->second ));

		return iter->second;
	}

/*
=================================================
	_CreateView
=================================================
*/
	bool VImage::_CreateView (const VDevice &dev, const HashedImageViewDesc &viewDesc, OUT VkImageView &outView) const
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

		const auto&				desc		= viewDesc.Get();
		const uint4				swizzle		= Min( uint4(uint(CountOf(components)-1)), desc.swizzle.ToVec() );
		VkImageViewCreateInfo	view_info	= {};

		view_info.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.pNext			= null;
		view_info.viewType		= VEnumCast( desc.viewType );
		view_info.flags			= 0;
		view_info.image			= Handle();
		view_info.format		= VEnumCast( desc.format );
		view_info.components	= { components[swizzle.x], components[swizzle.y], components[swizzle.z], components[swizzle.w] };

		view_info.subresourceRange.aspectMask		= VEnumCast( desc.aspectMask );
		view_info.subresourceRange.baseMipLevel		= desc.baseLevel.Get();
		view_info.subresourceRange.levelCount		= desc.levelCount;
		view_info.subresourceRange.baseArrayLayer	= desc.baseLayer.Get();
		view_info.subresourceRange.layerCount		= desc.layerCount;
		
		VK_CHECK( dev.vkCreateImageView( dev.GetVkDevice(), &view_info, null, OUT &outView ));
		return true;
	}
	
/*
=================================================
	IsReadOnly
=================================================
*/
	bool  VImage::IsReadOnly () const
	{
		SHAREDLOCK( _rcCheck );
		return not EnumEq( _desc.usage, EImageUsage::TransferDst | EImageUsage::ColorAttachment | EImageUsage::Storage |
										EImageUsage::DepthStencilAttachment | EImageUsage::TransientAttachment );
	}


}	// FG
