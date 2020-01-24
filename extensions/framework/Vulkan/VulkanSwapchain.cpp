// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VulkanSwapchain.h"
#include "stl/Algorithms/EnumUtils.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Memory/MemUtils.h"
#include "VulkanDevice.h"

namespace FGC
{

/*
=================================================
	constructor
=================================================
*/
	VulkanSwapchain::VulkanSwapchain () :
		_vkSwapchain{ VK_NULL_HANDLE },			_vkPhysicalDevice{ VK_NULL_HANDLE },
		_vkDevice{ VK_NULL_HANDLE },			_vkSurface{ VK_NULL_HANDLE },
		_currImageIndex{ UMax },
		_colorFormat{ VK_FORMAT_UNDEFINED },	_colorSpace{ VK_COLOR_SPACE_MAX_ENUM_KHR },
		_minImageCount{ 0 },
		_preTransform{ VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR },
		_presentMode{ VK_PRESENT_MODE_MAX_ENUM_KHR },
		_compositeAlpha{ VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR },
		_colorImageUsage{ 0 },
		_lastFpsUpdateTime{ TimePoint_t::clock::now() },
		_frameCounter{ 0 },
		_currentFPS{ 0.0f }
	{
	}
	
/*
=================================================
	constructor
=================================================
*/
	VulkanSwapchain::VulkanSwapchain (VkPhysicalDevice physicalDev, VkDevice logicalDev, VkSurfaceKHR surface, const VulkanDeviceFn &fn) : VulkanSwapchain()
	{
		_vkPhysicalDevice	= physicalDev;
		_vkDevice			= logicalDev;
		_vkSurface			= surface;

		CHECK( _vkPhysicalDevice and _vkDevice and _vkSurface );
		VulkanDeviceFn_Init( fn );
	}
	
	VulkanSwapchain::VulkanSwapchain (const VulkanDevice &dev) :
		VulkanSwapchain( dev.GetVkPhysicalDevice(), dev.GetVkDevice(), dev.GetVkSurface(), dev )
	{
	}

/*
=================================================
	destructor
=================================================
*/
	VulkanSwapchain::~VulkanSwapchain ()
	{
		CHECK( not _vkSwapchain );
	}
	
/*
=================================================
	ChooseColorFormat
=================================================
*/
	bool VulkanSwapchain::ChooseColorFormat (INOUT VkFormat &colorFormat, INOUT VkColorSpaceKHR &colorSpace) const
	{
		CHECK_ERR( _vkPhysicalDevice and _vkSurface );

		uint						count				= 0;
		Array< VkSurfaceFormatKHR >	surf_formats;
		const VkFormat				def_format			= VK_FORMAT_B8G8R8A8_UNORM;
		const VkColorSpaceKHR		def_space			= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		const VkFormat				required_format		= colorFormat;
		const VkColorSpaceKHR		required_colorspace	= colorSpace;

		VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( _vkPhysicalDevice, _vkSurface, OUT &count, null ));
		CHECK_ERR( count > 0 );

