// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "framegraph/Public/LowLevel/Types.h"

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

	enum VkSurfaceTransformFlags_t : uint64_t {};
	enum VkQueueFlags_t : uint32_t {};

	

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
		VkSurface_t					surface				= {};
		FixedArray<QueueInfo, 16>	queues;

		// required settings
		VkSurfaceTransformFlags_t	surfaceTransform	= {};
	};


}	// FG
