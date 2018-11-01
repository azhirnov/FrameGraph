// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Shared/ResourceBase.h"
#include "framegraph/Public/LowLevel/BufferDesc.h"
#include "framegraph/Public/LowLevel/MemoryDesc.h"
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
		//uint				_queueFamilyIndex	= ~0u;	// TODO
		DebugName_t			_debugName;


	// methods
	public:
		VBuffer () {}
		~VBuffer ();

		bool Create (const VDevice &dev, const BufferDesc &desc, RawMemoryID memId, INOUT VMemoryObj &memObj);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		
		//void Merge (BufferViewMap_t &, OUT AppendableVkResources_t) const;

		//ND_ VkBufferView		GetView (const HashedBufferViewDesc &) const;

		ND_ VkBuffer			Handle ()			const	{ return _buffer; }
		ND_ RawMemoryID			GetMemoryID ()		const	{ return _memoryId.Get(); }

		ND_ BufferDesc const&	Description ()		const	{ return _desc; }
		ND_ BytesU				Size ()				const	{ return _desc.size; }
	};
	

}	// FG
