// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"
#include "stl/Containers/FixedArray.h"
#include "extensions/vulkan_loader/VulkanLoader.h"
#include "extensions/vulkan_loader/VulkanCheckError.h"
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
		static constexpr uint		MaxSwapchainLength = 8;

		using SwapChainBuffers_t	= FixedArray< SwapchainBuffer, MaxSwapchainLength >;

		using TimePoint_t			= std::chrono::time_point< std::chrono::high_resolution_clock >;


	// variables
	private:
		VkSwapchainKHR			_vkSwapchain;
		SwapChainBuffers_t		_imageBuffers;
		uint2					_surfaceSize;

		VkPhysicalDevice		_vkPhysicalDevice;
		VkDevice				_vkDevice;
		VkSurfaceKHR			_vkSurface;

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
		explicit VulkanSwapchain (const class VulkanDevice &dev);
		VulkanSwapchain (VkPhysicalDevice physicalDev, VkDevice logicalDev, VkSurfaceKHR surface, const VulkanDeviceFn &fn);
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
					 const VkImageUsageFlags				colorImageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
					 ArrayView<uint>						queueFamilyIndices	= {});

		void Destroy ();

		bool Recreate (const uint2 &size);

		ND_ VkResult  AcquireNextImage (VkSemaphore imageAvailable);
		ND_ VkResult  Present (VkQueue queue, ArrayView<VkSemaphore> renderFinished);


		ND_ VkPhysicalDevice			GetVkPhysicalDevice ()			const	{ return _vkPhysicalDevice; }
		ND_ VkDevice					GetVkDevice ()					const	{ return _vkDevice; }
		ND_ VkSurfaceKHR				GetVkSurface ()					const	{ return _vkSurface; }

		ND_ VkSwapchainKHR				GetVkSwapchain ()				const	{ return _vkSwapchain; }

		ND_ uint2 const&				GetSurfaceSize ()				const	{ return _surfaceSize; }
		ND_ VkFormat					GetColorFormat ()				const	{ return _colorFormat; }
		ND_ VkColorSpaceKHR				GetColorSpace ()				const	{ return _colorSpace; }
		ND_ VkSurfaceTransformFlagBitsKHR	GetPreTransformFlags ()		const	{ return _preTransform; }
		ND_ VkPresentModeKHR			GetPresentMode ()				const	{ return _presentMode; }
		ND_ VkCompositeAlphaFlagBitsKHR	GetCompositeAlphaMode ()		const	{ return _compositeAlpha; }

		ND_ uint						GetSwapchainLength ()			const	{ return uint(_imageBuffers.size()); }
		ND_ uint						GetCurretImageIndex ()			const	{ return _currImageIndex; }
		ND_ bool						IsImageAcquired ()				const	{ return GetCurretImageIndex() < GetSwapchainLength(); }

		ND_ VkImage						GetImage (uint index)			const	{ return _imageBuffers[index].image; }
		ND_ VkImageView					GetImageView (uint index)		const	{ return _imageBuffers[index].view; }

		ND_ VkImageUsageFlags			GetImageUsage ()				const	{ return _colorImageUsage; }
		ND_ VkImage						GetCurrentImage ()				const;
		ND_ VkImageView					GetCurrentImageView ()			const;

		ND_ float						GetFramesPerSecond ()			const	{ return _currentFPS; }


	private:
		VulkanSwapchain ();

		bool _GetImageUsage (OUT VkImageUsageFlags &imageUsage,	VkPresentModeKHR presentMode, VkFormat colorFormat, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		bool _GetCompositeAlpha (INOUT VkCompositeAlphaFlagBitsKHR &compositeAlpha, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		void _GetPresentMode (INOUT VkPresentModeKHR &presentMode) const;
		void _GetSwapChainExtent (INOUT VkExtent2D &extent, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		void _GetSurfaceTransform (INOUT VkSurfaceTransformFlagBitsKHR &transform, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;
		void _GetSurfaceImageCount (INOUT uint &minImageCount, const VkSurfaceCapabilitiesKHR &surfaceCaps) const;

		bool _CreateColorAttachment ();

		void _UpdateFPS ();
	};


	using VulkanSwapchainPtr	= UniquePtr< VulkanSwapchain >;

}	// FG
