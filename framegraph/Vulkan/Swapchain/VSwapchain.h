// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Default Swapchain (KHR)
	//

	class VSwapchain
	{
	// types
	private:
		using SwapchainImages_t		= FixedArray< ImageID, 8 >;


	// variables
	protected:
		VDeviceQueueInfoPtr				_presentQueue;
		SwapchainImages_t				_imageIDs;
		uint2							_surfaceSize;

		VkSwapchainKHR					_vkSwapchain		= VK_NULL_HANDLE;
		VkSurfaceKHR					_vkSurface			= VK_NULL_HANDLE;
		uint							_currImageIndex		= UMax;
		VkSemaphore						_imageAvailable		= VK_NULL_HANDLE;
		VkSemaphore						_renderFinished		= VK_NULL_HANDLE;

		VkFormat						_colorFormat		= VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR					_colorSpace			= VK_COLOR_SPACE_MAX_ENUM_KHR;
		uint							_minImageCount		= 2;
		VkSurfaceTransformFlagBitsKHR	_preTransform		= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		VkPresentModeKHR				_presentMode		= VK_PRESENT_MODE_FIFO_KHR;
		VkCompositeAlphaFlagBitsKHR		_compositeAlpha		= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		VkImageUsageFlags				_colorImageUsage	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		EQueueFamilyMask				_queueFamilyMask	= Default;


	// methods
	public:
		explicit VSwapchain ();
		~VSwapchain ();

		bool Create (VFrameGraph &, const VulkanSwapchainCreateInfo &, StringView dbgName);
		void Destroy (VResourceManager &);

		bool Acquire (const VDevice &dev, ESwapchainImage type, OUT RawImageID &outImageId);
		bool Present (const VDevice &dev, RawImageID);


	private:
		bool _CreateSwapchain (VFrameGraph &, StringView dbgName);
		bool _CreateImages (VResourceManager &);
		void _DestroyImages (VResourceManager &);

		bool _CreateSemaphores (const VDevice &dev);
		bool _ChoosePresentQueue (const VFrameGraph &);
		/*
		bool _IsSupported (const VkSurfaceCapabilities2KHR &surfaceCaps, const uint2 &surfaceSize, VkPresentModeKHR presentMode,
						   VkFormat colorFormat, INOUT VkImageUsageFlags &colorImageUsage) const;

		bool _GetImageUsage (OUT VkImageUsageFlags &imageUsage,	VkPresentModeKHR presentMode,
							 VkFormat colorFormat, const VkSurfaceCapabilities2KHR &surfaceCaps) const;
		*/
		ND_ bool _IsImageAcquired () const;
	};


}	// FG
