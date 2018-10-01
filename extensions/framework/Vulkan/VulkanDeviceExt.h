// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Vulkan/VulkanDevice.h"

namespace FG
{

	//
	// Vulkan Device (extended)
	//

	class VulkanDeviceExt : public VulkanDevice
	{
	// types
	public:
		struct DebugReport
		{
			VkDebugReportFlagsEXT	flags;
			StringView				objectType;
			uint64_t				object;
			StringView				layerPrefix;
			StringView				message;
		};

		using DebugReport_t = std::function< void (const DebugReport &) >;


	// variables
	private:
		VkDebugReportCallbackEXT	_debugCallback;
		DebugReport_t				_callback;

		bool						_debugReportSupported;
		bool						_debugMarkersSupported;
		bool						_breakOnValidationError;
		
		VkPhysicalDeviceProperties				_deviceProperties;
		VkPhysicalDeviceFeatures				_deviceFeatures;
		VkPhysicalDeviceMemoryProperties		_deviceMemoryProperties;
		VkPhysicalDeviceIDProperties			_deviceIDProperties;
		VkPhysicalDeviceMaintenance3Properties	_deviceMaintenance3Properties;
		VkPhysicalDeviceSubgroupProperties		_deviceSubgroupProperties;


	// methods
	public:
		VulkanDeviceExt ();
		~VulkanDeviceExt ();
		
		bool CreateDebugCallback (VkDebugReportFlagsEXT flags, DebugReport_t &&callback = Default);
		
		bool GetMemoryTypeIndex (uint memoryTypeBits, VkMemoryPropertyFlags flags, OUT uint &memoryTypeIndex) const;
		bool CompareMemoryTypes (uint memoryTypeBits, VkMemoryPropertyFlags flags, uint memoryTypeIndex) const;
		
		bool SetObjectName (uint64_t id, StringView name, VkDebugReportObjectTypeEXT type) const;

		ND_ VkPhysicalDeviceProperties const&		GetDeviceProperties ()		 const	{ return _deviceProperties; }
		ND_ VkPhysicalDeviceFeatures const&			GetDeviceFeatures ()		 const	{ return _deviceFeatures; }
		ND_ VkPhysicalDeviceMemoryProperties const&	GetDeviceMemoryProperties () const	{ return _deviceMemoryProperties; }
		

	private:
		void _OnInstanceCreated (Array<const char*> &&instanceLayers, Array<const char*> &&instanceExtensions) override;
		void _OnLogicalDeviceCreated (Array<const char *> &&extensions) override;
		void _BeforeDestroy () override;
		void _AfterDestroy () override;
		
		void _WriteDeviceInfo ();

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
