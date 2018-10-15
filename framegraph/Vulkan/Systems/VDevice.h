// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "framegraph/Public/LowLevel/ShaderEnums.h"

namespace FG
{

	//
	// Vulkan Device
	//

	class VDevice final : public VulkanDeviceFn
	{
		friend class VFrameGraph;

	// types
	private:
		using InstanceLayers_t		= Array< VkLayerProperties >;
		using ExtensionName_t		= StaticString<VK_MAX_EXTENSION_NAME_SIZE>;
		using ExtensionSet_t		= HashSet< ExtensionName_t >;
		

		struct PerFrame
		{
			struct {
				Array< VkImage >		images;
				Array< VkImageView >	imageViews;
				Array< VkBuffer >		buffers;
				Array< VkPipeline >		pipelines;
			}						readyToDelete;
		};
		using PerFrameArray_t		= FixedArray< PerFrame, FG_MaxSwapchainLength >;


	// variables
	private:
		VkInstance							_vkInstance;
		VkPhysicalDevice					_vkPhysicalDevice;
		VkDevice							_vkDevice;
		VkSurfaceKHR						_vkSurface;
		EShaderLangFormat					_vkVersion;

		mutable PerFrameArray_t				_perFrame;
		uint								_currFrame	= 0;

		VkPhysicalDeviceProperties			_deviceProperties;
		VkPhysicalDeviceFeatures			_deviceFeatures;
		VkPhysicalDeviceMemoryProperties	_deviceMemoryProperties;
		mutable InstanceLayers_t			_instanceLayers;
		mutable ExtensionSet_t				_instanceExtensions;
		mutable ExtensionSet_t				_deviceExtensions;
		
		VulkanDeviceFnTable					_deviceFnTable;

		bool								_enableDebugMarkers;


	// methods
	public:
		explicit VDevice (const VulkanDeviceInfo &vdi);
		~VDevice ();

		ND_ VkDevice								GetVkDevice ()				const	{ return _vkDevice; }
		ND_ VkPhysicalDevice						GetVkPhysicalDevice ()		const	{ return _vkPhysicalDevice; }
		ND_ VkInstance								GetVkInstance ()			const	{ return _vkInstance; }
		ND_ VkSurfaceKHR							GetVkSurface ()				const	{ return _vkSurface; }
		ND_ EShaderLangFormat						GetVkVersion ()				const	{ return _vkVersion; }
		
		ND_ VkPhysicalDeviceProperties const&		GetDeviceProperties ()		 const	{ return _deviceProperties; }
		ND_ VkPhysicalDeviceFeatures const&			GetDeviceFeatures ()		 const	{ return _deviceFeatures; }
		ND_ VkPhysicalDeviceMemoryProperties const&	GetDeviceMemoryProperties () const	{ return _deviceMemoryProperties; }
		ND_ VkPhysicalDeviceLimits const&			GetDeviceLimits ()			 const	{ return _deviceProperties.limits; }
		ND_ VkPhysicalDeviceSparseProperties const&	GetDeviceSparseProperties () const	{ return _deviceProperties.sparseProperties; }


		// check extensions
		ND_ bool HasLayer (StringView name) const;
		ND_ bool HasExtension (StringView name) const;
		ND_ bool HasDeviceExtension (StringView name) const;
		
		bool SetObjectName (uint64_t id, StringView name, VkDebugReportObjectTypeEXT type) const;


		// delayed delete resource
		void DeleteImage (VkImage image) const;
		void DeleteImageView (VkImageView view) const;
		void DeleteBuffer (VkBuffer buf) const;
		void DeletePipeline (VkPipeline ppln) const;
		void FreeExternalBuffer (VkBuffer buf) const;
		void FreeExternalImage (VkImage image) const;


	private:
		void _Initialize (uint swapchainLength);
		void _Deinitialize ();

		void _OnBeginFrame (uint frameIdx);
		void _DeleteResources (INOUT PerFrame &frame) const;

		bool _LoadInstanceLayers () const;
		bool _LoadInstanceExtensions () const;
		bool _LoadDeviceExtensions () const;
	};


}	// FG
