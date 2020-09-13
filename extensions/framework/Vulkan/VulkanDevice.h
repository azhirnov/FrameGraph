// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_VULKAN

# include "framework/Window/IWindow.h"
# include "stl/Containers/StaticString.h"
# include "stl/Containers/Ptr.h"
# include "vulkan_loader/VulkanLoader.h"
# include "vulkan_loader/VulkanCheckError.h"

namespace FGC
{

	// same as EQueueType in FG
	enum class VQueueType : uint
	{
		Graphics,			// also supports compute and transfer commands
		AsyncCompute,		// separate compute queue
		AsyncTransfer,		// separate transfer queue
		//Present,			// queue must support present, may be a separate queue
		_Count,
		Unknown			= ~0u,
	};

	// same as EQueueMask in FG
	enum class VQueueMask : uint
	{
		Graphics		= 1 << uint(VQueueType::Graphics),
		AsyncCompute	= 1 << uint(VQueueType::AsyncCompute),
		AsyncTransfer	= 1 << uint(VQueueType::AsyncTransfer),
		All				= Graphics | AsyncCompute | AsyncTransfer,
		Unknown			= 0,
	};
	FG_BIT_OPERATORS( VQueueMask );



	//
	// Vulkan Device
	//

	class VulkanDevice : public VulkanDeviceFn
	{
	// types
	public:
		struct InstanceVersion
		{
			uint	major	: 16;
			uint	minor	: 16;

			InstanceVersion () : major{0}, minor{0} {}
			InstanceVersion (uint maj, uint min) : major{maj}, minor{min} {}

			ND_ bool  operator == (const InstanceVersion &rhs) const;
			ND_ bool  operator >  (const InstanceVersion &rhs) const;
			ND_ bool  operator >= (const InstanceVersion &rhs) const;
		};
		
		struct EnabledFeatures
		{
			// vulkan 1.1 core
			bool	bindMemory2				: 1;
			bool	dedicatedAllocation		: 1;
			bool	descriptorUpdateTemplate: 1;
			bool	imageViewUsage			: 1;
			bool	commandPoolTrim			: 1;
			bool	dispatchBase			: 1;
			bool	array2DCompatible		: 1;
			bool	blockTexelView			: 1;
			bool	maintenance3			: 1;
			// vulkan 1.2 core
			bool	samplerMirrorClamp		: 1;
			bool	shaderAtomicInt64		: 1;	// for uniform/storage buffer, for shared variables check features
			bool	float16Arithmetic		: 1;
			bool	int8Arithmetic			: 1;
			bool	bufferAddress			: 1;
			bool	descriptorIndexing		: 1;	// to use GL_EXT_nonuniform_qualifier 
			bool	renderPass2				: 1;
			bool	depthStencilResolve		: 1;
			bool	drawIndirectCount		: 1;
			bool	spirv14					: 1;
			bool	memoryModel				: 1;	// to use GL_KHR_memory_scope_semantics, SPV_KHR_vulkan_memory_model
			bool	samplerFilterMinmax		: 1;
			// window extensions
			bool	surface					: 1;
			bool	surfaceCaps2			: 1;
			bool	swapchain				: 1;
			// extensions
			bool	debugUtils				: 1;
			bool	meshShaderNV			: 1;
			bool	rayTracingNV			: 1;
			bool	shadingRateImageNV		: 1;
			bool	imageFootprintNV		: 1;
		//	bool	inlineUniformBlock		: 1;
			bool	shaderClock				: 1;
			bool	timelineSemaphore		: 1;
			bool	pushDescriptor			: 1;
			bool	robustness2				: 1;
			bool	shaderStencilExport		: 1;
			bool	extendedDynamicState	: 1;
			bool	rayTracing				: 1;
		};

