// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VMemoryManager.h"

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
		EXLOCK( _drCheck );
		ASSERT( _allocators.empty() );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VMemoryManager::Initialize ()
	{
		EXLOCK( _drCheck );

		using MT = EMemoryTypeExt;

		// TODO: use custom mem allocator?
#	ifdef FG_ENABLE_VULKAN_MEMORY_ALLOCATOR
		_allocators.push_back( _CreateVMA() );
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
		EXLOCK( _drCheck );

		_allocators.clear();
	}
	
/*
=================================================
	AllocateForImage
=================================================
*/
	bool VMemoryManager::AllocateForImage (VkImage image, const MemoryDesc &desc, INOUT Storage_t &data)
	{
		SHAREDLOCK( _drCheck );
		ASSERT( not _allocators.empty() );

		for (size_t i = 0; i < _allocators.size(); ++i)
		{
			auto&	alloc = _allocators[i];

			if ( alloc->IsSupported( desc.type ))
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
		SHAREDLOCK( _drCheck );
		ASSERT( not _allocators.empty() );

		for (size_t i = 0; i < _allocators.size(); ++i)
		{
			auto&	alloc = _allocators[i];

			if ( alloc->IsSupported( desc.type ))
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
	AllocateForAccelStruct
=================================================
*/
#ifdef VK_NV_ray_tracing
	bool VMemoryManager::AllocateForAccelStruct (VkAccelerationStructureNV accelStruct, const MemoryDesc &desc, OUT Storage_t &data)
	{
		SHAREDLOCK( _drCheck );
		ASSERT( not _allocators.empty() );

		for (size_t i = 0; i < _allocators.size(); ++i)
		{
			auto&	alloc = _allocators[i];

			if ( alloc->IsSupported( desc.type ))
			{
				CHECK_ERR( alloc->AllocForAccelStruct( accelStruct, desc, OUT data ));
				
				*data.Cast<uint>() = uint(i);
				return true;
			}
		}
		RETURN_ERR( "unsupported memory type" );
	}
#endif

/*
=================================================
	Deallocate
=================================================
*/
	bool VMemoryManager::Deallocate (INOUT Storage_t &data)
	{
		SHAREDLOCK( _drCheck );

		const uint	alloc_id = *data.Cast<uint>();
		CHECK_ERR( alloc_id < _allocators.size() );

		CHECK_ERR( _allocators[alloc_id]->Dealloc( INOUT data ));
		return true;
	}
	
/*
=================================================
	GetMemoryInfo
=================================================
*/
	bool VMemoryManager::GetMemoryInfo (const Storage_t &data, OUT MemoryInfo_t &info) const
	{
		SHAREDLOCK( _drCheck );

		const uint	alloc_id = *data.Cast<uint>();
		CHECK_ERR( alloc_id < _allocators.size() );
		
		CHECK_ERR( _allocators[alloc_id]->GetMemoryInfo( data, OUT info ));
		return true;
	}


}	// FG
