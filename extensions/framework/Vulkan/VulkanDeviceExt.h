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

		HashSet<String>				_instanceExtensions;
		HashSet<String>				_deviceExtensions;

		bool						_debugReportSupported;
		bool						_debugMarkersSupported;
		bool						_breakOnValidationError;
		
		VkPhysicalDeviceMemoryProperties		_deviceMemoryProperties {};

		struct {
			VkPhysicalDeviceProperties						main {};
			VkPhysicalDeviceIDProperties					deviceID {};
			VkPhysicalDeviceMaintenance3Properties			maintenance3 {};
			VkPhysicalDeviceSubgroupProperties				subgroup {};
			VkPhysicalDeviceMeshShaderPropertiesNV			meshShader {};
			VkPhysicalDeviceShadingRateImagePropertiesNV	shadingRateImage {};
			VkPhysicalDeviceRaytracingPropertiesNVX			rayTracing {};
		}	_properties;


	// methods
	public:
		VulkanDeviceExt ();
		~VulkanDeviceExt ();
		
		bool CreateDebugCallback (VkDebugReportFlagsEXT flags, DebugReport_t &&callback = Default);
		
		bool GetMemoryTypeIndex (uint memoryTypeBits, VkMemoryPropertyFlags flags, OUT uint &memoryTypeIndex) const;
		bool CompareMemoryTypes (uint memoryTypeBits, VkMemoryPropertyFlags flags, uint memoryTypeIndex) const;
		
		bool SetObjectName (uint64_t id, StringView name, VkDebugReportObjectTypeEXT type) const;

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
		ND_ VkPhysicalDeviceRaytracingPropertiesNVX const&		GetDeviceRaytracingProperties ()		const	{ return _properties.rayTracing; }


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