		struct DeviceProperties
		{
			VkPhysicalDeviceProperties							properties;
			VkPhysicalDeviceFeatures							features;
			VkPhysicalDeviceMemoryProperties					memoryProperties;
			#ifdef VK_VERSION_1_1
			VkPhysicalDeviceSubgroupProperties					subgroup;
			#endif
			#ifdef VK_KHR_vulkan_memory_model
			VkPhysicalDeviceVulkanMemoryModelFeaturesKHR		memoryModel;
			#endif
			#ifdef VK_NV_mesh_shader
			VkPhysicalDeviceMeshShaderFeaturesNV				meshShaderFeatures;
			VkPhysicalDeviceMeshShaderPropertiesNV				meshShaderProperties;
			#endif
			#ifdef VK_NV_shading_rate_image
			VkPhysicalDeviceShadingRateImageFeaturesNV			shadingRateImageFeatures;
			VkPhysicalDeviceShadingRateImagePropertiesNV		shadingRateImageProperties;
			#endif
			#ifdef VK_NV_ray_tracing
			VkPhysicalDeviceRayTracingPropertiesNV				rayTracingNVProperties;
			#endif
			#ifdef VK_NV_shader_image_footprint
			VkPhysicalDeviceShaderImageFootprintFeaturesNV		shaderImageFootprintFeatures;
			#endif
			#ifdef VK_KHR_shader_clock
			VkPhysicalDeviceShaderClockFeaturesKHR				shaderClockFeatures;
			#endif
			#ifdef VK_KHR_timeline_semaphore
			VkPhysicalDeviceTimelineSemaphorePropertiesKHR		timelineSemaphoreProps;
			#endif
			#ifdef VK_KHR_buffer_device_address
			VkPhysicalDeviceBufferDeviceAddressFeaturesKHR		bufferDeviceAddress;
			#endif
			#ifdef VK_KHR_depth_stencil_resolve
			VkPhysicalDeviceDepthStencilResolvePropertiesKHR	depthStencilResolve;
			#endif
			#ifdef VK_KHR_shader_atomic_int64
			VkPhysicalDeviceShaderAtomicInt64FeaturesKHR		shaderAtomicInt64;
			#endif
			#ifdef VK_EXT_descriptor_indexing
			VkPhysicalDeviceDescriptorIndexingFeaturesEXT		descriptorIndexingFeatures;
			VkPhysicalDeviceDescriptorIndexingPropertiesEXT		descriptorIndexingProperties;
			#endif
			#ifdef VK_VERSION_1_2
			//VkPhysicalDeviceVulkan11Properties				properties110;		// duplicates values from other properties
			//VkPhysicalDeviceVulkan12Properties				properties120;
			#endif
			#ifdef VK_EXT_robustness2
			VkPhysicalDeviceRobustness2FeaturesEXT				robustness2Features;
			VkPhysicalDeviceRobustness2PropertiesEXT			robustness2Properties;
			#endif
			#ifdef VK_KHR_maintenance3
			VkPhysicalDeviceMaintenance3Properties				maintenance3Properties;
			#endif
			#ifdef VK_EXT_sampler_filter_minmax
			VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT	samplerFilerMinmaxProperties;
			#endif
			#ifdef VK_EXT_extended_dynamic_state
			VkPhysicalDeviceExtendedDynamicStateFeaturesEXT		extendedDynamicStateFeatures;
			#endif
			#ifdef VK_KHR_ray_tracing
			VkPhysicalDeviceRayTracingFeaturesKHR				rayTracingFeatures;
			VkPhysicalDeviceRayTracingPropertiesKHR				rayTracingProperties;
			#endif
		};
		
		using DebugName_t			= StaticString<64>;
		using VQueueFamilyIndices_t	= FixedArray< uint, uint(VQueueType::_Count) >;
		
		struct VQueue
		{
			VkQueue				handle			= VK_NULL_HANDLE;
			VQueueType			type			= Default;
			uint				familyIndex		= UMax;
			uint				queueIndex		= UMax;
			VkQueueFlagBits		familyFlags		= Zero;
			float				priority		= 0.0f;
			bool				supportsPresent	= false;
			uint3				minImageTransferGranularity;
			DebugName_t			debugName;
		};
		using VQueuePtr = Ptr<VQueue>;