		surf_formats.resize( count );
		VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( _vkPhysicalDevice, _vkSurface, OUT &count, OUT surf_formats.data() ));
		
		if ( count == 1		and
			 surf_formats[0].format == VK_FORMAT_UNDEFINED )
		{
			colorFormat = required_format;
			colorSpace  = surf_formats[0].colorSpace;
		}
		else
		{
			const size_t	max_idx		= UMax;

			size_t	both_match_idx		= max_idx;
			size_t	format_match_idx	= max_idx;
			size_t	space_match_idx		= max_idx;
			size_t	def_format_idx		= 0;
			size_t	def_space_idx		= 0;

			for (size_t i = 0; i < surf_formats.size(); ++i)
			{
				const auto&	surf_fmt = surf_formats[i];

				if ( surf_fmt.format	 == required_format		and
					 surf_fmt.colorSpace == required_colorspace )
				{
					both_match_idx = i;
				}
				else
				// separate check
				if ( surf_fmt.format	 == required_format )
					format_match_idx = i;
				else
				if ( surf_fmt.colorSpace == required_colorspace )
					space_match_idx = i;

				// check with default
				if ( surf_fmt.format	 == def_format )
					def_format_idx = i;

				if ( surf_fmt.colorSpace == def_space )
					def_space_idx = i;
			}

			size_t	idx = 0;

			if ( both_match_idx != max_idx )
				idx = both_match_idx;
			else
			if ( format_match_idx != max_idx )
				idx = format_match_idx;
			else
			if ( def_format_idx != max_idx )
				idx = def_format_idx;

			// TODO: space_match_idx and def_space_idx are unused yet

			colorFormat	= surf_formats[ idx ].format;
			colorSpace	= surf_formats[ idx ].colorSpace;
		}

		return true;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool VulkanSwapchain::IsSupported (const uint imageArrayLayers, const VkSampleCountFlags samples, const VkPresentModeKHR presentMode,
									   const VkFormat colorFormat, const VkImageUsageFlags colorImageUsage) const
	{
		CHECK_ERR( _vkPhysicalDevice and _vkSurface );

		VkSurfaceCapabilitiesKHR	surf_caps;
		VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( _vkPhysicalDevice, _vkSurface, OUT &surf_caps ));
		
		VkImageUsageFlags	image_usage = 0;
		_GetImageUsage( OUT image_usage, presentMode, colorFormat, surf_caps );

		if ( not EnumEq( image_usage, colorImageUsage ) )
			return false;

		VkImageFormatProperties	image_props = {};
		VK_CALL( vkGetPhysicalDeviceImageFormatProperties( _vkPhysicalDevice, colorFormat, VK_IMAGE_TYPE_2D,
														   VK_IMAGE_TILING_OPTIMAL, colorImageUsage, 0, OUT &image_props ));

		if ( not EnumEq( image_props.sampleCounts, samples ) )
			return false;

		if ( imageArrayLayers < image_props.maxArrayLayers )
			return false;

		return true;
	}

/*
=================================================
	Create
=================================================
*/
	bool VulkanSwapchain::Create (const uint2							&viewSize,
								  const VkFormat						colorFormat,
								  const VkColorSpaceKHR					colorSpace,
								  const uint							minImageCount,
								  const VkPresentModeKHR				presentMode,
								  const VkSurfaceTransformFlagBitsKHR	transform,
								  const VkCompositeAlphaFlagBitsKHR		compositeAlpha,
								  const VkImageUsageFlags				colorImageUsage,
								  ArrayView<uint>						queueFamilyIndices)
	{
		CHECK_ERR( _vkPhysicalDevice and _vkDevice and _vkSurface );
		CHECK_ERR( not IsImageAcquired() );

		VkSurfaceCapabilitiesKHR	surf_caps;
		VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( _vkPhysicalDevice, _vkSurface, OUT &surf_caps ));

		VkSwapchainKHR				old_swapchain	= _vkSwapchain;
		VkSwapchainCreateInfoKHR	swapchain_info	= {};
		
		swapchain_info.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_info.pNext					= null;
		swapchain_info.surface					= _vkSurface;
		swapchain_info.imageFormat				= colorFormat;
		swapchain_info.imageColorSpace			= colorSpace;
		swapchain_info.imageExtent				= { viewSize.x, viewSize.y };
		swapchain_info.imageArrayLayers			= 1;
		swapchain_info.minImageCount			= minImageCount;
		swapchain_info.oldSwapchain				= old_swapchain;
		swapchain_info.clipped					= VK_TRUE;
		swapchain_info.preTransform				= transform;
		swapchain_info.presentMode				= presentMode;
		swapchain_info.compositeAlpha			= compositeAlpha;
		swapchain_info.imageSharingMode			= VK_SHARING_MODE_EXCLUSIVE;
		
		if ( queueFamilyIndices.size() > 1 ) {
			swapchain_info.queueFamilyIndexCount = uint(queueFamilyIndices.size());
			swapchain_info.pQueueFamilyIndices	 = queueFamilyIndices.data();
			swapchain_info.imageSharingMode		 = VK_SHARING_MODE_CONCURRENT;
		}

		_GetSurfaceImageCount( INOUT swapchain_info.minImageCount, surf_caps );
		_GetSurfaceTransform( INOUT swapchain_info.preTransform, surf_caps );
		_GetSwapChainExtent( INOUT swapchain_info.imageExtent, surf_caps );
		_GetPresentMode( INOUT swapchain_info.presentMode );
		CHECK_ERR( _GetImageUsage( OUT swapchain_info.imageUsage, swapchain_info.presentMode, colorFormat, surf_caps ));
		CHECK_ERR( _GetCompositeAlpha( INOUT swapchain_info.compositeAlpha, surf_caps ));
		
		swapchain_info.imageUsage &= colorImageUsage;

		VK_CHECK( vkCreateSwapchainKHR( _vkDevice, &swapchain_info, null, OUT &_vkSwapchain ));


		// destroy obsolete resources
		for (auto& buf : _imageBuffers) {
			vkDestroyImageView( _vkDevice, buf.view, null );
		}
		_imageBuffers.clear();

		if ( old_swapchain != VK_NULL_HANDLE )
			vkDestroySwapchainKHR( _vkDevice, old_swapchain, null );


		_colorFormat		= colorFormat;
		_colorSpace			= colorSpace;
		_minImageCount		= swapchain_info.minImageCount;
		_preTransform		= swapchain_info.preTransform;
		_presentMode		= swapchain_info.presentMode;
		_compositeAlpha		= swapchain_info.compositeAlpha;
		_colorImageUsage	= swapchain_info.imageUsage;
		_surfaceSize.x		= swapchain_info.imageExtent.width;
		_surfaceSize.y		= swapchain_info.imageExtent.height;


		CHECK_ERR( _CreateColorAttachment() );

		return true;
	}
	
