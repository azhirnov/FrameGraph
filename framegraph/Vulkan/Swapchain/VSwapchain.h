// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
		static constexpr uint		MaxImages = 8;
		using SwapchainImages_t		= FixedArray< ImageID, MaxImages >;


	// variables
	private:
		VDeviceQueueInfoPtr				_presentQueue;
		SwapchainImages_t				_imageIDs;
		uint2							_surfaceSize;

		VkSwapchainKHR					_vkSwapchain		= VK_NULL_HANDLE;
		VkSurfaceKHR					_vkSurface			= VK_NULL_HANDLE;
		mutable uint					_currImageIndex		= UMax;

		StaticArray< VkSemaphore, 2 >	_imageAvailable;
		StaticArray< VkSemaphore, 2 >	_renderFinished;
		VkFence							_fence				= VK_NULL_HANDLE;
		mutable uint					_semaphoreId : 1;

		VkFormat						_colorFormat		= VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR					_colorSpace			= VK_COLOR_SPACE_MAX_ENUM_KHR;
		uint							_minImageCount		= 2;
		VkSurfaceTransformFlagBitsKHR	_preTransform		= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		VkPresentModeKHR				_presentMode		= VK_PRESENT_MODE_FIFO_KHR;
		VkCompositeAlphaFlagBitsKHR		_compositeAlpha		= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		VkImageUsageFlags				_colorImageUsage	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		EQueueFamilyMask				_queueFamilyMask	= Default;
		
		RWDataRaceCheck					_drCheck;


	// methods
	public:
		VSwapchain ();
		VSwapchain (VSwapchain &&) = delete;
		VSwapchain (const VSwapchain &) = delete;
		~VSwapchain ();

		bool  Create (VFrameGraph &, const VulkanSwapchainCreateInfo &, StringView dbgName);
		void  Destroy (VResourceManager &);

		bool  Acquire (VCommandBuffer &, ESwapchainImage type, bool dbgSync, OUT RawImageID &outImageId) const;
		bool  Present (const VDevice &) const;

		ND_ VDeviceQueueInfoPtr  GetPresentQueue ()	const	{ SHAREDLOCK( _drCheck );  return _presentQueue; }


	private:
		bool  _CreateSwapchain (VFrameGraph &, StringView dbgName);
		bool  _CreateImages (VResourceManager &);
		void  _DestroyImages (VResourceManager &);

		bool  _CreateSemaphores (const VDevice &dev);
		bool  _CreateFence (const VDevice &dev);
		bool  _ChoosePresentQueue (const VFrameGraph &);
		
		ND_ bool  _IsImageAcquired () const;
	};


}	// FG
