// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/UntypedStorage.h"
#include "framegraph/Public/MemoryDesc.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Memory Object
	//

	class VMemoryObj final
	{
	// types
	public:
		using Storage_t = UntypedStorage< sizeof(uint64_t) * 4, alignof(uint64_t) >;

		struct MemoryInfo
		{
			VkDeviceMemory			mem			= VK_NULL_HANDLE;
			VkMemoryPropertyFlags	flags		= 0;
			BytesU					offset;
			BytesU					size;
			void *					mappedPtr	= null;
		};


	// variables
	private:
		Storage_t				_storage;
		MemoryDesc				_desc;
		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VMemoryObj () {}
		~VMemoryObj ();

		bool Create (const MemoryDesc &, StringView dbgName);
		void Destroy (VResourceManager &);

		bool AllocateForImage (VMemoryManager &, VkImage);
		bool AllocateForBuffer (VMemoryManager &, VkBuffer);

		#ifdef VK_NV_ray_tracing
		bool AllocateForAccelStruct (VMemoryManager &, VkAccelerationStructureNV);
		#endif

		bool GetInfo (VMemoryManager &, OUT MemoryInfo &) const;

		//ND_ MemoryDesc const&	Description ()	const	{ SHAREDLOCK( _drCheck );  return _desc; }
		ND_ EMemoryTypeExt	MemoryType ()		const	{ SHAREDLOCK( _drCheck );  return EMemoryTypeExt(_desc.type); }
	};


}	// FG