/*
=================================================
	_CreateColorAttachment
=================================================
*/
	bool VulkanSwapchain::_CreateColorAttachment ()
	{
		CHECK_ERR( _imageBuffers.empty() );

		FixedArray< VkImage, MaxSwapchainLength >	images;
		
		uint	count = 0;
		VK_CHECK( vkGetSwapchainImagesKHR( _vkDevice, _vkSwapchain, OUT &count, null ));
		CHECK_ERR( count > 0 );
		
		images.resize( count );
		VK_CHECK( vkGetSwapchainImagesKHR( _vkDevice, _vkSwapchain, OUT &count, OUT images.data() ));

		_imageBuffers.resize( count );


		for (size_t i = 0; i < _imageBuffers.size(); ++i)
		{
			VkImageViewCreateInfo	view_info	= {};

			view_info.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_info.pNext			= null;
			view_info.viewType		= VK_IMAGE_VIEW_TYPE_2D;
			view_info.flags			= 0;
			view_info.image			= images[i];
			view_info.format		= _colorFormat;
			view_info.components	= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
										VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

			view_info.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			view_info.subresourceRange.baseMipLevel		= 0;
			view_info.subresourceRange.levelCount		= 1;
			view_info.subresourceRange.baseArrayLayer	= 0;
			view_info.subresourceRange.layerCount		= 1;
		
			VkImageView		img_view;
			VK_CHECK( vkCreateImageView( _vkDevice, &view_info, null, OUT &img_view ) );

			_imageBuffers[i] = { images[i], img_view };
		}

		return true;
	}

/*
=================================================
	Recreate
----
	warning: you must delete all command buffers
	that used swapchain images!
=================================================
*/
	bool VulkanSwapchain::Recreate (const uint2 &size)
	{
		ASSERT( not IsImageAcquired() );

		CHECK_ERR( Create( size, _colorFormat, _colorSpace, _minImageCount, _presentMode,
						   _preTransform, _compositeAlpha, _colorImageUsage ));

		return true;
	}
	
/*
=================================================
	AcquireNextImage
=================================================
*/
	VkResult  VulkanSwapchain::AcquireNextImage (VkSemaphore imageAvailable, VkFence fence)
	{
		CHECK_ERR( _vkDevice and _vkSwapchain and (imageAvailable or fence), VK_RESULT_MAX_ENUM );
		CHECK_ERR( not IsImageAcquired(), VK_RESULT_MAX_ENUM );

		_currImageIndex = UMax;

		return vkAcquireNextImageKHR( _vkDevice, _vkSwapchain, UMax, imageAvailable, fence, OUT &_currImageIndex );
	}
	
/*
=================================================
	Present
=================================================
*/
	VkResult VulkanSwapchain::Present (VkQueue queue, ArrayView<VkSemaphore> renderFinished)
	{
		CHECK_ERR( queue and _vkSwapchain, VK_RESULT_MAX_ENUM );
		CHECK_ERR( IsImageAcquired(), VK_RESULT_MAX_ENUM );

		const VkSwapchainKHR	swap_chains[]		= { _vkSwapchain };
		const uint				image_indices[]		= { _currImageIndex };

		STATIC_ASSERT( CountOf(swap_chains) == CountOf(image_indices) );

		VkPresentInfoKHR	present_info = {};
		present_info.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.swapchainCount		= uint(CountOf( swap_chains ));
		present_info.pSwapchains		= swap_chains;
		present_info.pImageIndices		= image_indices;
		present_info.waitSemaphoreCount	= uint(renderFinished.size());
		present_info.pWaitSemaphores	= renderFinished.data();

		_currImageIndex	= UMax;

		VkResult	res = vkQueuePresentKHR( queue, &present_info );

		_UpdateFPS();
		return res;
	}
	