	private:
		using Queues_t			= FixedArray< VQueue, 4 >;
		using QueueTypes_t		= StaticArray< VQueuePtr, uint(VQueueType::_Count) >;
		
		using ExtensionName_t	= StaticString<VK_MAX_EXTENSION_NAME_SIZE>;
		using ExtensionSet_t	= HashSet< ExtensionName_t >;


	// variables
	protected:
		VkDevice				_vkLogicalDevice;

		Queues_t				_vkQueues;
		QueueTypes_t			_queueTypes;

		VkPhysicalDevice		_vkPhysicalDevice;
		VkInstance				_vkInstance;
		InstanceVersion			_vkVersion;
		VkSurfaceKHR			_vkSurface;
		
		EnabledFeatures			_features;

		VulkanDeviceFnTable		_deviceFnTable;
		
		DeviceProperties		_properties;
		
		ExtensionSet_t			_instanceExtensions;
		ExtensionSet_t			_deviceExtensions;


	// methods
	public:
		VulkanDevice ();
		~VulkanDevice ();
		
		ND_ EnabledFeatures const&	GetFeatures ()					const	{ return _features; }
		ND_ DeviceProperties const&	GetProperties ()				const	{ return _properties; }

		ND_ VkDevice				GetVkDevice ()					const	{ return _vkLogicalDevice; }
		ND_ VkPhysicalDevice		GetVkPhysicalDevice ()			const	{ return _vkPhysicalDevice; }
		ND_ VkInstance				GetVkInstance ()				const	{ return _vkInstance; }
		ND_ InstanceVersion			GetVkVersion ()					const	{ return _vkVersion; }
		ND_ VkSurfaceKHR			GetVkSurface ()					const	{ return _vkSurface; }
		ND_ ArrayView< VQueue >		GetVkQueues ()					const	{ return _vkQueues; }
		ND_ VQueuePtr				GetQueue (VQueueType type)		const	{ return uint(type) < _queueTypes.size() ? _queueTypes[uint(type)] : null; }

		// check extensions
		ND_ bool HasInstanceExtension (StringView name) const;
		ND_ bool HasDeviceExtension (StringView name) const;
		
		bool SetObjectName (uint64_t id, NtStringView name, VkObjectType type) const;

		void GetQueueFamilies (VQueueMask mask, OUT VQueueFamilyIndices_t &) const;
	};



	//
	// Vulkan Device Initializer
	//

	class VulkanDeviceInitializer final : public VulkanDevice
	{
	// types
	public:
		struct QueueCreateInfo
		{
			VkQueueFlagBits		flags		= Zero;
			float				priority	= 0.0f;
			DebugName_t			debugName;

			QueueCreateInfo () {}
			QueueCreateInfo (VkQueueFlagBits flags, float priority = 0.0f, StringView name = {}) : flags{flags}, priority{priority}, debugName{name} {}
		};
		
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
		using DebugReport_t = Function< void (const DebugReport &) >;


	// variable
	private:
		VkDebugUtilsMessengerEXT	_debugUtilsMessenger	= VK_NULL_HANDLE;
		DebugReport_t				_callback;
		
		bool						_usedSharedInstance		= false;
		
		bool						_breakOnValidationError	= true;
		Array<ObjectDbgInfo>		_tempObjectDbgInfos;
		String						_tempString;


	// methods
	public:
		VulkanDeviceInitializer ();
		~VulkanDeviceInitializer ();

		ND_ InstanceVersion  GetInstanceVersion () const;

		bool CreateInstance (NtStringView appName, NtStringView engineName, ArrayView<const char*> instanceLayers,
							 ArrayView<const char*> instanceExtensions = {}, InstanceVersion version = {1,2}, uint appVer = 0, uint engineVer = 0);
		bool SetInstance (VkInstance value, ArrayView<const char*> instanceExtensions = {});
		
