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

		return true;
	}
	
/*
=================================================
	EBufferUsage_FromVk
=================================================
*/
	ND_ static EBufferUsage  EBufferUsage_FromVk (VkBufferUsageFlags flags)
	{
		EBufferUsage	result = Default;

		for (VkBufferUsageFlags t = 0; t < VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM; t <<= 1)
		{
			if ( not EnumEq( flags, t ) )
				continue;
			
			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case VK_BUFFER_USAGE_TRANSFER_SRC_BIT :			result |= EBufferUsage::TransferSrc;	break;
				case VK_BUFFER_USAGE_TRANSFER_DST_BIT :			result |= EBufferUsage::TransferDst;	break;
				case VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT :	result |= EBufferUsage::UniformTexel;	break;
				case VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT :	result |= EBufferUsage::StorageTexel;	break;
				case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT :		result |= EBufferUsage::Uniform;		break;
				case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT :		result |= EBufferUsage::Storage;		break;
				case VK_BUFFER_USAGE_INDEX_BUFFER_BIT :			result |= EBufferUsage::Index;			break;
				case VK_BUFFER_USAGE_VERTEX_BUFFER_BIT :		result |= EBufferUsage::Vertex;			break;
				case VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT :		result |= EBufferUsage::Indirect;		break;
				case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT :
				case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT :	// TODO
				case VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT :
				case VK_BUFFER_USAGE_RAYTRACING_BIT_NVX :
				case VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM :
				default :										RETURN_ERR( "invalid buffer usage" );
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
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
		_desc.usage			= EBufferUsage_FromVk( desc.usage );
		_desc.isExternal	= true;

		if ( not _debugName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_buffer), _debugName, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT );
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
			readyToDelete.emplace_back( VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, uint64_t(_buffer) );
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


}	// FG