/*
=================================================
	_UpdateFPS
=================================================
*/
	void VulkanSwapchain::_UpdateFPS ()
	{
		using namespace std::chrono;

		++_frameCounter;

		TimePoint_t		now			= TimePoint_t::clock::now();
		int64_t			duration	= duration_cast<milliseconds>(now - _lastFpsUpdateTime).count();

		if ( duration > _fpsUpdateIntervalMillis )
		{
			_currentFPS			= float(_frameCounter) / float(duration) * 1000.0f;
			_lastFpsUpdateTime	= now;
			_frameCounter		= 0;
		}
	}

/*
=================================================
	GetCurrentImage
=================================================
*/
	VkImage  VulkanSwapchain::GetCurrentImage () const
	{
		if ( _currImageIndex < _imageBuffers.size() )
			return _imageBuffers[_currImageIndex].image;

		return VK_NULL_HANDLE;
	}
	
/*
=================================================
	GetCurrentImageView
=================================================
*/
	VkImageView  VulkanSwapchain::GetCurrentImageView () const
	{
		if ( _currImageIndex < _imageBuffers.size() )
			return _imageBuffers[_currImageIndex].view;

		return VK_NULL_HANDLE;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VulkanSwapchain::Destroy ()
	{
		ASSERT( not IsImageAcquired() );

		if ( _vkDevice )
		{
			for (auto& buf : _imageBuffers)
			{
				vkDestroyImageView( _vkDevice, buf.view, null );
			}

			if ( _vkSwapchain != VK_NULL_HANDLE )
				vkDestroySwapchainKHR( _vkDevice, _vkSwapchain, null );
		}

		_imageBuffers.clear();
		
		_vkPhysicalDevice	= VK_NULL_HANDLE;
		_vkDevice			= VK_NULL_HANDLE;

		_vkSurface			= VK_NULL_HANDLE;
		_vkSwapchain		= VK_NULL_HANDLE;
		_surfaceSize		= uint2();

		_currImageIndex		= UMax;
		
		_colorFormat		= VK_FORMAT_UNDEFINED;
		_colorSpace			= VK_COLOR_SPACE_MAX_ENUM_KHR;
		_minImageCount		= 0;
		_preTransform		= VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR;
		_presentMode		= VK_PRESENT_MODE_MAX_ENUM_KHR;
		_compositeAlpha		= VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;
		_colorImageUsage	= 0;
	}

/*
=================================================
	_GetCompositeAlpha
=================================================
*/
	bool VulkanSwapchain::_GetCompositeAlpha (INOUT VkCompositeAlphaFlagBitsKHR &compositeAlpha, const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		const VkCompositeAlphaFlagBitsKHR		composite_alpha_flags[] = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};
		
		if ( EnumEq( surfaceCaps.supportedCompositeAlpha, compositeAlpha ) )
			return true;	// keep current
		
		compositeAlpha = VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;

		for (auto& flag : composite_alpha_flags)
		{
			if ( EnumEq( surfaceCaps.supportedCompositeAlpha, flag ) )
			{
				compositeAlpha = flag;
				return true;
			}
		}

		RETURN_ERR( "no suitable composite alpha flags found!" );
	}
	
/*
=================================================
	_GetSwapChainExtent
=================================================
*/
	void VulkanSwapchain::_GetSwapChainExtent (INOUT VkExtent2D &extent, const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( surfaceCaps.currentExtent.width  == UMax and
			 surfaceCaps.currentExtent.height == UMax )
		{
			// keep window size
		}
		else
		{
			extent.width  = surfaceCaps.currentExtent.width;
			extent.height = surfaceCaps.currentExtent.height;
		}
	}
	
