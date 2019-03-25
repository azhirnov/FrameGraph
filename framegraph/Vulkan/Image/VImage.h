// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/MemoryDesc.h"
#include "framegraph/Shared/ImageViewDesc.h"
#include "framegraph/Public/FrameGraph.h"
#include "VCommon.h"
#include <shared_mutex>

namespace FG
{

	//
	// Vulkan Image immutable data
	//

	class VImage final
	{
		friend class VImageUnitTest;

	// types
	public:
		using ImageViewMap_t	= HashMap< HashedImageViewDesc, VkImageView, HashOfImageViewDesc >;
		using OnRelease_t		= IFrameGraph::OnExternalImageReleased_t;


	// variables
	private:
		VkImage						_image				= VK_NULL_HANDLE;
		ImageDesc					_desc;

		mutable std::shared_mutex	_viewMapLock;
		mutable ImageViewMap_t		_viewMap;

		MemoryID					_memoryId;
		VkImageAspectFlags			_aspectMask			= 0;
		VkImageLayout				_defaultLayout		= VK_IMAGE_LAYOUT_MAX_ENUM;
		VkAccessFlags				_readAccessMask		= 0;
		EQueueFamilyMask			_queueFamilyMask	= Default;

		DebugName_t					_debugName;
		OnRelease_t					_onRelease;

		RWRaceConditionCheck		_rcCheck;


	// methods
	public:
		VImage () {}
		VImage (VImage &&) = default;
		~VImage ();

		bool Create (VResourceManager &, const ImageDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
					 EQueueFamilyMask queueFamilyMask, VkImageLayout defaultLayout, StringView dbgName);

		bool Create (const VDevice &dev, const VulkanImageDesc &desc, StringView dbgName, OnRelease_t &&onRelease);

		void Destroy (VResourceManager &);

		ND_ VkImageView			GetView (const VDevice &, const HashedImageViewDesc &) const;
		ND_ VkImageView			GetView (const VDevice &, bool isDefault, INOUT ImageViewDesc &) const;
		
		ND_ bool				IsReadOnly ()			const;

		ND_ VkImage				Handle ()				const	{ SHAREDLOCK( _rcCheck );  return _image; }
		ND_ RawMemoryID			GetMemoryID ()			const	{ SHAREDLOCK( _rcCheck );  return _memoryId.Get(); }

		ND_ ImageDesc const&	Description ()			const	{ SHAREDLOCK( _rcCheck );  return _desc; }
		ND_ VkImageAspectFlags	AspectMask ()			const	{ SHAREDLOCK( _rcCheck );  return _aspectMask; }
		ND_ uint3				Dimension ()			const	{ SHAREDLOCK( _rcCheck );  return _desc.dimension; }
		ND_ VkImageLayout		DefaultLayout ()		const	{ SHAREDLOCK( _rcCheck );  return _defaultLayout; }

		ND_ uint const			Width ()				const	{ SHAREDLOCK( _rcCheck );  return _desc.dimension.x; }
		ND_ uint const			Height ()				const	{ SHAREDLOCK( _rcCheck );  return _desc.dimension.y; }
		ND_ uint const			Depth ()				const	{ SHAREDLOCK( _rcCheck );  return _desc.dimension.z; }
		ND_ uint const			ArrayLayers ()			const	{ SHAREDLOCK( _rcCheck );  return _desc.arrayLayers.Get(); }
		ND_ uint const			MipmapLevels ()			const	{ SHAREDLOCK( _rcCheck );  return _desc.maxLevel.Get(); }
		ND_ EPixelFormat		PixelFormat ()			const	{ SHAREDLOCK( _rcCheck );  return _desc.format; }
		ND_ EImage				ImageType ()			const	{ SHAREDLOCK( _rcCheck );  return _desc.imageType; }
		ND_ uint const			Samples ()				const	{ SHAREDLOCK( _rcCheck );  return _desc.samples.Get(); }
		
		ND_ VkAccessFlags		GetAllReadAccessMask ()	const	{ SHAREDLOCK( _rcCheck );  return _readAccessMask; }

		ND_ bool				IsExclusiveSharing ()	const	{ SHAREDLOCK( _rcCheck );  return _queueFamilyMask == Default; }
		ND_ EQueueFamilyMask	GetQueueFamilyMask ()	const	{ SHAREDLOCK( _rcCheck );  return _queueFamilyMask; }
		ND_ StringView			GetDebugName ()			const	{ SHAREDLOCK( _rcCheck );  return _debugName; }


	private:
		bool _CreateView (const VDevice &, const HashedImageViewDesc &, OUT VkImageView &) const;
	};
	

}	// FG
