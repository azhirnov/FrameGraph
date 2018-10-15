// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VMemoryManager.h"
#include "VDevice.h"

#ifdef FG_ENABLE_VULKAN_MEMORY_ALLOCATOR

#define VMA_USE_STL_CONTAINERS		1
#define VMA_STATIC_VULKAN_FUNCTIONS	0
#define VMA_RECORDING_ENABLED		0
#define VMA_DEDICATED_ALLOCATION	1
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"

namespace FG
{

	//
	// AMD Memory Allocator
	//

	class VMemoryManager::AMDMemAllocator final : public IMemoryAllocator
	{
	// variables
	private:
		VDevice const&		_device;
		VmaAllocator		_allocator;


	// methods
	public:
		AMDMemAllocator (const VDevice &dev, EMemoryTypeExt memType);
        ~AMDMemAllocator () override;

		bool IsSupported (EMemoryTypeExt memType) const override;
			
		bool AllocForImage (uint frameIdx, VkImage image, EMemoryTypeExt memType, OUT VMemoryPtr &handle) override;
		bool AllocForBuffer (uint frameIdx, VkBuffer buffer, EMemoryTypeExt memType, OUT VMemoryPtr &handle) override;

		bool Dealloc (uint frameIdx, VMemoryPtr handle) override;
		void OnBeginFrame (uint frameIdx) override;
		
		bool MapMemory (uint frameIdx, VMemoryPtr handle, EMemoryMapFlags flags, ArrayView<MemRange> InvalidateRanges, OUT void* &ptr) override;
		bool UnmapMemory (uint frameIdx, VMemoryPtr handle, ArrayView<MemRange> flushRanges) override;

	private:
		bool _CreateAllocator (OUT VmaAllocator &alloc) const;

		ND_ static VmaMemoryUsage			_ConvertToMemoryUsage (EMemoryTypeExt memType);
		ND_ static VkMemoryPropertyFlags	_ConvertToMemoryProperties (EMemoryTypeExt memType);

