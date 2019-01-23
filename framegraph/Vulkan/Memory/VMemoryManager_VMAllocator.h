// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VMemoryManager.h"
#include "VDevice.h"

#ifdef FG_ENABLE_VULKAN_MEMORY_ALLOCATOR

#define VMA_USE_STL_CONTAINERS		1
#define VMA_STATIC_VULKAN_FUNCTIONS	0
#define VMA_RECORDING_ENABLED		0
#define VMA_DEDICATED_ALLOCATION	1

#ifdef COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef COMPILER_MSVC
#	pragma warning (push)
#	pragma warning (disable: 4100)
#	pragma warning (disable: 4127)
#endif

#define VMA_IMPLEMENTATION	1
#define VMA_ASSERT(expr)	{}
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"

#ifdef COMPILER_GCC
#   pragma GCC diagnostic pop
#endif

#ifdef COMPILER_MSVC
#	pragma warning (pop)
#endif


namespace FG
{

	//
	// Vulkan Memory Allocator
	//

	class VMemoryManager::VulkanMemoryAllocator final : public IMemoryAllocator
	{
	// types
	private:
		struct Data
		{
			VmaAllocation	allocation;
		};


	// variables
	private:
		VDevice const&		_device;
		VmaAllocator		_allocator;


	// methods
	public:
		VulkanMemoryAllocator (const VDevice &dev, EMemoryTypeExt memType);
		~VulkanMemoryAllocator () override;

		bool IsSupported (EMemoryType memType) const override;
			
		bool AllocForImage (VkImage image, const MemoryDesc &mem, OUT Storage_t &data) override;
		bool AllocForBuffer (VkBuffer buffer, const MemoryDesc &mem, OUT Storage_t &data) override;
		bool AllocateForAccelStruct (VkAccelerationStructureNV as, const MemoryDesc &desc, OUT Storage_t &data) override;

		bool Dealloc (INOUT Storage_t &data, OUT AppendableVkResources_t) override;
		
		bool GetMemoryInfo (const Storage_t &data, OUT MemoryInfo_t &info) const override;

	private:
		bool _CreateAllocator (OUT VmaAllocator &alloc) const;

		ND_ static Data *					_CastStorage (Storage_t &data);
		ND_ static Data const*				_CastStorage (const Storage_t &data);
		
