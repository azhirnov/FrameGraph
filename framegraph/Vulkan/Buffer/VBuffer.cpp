// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VBuffer.h"
#include "VEnumCast.h"
#include "FGEnumCast.h"
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
		CHECK_ERR( _buffer == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );
		CHECK_ERR( not desc.isExternal );

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

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_buffer), dbgName, VK_OBJECT_TYPE_BUFFER );
		}

		_currQueueFamily	= queueFamily;
		_debugName			= dbgName;

		return true;
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VBuffer::Create (const VDevice &dev, const VulkanBufferDesc &desc, StringView dbgName, OnRelease_t &&onRelease)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _buffer == VK_NULL_HANDLE );

		_buffer				= BitCast<VkBuffer>( desc.buffer );
		_desc.size			= desc.size;
		_desc.usage			= FGEnumCast( BitCast<VkBufferUsageFlagBits>( desc.usage ));
		_desc.isExternal	= true;

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_buffer), dbgName, VK_OBJECT_TYPE_BUFFER );
		}

		//_semaphore		= BitCast<VkSemaphore>( desc.semaphore );
		_currQueueFamily	= BitCast<EQueueFamily>( desc.queueFamily );
		_debugName			= dbgName;
		_onRelease			= std::move(onRelease);

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

		if ( _desc.isExternal and _onRelease ) {
			_onRelease( BitCast<BufferVk_t>(_buffer) );
		}

		if ( not _desc.isExternal and _buffer ) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_BUFFER, uint64_t(_buffer) );
		}

		if ( _memoryId ) {
			unassignIDs.emplace_back( _memoryId.Release() );
		}

		_buffer				= VK_NULL_HANDLE;
		_memoryId			= Default;
		_desc				= Default;
		_currQueueFamily	= Default;
		_onRelease			= {};
		//_semaphore		= VK_NULL_HANDLE;	// TODO: delete semaphore?

		_debugName.clear();
	}
	
/*
=================================================
	IsReadOnly
=================================================
*/
	bool VBuffer::IsReadOnly () const
	{
		SHAREDLOCK( _rcCheck );
		return not EnumAny( _desc.usage, EBufferUsage::TransferDst | EBufferUsage::StorageTexel | EBufferUsage::Storage | EBufferUsage::RayTracing );
	}


}	// FG
