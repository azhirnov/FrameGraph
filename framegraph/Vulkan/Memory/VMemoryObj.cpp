// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VMemoryObj.h"
#include "VMemoryManager.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VMemoryObj::~VMemoryObj ()
	{
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VMemoryObj::Create (const MemoryDesc &desc, VMemoryManager &alloc, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );

		_desc		= desc;
		_manager	= alloc.weak_from_this();
		_debugName	= dbgName;
		
		return true;
	}
	
/*
=================================================
	AllocateForBuffer
=================================================
*/
	bool VMemoryObj::AllocateForBuffer (VkBuffer buf)
	{
		SCOPELOCK( _rcCheck );

		auto	mem_mngr = _manager.lock();
		CHECK_ERR( mem_mngr and mem_mngr->AllocateForBuffer( buf, _desc, INOUT _storage ));

		return true;
	}
	
/*
=================================================
	AllocateForImage
=================================================
*/
	bool VMemoryObj::AllocateForImage (VkImage img)
	{
		SCOPELOCK( _rcCheck );

		auto	mem_mngr = _manager.lock();
		CHECK_ERR( mem_mngr and mem_mngr->AllocateForImage( img, _desc, INOUT _storage ));
		
		return true;
	}
	
/*
=================================================
	AllocateForAccelStruct
=================================================
*/
	bool VMemoryObj::AllocateForAccelStruct (VkAccelerationStructureNV accelStruct)
	{
		SCOPELOCK( _rcCheck );
		
		auto	mem_mngr = _manager.lock();
		CHECK_ERR( mem_mngr and mem_mngr->AllocateForAccelStruct( accelStruct, _desc, INOUT _storage ));

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VMemoryObj::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t)
	{
		SCOPELOCK( _rcCheck );

		auto	mem_mngr = _manager.lock();
		ASSERT( mem_mngr );

		if ( mem_mngr ) {
			CHECK( mem_mngr->Deallocate( INOUT _storage, OUT readyToDelete ));
		}

		_manager.reset();
		_debugName.clear();
	}
	
/*
=================================================
	GetInfo
=================================================
*/
	bool VMemoryObj::GetInfo (OUT MemoryInfo &info) const
	{
		SHAREDLOCK( _rcCheck );

		auto	mem_mngr = _manager.lock();
		CHECK_ERR( mem_mngr );

		return mem_mngr->GetMemoryInfo( _storage, OUT info );
	}


}	// FG
