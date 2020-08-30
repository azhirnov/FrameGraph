// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/BufferDesc.h"
#include "framegraph/Public/MemoryDesc.h"
#include "framegraph/Public/FrameGraph.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Buffer immutable data
	//

	class VBuffer final
	{
		friend class VBufferUnitTest;

	// types
	private:
		using OnRelease_t	= IFrameGraph::OnExternalBufferReleased_t;
		using BufferViewMap_t	= HashMap< BufferViewDesc, VkBufferView >;


	// variables
	private:
		VkBuffer					_buffer				= VK_NULL_HANDLE;
		MemoryID					_memoryId;
		BufferDesc					_desc;
		
		mutable std::shared_mutex	_viewMapLock;
		mutable BufferViewMap_t		_viewMap;

		EQueueFamilyMask			_queueFamilyMask	= Default;
		VkAccessFlagBits			_readAccessMask		= Zero;

		DebugName_t					_debugName;
		OnRelease_t					_onRelease;

		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VBuffer () {}
		VBuffer (VBuffer &&) = delete;
		VBuffer (const VBuffer &) = delete;
		~VBuffer ();

		bool Create (VResourceManager &, const BufferDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
					 EQueueFamilyMask queueFamilyMask, StringView dbgName);

		bool Create (const VDevice &dev, const VulkanBufferDesc &desc, StringView dbgName, OnRelease_t &&onRelease);

		void Destroy (VResourceManager &);

		//void Merge (BufferViewMap_t &, OUT AppendableVkResources_t) const;

		ND_ VkBufferView		GetView (const VDevice &, const BufferViewDesc &) const;
		
		ND_ VulkanBufferDesc	GetApiSpecificDescription () const;

		ND_ bool				IsReadOnly ()			const;

		ND_ VkBuffer			Handle ()				const	{ SHAREDLOCK( _drCheck );  return _buffer; }
		ND_ RawMemoryID			GetMemoryID ()			const	{ SHAREDLOCK( _drCheck );  return _memoryId.Get(); }

		ND_ BufferDesc const&	Description ()			const	{ SHAREDLOCK( _drCheck );  return _desc; }
		ND_ BytesU				Size ()					const	{ SHAREDLOCK( _drCheck );  return _desc.size; }
		
		ND_ VkAccessFlagBits	GetAllReadAccessMask ()	const	{ SHAREDLOCK( _drCheck );  return _readAccessMask; }

		ND_ bool				IsExclusiveSharing ()	const	{ SHAREDLOCK( _drCheck );  return _queueFamilyMask == Default; }
		ND_ EQueueFamilyMask	GetQueueFamilyMask ()	const	{ SHAREDLOCK( _drCheck );  return _queueFamilyMask; }
		ND_ StringView			GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }

		ND_ static bool  IsSupported (const VDevice &dev, const BufferDesc &desc, EMemoryType memType);
		ND_ bool		 IsSupported (const VDevice &dev, const BufferViewDesc &desc) const;

	private:
		bool  _CreateView (const VDevice &, const BufferViewDesc &, OUT VkBufferView &) const;
	};
	

}	// FG
