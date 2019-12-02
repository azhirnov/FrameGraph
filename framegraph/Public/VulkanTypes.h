// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	using InstanceVk_t			= struct __VkInstanceType *;
	using PhysicalDeviceVk_t	= struct __VkPhysicalDeviceType *;
	using DeviceVk_t			= struct __VkDeviceType *;
	using QueueVk_t				= struct __VkQueueType *;
	using CommandBufferVk_t		= struct __VkCommandBufferType *;

	enum RenderPassVk_t			: uint64_t {};
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
	enum ImageFlagsVk_t				: uint {};
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
			QueueVk_t			handle			= null;
			uint				familyIndex		= UMax;
			QueueFlagsVk_t		familyFlags		= {};
			float				priority		= 0.0f;
			StringView			debugName;
		};
		using Queues_t = FixedArray< QueueInfo, 8 >;

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
		uint							minImageCount	= 2;
		RequiredColorFormats_t			formats;		// from highest priority to lowest
		RequiredPresentMode_t			presentModes;	// from highest priority to lowest
		SurfaceTransformFlagsVk_t		preTransform	= {};
		CompositeAlphaFlagBitsVk_t		compositeAlpha	= {};
		ImageUsageVk_t					requiredUsage	= {};
		ImageUsageVk_t					optionalUsage	= {};
	};



	//
	// Vulkan Image Description
	//
	struct VulkanImageDesc
	{
		ImageVk_t				image			= {};
		ImageTypeVk_t			imageType		= {};
		ImageFlagsVk_t			flags			= {};
		ImageUsageVk_t			usage			= {};
		FormatVk_t				format			= {};
		ImageLayoutVk_t			currentLayout	= {};
		ImageLayoutVk_t			defaultLayout	= ImageLayoutVk_t(0x7FFFFFFF);
		SampleCountFlagBitsVk_t	samples			= {};
		uint3					dimension;
		uint					arrayLayers		= 0;
		uint					maxLevels		= 0;
		uint					queueFamily		= UMax;	// queue family that owns image, you must specify this correctly
														// if image created with exclusive sharing mode and you need to
														// keep current content of the image, otherwise keep default value.

		ArrayView<uint>			queueFamilyIndices;		// required if sharing mode is concurent.
	};



	//
	// Vulkan Buffer Description
	//
	struct VulkanBufferDesc
	{
		BufferVk_t				buffer		= {};
		BufferUsageFlagsVk_t	usage		= {};
		BytesU					size;
		
		uint					queueFamily	= UMax;		// queue family that owns buffer, you must specify this correctly
														// if buffer created with exclusive sharing mode and you need to
														// keep current content of the buffer, otherwise keep default value.

		ArrayView<uint>			queueFamilyIndices;		// required if sharing mode is concurent.
	};



	//
	// Vulkan Command Batch
	//
	struct VulkanCommandBatch
	{
		uint													queueFamilyIndex	= UMax;
		ArrayView<CommandBufferVk_t>							commands;
		ArrayView<SemaphoreVk_t>								signalSemaphores;
		ArrayView<Pair<SemaphoreVk_t, PipelineStageFlags_t>>	waitSemaphores;
	};

	

	//
	// Vulkan Context
	//
	struct VulkanContext
	{
		uint				queueFamilyIndex	= UMax;
		CommandBufferVk_t	commandBuffer		= {};
	};



	//
	// Vulkan Draw Context
	//
	struct VulkanDrawContext
	{
		uint				queueFamilyIndex	= UMax;
		CommandBufferVk_t	commandBuffer		= {};
		RenderPassVk_t		renderPass			= {};
		uint				subpassIndex		= UMax;
	};


}	// FG
