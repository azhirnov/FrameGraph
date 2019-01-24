// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VMemoryObj.h"

namespace FG
{

	//
	// Vulkan Memory Manager
	//

	class VMemoryManager final : public std::enable_shared_from_this<VMemoryManager>
	{
	// types
	public:

	private:
		using Storage_t		= VMemoryObj::Storage_t;
		using MemoryInfo_t	= VMemoryObj::MemoryInfo;

		class DedicatedMemAllocator;
		class HostMemAllocator;
		class DeviceMemAllocator;
		class VirtualMemAllocator;
		class VulkanMemoryAllocator;


		class IMemoryAllocator
		{
		// interface
		public:
			virtual ~IMemoryAllocator () {}
			
			virtual bool IsSupported (EMemoryType memType) const = 0;
			
			virtual bool AllocForImage (VkImage image, const MemoryDesc &desc, OUT Storage_t &data) = 0;
			virtual bool AllocForBuffer (VkBuffer buffer, const MemoryDesc &desc, OUT Storage_t &data) = 0;
			virtual bool AllocateForAccelStruct (VkAccelerationStructureNV as, const MemoryDesc &desc, OUT Storage_t &data) = 0;

			virtual bool Dealloc (INOUT Storage_t &data, OUT AppendableVkResources_t) = 0;
			
			virtual bool GetMemoryInfo (const Storage_t &data, OUT MemoryInfo_t &info) const = 0;
		};

		using AllocatorPtr	= UniquePtr< IMemoryAllocator >;
		using Allocators_t	= FixedArray< AllocatorPtr, 16 >;
		

	// variables
	private:
		VDevice const &		_device;
		Allocators_t		_allocators;


	// methods
	public:
		VMemoryManager (const VDevice &dev);
		~VMemoryManager ();

		bool Initialize ();
		void Deinitialize ();

		bool AllocateForImage (VkImage image, const MemoryDesc &desc, OUT Storage_t &data);
		bool AllocateForBuffer (VkBuffer buffer, const MemoryDesc &desc, OUT Storage_t &data);
		bool AllocateForAccelStruct (VkAccelerationStructureNV as, const MemoryDesc &desc, OUT Storage_t &data);
		bool Deallocate (INOUT Storage_t &data, OUT AppendableVkResources_t);

		bool GetMemoryInfo (const Storage_t &data, OUT MemoryInfo_t &info) const;


	private:
		ND_ AllocatorPtr  _CreateVMA ();
	};


}	// FG
