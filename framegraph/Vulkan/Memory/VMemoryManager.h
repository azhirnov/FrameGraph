// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "VEnums.h"

namespace FG
{

	//
	// Vulkan Memory Manager
	//

	class VMemoryManager final
	{
	// types
	public:
		struct MemRange
		{
			BytesU		offset;
			BytesU		size;
		};

		using VMemoryPtr	= struct _VMemory_t*;


		class IMemoryAllocator
		{
		// interface
		public:
			virtual ~IMemoryAllocator () {}
			
			virtual bool IsSupported (EMemoryTypeExt memType) const = 0;
			
			virtual bool AllocForImage (uint frameIdx, VkImage image, EMemoryTypeExt memType, OUT VMemoryPtr &handle) = 0;
			virtual bool AllocForBuffer (uint frameIdx, VkBuffer buffer, EMemoryTypeExt memType, OUT VMemoryPtr &handle) = 0;

			virtual bool Dealloc (uint frameIdx, VMemoryPtr handle) = 0;
			virtual void OnBeginFrame (uint frameIdx) = 0;
			
			virtual bool MapMemory (uint frameIdx, VMemoryPtr handle, EMemoryMapFlags flags, ArrayView<MemRange> InvalidateRanges, OUT void* &ptr) = 0;
			virtual bool UnmapMemory (uint frameIdx, VMemoryPtr handle, ArrayView<MemRange> flushRanges) = 0;
		};


		using AllocatorPtr	= UniquePtr< IMemoryAllocator >;
		using Allocators_t	= FixedArray< AllocatorPtr, 16 >;

		class DedicatedMemAllocator;
		class HostMemAllocator;
		class DeviceMemAllocator;
		class VirtualMemAllocator;
		class AMDMemAllocator;


	// variables
	private:
		VDevice const &		_device;
		Allocators_t		_allocators;
		uint				_currFrame;


	// methods
	public:
		VMemoryManager (const VDevice &dev);
		~VMemoryManager ();

		bool Initialize ();
		void Deinitialize ();

		void OnBeginFrame (uint frameIdx);
		void OnEndFrame ();

		bool AllocForImage (VkImage image, EMemoryTypeExt memType, OUT VMemoryHandle &handle);
		bool AllocForBuffer (VkBuffer buffer, EMemoryTypeExt memType, OUT VMemoryHandle &handle);
		bool Dealloc (VMemoryHandle handle);
		
		bool MapMemory (OUT void* &ptr, VMemoryHandle handle, EMemoryMapFlags flags, ArrayView<MemRange> InvalidateRanges = Default);
		bool UnmapMemory (VMemoryHandle handle, ArrayView<MemRange> flushRanges = Default);
	};


}	// FG
