// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VSwapchain.h"

namespace FG
{

	//
	// Vulkan Default Swapchain (KHR)
	//

	class VSwapchainKHR final : public VSwapchain
	{
	// types
	private:
		using SwapchainImages_t		= FixedArray< ImageID, FG_MaxRingBufferSize >;


	// variables
	protected:
		VFrameGraphThread &				_frameGraph;
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


	// methods
	public:
		explicit VSwapchainKHR (VFrameGraphThread &fg);
		~VSwapchainKHR () override;

		bool Create (const VulkanSwapchainCreateInfo &info, uint minImageCount);


	// VSwapchain
	public:
		bool Acquire (ESwapchainImage type, OUT RawImageID &outImageId) override;
		bool Present (RawImageID) override;
		
		void Deinitialize () override;


	private:
		bool _CreateSwapchain ();
		bool _CreateImages ();
		void _DestroyImages ();

		bool _CreateSemaphores ();
		bool _ChoosePresentQueue ();

		bool _IsSupported (const VkSurfaceCapabilities2KHR &surfaceCaps, const uint2 &surfaceSize, VkPresentModeKHR presentMode,
						   VkFormat colorFormat, INOUT VkImageUsageFlags &colorImageUsage) const;

		bool _GetImageUsage (OUT VkImageUsageFlags &imageUsage,	VkPresentModeKHR presentMode,
							 VkFormat colorFormat, const VkSurfaceCapabilities2KHR &surfaceCaps) const;

		ND_ bool _IsImageAcquired () const;

		ND_ VDevice const&		_GetDevice ()			const;
		ND_ VkDevice			_GetVkDevice ()			const;
		ND_ VkPhysicalDevice	_GetVkPhysicalDevice ()	const;
	};


}	// FG
