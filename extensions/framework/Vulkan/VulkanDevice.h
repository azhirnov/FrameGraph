// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"

namespace FG
{
	static constexpr VkQueueFlagBits	VK_QUEUE_PRESENT_BIT = VkQueueFlagBits(0x80000000u);


	//
	// Vulkan Device
	//

	class VulkanDevice : public VulkanDeviceFn
	{
	// types
	public:
		struct QueueCreateInfo
		{
			VkQueueFlags			flags		= 0;
			float					priority	= 0.0f;

			QueueCreateInfo () {}
			QueueCreateInfo (int flags, float priority = 0.0f) : flags{VkQueueFlags(flags)}, priority{priority} {}
			QueueCreateInfo (VkQueueFlags flags, float priority = 0.0f) : flags{flags}, priority{priority} {}
		};

		struct VulkanQueue
		{
			VkQueue					id			= VK_NULL_HANDLE;
			uint					familyIndex	= ~0u;
			uint					queueIndex	= ~0u;
			VkQueueFlags			flags		= 0;
			float					priority	= 0.0f;
		};

		static constexpr uint	maxQueues = 16;

		using SurfaceCtor_t = std::function< VkSurfaceKHR (VkInstance) >;
		using Queues_t		= FixedArray< VulkanQueue, maxQueues >;


	// variables
	private:
		VkDevice					_vkDevice;
		Queues_t					_vkQueues;

		VkSurfaceKHR				_vkSurface;
		VkPhysicalDevice			_vkPhysicalDevice;
		VkInstance					_vkInstance;
		bool						_usedSharedInstance;

		VulkanDeviceFnTable			_deviceFnTable;

		// enabled features
		struct {
			VkPhysicalDeviceFeatures								main {};
			//VkDeviceGeneratedCommandsFeaturesNVX					generatedCommands {};
			VkPhysicalDeviceMultiviewFeatures						multiview {};
			//VkPhysicalDeviceShaderAtomicInt64FeaturesKHR			shaderAtomicI64 {};
			VkPhysicalDevice8BitStorageFeaturesKHR					storage8Bit {};
			VkPhysicalDevice16BitStorageFeatures					storage16Bit {};
			VkPhysicalDeviceSamplerYcbcrConversionFeatures			samplerYcbcrConversion {};
			VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT		blendOpAdvanced {};
			VkPhysicalDeviceConditionalRenderingFeaturesEXT			conditionalRendering {};
			VkPhysicalDeviceShaderDrawParameterFeatures				shaderDrawParameters {};
			VkPhysicalDeviceMeshShaderFeaturesNV					meshShader {};
			//VkPhysicalDeviceDescriptorIndexingFeaturesEXT			descriptorIndexing {};
			//VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT		vertexAttribDivisor {};
			//VkPhysicalDeviceASTCDecodeFeaturesEXT					astcDecode {};
			VkPhysicalDeviceVulkanMemoryModelFeaturesKHR			memoryModel {};
			VkPhysicalDeviceInlineUniformBlockFeaturesEXT			inlineUniformBlock {};
			VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV	representativeFragmentTest {};
			//VkPhysicalDeviceExclusiveScissorFeaturesNV			exclusiveScissorTest {};
			//VkPhysicalDeviceCornerSampledImageFeaturesNV			cornerSampledImage {};
			//VkPhysicalDeviceComputeShaderDerivativesFeaturesNV	computeShaderDerivatives {};
			VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV		fragmentShaderBarycentric {};
			VkPhysicalDeviceShaderImageFootprintFeaturesNV			shaderImageFootprint {};
			VkPhysicalDeviceShadingRateImageFeaturesNV				shadingRateImage {};

		}	_features;


	// methods
	public:
		VulkanDevice ();
		~VulkanDevice ();
		
		bool Create (UniquePtr<IVulkanSurface> &&surf,
					 StringView					applicationName,
					 StringView					engineName,
					 uint						version				= VK_API_VERSION_1_1,
					 StringView					deviceName			= Default,
					 ArrayView<QueueCreateInfo>	queues				= Default,
					 ArrayView<const char*>		instanceLayers		= GetRecomendedInstanceLayers(),
					 ArrayView<const char*>		instanceExtensions	= GetRecomendedInstanceExtensions(),
					 ArrayView<const char*>		deviceExtensions	= GetRecomendedDeviceExtensions());

		bool Create (VkInstance					instance,
					 UniquePtr<IVulkanSurface> &&surf,
					 StringView					deviceName			= Default,
					 ArrayView<QueueCreateInfo>	queues				= Default,
					 ArrayView<const char*>		deviceExtensions	= GetRecomendedDeviceExtensions());

