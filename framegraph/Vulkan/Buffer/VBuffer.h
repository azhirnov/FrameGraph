// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Shared/ResourceBase.h"
#include "framegraph/Public/BufferDesc.h"
#include "framegraph/Public/MemoryDesc.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Buffer immutable data
	//

	class VBuffer final : public ResourceBase
	{
		friend class VBufferUnitTest;

	// variables
	private:
		VkBuffer			_buffer				= VK_NULL_HANDLE;
		MemoryID			_memoryId;
		BufferDesc			_desc;
		EQueueFamily		_currQueueFamily	= Default;
		DebugName_t			_debugName;


	// methods
	public:
		VBuffer () {}
		~VBuffer ();

		bool Create (const VDevice &dev, const BufferDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
					 EQueueFamily queueFamily, StringView dbgName);

		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		
		//void Merge (BufferViewMap_t &, OUT AppendableVkResources_t) const;

		//ND_ VkBufferView		GetView (const HashedBufferViewDesc &) const;

		ND_ VkBuffer			Handle ()				const	{ SHAREDLOCK( _rcCheck );  return _buffer; }
		ND_ RawMemoryID			GetMemoryID ()			const	{ SHAREDLOCK( _rcCheck );  return _memoryId.Get(); }

		ND_ BufferDesc const&	Description ()			const	{ SHAREDLOCK( _rcCheck );  return _desc; }
		ND_ BytesU				Size ()					const	{ SHAREDLOCK( _rcCheck );  return _desc.size; }
		
		ND_ EQueueFamily		CurrentQueueFamily ()	const	{ SHAREDLOCK( _rcCheck );  return _currQueueFamily; }
		ND_ StringView			GetDebugName ()			const	{ SHAREDLOCK( _rcCheck );  return _debugName; }
	};
	

}	// FG