/*
=================================================
	_GetPresentMode
=================================================
*/
	void VulkanSwapchain::_GetPresentMode (INOUT VkPresentModeKHR &presentMode) const
	{
		uint						count		= 0;
		Array< VkPresentModeKHR >	present_modes;

		VK_CALL( vkGetPhysicalDeviceSurfacePresentModesKHR( _vkPhysicalDevice, _vkSurface, OUT &count, null ));
		CHECK_ERR( count > 0, void() );

		present_modes.resize( count );
		VK_CALL( vkGetPhysicalDeviceSurfacePresentModesKHR( _vkPhysicalDevice, _vkSurface, OUT &count, OUT present_modes.data() ));

		bool	required_mode_supported		= false;
		bool	fifo_mode_supported			= false;
		bool	mailbox_mode_supported		= false;
		bool	immediate_mode_supported	= false;

		for (size_t i = 0; i < present_modes.size(); ++i)
		{
			required_mode_supported		|= (present_modes[i] == presentMode);
			fifo_mode_supported			|= (present_modes[i] == VK_PRESENT_MODE_FIFO_KHR);
			mailbox_mode_supported		|= (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR);
			immediate_mode_supported	|= (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR);
		}

		if ( required_mode_supported )
			return;	// keep current

		if ( mailbox_mode_supported )
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		else
		if ( fifo_mode_supported )
			presentMode = VK_PRESENT_MODE_FIFO_KHR;
		else
		if ( immediate_mode_supported )
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

/*
=================================================
	_GetSurfaceImageCount
=================================================
*/
	void VulkanSwapchain::_GetSurfaceImageCount (INOUT uint &minImageCount, const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( minImageCount < surfaceCaps.minImageCount )
		{
			minImageCount = surfaceCaps.minImageCount;
		}

		if ( surfaceCaps.maxImageCount > 0 and minImageCount > surfaceCaps.maxImageCount )
		{
			minImageCount = surfaceCaps.maxImageCount;
		}
	}
	
/*
=================================================
	_GetSurfaceTransform
=================================================
*/
	void VulkanSwapchain::_GetSurfaceTransform (INOUT VkSurfaceTransformFlagBitsKHR &transform,
												const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( EnumEq( surfaceCaps.supportedTransforms, transform ) )
			return;	// keep current
		
		if ( EnumEq( surfaceCaps.supportedTransforms, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) )
		{
			transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			transform = surfaceCaps.currentTransform;
		}
	}
	
/*
=================================================
	_GetImageUsage
=================================================
*/
	bool VulkanSwapchain::_GetImageUsage (OUT VkImageUsageFlags &imageUsage, const VkPresentModeKHR presentMode,
										  const VkFormat colorFormat, const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR	or
			 presentMode == VK_PRESENT_MODE_MAILBOX_KHR		or
			 presentMode == VK_PRESENT_MODE_FIFO_KHR		or
			 presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR )
		{
			imageUsage = surfaceCaps.supportedUsageFlags;
		}
		else
		if ( presentMode == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR	or
			 presentMode == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR )
		{
			VkPhysicalDeviceSurfaceInfo2KHR	surf_info = {};
			surf_info.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
			surf_info.surface	= _vkSurface;

			VkSurfaceCapabilities2KHR	surf_caps2;
			VK_CALL( vkGetPhysicalDeviceSurfaceCapabilities2KHR( _vkPhysicalDevice, &surf_info, OUT &surf_caps2 ) );

			for (VkBaseInStructure const *iter = reinterpret_cast<VkBaseInStructure const *>(&surf_caps2);
				 iter != null;
				 iter = iter->pNext)
			{
				if ( iter->sType == VK_STRUCTURE_TYPE_SHARED_PRESENT_SURFACE_CAPABILITIES_KHR )
				{
					imageUsage = reinterpret_cast<VkSharedPresentSurfaceCapabilitiesKHR const*>(iter)->sharedPresentSupportedUsageFlags;
					break;
				}
			}
		}
		else
		{
			//RETURN_ERR( "unsupported presentMode, can't choose imageUsage!" );
			return false;
		}

		ASSERT( EnumEq( imageUsage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) );
		imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		

		// validation:
		VkFormatProperties	format_props;
		vkGetPhysicalDeviceFormatProperties( _vkPhysicalDevice, colorFormat, OUT &format_props );

		CHECK_ERR( EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ) );
		ASSERT( EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT ) );
		
		if ( EnumEq( imageUsage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT ) and
			 (not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_TRANSFER_SRC_BIT ) or
			  not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_BLIT_DST_BIT )) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		
		if ( EnumEq( imageUsage, VK_IMAGE_USAGE_TRANSFER_DST_BIT ) and
			 not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_TRANSFER_DST_BIT ) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		
		if ( EnumEq( imageUsage, VK_IMAGE_USAGE_STORAGE_BIT ) and
			 not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT ) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
		}

		if ( EnumEq( imageUsage, VK_IMAGE_USAGE_SAMPLED_BIT ) and
			 not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
		}

		if ( EnumEq( imageUsage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) and
			 not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT ) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		return true;
	}


}	// FGC