		ND_ static VmaAllocationCreateFlags	_ConvertToMemoryFlags (EMemoryType memType);
		ND_ static VmaMemoryUsage			_ConvertToMemoryUsage (EMemoryType memType);
		ND_ static VkMemoryPropertyFlags	_ConvertToMemoryProperties (EMemoryType memType);
	};
	

	
/*
=================================================
	constructor
=================================================
*/
	VMemoryManager::VulkanMemoryAllocator::VulkanMemoryAllocator (const VDevice &dev, EMemoryTypeExt) :
		_device{ dev },		_allocator{ null }
	{
		CHECK( _CreateAllocator( OUT _allocator ) );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VMemoryManager::VulkanMemoryAllocator::~VulkanMemoryAllocator ()
	{
		if ( _allocator ) {
			vmaDestroyAllocator( _allocator );
		}
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool VMemoryManager::VulkanMemoryAllocator::IsSupported (EMemoryType) const
	{
		return true;
	}
	
/*
=================================================
	_CastStorage
=================================================
*/
	VMemoryManager::VulkanMemoryAllocator::Data *
		VMemoryManager::VulkanMemoryAllocator::_CastStorage (Storage_t &data)
	{
		return data.Cast<Data>( SizeOf<uint> );
	}

	VMemoryManager::VulkanMemoryAllocator::Data const *
		VMemoryManager::VulkanMemoryAllocator::_CastStorage (const Storage_t &data)
	{
		return data.Cast<Data>( SizeOf<uint> );
	}

/*
=================================================
	AllocForImage
=================================================
*/
	bool VMemoryManager::VulkanMemoryAllocator::AllocForImage (VkImage image, const MemoryDesc &desc, OUT Storage_t &data)
	{
		VmaAllocationCreateInfo		info = {};
		info.flags			= _ConvertToMemoryFlags( desc.type );
		info.usage			= _ConvertToMemoryUsage( desc.type );
		info.requiredFlags	= _ConvertToMemoryProperties( desc.type );
		info.preferredFlags	= 0;
		info.memoryTypeBits	= 0;
		info.pool			= VK_NULL_HANDLE;
		info.pUserData		= null;

		VmaAllocation	mem = null;
		VK_CHECK( vmaAllocateMemoryForImage( _allocator, image, &info, OUT &mem, null ));

		VK_CHECK( vmaBindImageMemory( _allocator, mem, image ));
		
		_CastStorage( data )->allocation = mem;
		return true;
	}
	
/*
=================================================
	AllocForBuffer
=================================================
*/
	bool VMemoryManager::VulkanMemoryAllocator::AllocForBuffer (VkBuffer buffer, const MemoryDesc &desc, OUT Storage_t &data)
	{
		VmaAllocationCreateInfo		info = {};
		info.flags			= _ConvertToMemoryFlags( desc.type );
		info.usage			= _ConvertToMemoryUsage( desc.type );
		info.requiredFlags	= _ConvertToMemoryProperties( desc.type );
		info.preferredFlags	= 0;
		info.memoryTypeBits	= 0;
		info.pool			= VK_NULL_HANDLE;
		info.pUserData		= null;

		VmaAllocation	mem = null;
		VK_CHECK( vmaAllocateMemoryForBuffer( _allocator, buffer, &info, OUT &mem, null ));

		VK_CHECK( vmaBindBufferMemory( _allocator, mem, buffer ));
		
		_CastStorage( data )->allocation = mem;
		return true;
	}
	
/*
=================================================
	AllocateForAccelStruct
=================================================
*/
	bool VMemoryManager::VulkanMemoryAllocator::AllocateForAccelStruct (VkAccelerationStructureNV accelStruct, const MemoryDesc &desc, OUT Storage_t &data)
	{
		VkAccelerationStructureMemoryRequirementsInfoNV	mem_info = {};
		mem_info.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		mem_info.type					= VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
		mem_info.accelerationStructure	= accelStruct;

		VkMemoryRequirements2	mem_req = {};
		_device.vkGetAccelerationStructureMemoryRequirementsNV( _device.GetVkDevice(), &mem_info, OUT &mem_req );
		
		VmaAllocationCreateInfo		info = {};
		info.flags			= _ConvertToMemoryFlags( desc.type );
		info.usage			= _ConvertToMemoryUsage( desc.type );
		info.requiredFlags	= _ConvertToMemoryProperties( desc.type );
		info.preferredFlags	= 0;
		info.memoryTypeBits	= 0;
		info.pool			= VK_NULL_HANDLE;
		info.pUserData		= null;
		
		VmaAllocation	mem = null;
		VK_CHECK( _allocator->AllocateMemory( mem_req.memoryRequirements, false, false, VK_NULL_HANDLE, VK_NULL_HANDLE, info, VMA_SUBALLOCATION_TYPE_UNKNOWN, 1, OUT &mem ));
		
		VmaAllocationInfo	alloc_info	= {};
		vmaGetAllocationInfo( _allocator, mem, OUT &alloc_info );
		
		VkBindAccelerationStructureMemoryInfoNV		bind_info = {};
		bind_info.sType					= VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		bind_info.accelerationStructure	= accelStruct;
		bind_info.memory				= alloc_info.deviceMemory;
		bind_info.memoryOffset			= alloc_info.offset;
		VK_CHECK( _device.vkBindAccelerationStructureMemoryNV( _device.GetVkDevice(), 1, &bind_info ));

		_CastStorage( data )->allocation = mem;
		return true;
	}

/*
=================================================
	Dealloc
=================================================
*/
	bool VMemoryManager::VulkanMemoryAllocator::Dealloc (INOUT Storage_t &data, OUT AppendableVkResources_t)
	{
		VmaAllocation&	mem = _CastStorage( data )->allocation;

		vmaFreeMemory( _allocator, mem );

		mem = null;
		return true;
	}
	
/*
=================================================
	GetMemoryInfo
=================================================
*/
	bool VMemoryManager::VulkanMemoryAllocator::GetMemoryInfo (const Storage_t &data, OUT MemoryInfo_t &info) const
	{
		VmaAllocation		mem			= _CastStorage( data )->allocation;
		VmaAllocationInfo	alloc_info	= {};
		vmaGetAllocationInfo( _allocator, mem, OUT &alloc_info );
		
		const auto&		mem_props = _device.GetDeviceMemoryProperties();
		ASSERT( alloc_info.memoryType < mem_props.memoryTypeCount );

		info.mem		= alloc_info.deviceMemory;
		info.flags		= mem_props.memoryTypes[ alloc_info.memoryType ].propertyFlags;
		info.offset		= BytesU(alloc_info.offset);
		info.size		= BytesU(alloc_info.size);
		info.mappedPtr	= alloc_info.pMappedData;
		return true;
	}
	
/*
=================================================
	_ConvertToMemoryFlags
=================================================
*/
	VmaAllocationCreateFlags  VMemoryManager::VulkanMemoryAllocator::_ConvertToMemoryFlags (EMemoryType memType)
	{
		VmaAllocationCreateFlags	result = 0;

		if ( EnumEq( memType, EMemoryType::Dedicated ))
			result |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		
		if ( EnumEq( memType, EMemoryTypeExt::HostRead ) or EnumEq( memType, EMemoryTypeExt::HostWrite ))
			result |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

		// TODO: VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT

		return result;
	}

/*
=================================================
	_ConvertToMemoryUsage
=================================================
*/
	VmaMemoryUsage  VMemoryManager::VulkanMemoryAllocator::_ConvertToMemoryUsage (EMemoryType memType)
	{
		if ( EnumEq( memType, EMemoryTypeExt::HostRead ) )
			return VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_TO_CPU;

		if ( EnumEq( memType, EMemoryTypeExt::HostWrite ) )
			return VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;

		return VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
	}
	
/*
=================================================
	_ConvertToMemoryProperties
=================================================
*/
	VkMemoryPropertyFlags  VMemoryManager::VulkanMemoryAllocator::_ConvertToMemoryProperties (EMemoryType memType)
	{
		const EMemoryTypeExt	values	= EMemoryTypeExt(memType);
		VkMemoryPropertyFlags	flags	= 0;

		for (EMemoryTypeExt t = EMemoryTypeExt(1 << 0); t < EMemoryTypeExt::_Last; t = EMemoryTypeExt(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;

			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case EMemoryTypeExt::HostRead :			flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;	break;
				case EMemoryTypeExt::HostWrite :		flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;	break;
				case EMemoryTypeExt::LocalInGPU :		flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;	break;
				case EMemoryTypeExt::HostCoherent :		flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;	break;
				case EMemoryTypeExt::HostCached :		flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;	break;
				case EMemoryTypeExt::Dedicated :
				case EMemoryTypeExt::AllowAliasing :
				case EMemoryTypeExt::Sparse :
				case EMemoryTypeExt::ForBuffer :
				case EMemoryTypeExt::ForImage :			break;
				case EMemoryTypeExt::_Last :
				case EMemoryTypeExt::All :
				case EMemoryTypeExt::HostVisible :
				case EMemoryTypeExt::Virtual :			break;	// to shutup warnings
				default :								RETURN_ERR( "unknown memory type flag!" );
			}
			DISABLE_ENUM_CHECKS();
		}
		return flags;
	}

/*
=================================================
	_CreateAllocator
=================================================
*/
	bool VMemoryManager::VulkanMemoryAllocator::_CreateAllocator (OUT VmaAllocator &alloc) const
	{
		VkDevice				dev = _device.GetVkDevice();
		VmaVulkanFunctions		funcs = {};

		funcs.vkGetPhysicalDeviceProperties			= vkGetPhysicalDeviceProperties;
		funcs.vkGetPhysicalDeviceMemoryProperties	= vkGetPhysicalDeviceMemoryProperties;
		funcs.vkAllocateMemory						= BitCast<PFN_vkAllocateMemory>(vkGetDeviceProcAddr( dev, "vkAllocateMemory" ));
		funcs.vkFreeMemory							= BitCast<PFN_vkFreeMemory>(vkGetDeviceProcAddr( dev, "vkFreeMemory" ));
		funcs.vkMapMemory							= BitCast<PFN_vkMapMemory>(vkGetDeviceProcAddr( dev, "vkMapMemory" ));
		funcs.vkUnmapMemory							= BitCast<PFN_vkUnmapMemory>(vkGetDeviceProcAddr( dev, "vkUnmapMemory" ));
		funcs.vkBindBufferMemory					= BitCast<PFN_vkBindBufferMemory>(vkGetDeviceProcAddr( dev, "vkBindBufferMemory" ));
		funcs.vkBindImageMemory						= BitCast<PFN_vkBindImageMemory>(vkGetDeviceProcAddr( dev, "vkBindImageMemory" ));
		funcs.vkGetBufferMemoryRequirements			= BitCast<PFN_vkGetBufferMemoryRequirements>(vkGetDeviceProcAddr( dev, "vkGetBufferMemoryRequirements" ));
		funcs.vkGetImageMemoryRequirements			= BitCast<PFN_vkGetImageMemoryRequirements>(vkGetDeviceProcAddr( dev, "vkGetImageMemoryRequirements" ));
		funcs.vkCreateBuffer						= null;
		funcs.vkDestroyBuffer						= null;
		funcs.vkCreateImage							= null;
		funcs.vkDestroyImage						= null;
		funcs.vkGetBufferMemoryRequirements2KHR		= BitCast<PFN_vkGetBufferMemoryRequirements2KHR>(vkGetDeviceProcAddr( dev, "vkGetBufferMemoryRequirements2KHR" ));
		funcs.vkGetImageMemoryRequirements2KHR		= BitCast<PFN_vkGetImageMemoryRequirements2KHR>(vkGetDeviceProcAddr( dev, "vkGetImageMemoryRequirements2KHR" ));
		funcs.vkFlushMappedMemoryRanges				= BitCast<PFN_vkFlushMappedMemoryRanges>(vkGetDeviceProcAddr( dev, "vkFlushMappedMemoryRanges" ));
		funcs.vkInvalidateMappedMemoryRanges		= BitCast<PFN_vkInvalidateMappedMemoryRanges>(vkGetDeviceProcAddr( dev, "vkInvalidateMappedMemoryRanges" ));

		VmaAllocatorCreateInfo	info = {};
		info.flags			= VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
		info.physicalDevice	= _device.GetVkPhysicalDevice();
		info.device			= dev;

		info.preferredLargeHeapBlockSize	= VkDeviceSize(FG_VkDevicePageSizeMb) << 20;
		info.pAllocationCallbacks			= null;
		info.pDeviceMemoryCallbacks			= null;
		//info.frameInUseCount	// ignore
		info.pHeapSizeLimit					= null;		// TODO
		info.pVulkanFunctions				= &funcs;

		VK_CHECK( vmaCreateAllocator( &info, OUT &alloc ));
		return true;
	}


}	// FG

#endif	// FG_ENABLE_VULKAN_MEMORY_ALLOCATOR
