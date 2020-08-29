// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VMemoryObj.h"
#include "VMemoryManager.h"
#include "VResourceManager.h"

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
	bool VMemoryObj::Create (const MemoryDesc &desc, StringView dbgName)
	{
		EXLOCK( _drCheck );

		_desc		= desc;
		_debugName	= dbgName;
		
		return true;
	}
	
/*
=================================================
	AllocateForBuffer
=================================================
*/
	bool VMemoryObj::AllocateForBuffer (VMemoryManager &memMngr, VkBuffer buf)
	{
		EXLOCK( _drCheck );

		CHECK_ERR( memMngr.AllocateForBuffer( buf, _desc, INOUT _storage ));
		return true;
	}
	
/*
=================================================
	AllocateForImage
=================================================
*/
	bool VMemoryObj::AllocateForImage (VMemoryManager &memMngr, VkImage img)
	{
		EXLOCK( _drCheck );

		CHECK_ERR( memMngr.AllocateForImage( img, _desc, INOUT _storage ));
		return true;
	}
	
/*
=================================================
	AllocateForAccelStruct
=================================================
*/
#ifdef VK_NV_ray_tracing
	bool VMemoryObj::AllocateForAccelStruct (VMemoryManager &memMngr, VkAccelerationStructureNV accelStruct)
	{
		EXLOCK( _drCheck );
		
		CHECK_ERR( memMngr.AllocateForAccelStruct( accelStruct, _desc, INOUT _storage ));
		return true;
	}
#endif

/*
=================================================
	Destroy
=================================================
*/
	void VMemoryObj::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		resMngr.GetMemoryManager().Deallocate( INOUT _storage );

		_debugName.clear();
	}
	
/*
=================================================
	GetInfo
=================================================
*/
	bool VMemoryObj::GetInfo (VMemoryManager &memMngr, OUT MemoryInfo &info) const
	{
		SHAREDLOCK( _drCheck );

		return memMngr.GetMemoryInfo( _storage, OUT info );
	}


}	// FG
