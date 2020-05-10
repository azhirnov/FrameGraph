// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
	static VkAccessFlagBits  GetAllBufferReadAccessMasks (VkBufferUsageFlags usage)
	{
		VkAccessFlagBits	result = Zero;

		for (VkBufferUsageFlags t = 1; t <= usage; t <<= 1)
		{
			if ( not EnumEq( usage, t ))
				continue;

			BEGIN_ENUM_CHECKS();
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
			END_ENUM_CHECKS();
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
		
		auto&	dev = resMngr.GetDevice();
		ASSERT( IsSupported( dev, desc, EMemoryType(memObj.MemoryType()) ));

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
		
		ASSERT( IsSupported( dev, _desc, EMemoryType::Default ));

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
		
		{
			SHAREDLOCK( _viewMapLock );
			for (auto& view : _viewMap) {
				dev.vkDestroyBufferView( dev.GetVkDevice(), view.second, null );
			}
			_viewMap.clear();
		}

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
	GetView
=================================================
*/
	VkBufferView  VBuffer::GetView (const VDevice &dev, const BufferViewDesc &desc) const
	{
		SHAREDLOCK( _drCheck );

		// find already created image view
		{
			SHAREDLOCK( _viewMapLock );

			auto	iter = _viewMap.find( desc );

			if ( iter != _viewMap.end() )
				return iter->second;
		}

		// create new image view
		EXLOCK( _viewMapLock );

		auto[iter, inserted] = _viewMap.insert({ desc, VK_NULL_HANDLE });

		if ( not inserted )
			return iter->second;	// other thread create view before
		
		CHECK_ERR( _CreateView( dev, desc, OUT iter->second ));

		return iter->second;
	}

/*
=================================================
	_CreateView
=================================================
*/
	bool  VBuffer::_CreateView (const VDevice &dev, const BufferViewDesc &desc, OUT VkBufferView &outView) const
	{
		VkBufferViewCreateInfo	info = {};
		info.sType		= VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		info.flags		= 0;
		info.buffer		= _buffer;
		info.format		= VEnumCast( desc.format );
		info.offset		= VkDeviceSize(desc.offset);
		info.range		= VkDeviceSize(desc.size);

		VK_CHECK( dev.vkCreateBufferView( dev.GetVkDevice(), &info, null, OUT &outView ));
		return true;
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
	
/*
=================================================
	GetApiSpecificDescription
=================================================
*/
	VulkanBufferDesc  VBuffer::GetApiSpecificDescription () const
	{
		VulkanBufferDesc	desc;
		desc.buffer			= BitCast<BufferVk_t>( _buffer );
		desc.usage			= BitCast<BufferUsageFlagsVk_t>( VEnumCast( _desc.usage ));
		desc.size			= _desc.size;
		desc.queueFamily	= VK_QUEUE_FAMILY_IGNORED;
		//desc.queueFamilyIndices	// TODO
		return desc;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VBuffer::IsSupported (const VDevice &dev, const BufferDesc &desc, EMemoryType memType)
	{
		FG_UNUSED( memType );

		for (EBufferUsage t = EBufferUsage(1); t <= desc.usage; t = EBufferUsage(uint(t) << 1))
		{
			if ( not EnumEq( desc.usage, t ))
				continue;

			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EBufferUsage::UniformTexel :		break;
				case EBufferUsage::StorageTexel :		break;
				case EBufferUsage::StorageTexelAtomic:	break;
				case EBufferUsage::TransferSrc :		break;
				case EBufferUsage::TransferDst :		break;
				case EBufferUsage::Uniform :			break;
				case EBufferUsage::Storage :			break;
				case EBufferUsage::Index :				break;
				case EBufferUsage::Vertex :				break;
				case EBufferUsage::Indirect :			break;
				case EBufferUsage::RayTracing :			if ( not dev.IsRayTracingEnabled() ) return false;								break;
				case EBufferUsage::VertexPplnStore :	if ( not dev.GetDeviceFeatures().vertexPipelineStoresAndAtomics ) return false;	break;
				case EBufferUsage::FragmentPplnStore :	if ( not dev.GetDeviceFeatures().fragmentStoresAndAtomics ) return false;		break;
				case EBufferUsage::_Last :
				case EBufferUsage::All :
				case EBufferUsage::Transfer :
				case EBufferUsage::Unknown :
				default :								ASSERT(false);	break;
			}
			END_ENUM_CHECKS();
		}
		return true;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool  VBuffer::IsSupported (const VDevice &dev, const BufferViewDesc &desc) const
	{
		SHAREDLOCK( _drCheck );

		VkFormatProperties	props = {};
		vkGetPhysicalDeviceFormatProperties( dev.GetVkPhysicalDevice(), VEnumCast( desc.format ), OUT &props );
		
		const VkFormatFeatureFlags	dev_flags	= props.bufferFeatures;
		VkFormatFeatureFlags		buf_flags	= 0;
		
		for (EBufferUsage t = EBufferUsage(1); t <= _desc.usage; t = EBufferUsage(uint(t) << 1))
		{
			if ( not EnumEq( _desc.usage, t ))
				continue;

			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EBufferUsage::UniformTexel :		buf_flags |= VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;		break;
				case EBufferUsage::StorageTexel :		buf_flags |= VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT;		break;
				case EBufferUsage::StorageTexelAtomic:	buf_flags |= VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;	break;
				case EBufferUsage::TransferSrc :		break;
				case EBufferUsage::TransferDst :		break;
				case EBufferUsage::Uniform :			break;
				case EBufferUsage::Storage :			break;
				case EBufferUsage::Index :				break;
				case EBufferUsage::Vertex :				break;
				case EBufferUsage::Indirect :			break;
				case EBufferUsage::RayTracing :			break;
				case EBufferUsage::VertexPplnStore :	break;
				case EBufferUsage::FragmentPplnStore :	break;
				case EBufferUsage::_Last :
				case EBufferUsage::All :
				case EBufferUsage::Transfer :
				case EBufferUsage::Unknown :
				default :								ASSERT(false);	break;
			}
			END_ENUM_CHECKS();
		}

		return (dev_flags & buf_flags);
	}

}	// FG
