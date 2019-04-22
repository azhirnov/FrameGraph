// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VImage.h"
#include "VEnumCast.h"
#include "FGEnumCast.h"
#include "VDevice.h"
#include "VMemoryObj.h"
#include "VResourceManager.h"

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
	GetImageFlags
=================================================
*/
	ND_ static VkImageCreateFlags  GetImageFlags (EImage imageType)
	{
		VkImageCreateFlags	flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
			
		if ( EImage_IsCube( imageType ) )
			flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		// TODO: VK_IMAGE_CREATE_EXTENDED_USAGE_BIT
		// TODO: VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT for 3D image
		// TODO: VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT 
		// TODO: VK_IMAGE_CREATE_ALIAS_BIT 
		// TODO: VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT 

		return flags;
	}
	
/*
=================================================
	_ChooseAspect
=================================================
*/
	ND_ static VkImageAspectFlags  ChooseAspect (EPixelFormat format)
	{
		VkImageAspectFlags	result = 0;

		if ( EPixelFormat_IsColor( format ) )
			result |= VK_IMAGE_ASPECT_COLOR_BIT;
		else
		{
			if ( EPixelFormat_HasDepth( format ) )
				result |= VK_IMAGE_ASPECT_DEPTH_BIT;

			if ( EPixelFormat_HasStencil( format ) )
				result |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return result;
	}
	
/*
=================================================
	_ChooseDefaultLayout
=================================================
*/
	ND_ static VkImageLayout  ChooseDefaultLayout (EImageUsage usage, VkImageLayout defaultLayout)
	{
		VkImageLayout	result = VK_IMAGE_LAYOUT_GENERAL;

		if ( defaultLayout != VK_IMAGE_LAYOUT_MAX_ENUM )
			result = defaultLayout;
		else
		// render target layouts has high priority to avoid unnecessary decompressions
		if ( EnumEq( usage, EImageUsage::ColorAttachment ) )
			result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		else
		if ( EnumEq( usage, EImageUsage::DepthStencilAttachment ) )
			result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		else
		if ( EnumEq( usage, EImageUsage::Sampled ) )
			result = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		else
		if ( EnumEq( usage, EImageUsage::Storage ) )
			result = VK_IMAGE_LAYOUT_GENERAL;

		return result;
	}
	
/*
=================================================
	GetAllImageAccessMasks
=================================================
*/
	ND_ static VkAccessFlags  GetAllImageAccessMasks (VkImageUsageFlags usage)
	{
		VkAccessFlags	result = 0;

		for (VkImageUsageFlags t = 1; t <= usage; t <<= 1)
		{
			if ( not EnumEq( usage, t ) )
				continue;

			ENABLE_ENUM_CHECKS();
			switch ( VkImageUsageFlagBits(t) )
			{
				case VK_IMAGE_USAGE_TRANSFER_SRC_BIT :				result |= VK_ACCESS_TRANSFER_READ_BIT;					break;
				case VK_IMAGE_USAGE_TRANSFER_DST_BIT :				break;
				case VK_IMAGE_USAGE_SAMPLED_BIT :					result |= VK_ACCESS_SHADER_READ_BIT;					break;
				case VK_IMAGE_USAGE_STORAGE_BIT :					result |= VK_ACCESS_SHADER_READ_BIT;					break;
				case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT :			result |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;			break;
				case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :	result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;	break;
				case VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT :		break;
				case VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT :			result |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;			break;
				case VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV :		result |= VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV;		break;
				case VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT :
				case VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM :			break;	// to shutup compiler warnings
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}

/*
=================================================
	Create
=================================================
*/
	bool VImage::Create (VResourceManager &resMngr, const ImageDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
						 EQueueFamilyMask queueFamilyMask, VkImageLayout defaultLayout, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _image == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );
		CHECK_ERR( not desc.isExternal );
		
		const bool	opt_tiling	= not uint(memObj.MemoryType() & EMemoryTypeExt::HostVisible);

		_desc		= desc;		_desc.Validate();
		_memoryId	= MemoryID{ memId };
		
		// create image
		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext			= null;
		info.flags			= GetImageFlags( _desc.imageType );
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
		
		StaticArray<uint32_t, 8>	queue_family_indices = {};

		// setup sharing mode
		if ( queueFamilyMask != Default )
		{
			info.sharingMode			= VK_SHARING_MODE_CONCURRENT;
			info.pQueueFamilyIndices	= queue_family_indices.data();
			
			for (uint i = 0, mask = (1u<<i);
				 mask <= uint(queueFamilyMask) and info.queueFamilyIndexCount < queue_family_indices.size();
				 ++i, mask = (1u<<i))
			{
				if ( EnumEq( queueFamilyMask, mask ) )
					queue_family_indices[ info.queueFamilyIndexCount++ ] = i;
			}
		}

		// reset to exclusive mode
		if ( info.queueFamilyIndexCount < 2 )
		{
			info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
			info.pQueueFamilyIndices	= null;
			info.queueFamilyIndexCount	= 0;
		}

		auto&	dev = resMngr.GetDevice();

		VK_CHECK( dev.vkCreateImage( dev.GetVkDevice(), &info, null, OUT &_image ));

		CHECK_ERR( memObj.AllocateForImage( resMngr.GetMemoryManager(), _image ));
		
		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_image), dbgName, VK_OBJECT_TYPE_IMAGE );
		}

		_readAccessMask		= GetAllImageAccessMasks( info.usage );
		_aspectMask			= ChooseAspect( _desc.format );
		_defaultLayout		= ChooseDefaultLayout( _desc.usage, defaultLayout );
		_queueFamilyMask	= queueFamilyMask;
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
		EXLOCK( _drCheck );
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
		
		CHECK( desc.queueFamily == VK_QUEUE_FAMILY_IGNORED );	// not supported yet
		CHECK( desc.queueFamilyIndices.empty() or desc.queueFamilyIndices.size() >= 2 );

		_queueFamilyMask = Default;

		for (auto idx : desc.queueFamilyIndices) {
			_queueFamilyMask |= BitCast<EQueueFamily>(idx);
		}

		_aspectMask		= ChooseAspect( _desc.format );
		_defaultLayout	= ChooseDefaultLayout( _desc.usage, BitCast<VkImageLayout>(desc.defaultLayout) );
		_debugName		= dbgName;
		_onRelease		= std::move(onRelease);

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VImage::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );
		
		auto&	dev = resMngr.GetDevice();

		for (auto& view : _viewMap) {
			dev.vkDestroyImageView( dev.GetVkDevice(), view.second, null );
		}
		
		if ( _desc.isExternal and _onRelease ) {
			_onRelease( BitCast<ImageVk_t>(_image) );
		}

		if ( not _desc.isExternal and _image ) {
			dev.vkDestroyImage( dev.GetVkDevice(), _image, null );
		}

		if ( _memoryId ) {
			resMngr.ReleaseResource( _memoryId.Release() );
		}
		
		_viewMap.clear();
		_debugName.clear();
		_image				= VK_NULL_HANDLE;
		_memoryId			= Default;
		_desc				= Default;
		_aspectMask			= 0;
		_defaultLayout		= VK_IMAGE_LAYOUT_MAX_ENUM;
		_queueFamilyMask	= Default;
		_onRelease			= {};
	}
	