		bool CreateInstance (UniquePtr<IVulkanSurface> surface, NtStringView appName, NtStringView engineName, ArrayView<const char*> instanceLayers,
							 ArrayView<const char*> instanceExtensions = {}, InstanceVersion version = {1,2}, uint appVer = 0, uint engineVer = 0);
		bool SetInstance (UniquePtr<IVulkanSurface> surface, VkInstance value, ArrayView<const char*> instanceExtensions = {});

		bool DestroyInstance ();

		bool ChooseDevice (StringView deviceName);
		bool ChooseHighPerformanceDevice ();
		bool SetPhysicalDevice (VkPhysicalDevice value);
		
		bool CreateLogicalDevice (ArrayView<QueueCreateInfo> queues, ArrayView<const char*> extensions = {});
		bool SetLogicalDevice (VkDevice value, ArrayView<const char*> extensions = {});
		bool DestroyLogicalDevice ();
		
		bool CreateDebugCallback (VkDebugUtilsMessageSeverityFlagsEXT severity, DebugReport_t &&callback = Default);
		void DestroyDebugCallback ();
		
		bool GetMemoryTypeIndex (uint memoryTypeBits, VkMemoryPropertyFlags flags, OUT uint &memoryTypeIndex) const;
		bool CompareMemoryTypes (uint memoryTypeBits, VkMemoryPropertyFlags flags, uint memoryTypeIndex) const;

		ND_ static ArrayView<const char*>	GetRecomendedInstanceLayers ();


	private:
		// TODO: make public?
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v100 ();
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v110 ();
		ND_ static ArrayView<const char*>	GetInstanceExtensions_v120 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v100 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v110 ();
		ND_ static ArrayView<const char*>	GetDeviceExtensions_v120 ();

		ND_ static ArrayView<const char*>	_GetInstanceExtensions (InstanceVersion ver);
		ND_ static ArrayView<const char*>	_GetDeviceExtensions (InstanceVersion ver);

		void _ValidateInstanceVersion (INOUT uint &version) const;
		void _ValidateInstanceLayers (INOUT Array<const char*> &layers) const;
		void _ValidateInstanceExtensions (INOUT Array<const char*> &ext) const;
		void _ValidateDeviceExtensions (INOUT Array<const char*> &ext) const;

		void _LogPhysicalDevices () const;

		bool _SetupQueues (ArrayView<QueueCreateInfo> queue);
		bool _ChooseQueueIndex (ArrayView<VkQueueFamilyProperties> props, INOUT VkQueueFlagBits &flags, OUT uint &index) const;
		bool _InitDeviceFeatures ();
		void _UpdateInstanceVersion (uint ver);

		bool _SetupQueueTypes ();
		bool _AddGraphicsQueue ();
		bool _AddAsyncComputeQueue ();
		bool _AddAsyncTransferQueue ();
		
		VKAPI_ATTR static VkBool32 VKAPI_CALL
			_DebugUtilsCallback (VkDebugUtilsMessageSeverityFlagBitsEXT			messageSeverity,
								 VkDebugUtilsMessageTypeFlagsEXT				messageTypes,
								 const VkDebugUtilsMessengerCallbackDataEXT*	pCallbackData,
								 void*											pUserData);

		ND_ static StringView  _ObjectTypeToString (VkObjectType objType);

		void _DebugReport (const DebugReport &);
	};
	
	static constexpr VkDebugUtilsMessageSeverityFlagsEXT	DefaultDebugMessageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
																							//VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
																							VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
																							VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	
	

	forceinline constexpr VQueueMask&  operator |= (VQueueMask &lhs, VQueueType rhs)
	{
		ASSERT( uint(rhs) < 32 );
		return lhs = BitCast<VQueueMask>( uint(lhs) | (1u << (uint(rhs) & 31)) );
	}

	forceinline constexpr VQueueMask   operator |  (VQueueMask lhs, VQueueType rhs)
	{
		ASSERT( uint(rhs) < 32 );
		return BitCast<VQueueMask>( uint(lhs) | (1u << (uint(rhs) & 31)) );
	}

}	// FGC

#endif	// FG_ENABLE_VULKAN
