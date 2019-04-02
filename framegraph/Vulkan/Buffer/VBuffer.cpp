// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VBuffer.h"
#include "VEnumCast.h"
#include "FGEnumCast.h"
#include "VDevice.h"
#include "VMemoryObj.h"
#include "VResourceManager.h"

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
	GetAllBufferReadAccessMasks
=================================================
*/
	static VkAccessFlags  GetAllBufferReadAccessMasks (VkBufferUsageFlags usage)
	{
		VkAccessFlags	result = 0;

		for (VkBufferUsageFlags t = 1; t <= usage; t <<= 1)
		{
			if ( not EnumEq( usage, t ) )
				continue;

			ENABLE_ENUM_CHECKS();
			switch ( VkBufferUsageFlagBits(t) )
			{
				case VK_BUFFER_USAGE_TRANSFER_SRC_BIT :			result |= VK_ACCESS_TRANSFER_READ_BIT;			break;
				case VK_BUFFER_USAGE_TRANSFER_DST_BIT :			break;
				case VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT :	result |= VK_ACCESS_SHADER_READ_BIT;			break;
				case VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT :	result |= VK_ACCESS_SHADER_READ_BIT;			break;
				case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT :		result |= VK_ACCESS_UNIFORM_READ_BIT;			break;
				case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT :		result |= VK_ACCESS_SHADER_READ_BIT;			break;
				case VK_BUFFER_USAGE_INDEX_BUFFER_BIT :			result |= VK_ACCESS_INDEX_READ_BIT;				break;
				case VK_BUFFER_USAGE_VERTEX_BUFFER_BIT :		result |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;	break;
				case VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT :		result |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;	break;
				case VK_BUFFER_USAGE_RAY_TRACING_BIT_NV :		break;
				case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT :
				case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT :
				case VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT :
				case VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT :
				case VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM :		break;	// to shutup compiler warnings
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
	bool VBuffer::Create (VResourceManager &resMngr, const BufferDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
						  EQueueFamilyMask queueFamilyMask, StringView dbgName)
	{
		EXLOCK( _drCheck );
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

		auto&	dev = resMngr.GetDevice();
		VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &info, null, OUT &_buffer ));

		CHECK_ERR( memObj.AllocateForBuffer( resMngr.GetMemoryManager(), _buffer ));

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_buffer), dbgName, VK_OBJECT_TYPE_BUFFER );
		}

		_readAccessMask		= GetAllBufferReadAccessMasks( info.usage );
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
		EXLOCK( _drCheck );
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
	void VBuffer::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		auto&	dev = resMngr.GetDevice();

		if ( _desc.isExternal and _onRelease ) {
			_onRelease( BitCast<BufferVk_t>(_buffer) );
		}

		if ( not _desc.isExternal and _buffer ) {
			dev.vkDestroyBuffer( dev.GetVkDevice(), _buffer, null );
		}

		if ( _memoryId ) {
			resMngr.ReleaseResource( _memoryId.Release() );
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
		SHAREDLOCK( _drCheck );
		return not EnumAny( _desc.usage, EBufferUsage::TransferDst | EBufferUsage::StorageTexel | EBufferUsage::Storage | EBufferUsage::RayTracing );
	}


}	// FG
