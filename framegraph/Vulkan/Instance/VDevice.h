// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "framegraph/Public/ShaderEnums.h"

namespace FG
{

	//
	// Device Queue
	//

	struct VDeviceQueueInfo
	{
	// variables
		mutable std::mutex	lock;			// use when call vkQueueSubmit, vkQueueWaitIdle, vkQueueBindSparse, vkQueuePresentKHR
		VkQueue				handle			= VK_NULL_HANDLE;
		EQueueFamily		familyIndex		= Default;
		VkQueueFlags		familyFlags		= {};
		float				priority		= 0.0f;
		uint3				minImageTransferGranularity;
		DebugName_t			debugName;
		
	// methods
		VDeviceQueueInfo () {}

		VDeviceQueueInfo (VDeviceQueueInfo &&other) :
			handle{other.handle}, familyIndex{other.familyIndex}, familyFlags{other.familyFlags},
			priority{other.priority}, minImageTransferGranularity{other.minImageTransferGranularity},
			debugName{std::move(other.debugName)}
		{}
	};



	//
	// Vulkan Device
	//

	class VDevice final : public VulkanDeviceFn
	{
		friend class VFrameGraphInstance;

	// types
	private:
		using Queues_t				= FixedArray< VDeviceQueueInfo, 16 >;
		using InstanceLayers_t		= Array< VkLayerProperties >;
		using ExtensionName_t		= StaticString<VK_MAX_EXTENSION_NAME_SIZE>;
		using ExtensionSet_t		= HashSet< ExtensionName_t >;


	// variables
	private:
		VkInstance								_vkInstance;
		VkPhysicalDevice						_vkPhysicalDevice;
		VkDevice								_vkDevice;
		EShaderLangFormat						_vkVersion;
		Queues_t								_vkQueues;

		struct {
			VkPhysicalDeviceProperties						properties;
			VkPhysicalDeviceFeatures						features;
			VkPhysicalDeviceMemoryProperties				memoryProperties;
			VkPhysicalDeviceMeshShaderFeaturesNV			meshShaderFeatures;
			VkPhysicalDeviceMeshShaderPropertiesNV			meshShaderProperties;
			VkPhysicalDeviceShadingRateImageFeaturesNV		shadingRateImageFeatures;
			VkPhysicalDeviceShadingRateImagePropertiesNV	shadingRateImageProperties;
			VkPhysicalDeviceRayTracingPropertiesNV			rayTracingProperties;
		}										_deviceInfo;

		mutable ExtensionSet_t					_instanceExtensions;
		mutable ExtensionSet_t					_deviceExtensions;
		
		VulkanDeviceFnTable						_deviceFnTable;

		bool									_enableDebugUtils			: 1;
		bool									_enableMeshShaderNV			: 1;
		bool									_enableRayTracingNV			: 1;
		bool									_samplerMirrorClamp			: 1;
		bool									_enableShadingRateImageNV	: 1;


	// methods
	public:
		explicit VDevice (const VulkanDeviceInfo &vdi);
		~VDevice ();

		ND_ bool							IsDebugUtilsEnabled ()			const	{ return _enableDebugUtils; }
		ND_ bool							IsMeshShaderEnabled ()			const	{ return _enableMeshShaderNV; }
		ND_ bool							IsRayTracingEnabled ()			const	{ return _enableRayTracingNV; }
		ND_ bool							IsSamplerMirrorClampEnabled ()	const	{ return _samplerMirrorClamp; }
		ND_ bool							IsShadingRateImageEnabled ()	const	{ return _enableShadingRateImageNV; }

		ND_ VkDevice						GetVkDevice ()					const	{ return _vkDevice; }
		ND_ VkPhysicalDevice				GetVkPhysicalDevice ()			const	{ return _vkPhysicalDevice; }
		ND_ VkInstance						GetVkInstance ()				const	{ return _vkInstance; }
		ND_ EShaderLangFormat				GetVkVersion ()					const	{ return _vkVersion; }
		ND_ ArrayView< VDeviceQueueInfo >	GetVkQueues ()					const	{ return _vkQueues; }
		

		ND_ VkPhysicalDeviceProperties const&					GetDeviceProperties ()					const	{ return _deviceInfo.properties; }
		ND_ VkPhysicalDeviceFeatures const&						GetDeviceFeatures ()					const	{ return _deviceInfo.features; }
		ND_ VkPhysicalDeviceMemoryProperties const&				GetDeviceMemoryProperties ()			const	{ return _deviceInfo.memoryProperties; }
		ND_ VkPhysicalDeviceLimits const&						GetDeviceLimits ()						const	{ return _deviceInfo.properties.limits; }
		ND_ VkPhysicalDeviceSparseProperties const&				GetDeviceSparseProperties ()			const	{ return _deviceInfo.properties.sparseProperties; }
		ND_ VkPhysicalDeviceMeshShaderPropertiesNV const&		GetDeviceMeshShaderProperties ()		const	{ return _deviceInfo.meshShaderProperties; }
		ND_ VkPhysicalDeviceRayTracingPropertiesNV const&		GetDeviceRayTracingProperties ()		const	{ return _deviceInfo.rayTracingProperties; }
		ND_ VkPhysicalDeviceShadingRateImagePropertiesNV const&	GetDeviceShadingRateImageProperties ()	const	{ return _deviceInfo.shadingRateImageProperties; }


		// check extensions
		ND_ bool HasExtension (StringView name) const;
		ND_ bool HasDeviceExtension (StringView name) const;
		
		bool SetObjectName (uint64_t id, StringView name, VkObjectType type) const;


	private:
		bool _LoadInstanceExtensions () const;
		bool _LoadDeviceExtensions () const;
	};


}	// FG
