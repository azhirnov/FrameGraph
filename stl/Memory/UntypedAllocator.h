// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Memory/MemUtils.h"

namespace FG
{

	template <typename T>
	struct StdAllocator;


	//
	// Untyped Default Allocator
	//

	struct UntypedAllocator
	{
	// types
		template <typename T>	using StdAllocator_t = StdAllocator<T>;

	// methods
		ND_ FG_ALLOCATOR static void*  Allocate (BytesU size)
		{
			return ::operator new ( size_t(size), std::nothrow_t{} );
		}

		static void  Deallocate (void *ptr)
		{
			::operator delete ( ptr, std::nothrow_t() );
		}

		static void  Deallocate (void *ptr, BytesU size)
		{
			::operator delete ( ptr, size_t(size) );
		}

		ND_ bool  operator == (const UntypedAllocator &) const
		{
			return true;
		}
	};
	


	//
	// Untyped Aligned Allocator
	//

	struct UntypedAlignedAllocator
	{
	// types
		template <typename T>	using StdAllocator_t = StdAllocator<T>;

	// methods
		ND_ FG_ALLOCATOR static void*  Allocate (BytesU size, BytesU align)
		{
			return ::operator new ( size_t(size), std::align_val_t(size_t(align)), std::nothrow_t{} );
		}

		static void  Deallocate (void *ptr, BytesU align)
		{
			::operator delete ( ptr, std::align_val_t(size_t(align)), std::nothrow_t() );
		}

		static void  Deallocate (void *ptr, BytesU size, BytesU align)
		{
			::operator delete ( ptr, size_t(size), std::align_val_t(size_t(align)) );
		}

		ND_ bool  operator == (const UntypedAlignedAllocator &) const
		{
			return true;
		}
	};

	

	//
	// STD Allocator
	//

	template <typename T>
	struct StdAllocator final : std::allocator<T>
	{
		StdAllocator () {}
		StdAllocator (const UntypedAllocator &) {}
		StdAllocator (const UntypedAlignedAllocator &) {}
	};


}	// FG
