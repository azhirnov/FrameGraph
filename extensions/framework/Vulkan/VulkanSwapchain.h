// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Vulkan/VulkanDevice.h"
#include <chrono>

namespace FG
{

	//
	// Vulkan Swapchain
	//

	class VulkanSwapchain final : public VulkanDeviceFn
	{
	// types
	private:
		struct SwapchainBuffer
		{
			VkImage			image;
			VkImageView		view;
		};
		using SwapChainBuffers_t	= FixedArray< SwapchainBuffer, FG_MaxSwapchainLength >;

		using TimePoint_t			= std::chrono::time_point< std::chrono::high_resolution_clock >;


	// variables
	private:
		VkSwapchainKHR			_vkSwapchain;
		SwapChainBuffers_t		_imageBuffers;
		uint2					_surfaceSize;

		VkPhysicalDevice		_vkPhysicalDevice;
		VkDevice				_vkDevice;
		VkSurfaceKHR			_vkSurface;

		VkSemaphore				_imageAvailable;
		VkSemaphore				_renderFinished;
		uint					_currImageIndex;

		VkFormat						_colorFormat;
		VkColorSpaceKHR					_colorSpace;
		uint							_minImageCount;
		uint							_imageArrayLayers;
		VkSurfaceTransformFlagBitsKHR	_preTransform;
		VkPresentModeKHR				_presentMode;
		VkCompositeAlphaFlagBitsKHR		_compositeAlpha;
		VkImageUsageFlags				_colorImageUsage;

		// FPS counter
		TimePoint_t						_lastFpsUpdateTime;
		uint							_frameCounter;
		float							_currentFPS;
		static constexpr uint			_fpsUpdateIntervalMillis	= 500;


	// methods
	public:
		explicit VulkanSwapchain (const VulkanDevice &dev);
		VulkanSwapchain (VkPhysicalDevice physicalDev, VkDevice logicalDev, VkSurfaceKHR surface);
		~VulkanSwapchain ();

		bool IsSupported (uint imageArrayLayers, VkSampleCountFlags samples, VkPresentModeKHR presentMode,
						  VkFormat colorFormat, VkImageUsageFlags colorImageUsage) const;
		
		bool ChooseColorFormat (INOUT VkFormat &colorFormat, INOUT VkColorSpaceKHR &colorSpace) const;

		bool Create (const uint2							&viewSize,
					 const VkFormat							colorFormat			= VK_FORMAT_B8G8R8A8_UNORM,
					 const VkColorSpaceKHR					colorSpace			= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
					 const uint								minImageCount		= 2,
					 const uint								imageArrayLayers	= 1,
					 const VkSurfaceTransformFlagBitsKHR	transform			= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
					 const VkPresentModeKHR					presentMode			= VK_PRESENT_MODE_FIFO_KHR,
					 const VkCompositeAlphaFlagBitsKHR		compositeAlpha		= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
					 const VkImageUsageFlags				colorImageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);

		void Destroy ();

		bool Recreate (const uint2 &size);

		ND_ VkResult  AcquireNextImage ();
		ND_ VkResult  AcquireNextImage (VkSemaphore imageAvailable);

		bool Present (VkQueue queue);
		bool Present (VkQueue queue, VkSemaphore renderFinished);


		ND_ VkSwapchainKHR				GetVkSwapchain ()				const	{ return _vkSwapchain; }
		ND_ VkSemaphore					GetImageAvailableSemaphore ()	const	{ return _imageAvailable; }
		ND_ VkSemaphore					GetRenderFinishedSemaphore ()	const	{ return _renderFinished; }

		ND_ uint2 const&				GetSurfaceSize ()				const	{ return _surfaceSize; }
		ND_ VkPresentModeKHR			GetPresentMode ()				const	{ return _presentMode; }
		ND_ VkCompositeAlphaFlagBitsKHR	GetCompositeAlphaMode ()		const	{ return _compositeAlpha; }

		ND_ uint						GetSwapchainLength ()			const	{ return uint(_imageBuffers.size()); }
		ND_ uint						GetCurretImageIndex ()			const	{ return _currImageIndex; }
		ND_ bool						IsImageAcquired ()				const	{ return GetCurretImageIndex() < GetSwapchainLength(); }

		ND_ VkImageUsageFlags			GetImageUsage ()				const	{ return _colorImageUsage; }
		ND_ VkImage						GetCurrentImage ()				const;
		ND_ VkImageView					GetCurrentImageView ()			const;

		ND_ float						GetFramesPerSecond ()			const	{ return _currentFPS; }


	private:
		VulkanSwapchain ();

		bool _GetImageUsage (OUT VkImageUsageFlags &imageUsage,	VkPresentModeKHR presentMode, VkFormat colorFormat, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		bool _GetCompositeAlpha (INOUT VkCompositeAlphaFlagBitsKHR &compositeAlpha, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		void _GetSharingMode (OUT VkSharingMode &sharingMode) const;
		void _GetPresentMode (INOUT VkPresentModeKHR &presentMode) const;
		void _GetSwapChainExtent (INOUT VkExtent2D &extent, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		void _GetSurfaceTransform (INOUT VkSurfaceTransformFlagBitsKHR &transform, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		void _GetSurfaceImageCount (INOUT uint &minImageCount, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;

		bool _CreateColorAttachment ();
		bool _CreateSemaphores ();

		void _UpdateFPS ();
	};


	using VulkanSwapchainPtr	= UniquePtr< VulkanSwapchain >;

}	// FG
