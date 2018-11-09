// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/MemoryDesc.h"
#include "framegraph/Shared/ImageViewDesc.h"
#include "framegraph/Shared/ResourceBase.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Image immutable data
	//

	class VImage final : public ResourceBase
	{
		friend class VImageUnitTest;

	// types
	public:
		using ImageViewMap_t	= HashMap< HashedImageViewDesc, VkImageView, HashOfImageViewDesc >;


	// variables
	private:
		VkImage					_image				= VK_NULL_HANDLE;
		ImageDesc				_desc;
		mutable ImageViewMap_t	_viewMap;

		MemoryID				_memoryId;
		VkImageAspectFlags		_aspectMask			= 0;
		VkImageLayout			_initialLayout		= VK_IMAGE_LAYOUT_MAX_ENUM;
		VkImageLayout			_defaultLayout		= VK_IMAGE_LAYOUT_MAX_ENUM;
		//uint					_queueFamilyIndex	= ~0u;	// TODO

		DebugName_t				_debugName;


	// methods
	public:
		VImage () {}
		~VImage ();

		bool Create (const VDevice &dev, const ImageDesc &desc, RawMemoryID memId, INOUT VMemoryObj &memObj, StringView dbgName);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		void Merge (ImageViewMap_t &, OUT AppendableVkResources_t) const;

		ND_ VkImageView			GetView (const HashedImageViewDesc &) const;

		ND_ VkImage				Handle ()			const	{ SHAREDLOCK( _rcCheck );  return _image; }
		ND_ RawMemoryID			GetMemoryID ()		const	{ SHAREDLOCK( _rcCheck );  return _memoryId.Get(); }

		ND_ ImageDesc const&	Description ()		const	{ SHAREDLOCK( _rcCheck );  return _desc; }
		ND_ VkImageAspectFlags	AspectMask ()		const	{ SHAREDLOCK( _rcCheck );  return _aspectMask; }
		ND_ uint3				Dimension ()		const	{ SHAREDLOCK( _rcCheck );  return _desc.dimension; }
		
		ND_ VkImageLayout		InitialLayout ()	const	{ SHAREDLOCK( _rcCheck );  return _initialLayout; }
		ND_ VkImageLayout		DefaultLayout ()	const	{ SHAREDLOCK( _rcCheck );  return _defaultLayout; }

		ND_ uint const			Width ()			const	{ SHAREDLOCK( _rcCheck );  return _desc.dimension.x; }
		ND_ uint const			Height ()			const	{ SHAREDLOCK( _rcCheck );  return _desc.dimension.y; }
		ND_ uint const			Depth ()			const	{ SHAREDLOCK( _rcCheck );  return _desc.dimension.z; }
		ND_ uint const			ArrayLayers ()		const	{ SHAREDLOCK( _rcCheck );  return _desc.arrayLayers.Get(); }
		ND_ uint const			MipmapLevels ()		const	{ SHAREDLOCK( _rcCheck );  return _desc.maxLevel.Get(); }
		ND_ EPixelFormat		PixelFormat ()		const	{ SHAREDLOCK( _rcCheck );  return _desc.format; }
		ND_ EImage				ImageType ()		const	{ SHAREDLOCK( _rcCheck );  return _desc.imageType; }
		ND_ uint const			Samples ()			const	{ SHAREDLOCK( _rcCheck );  return _desc.samples.Get(); }

		ND_ StringView			GetDebugName ()		const	{ SHAREDLOCK( _rcCheck );  return _debugName; }
	};
	

}	// FG
