// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	using VkInstance_t			= struct __VkInstanceType *;
	using VkPhysicalDevice_t	= struct __VkPhysicalDeviceType *;
	using VkDevice_t			= struct __VkDeviceType *;
	using VkQueue_t				= struct __VkQueueType *;
	using VkCommandBuffer_t		= struct __VkCommandBufferType *;

	enum VkSurface_t			: uint64_t {};
	enum VkEvent_t				: uint64_t {};
	enum VkFence_t				: uint64_t {};
	enum VkBuffer_t				: uint64_t {};
	enum VkImage_t				: uint64_t {};
	enum VkShaderModule_t		: uint64_t {};

	enum VkCompositeAlphaFlagBits_t : uint {};
	enum VkSurfaceTransformFlags_t : uint {};
	enum VkPresentModeKHR_t : uint {};
	enum VkQueueFlags_t : uint {};

	

	//
	// Vulkan Device Info
	//
	struct VulkanDeviceInfo
	{
	// types
		struct QueueInfo
		{
			VkQueue_t			id				= null;
			uint				familyIndex		= ~0u;
			VkQueueFlags_t		familyFlags		= {};
			float				priority		= 0.0f;
			StaticString<64>	debugName;
		};

	// variables
		VkInstance_t				instance			= null;
		VkPhysicalDevice_t			physicalDevice		= null;
		VkDevice_t					device				= null;
		FixedArray<QueueInfo, 16>	queues;
	};


	//
	// Vulkan Swapchain Info
	//
	struct VulkanSwapchainInfo
	{
		VkSurface_t							surface			= {};
		uint2								surfaceSize;
		FixedArray<VkPresentModeKHR_t,4>	presentModes;	// from highest priority to lowest
		VkSurfaceTransformFlags_t			preTransform	= {};
		VkCompositeAlphaFlagBits_t			compositeAlpha	= {};
	};


	//
	// Vulkan VR Emulator Swapchain Info
	//
	struct VulkanVREmulatorSwapchainInfo
	{
		VkSurface_t			surface			= {};
		uint2				eyeImageSize	= { 1024, 1024 };
	};


}	// FG
