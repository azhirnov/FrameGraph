// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
		VkImageAspectFlagBits		_aspectMask			= Zero;
		VkImageLayout				_defaultLayout		= Zero;
		VkAccessFlagBits			_readAccessMask		= Zero;
		EQueueFamilyMask			_queueFamilyMask	= Default;

		DebugName_t					_debugName;
		OnRelease_t					_onRelease;

		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VImage () {}
		VImage (VImage &&) = default;
		~VImage ();

		bool Create (VResourceManager &, const ImageDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
					 EQueueFamilyMask queueFamilyMask, EResourceState defaultState, StringView dbgName);

		bool Create (const VDevice &dev, const VulkanImageDesc &desc, StringView dbgName, OnRelease_t &&onRelease);

		void Destroy (VResourceManager &);

		ND_ VulkanImageDesc		GetApiSpecificDescription () const;

		ND_ VkImageView			GetView (const VDevice &, const HashedImageViewDesc &) const;
		ND_ VkImageView			GetView (const VDevice &, bool isDefault, INOUT ImageViewDesc &) const;
		
		ND_ bool				IsReadOnly ()			const;

		ND_ VkImage				Handle ()				const	{ SHAREDLOCK( _drCheck );  return _image; }
		ND_ RawMemoryID			GetMemoryID ()			const	{ SHAREDLOCK( _drCheck );  return _memoryId.Get(); }

		ND_ ImageDesc const&	Description ()			const	{ SHAREDLOCK( _drCheck );  return _desc; }
		ND_ VkImageAspectFlags	AspectMask ()			const	{ SHAREDLOCK( _drCheck );  return _aspectMask; }
		ND_ uint3				Dimension ()			const	{ SHAREDLOCK( _drCheck );  return _desc.dimension; }
		ND_ VkImageLayout		DefaultLayout ()		const	{ SHAREDLOCK( _drCheck );  return _defaultLayout; }

		ND_ uint const			Width ()				const	{ SHAREDLOCK( _drCheck );  return _desc.dimension.x; }
		ND_ uint const			Height ()				const	{ SHAREDLOCK( _drCheck );  return _desc.dimension.y; }
		ND_ uint const			Depth ()				const	{ SHAREDLOCK( _drCheck );  return _desc.dimension.z; }
		ND_ uint const			ArrayLayers ()			const	{ SHAREDLOCK( _drCheck );  return _desc.arrayLayers.Get(); }
		ND_ uint const			MipmapLevels ()			const	{ SHAREDLOCK( _drCheck );  return _desc.maxLevel.Get(); }
		ND_ EPixelFormat		PixelFormat ()			const	{ SHAREDLOCK( _drCheck );  return _desc.format; }
		ND_ EImage				ImageType ()			const	{ SHAREDLOCK( _drCheck );  return _desc.imageType; }
		ND_ uint const			Samples ()				const	{ SHAREDLOCK( _drCheck );  return _desc.samples.Get(); }
		
		ND_ VkAccessFlagBits	GetAllReadAccessMask ()	const	{ SHAREDLOCK( _drCheck );  return _readAccessMask; }

		ND_ bool				IsExclusiveSharing ()	const	{ SHAREDLOCK( _drCheck );  return _queueFamilyMask == Default; }
		ND_ EQueueFamilyMask	GetQueueFamilyMask ()	const	{ SHAREDLOCK( _drCheck );  return _queueFamilyMask; }
		ND_ StringView			GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }


	private:
		bool _CreateView (const VDevice &, const HashedImageViewDesc &, OUT VkImageView &) const;
	};
	

}	// FG