		ND_ static VmaAllocation			_CastAllocation (VMemoryPtr mem);
		ND_ static VMemoryPtr				_CastMemory (VmaAllocation alloc);
	};
	

	
/*
=================================================
	constructor
=================================================
*/
	VMemoryManager::AMDMemAllocator::AMDMemAllocator (const VDevice &dev, EMemoryTypeExt) :
		_device{ dev },
		_allocator{ null }
	{
		CHECK( _CreateAllocator( OUT _allocator ) );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VMemoryManager::AMDMemAllocator::~AMDMemAllocator ()
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
	bool VMemoryManager::AMDMemAllocator::IsSupported (EMemoryTypeExt) const
	{
		return true;
	}
	
/*
=================================================
	AllocForImage
=================================================
*/
	bool VMemoryManager::AMDMemAllocator::AllocForImage (uint, VkImage image, EMemoryTypeExt memType, OUT VMemoryPtr &handle)
	{
		VmaAllocationCreateInfo		info = {};
		info.flags			= 0;
		info.usage			= _ConvertToMemoryUsage( memType );
		info.requiredFlags	= _ConvertToMemoryProperties( memType );
		info.preferredFlags	= 0;
		info.memoryTypeBits	= 0;
		info.pool			= VK_NULL_HANDLE;
		info.pUserData		= null;

		VmaAllocation	mem;

		VK_CHECK( vmaAllocateMemoryForImage( _allocator, image, &info, OUT &mem, null ));

		VK_CHECK( vmaBindImageMemory( _allocator, mem, image ));
		
		handle = _CastMemory( mem );
		return true;
	}
	
/*
=================================================
	AllocForBuffer
=================================================
*/
	bool VMemoryManager::AMDMemAllocator::AllocForBuffer (uint, VkBuffer buffer, EMemoryTypeExt memType, OUT VMemoryPtr &handle)
	{
		VmaAllocationCreateInfo		info = {};
		info.flags			= 0;
		info.usage			= _ConvertToMemoryUsage( memType );
		info.requiredFlags	= _ConvertToMemoryProperties( memType );
		info.preferredFlags	= 0;
		info.memoryTypeBits	= 0;
		info.pool			= VK_NULL_HANDLE;
		info.pUserData		= null;

		VmaAllocation	mem = null;

		VK_CHECK( vmaAllocateMemoryForBuffer( _allocator, buffer, &info, OUT &mem, null ));

		VK_CHECK( vmaBindBufferMemory( _allocator, mem, buffer ));

		handle = _CastMemory( mem );
		return true;
	}
	
/*
=================================================
	Dealloc
=================================================
*/
	bool VMemoryManager::AMDMemAllocator::Dealloc (uint, VMemoryPtr handle)
	{
		vmaFreeMemory( _allocator, _CastAllocation( handle ) );
		return true;
	}
	
/*
=================================================
	OnBeginFrame
=================================================
*/
	void VMemoryManager::AMDMemAllocator::OnBeginFrame (uint)
	{
	}
	
/*
=================================================
	MapMemory
=================================================
*/
	bool VMemoryManager::AMDMemAllocator::MapMemory (uint, VMemoryPtr handle, EMemoryMapFlags flags, ArrayView<MemRange> InvalidateRanges, OUT void* &ptr)
	{
		VmaAllocation	mem = _CastAllocation( handle );

		VK_CHECK( vmaMapMemory( _allocator, mem, OUT &ptr ));

		// make device changes visible in host
		if ( flags == EMemoryMapFlags::WriteDiscard )
		{
			ASSERT( InvalidateRanges.empty() );	// invalidate ranged was ignored becouse of mapped with 'WriteDiscard' flag
		}
		else
		{
			VmaAllocationInfo	info = {};
			vmaGetAllocationInfo( _allocator, mem, OUT &info );

			FixedArray< VkMappedMemoryRange, FG_MaxFlushMemRanges >		ranges;
			ASSERT( ranges.capacity() >= InvalidateRanges.size() );

			if ( InvalidateRanges.empty() )
			{
				VkMappedMemoryRange	range;
				range.sType		= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				range.pNext		= null;
				range.memory	= info.deviceMemory;
				range.offset	= info.offset;
				range.size		= info.size;
				ranges.push_back( std::move(range) );
			}
			else
			for (auto& src : InvalidateRanges)
			{
				VkMappedMemoryRange		dst = {};
				dst.sType	= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				dst.pNext	= null;
				dst.memory	= info.deviceMemory;
				dst.offset	= info.offset + VkDeviceSize(src.offset);
				dst.size	= VkDeviceSize(src.size);
				ranges.push_back( std::move(dst) );
			}
				
			VK_CALL( _device.vkInvalidateMappedMemoryRanges( _device.GetVkDevice(), uint(ranges.size()), ranges.data() ));
		}
		return true;
	}
	
/*
=================================================
	UnmapMemory
=================================================
*/
	bool VMemoryManager::AMDMemAllocator::UnmapMemory (uint, VMemoryPtr handle, ArrayView<MemRange> flushRanges)
	{
		VmaAllocation	mem = _CastAllocation( handle );

		// make host changes visible in device
		if ( not flushRanges.empty() )
		{
			VmaAllocationInfo	info = {};
			vmaGetAllocationInfo( _allocator, mem, OUT &info );

			FixedArray< VkMappedMemoryRange, FG_MaxFlushMemRanges >		ranges;
			ASSERT( ranges.capacity() >= flushRanges.size() );

			for (auto& src : flushRanges)
			{
				VkMappedMemoryRange		dst = {};
				dst.sType	= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				dst.pNext	= null;
				dst.memory	= info.deviceMemory;
				dst.offset	= info.offset + VkDeviceSize(src.offset);
				dst.size	= VkDeviceSize(src.size);

				ranges.push_back( std::move(dst) );
			}

			VK_CALL( _device.vkFlushMappedMemoryRanges( _device.GetVkDevice(), uint(ranges.size()), ranges.data() ));
		}

		vmaUnmapMemory( _allocator, mem );
		return true;
	}
	
/*
=================================================
	_ConvertToMemoryUsage
=================================================
*/
	VmaMemoryUsage  VMemoryManager::AMDMemAllocator::_ConvertToMemoryUsage (EMemoryTypeExt memType)
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
	VkMemoryPropertyFlags  VMemoryManager::AMDMemAllocator::_ConvertToMemoryProperties (EMemoryTypeExt values)
	{
		VkMemoryPropertyFlags	flags = 0;

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
	_CastAllocation
=================================================
*/
	VmaAllocation  VMemoryManager::AMDMemAllocator::_CastAllocation (VMemoryPtr mem)
	{
		return BitCast<VmaAllocation>( mem );
	}
	
/*
=================================================
	_CastMemory
=================================================
*/
	VMemoryManager::VMemoryPtr  VMemoryManager::AMDMemAllocator::_CastMemory (VmaAllocation alloc)
	{
		return BitCast<VMemoryPtr>( alloc );
	}

/*
=================================================
	_CreateAllocator
=================================================
*/
	bool VMemoryManager::AMDMemAllocator::_CreateAllocator (OUT VmaAllocator &alloc) const
	{
		VmaVulkanFunctions		funcs = {};
		funcs.vkGetPhysicalDeviceProperties			= vkGetPhysicalDeviceProperties;
		funcs.vkGetPhysicalDeviceMemoryProperties	= vkGetPhysicalDeviceMemoryProperties;
        funcs.vkAllocateMemory						= BitCast<PFN_vkAllocateMemory>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkAllocateMemory" ));
        funcs.vkFreeMemory							= BitCast<PFN_vkFreeMemory>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkFreeMemory" ));
        funcs.vkMapMemory							= BitCast<PFN_vkMapMemory>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkMapMemory" ));
        funcs.vkUnmapMemory							= BitCast<PFN_vkUnmapMemory>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkUnmapMemory" ));
        funcs.vkBindBufferMemory					= BitCast<PFN_vkBindBufferMemory>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkBindBufferMemory" ));
        funcs.vkBindImageMemory						= BitCast<PFN_vkBindImageMemory>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkBindImageMemory" ));
        funcs.vkGetBufferMemoryRequirements			= BitCast<PFN_vkGetBufferMemoryRequirements>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkGetBufferMemoryRequirements" ));
        funcs.vkGetImageMemoryRequirements			= BitCast<PFN_vkGetImageMemoryRequirements>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkGetImageMemoryRequirements" ));
		funcs.vkCreateBuffer						= null;
		funcs.vkDestroyBuffer						= null;
		funcs.vkCreateImage							= null;
		funcs.vkDestroyImage						= null;
        funcs.vkGetBufferMemoryRequirements2KHR		= BitCast<PFN_vkGetBufferMemoryRequirements2KHR>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkGetBufferMemoryRequirements2KHR" ));
        funcs.vkGetImageMemoryRequirements2KHR		= BitCast<PFN_vkGetImageMemoryRequirements2KHR>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkGetImageMemoryRequirements2KHR" ));
        funcs.vkFlushMappedMemoryRanges             = BitCast<PFN_vkFlushMappedMemoryRanges>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkFlushMappedMemoryRanges" ));
        funcs.vkInvalidateMappedMemoryRanges        = BitCast<PFN_vkInvalidateMappedMemoryRanges>(vkGetDeviceProcAddr( _device.GetVkDevice(), "vkInvalidateMappedMemoryRanges" ));

		VmaAllocatorCreateInfo	info = {};
		info.flags			= VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
		info.physicalDevice	= _device.GetVkPhysicalDevice();
		info.device			= _device.GetVkDevice();

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
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"

#ifdef COMPILER_GCC
#   pragma GCC diagnostic pop
#endif

#ifdef COMPILER_MSVC
#	pragma warning (pop)
#endif

#endif	// FG_ENABLE_VULKAN_MEMORY_ALLOCATOR
