// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/UntypedStorage.h"
#include "framegraph/Public/LowLevel/MemoryDesc.h"
#include "framegraph/Shared/ResourceBase.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Memory Object
	//

	class VMemoryObj final : public ResourceBase
	{
	// types
	public:
		using Storage_t = UntypedStorage< sizeof(uint64_t) * 4, alignof(uint64_t) >;

		struct MemoryInfo
		{
			VkDeviceMemory	mem			= VK_NULL_HANDLE;
			BytesU			offset;
			BytesU			size;
			void *			mappedPtr	= null;
		};


	// variables
	private:
		Storage_t					_storage;
		WeakPtr<VMemoryManager>		_manager;
		MemoryDesc					_desc;


	// methods
	public:
		VMemoryObj () {}
		~VMemoryObj ();

		bool Create (const MemoryDesc &, VMemoryManager &);
		bool AllocateForBuffer (VkBuffer buf);
		bool AllocateForImage (VkImage img);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		bool GetInfo (OUT MemoryInfo &) const;

		//ND_ MemoryDesc const&	Description ()		const	{ return _desc; }
		ND_ EMemoryTypeExt	MemoryType ()		const	{ return EMemoryTypeExt(_desc.type); }
	};


}	// FG