/*
=================================================
	GetView
=================================================
*/
	VkImageView  VImage::GetView (const VDevice &dev, const HashedImageViewDesc &desc) const
	{
		SHAREDLOCK( _drCheck );

		// find already created image view
		{
			SHAREDLOCK( _viewMapLock );

			auto	iter = _viewMap.find( desc );

			if ( iter != _viewMap.end() )
				return iter->second;
		}

		// create new image view
		EXLOCK( _viewMapLock );

		auto[iter, inserted] = _viewMap.insert({ desc, VK_NULL_HANDLE });

		if ( not inserted )
			return iter->second;	// other thread create view before
		
		CHECK_ERR( _CreateView( dev, desc, OUT iter->second ));

		return iter->second;
	}
	
/*
=================================================
	GetView
=================================================
*/
	VkImageView  VImage::GetView (const VDevice &dev, bool isDefault, INOUT ImageViewDesc &viewDesc) const
	{
		SHAREDLOCK( _drCheck );

		if ( isDefault )
			viewDesc = ImageViewDesc{ _desc };
		else
			viewDesc.Validate( _desc );

		return GetView( dev, viewDesc );
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
		SHAREDLOCK( _drCheck );
		return not EnumEq( _desc.usage, EImageUsage::TransferDst | EImageUsage::ColorAttachment | EImageUsage::Storage |
										EImageUsage::DepthStencilAttachment | EImageUsage::TransientAttachment );
	}
	
/*
=================================================
	GetApiSpecificDescription
=================================================
*/
	VulkanImageDesc  VImage::GetApiSpecificDescription () const
	{
		VulkanImageDesc		desc;
		desc.image			= BitCast<ImageVk_t>( _image );
		desc.imageType		= BitCast<ImageTypeVk_t>( GetImageType( _desc.imageType ));
		desc.flags			= BitCast<ImageFlagsVk_t>( GetImageFlags( _desc.imageType ));
		desc.usage			= BitCast<ImageUsageVk_t>( VEnumCast( _desc.usage ));
		desc.format			= BitCast<FormatVk_t>( VEnumCast( _desc.format ));
		desc.currentLayout	= BitCast<ImageLayoutVk_t>( _defaultLayout );	// TODO
		desc.defaultLayout	= desc.currentLayout;
		desc.samples		= BitCast<SampleCountFlagBitsVk_t>( VEnumCast( _desc.samples ));
		desc.dimension		= _desc.dimension;
		desc.arrayLayers	= _desc.arrayLayers.Get();
		desc.maxLevels		= _desc.maxLevel.Get();
		desc.queueFamily	= VK_QUEUE_FAMILY_IGNORED;
		//desc.queueFamilyIndices	// TODO
		return desc;
	}


}	// FG
