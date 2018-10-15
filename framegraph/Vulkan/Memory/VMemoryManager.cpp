// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VMemoryManager.h"
#include "VEnumCast.h"

#include "VMemoryManager_AMDMemAllocator.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VMemoryManager::VMemoryManager (const VDevice &dev) :
		_device{ dev },
		_currFrame{ 0 }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VMemoryManager::~VMemoryManager ()
	{
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VMemoryManager::Initialize ()
	{
		using MT = EMemoryTypeExt;
		
#	ifdef FG_ENABLE_VULKAN_MEMORY_ALLOCATOR
		_allocators.push_back(AllocatorPtr{ new AMDMemAllocator{ _device, MT::All } });
#	else
		_allocators.push_back(AllocatorPtr{ new HostMemAllocator{ _frameGraph, MT::HostCached | MT::HostWrite | MT::ForBuffer } });
		_allocators.push_back(AllocatorPtr{ new HostMemAllocator{ _frameGraph, MT::HostCached | MT::HostRead | MT::ForBuffer } });
		_allocators.push_back(AllocatorPtr{ new DedicatedMemAllocator{ _frameGraph, MT::LocalInGPU | MT::Dedicated | MT::ForBuffer | MT::ForImage } });
		_allocators.push_back(AllocatorPtr{ new DeviceMemAllocator{ _frameGraph, MT::LocalInGPU | MT::ForBuffer | MT::ForImage } });
		_allocators.push_back(AllocatorPtr{ new VirtualMemAllocator{ _frameGraph, MT::LocalInGPU | MT::Virtual | MT::AllowAliasing | MT::ForBuffer | MT::ForImage } });
#	endif

		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VMemoryManager::Deinitialize ()
	{
		_allocators.clear();
	}
	
/*
=================================================
	OnBeginFrame
=================================================
*/
	void VMemoryManager::OnBeginFrame (uint frameIdx)
	{
		_currFrame = frameIdx;

		for (auto& alloc : _allocators)
		{
			alloc->OnBeginFrame( frameIdx );
		}
	}
	
/*
=================================================
	OnEndFrame
=================================================
*/
	void VMemoryManager::OnEndFrame ()
	{
		//_currFrame = ~0u;
	}

/*
=================================================
	AllocForImage
=================================================
*/
	bool VMemoryManager::AllocForImage (VkImage image, EMemoryTypeExt memType, OUT VMemoryHandle &handle)
	{
		for (size_t i = 0; i < _allocators.size(); ++i)
		{
			auto&	alloc = _allocators[i];

			if ( alloc->IsSupported( memType ) )
			{
				CHECK_ERR( alloc->AllocForImage( _currFrame, image, memType, OUT handle.memory ) );
				
				handle.allocator = uint(i);
				return true;
			}
		}
		RETURN_ERR( "unsupported memory type" );
	}
	
/*
=================================================
	AllocForBuffer
=================================================
*/
	bool VMemoryManager::AllocForBuffer (VkBuffer buffer, EMemoryTypeExt memType, OUT VMemoryHandle &handle)
	{
		for (size_t i = 0; i < _allocators.size(); ++i)
		{
			auto&	alloc = _allocators[i];

			if ( alloc->IsSupported( memType ) )
			{
				CHECK_ERR( alloc->AllocForBuffer( _currFrame, buffer, memType, OUT handle.memory ) );
				
				handle.allocator = uint(i);
				return true;
			}
		}
		RETURN_ERR( "unsupported memory type" );
	}
	
/*
=================================================
	Dealloc
=================================================
*/
	bool VMemoryManager::Dealloc (VMemoryHandle handle)
	{
		CHECK_ERR( handle.allocator < _allocators.size() );

		CHECK_ERR( _allocators[ handle.allocator ]->Dealloc( _currFrame, handle.memory ));
		return true;
	}
	
/*
=================================================
	MapMemory
=================================================
*/
	bool VMemoryManager::MapMemory (OUT void* &ptr, VMemoryHandle handle, EMemoryMapFlags flags, ArrayView<MemRange> InvalidateRanges)
	{
		CHECK_ERR( handle.allocator < _allocators.size() );

		return _allocators[ handle.allocator ]->MapMemory( _currFrame, handle.memory, flags, InvalidateRanges, OUT ptr );
	}
	
/*
=================================================
	UnmapMemory
=================================================
*/
	bool VMemoryManager::UnmapMemory (VMemoryHandle handle, ArrayView<MemRange> flushRanges)
	{
		CHECK_ERR( handle.allocator < _allocators.size() );

		return _allocators[ handle.allocator ]->UnmapMemory( _currFrame, handle.memory, flushRanges );
	}
	

}	// FG
