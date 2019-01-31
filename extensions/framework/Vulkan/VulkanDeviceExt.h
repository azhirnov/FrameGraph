// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
		struct ObjectDbgInfo
		{
			StringView				type;
			StringView				name;
			uint64_t				handle;
		};

		struct DebugReport
		{
			ArrayView<ObjectDbgInfo>	objects;
			StringView					message;
			bool						isError		= false;
		};

		using DebugReport_t = std::function< void (const DebugReport &) >;


	// variables
	private:
		VkDebugReportCallbackEXT	_debugReportCallback	= VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT	_debugUtilsMessenger	= VK_NULL_HANDLE;

		DebugReport_t				_callback;

		HashSet<String>				_instanceExtensions;
		HashSet<String>				_deviceExtensions;

		bool						_debugReportSupported	= false;
		bool						_debugUtilsSupported	= false;
		bool						_debugMarkersSupported	= false;

		bool						_breakOnValidationError	= true;
		
		Array<ObjectDbgInfo>		_tempObjectDbgInfos;
		String						_tempString;

		VkPhysicalDeviceMemoryProperties		_deviceMemoryProperties {};

		struct {
			VkPhysicalDeviceProperties						main;
			VkPhysicalDeviceIDProperties					deviceID;
			VkPhysicalDeviceMaintenance3Properties			maintenance3;
			VkPhysicalDeviceSubgroupProperties				subgroup;
			VkPhysicalDeviceMeshShaderPropertiesNV			meshShader;
			VkPhysicalDeviceShadingRateImagePropertiesNV	shadingRateImage;
			VkPhysicalDeviceRayTracingPropertiesNV			rayTracing;

		}	_properties;


	// methods
	public:
		VulkanDeviceExt ();
		~VulkanDeviceExt ();
		
		bool CreateDebugReportCallback (VkDebugReportFlagsEXT flags, DebugReport_t &&callback = Default);
		bool CreateDebugUtilsCallback (VkDebugUtilsMessageSeverityFlagsEXT severity, DebugReport_t &&callback = Default);
		
		bool GetMemoryTypeIndex (uint memoryTypeBits, VkMemoryPropertyFlags flags, OUT uint &memoryTypeIndex) const;
		bool CompareMemoryTypes (uint memoryTypeBits, VkMemoryPropertyFlags flags, uint memoryTypeIndex) const;
		
		bool SetObjectName (uint64_t id, StringView name, VkObjectType type) const;

		void SetBreakOnValidationError (bool value);

		ND_ bool HasInstanceExtension (const String &name)	const	{ return !!_instanceExtensions.count( name ); }
		ND_ bool HasDeviceExtension (const String &name)	const	{ return !!_deviceExtensions.count( name ); }

		ND_ VkPhysicalDeviceProperties const&					GetDeviceProperties ()					const	{ return _properties.main; }
		ND_ VkPhysicalDeviceMemoryProperties const&				GetDeviceMemoryProperties ()			const	{ return _deviceMemoryProperties; }
		ND_ VkPhysicalDeviceIDProperties const&					GetDeviceIDProperties ()				const	{ return _properties.deviceID; }
		ND_ VkPhysicalDeviceMaintenance3Properties const&		GetDeviceMaintenance3Properties ()		const	{ return _properties.maintenance3; }
		ND_ VkPhysicalDeviceSubgroupProperties const&			GetDeviceSubgroupProperties ()			const	{ return _properties.subgroup; }
		ND_ VkPhysicalDeviceMeshShaderPropertiesNV const&		GetDeviceMeshShaderProperties ()		const	{ return _properties.meshShader; }
		ND_ VkPhysicalDeviceShadingRateImagePropertiesNV const&	GetDeviceShadingRateImageProperties ()	const	{ return _properties.shadingRateImage; }
		ND_ VkPhysicalDeviceRayTracingPropertiesNV const&		GetDeviceRayTracingProperties ()		const	{ return _properties.rayTracing; }


	private:
		void _OnInstanceCreated (Array<const char*> &&instanceLayers, Array<const char*> &&instanceExtensions) override;
		void _OnLogicalDeviceCreated (Array<const char *> &&extensions) override;
		void _BeforeDestroy () override;
		void _AfterDestroy () override;
		
		void _WriteDeviceInfo ();

		VKAPI_ATTR static VkBool32 VKAPI_CALL
			_DebugReportCallback (VkDebugReportFlagsEXT flags,
								  VkDebugReportObjectTypeEXT objectType,
								  uint64_t object,
								  size_t location,
								  int32_t messageCode,
								  const char* pLayerPrefix,
								  const char* pMessage,
								  void* pUserData);

		VKAPI_ATTR static VkBool32 VKAPI_CALL
			_DebugUtilsCallback (VkDebugUtilsMessageSeverityFlagBitsEXT			messageSeverity,
								 VkDebugUtilsMessageTypeFlagsEXT				messageTypes,
								 const VkDebugUtilsMessengerCallbackDataEXT*	pCallbackData,
								 void*											pUserData);

		void _DebugReport (const DebugReport &);
	};

	
	static constexpr VkDebugReportFlagsEXT		DebugReportFlags_All =	//VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
																		VK_DEBUG_REPORT_WARNING_BIT_EXT |
																		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
																		VK_DEBUG_REPORT_ERROR_BIT_EXT |
																		VK_DEBUG_REPORT_DEBUG_BIT_EXT;

	static constexpr VkDebugUtilsMessageSeverityFlagsEXT	DebugUtilsMessageSeverity_All = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
																							//VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
																							VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
																							VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

}	// FG