		void Destroy ();
		
		ND_ VkInstance					GetVkInstance ()			const	{ return _vkInstance; }
		ND_ VkPhysicalDevice			GetVkPhysicalDevice ()		const	{ return _vkPhysicalDevice; }
		ND_ VkDevice					GetVkDevice ()				const	{ return _vkDevice; }
		ND_ VkSurfaceKHR				GetVkSurface ()				const	{ return _vkSurface; }
		ND_ ArrayView<VulkanQueue>		GetVkQuues ()				const	{ return _vkQueues; }

		ND_ VkPhysicalDeviceFeatures const&								GetDeviceFeatures ()							const	{ return _features.main; }
		ND_ VkPhysicalDeviceMultiviewFeatures const&					GetDeviceMultiviewFeatures ()					const	{ return _features.multiview; }
		ND_ VkPhysicalDevice8BitStorageFeaturesKHR const&				GetDevice8BitStorageFeatures ()					const	{ return _features.storage8Bit; }
		ND_ VkPhysicalDevice16BitStorageFeatures const&					GetDevice16BitStorageFeatures ()				const	{ return _features.storage16Bit; }
		ND_ VkPhysicalDeviceSamplerYcbcrConversionFeatures const&		GetDeviceSamplerYcbcrConversionFeatures ()		const	{ return _features.samplerYcbcrConversion; }
		ND_ VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT const&	GetDeviceBlendOperationAdvancedFeatures ()		const	{ return _features.blendOpAdvanced; }
		ND_ VkPhysicalDeviceConditionalRenderingFeaturesEXT const&		GetDeviceConditionalRenderingFeatures ()		const	{ return _features.conditionalRendering; }
		ND_ VkPhysicalDeviceMeshShaderFeaturesNV const&					GetDeviceMeshShaderFeatures ()					const	{ return _features.meshShader; }
		ND_ VkPhysicalDeviceVulkanMemoryModelFeaturesKHR const&			GetDeviceVulkanMemoryModelFeatures ()			const	{ return _features.memoryModel; }
		ND_ VkPhysicalDeviceInlineUniformBlockFeaturesEXT const&		GetDeviceInlineUniformBlockFeatures ()			const	{ return _features.inlineUniformBlock; }
		ND_ VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV const&	GetDeviceRepresentativeFragmentTestFeatures ()	const	{ return _features.representativeFragmentTest; }
		ND_ VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV const&	GetDeviceFragmentShaderBarycentricFeatures ()	const	{ return _features.fragmentShaderBarycentric; }
		ND_ VkPhysicalDeviceShaderImageFootprintFeaturesNV const&		GetDeviceShaderImageFootprintFeatures ()		const	{ return _features.shaderImageFootprint; }
		ND_ VkPhysicalDeviceShadingRateImageFeaturesNV const&			GetDeviceShadingRateImageFeatures ()			const	{ return _features.shadingRateImage; }

		ND_ static ArrayView<const char*>	GetRecomendedInstanceLayers ();
		ND_ static ArrayView<const char*>	GetRecomendedInstanceExtensions ();
		ND_ static ArrayView<const char*>	GetRecomendedDeviceExtensions ();
		ND_ static ArrayView<const char*>	GetAllDeviceExtensions_v100 ();
		ND_ static ArrayView<const char*>	GetAllDeviceExtensions ();


	protected:
		virtual void _OnInstanceCreated (Array<const char*> &&, Array<const char*> &&) {}
		virtual void _OnLogicalDeviceCreated (Array<const char *> &&) {}
		virtual void _BeforeDestroy () {}
		virtual void _AfterDestroy () {}
		
		ND_ static StringView  _GetVendorNameByID (uint id);


	private:
		bool _CreateInstance (StringView appName, StringView engineName, ArrayView<const char*> instanceLayers, Array<const char*> &&instanceExtensions, uint version);
		bool _ChooseGpuDevice (StringView deviceName);
		bool _SetupQueues (ArrayView<QueueCreateInfo> queue);
		bool _CreateDevice (ArrayView<const char*> extensions);
		bool _SetupDeviceFeatures (void** next, ArrayView<const char*> extensions);
		bool _ChooseQueueIndex (ArrayView<VkQueueFamilyProperties> props, INOUT VkQueueFlags &flags, OUT uint &index) const;
		void _DestroyDevice ();

		void _ValidateInstanceVersion (INOUT uint &version) const;
		void _ValidateInstanceLayers (INOUT Array<const char*> &layers) const;
		void _ValidateInstanceExtensions (INOUT Array<const char*> &ext) const;
		void _ValidateDeviceExtensions (INOUT Array<const char*> &ext) const;
	};


}	// FG
