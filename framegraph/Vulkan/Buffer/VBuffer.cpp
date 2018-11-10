// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VBuffer.h"
#include "VEnumCast.h"
#include "VDevice.h"
#include "VMemoryObj.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VBuffer::~VBuffer ()
	{
		ASSERT( _buffer == VK_NULL_HANDLE );
		ASSERT( not _memoryId );
	}

/*
=================================================
	Create
=================================================
*/
	bool VBuffer::Create (const VDevice &dev, const BufferDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
						  EQueueFamily queueFamily, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( GetState() == EState::Initial );
		CHECK_ERR( _buffer == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );

		_desc		= desc;
		_memoryId	= MemoryID{ memId };

		// create buffer
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.pNext			= null;
		info.flags			= 0;
		info.usage			= VEnumCast( _desc.usage );
		info.size			= VkDeviceSize( _desc.size );
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &info, null, OUT &_buffer ));

		CHECK_ERR( memObj.AllocateForBuffer( _buffer ));

		if ( not _debugName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_buffer), _debugName, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT );
		}

		_currQueueFamily	= queueFamily;
		_debugName			= dbgName;

		_OnCreate();
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VBuffer::Destroy (AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		SCOPELOCK( _rcCheck );

		if ( _buffer ) {
			readyToDelete.emplace_back( VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, uint64_t(_buffer) );
		}

		if ( _memoryId ) {
			unassignIDs.emplace_back( _memoryId.Release() );
		}

		_buffer				= VK_NULL_HANDLE;
		_memoryId			= Default;
		_desc				= Default;
		_currQueueFamily	= Default;

		_debugName.clear();

		_OnDestroy();
	}


}	// FG
