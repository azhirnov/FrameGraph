// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
						  EQueueFamilyMask queueFamilyMask, StringView dbgName)
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

		StaticArray<uint32_t, 8>	queue_family_indices = {};

		// setup sharing mode
		if ( queueFamilyMask != Default )
		{
			info.sharingMode			= VK_SHARING_MODE_CONCURRENT;
			info.pQueueFamilyIndices	= queue_family_indices.data();
			
			for (uint i = 0, mask = (1u<<i);
				 mask <= uint(queueFamilyMask) and info.queueFamilyIndexCount < queue_family_indices.size();
				 ++i, mask = (1u<<i))
			{
				if ( EnumEq( queueFamilyMask, mask ) )
					queue_family_indices[ info.queueFamilyIndexCount++ ] = i;
			}
		}

		// reset to exclusive mode
		if ( info.queueFamilyIndexCount < 2 )
		{
			info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
			info.pQueueFamilyIndices	= null;
			info.queueFamilyIndexCount	= 0;
		}

		VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &info, null, OUT &_buffer ));

		CHECK_ERR( memObj.AllocateForBuffer( _buffer ));

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_buffer), dbgName, VK_OBJECT_TYPE_BUFFER );
		}

		_queueFamilyMask	= queueFamilyMask;
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
		
		CHECK( desc.queueFamily == VK_QUEUE_FAMILY_IGNORED );	// not supported yet
		CHECK( desc.queueFamilyIndices.empty() or desc.queueFamilyIndices.size() >= 2 );

		_queueFamilyMask = Default;

		for (auto idx : desc.queueFamilyIndices) {
			_queueFamilyMask |= BitCast<EQueueFamily>(idx);
		}

		_debugName	= dbgName;
		_onRelease	= std::move(onRelease);

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
		_queueFamilyMask	= Default;
		_onRelease			= {};

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
