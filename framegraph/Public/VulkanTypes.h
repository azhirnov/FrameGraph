// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	using InstanceVk_t			= struct __VkInstanceType *;
	using PhysicalDeviceVk_t	= struct __VkPhysicalDeviceType *;
	using DeviceVk_t			= struct __VkDeviceType *;
	using QueueVk_t				= struct __VkQueueType *;
	using CommandBufferVk_t		= struct __VkCommandBufferType *;

	enum SurfaceVk_t			: uint64_t {};
	enum EventVk_t				: uint64_t {};
	enum FenceVk_t				: uint64_t {};
	enum BufferVk_t				: uint64_t {};
	enum ImageVk_t				: uint64_t {};
	enum ShaderModuleVk_t		: uint64_t {};
	enum SemaphoreVk_t			: uint64_t {};

	enum CompositeAlphaFlagBitsVk_t	: uint {};
	enum SurfaceTransformFlagsVk_t	: uint {};
	enum PresentModeVk_t			: uint {};
	enum QueueFlagsVk_t				: uint {};
	enum FormatVk_t					: uint {};
	enum ColorSpaceVk_t				: uint {};
	enum ImageUsageVk_t				: uint {};
	enum BufferUsageFlagsVk_t		: uint {};
	enum ImageLayoutVk_t			: uint {};
	enum ImageTypeVk_t				: uint {};
	enum SampleCountFlagBitsVk_t	: uint {};
	enum PipelineStageFlags_t		: uint {};



	//
	// Vulkan Device Info
	//
	struct VulkanDeviceInfo
	{
	// types
		struct QueueInfo
		{
			QueueVk_t			id				= null;
			uint				familyIndex		= ~0u;
			QueueFlagsVk_t		familyFlags		= {};
			float				priority		= 0.0f;
			StaticString<64>	debugName;
		};
		using Queues_t = FixedArray< QueueInfo, 16 >;

	// variables
		InstanceVk_t		instance			= null;
		PhysicalDeviceVk_t	physicalDevice		= null;
		DeviceVk_t			device				= null;
		Queues_t			queues;
	};


	//
	// Vulkan Swapchain Create Info
	//
	struct VulkanSwapchainCreateInfo
	{
	// types
		using RequiredColorFormats_t	= FixedArray< Pair<FormatVk_t, ColorSpaceVk_t>, 4 >;
		using RequiredPresentMode_t		= FixedArray< PresentModeVk_t, 4 >;

	// variables
		SurfaceVk_t						surface			= {};
		uint2							surfaceSize;
		RequiredColorFormats_t			formats;		// from highest priority to lowest
		RequiredPresentMode_t			presentModes;	// from highest priority to lowest
		SurfaceTransformFlagsVk_t		preTransform	= {};
		CompositeAlphaFlagBitsVk_t		compositeAlpha	= {};
		ImageUsageVk_t					requiredUsage	= {};
		ImageUsageVk_t					optionalUsage	= {};
	};


	//
	// Vulkan VR Emulator Swapchain Create Info
	//
	struct VulkanVREmulatorSwapchainCreateInfo
	{
	// types
		using RequiredColorFormats_t = FixedArray< FormatVk_t, 4 >;

	// variables
		SurfaceVk_t					surface			= {};
		uint2						eyeImageSize	= { 1024, 1024 };
		RequiredColorFormats_t		formats;		// from highest priority to lowest
	};


	//
	// Vulkan Image Description
	//
	struct VulkanImageDesc
	{
		ImageVk_t				image		= {};
		ImageTypeVk_t			imageType	= {};
		ImageUsageVk_t			usage		= {};
		FormatVk_t				format		= {};
		ImageLayoutVk_t			layout		= {};
		SampleCountFlagBitsVk_t	samples		= {};
		uint3					dimension;
		uint					arrayLayers	= 0;
		uint					maxLevels	= 0;

		// queue family that owns image
		uint					queueFamily	= ~0u;

		// wait semaphore before use image
		SemaphoreVk_t			semaphore	= {};
	};


	//
	// Vulkan Buffer Description
	//
	struct VulkanBufferDesc
	{
		BufferVk_t				buffer		= {};
		BufferUsageFlagsVk_t	usage		= {};
		BytesU					size;
		
		// queue family that owns buffer
		uint					queueFamily	= ~0u;

		// wait semaphore before use buffer
		SemaphoreVk_t			semaphore	= {};
	};


	//
	// Vulkan Command Batch
	//
	struct VulkanCommandBatch
	{
		QueueVk_t												queue = {};
		ArrayView<CommandBufferVk_t>							commands;
		ArrayView<SemaphoreVk_t>								signalSemaphores;
		ArrayView<Pair<SemaphoreVk_t, PipelineStageFlags_t>>	waitSemaphores;
	};


}	// FG
