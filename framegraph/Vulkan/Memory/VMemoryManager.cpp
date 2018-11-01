// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VMemoryManager.h"
#include "VMemoryManager_VMAllocator.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VMemoryManager::VMemoryManager (const VDevice &dev) :
		_device{ dev }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VMemoryManager::~VMemoryManager ()
	{
		ASSERT( _allocators.empty() );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VMemoryManager::Initialize ()
	{
		using MT = EMemoryTypeExt;

		// TODO: use custom mem allocator?
#	ifdef FG_ENABLE_VULKAN_MEMORY_ALLOCATOR
		_allocators.push_back(AllocatorPtr{ new VulkanMemoryAllocator{ _device, MT::All } });
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
	AllocateForImage
=================================================
*/
	bool VMemoryManager::AllocateForImage (VkImage image, const MemoryDesc &desc, INOUT Storage_t &data)
	{
		ASSERT( not _allocators.empty() );

		for (size_t i = 0; i < _allocators.size(); ++i)
		{
			auto&	alloc = _allocators[i];

			if ( alloc->IsSupported( desc.type ) )
			{
				CHECK_ERR( alloc->AllocForImage( image, desc, OUT data ));
				
				*data.Cast<uint>() = uint(i);
				return true;
			}
		}
		RETURN_ERR( "unsupported memory type" );
	}
	
/*
=================================================
	AllocateForBuffer
=================================================
*/
	bool VMemoryManager::AllocateForBuffer (VkBuffer buffer, const MemoryDesc &desc, INOUT Storage_t &data)
	{
		ASSERT( not _allocators.empty() );

		for (size_t i = 0; i < _allocators.size(); ++i)
		{
			auto&	alloc = _allocators[i];

			if ( alloc->IsSupported( desc.type ) )
			{
				CHECK_ERR( alloc->AllocForBuffer( buffer, desc, OUT data ));
				
				*data.Cast<uint>() = uint(i);
				return true;
			}
		}
		RETURN_ERR( "unsupported memory type" );
	}
	
/*
=================================================
	Deallocate
=================================================
*/
	bool VMemoryManager::Deallocate (INOUT Storage_t &data, OUT AppendableVkResources_t readyToDelete)
	{
		const uint	alloc_id = *data.Cast<uint>();
		CHECK_ERR( alloc_id < _allocators.size() );

		CHECK_ERR( _allocators[alloc_id]->Dealloc( INOUT data, OUT readyToDelete ));
		return true;
	}
	
/*
=================================================
	GetMemoryInfo
=================================================
*/
	bool VMemoryManager::GetMemoryInfo (const Storage_t &data, OUT MemoryInfo_t &info) const
	{
		const uint	alloc_id = *data.Cast<uint>();
		CHECK_ERR( alloc_id < _allocators.size() );
		
		CHECK_ERR( _allocators[alloc_id]->GetMemoryInfo( data, OUT info ));
		return true;
	}


}	// FG
