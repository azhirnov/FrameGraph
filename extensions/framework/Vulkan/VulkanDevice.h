// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "framework/Window/IWindow.h"

namespace FG
{
	static constexpr VkQueueFlagBits	VK_QUEUE_PRESENT_BIT = VkQueueFlagBits(0x80000000u);


	//
	// Vulkan Device
	//

	class VulkanDevice final : public VulkanDeviceFn
	{
	// types
	public:
		struct QueueCreateInfo
		{
			VkQueueFlags			flags		= 0;
			float					priority	= 0.0f;
		};

		struct VulkanQueue
		{
			VkQueue					id			= VK_NULL_HANDLE;
			uint					familyIndex	= ~0u;
			uint					queueIndex	= ~0u;
			VkQueueFlags			flags		= 0;
			float					priority	= 0.0f;
		};

		struct DebugReport
		{
			VkDebugReportFlagsEXT	flags;
			StringView				objectType;
			uint64_t				object;
			StringView				layerPrefix;
			StringView				message;
		};

		static constexpr uint	maxQueues = 16;

		using DebugReport_t = std::function< void (const DebugReport &) >;
		using SurfaceCtor_t = std::function< VkSurfaceKHR (VkInstance) >;
		using Queues_t		= FixedArray< VulkanQueue, maxQueues >;


	// variables
	private:
		VkDevice					_vkDevice;
		Queues_t					_vkQueues;

		VkSurfaceKHR				_vkSurface;
		VkPhysicalDevice			_vkPhysicalDevice;
		VkInstance					_vkInstance;

		VkDebugReportCallbackEXT	_debugCallback;
		DebugReport_t				_callback;

		bool						_debugReportSupported;
		bool						_usedSharedInstance;
		
		VkPhysicalDeviceProperties			_deviceProperties;
		VkPhysicalDeviceFeatures			_deviceFeatures;
		VkPhysicalDeviceMemoryProperties	_deviceMemoryProperties;

		VulkanDeviceFnTable			_deviceFnTable;


	// methods
	public:
		VulkanDevice ();
		~VulkanDevice ();
		
		bool Create (UniquePtr<IVulkanSurface> &&surf,
					 StringView					applicationName,
					 uint						version		= VK_API_VERSION_1_1,
					 StringView					deviceName	= Default,
					 ArrayView<QueueCreateInfo>	queues		= Default);

		bool Create (VkInstance					instance,
					 UniquePtr<IVulkanSurface> &&surf,
					 StringView					deviceName	= Default,
					 ArrayView<QueueCreateInfo>	queues		= Default);

		bool CreateDebugCallback (VkDebugReportFlagsEXT flags, DebugReport_t &&callback = Default);
		void Destroy ();
		
		ND_ VkPhysicalDeviceProperties const&		GetDeviceProperties ()		 const	{ return _deviceProperties; }
		ND_ VkPhysicalDeviceFeatures const&			GetDeviceFeatures ()		 const	{ return _deviceFeatures; }
		ND_ VkPhysicalDeviceMemoryProperties const&	GetDeviceMemoryProperties () const	{ return _deviceMemoryProperties; }
		
		ND_ VkInstance								GetVkInstance ()			const	{ return _vkInstance; }
		ND_ VkPhysicalDevice						GetVkPhysicalDevice ()		const	{ return _vkPhysicalDevice; }
		ND_ VkDevice								GetVkDevice ()				const	{ return _vkDevice; }
		ND_ VkSurfaceKHR							GetVkSurface ()				const	{ return _vkSurface; }
		ND_ ArrayView<VulkanQueue>					GetVkQuues ()				const	{ return _vkQueues; }


	private:
		bool _CreateInstance (StringView appName, Array<const char*> &&instanceExtensions, uint version);
		bool _ChooseGpuDevice (StringView deviceName);
		bool _SetupQueues (ArrayView<QueueCreateInfo> queue);
		bool _CreateDevice ();
		bool _ChooseQueueIndex (INOUT VkQueueFlags &flags, OUT uint32_t &index) const;
		void _DestroyDevice ();
		void _WriteDeviceInfo ();

        void _ValidateInstanceLayers (INOUT Array<const char*> &layers) const;
		void _ValidateInstanceExtensions (INOUT Array<const char*> &ext) const;
		void _ValidateDeviceExtensions (INOUT Array<const char*> &ext) const;
		
		static VkBool32 VKAPI_CALL _DebugReportCallback (VkDebugReportFlagsEXT flags,
														 VkDebugReportObjectTypeEXT objectType,
														 uint64_t object,
														 size_t /*location*/,
														 int32_t /*messageCode*/,
														 const char* pLayerPrefix,
														 const char* pMessage,
														 void* pUserData);

		void _DebugReport (VkDebugReportFlagsEXT flags, StringView objectType, uint64_t object, StringView layerPrefix, StringView message) const;
	};


}	// FG
