// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/Math/Bytes.h"
#include "stl/Memory/UntypedAllocator.h"

namespace FG
{
	template <typename AllocatorType>				struct PoolAllocator;
	template <typename T, typename AllocatorType>	struct StdPoolAllocator;
	template <typename AllocatorType>				struct UntypedPoolAllocator;



	//
	// Pool Allocator
	//

	template <typename AllocatorType = UntypedAllocator>
	struct PoolAllocator final
	{
	// types
	private:
		struct Block
		{
			void *		ptr			= null;
			BytesU		size;		// used memory size
			BytesU		capacity;	// size of block
		};

		using Allocator_t	= AllocatorType;


	// variables
	private:
		Array< Block >		_blocks;
		BytesU				_blockSize	= 1024_b;
		Allocator_t			_alloc;


	// methods
	public:
		PoolAllocator () {}
		explicit PoolAllocator (const Allocator_t &alloc) : _alloc{alloc} {}

		PoolAllocator (const PoolAllocator &) = delete;
		PoolAllocator (PoolAllocator &&) = delete;

		PoolAllocator& operator = (const PoolAllocator &) = delete;
		PoolAllocator& operator = (PoolAllocator &&) = delete;

		~PoolAllocator ()
		{
			Release();
			FG_LOGI( "PoolAllocator - dtor" );
		}


		void SetBlockSize (BytesU size) noexcept
		{
			_blockSize = size;
		}


		ND_ FG_ALLOCATOR void*  Alloc (BytesU size, BytesU align) noexcept
		{
			for (;;)
			{
				if ( not _blocks.empty() )
				{
					auto&	block	= _blocks.back();
					BytesU	offset	= AlignToLarger( size_t(block.ptr) + block.size, align ) - size_t(block.ptr);

					if ( size <= (block.capacity - offset) )
					{
						block.size = offset + size;
						return block.ptr + offset;
					}
				}

				_blocks.push_back(Block{ _alloc.Allocate( _blockSize ), 0_b, _blockSize });	// TODO: check for null
			}
		}


		template <typename T>
		ND_ FG_ALLOCATOR T*  Alloc (size_t count = 1) noexcept
		{
			return BitCast<T *>( Alloc( SizeOf<T> * count, AlignOf<T> ));
		}


		void Discard () noexcept
		{
			for (auto& block : _blocks) {
				block.size = 0_b;
			}
		}

		void Release () noexcept
		{
			for (auto& block : _blocks) {
				_alloc.Deallocate( block.ptr, block.capacity );
			}
			_blocks.clear();
		}
	};



	//
	// Untyped Pool Allocator
	//
	
	template <typename AllocatorType = UntypedAllocator>
	struct UntypedPoolAllocator final
	{
	// types
	public:
		using Pool_t		= PoolAllocator< AllocatorType >;
		using Allocator_t	= AllocatorType;
		using Self			= UntypedPoolAllocator< AllocatorType >;

		template <typename T>
		using StdAllocator_t = StdPoolAllocator< T, AllocatorType >;


	// variables
	private:
		Pool_t &	_pool;
		

	// methods
	public:
		UntypedPoolAllocator (Pool_t &pool) noexcept : _pool{pool} {}
		UntypedPoolAllocator (Self &&other) noexcept : _pool{other._pool} {}
		UntypedPoolAllocator (const Self &other) noexcept : _pool{other._pool} {}


		ND_ FG_ALLOCATOR void*  Allocate (BytesU size, BytesU align)
		{
			return _pool.Alloc( size, align );
		}

		void  Deallocate (void *, BytesU)
		{
		}

		ND_ Pool_t&  GetPool () const
		{
			return _pool;
		}

		ND_ bool operator == (const Self &rhs) const
		{
			return &_pool == &rhs._pool;
		}
	};
	


	//
	// Std Pool Allocator
	//

	template <typename T,
			  typename AllocatorType = UntypedAllocator>
	struct StdPoolAllocator final
	{
	// types
	public:
		using value_type			= T;
		using Pool_t				= PoolAllocator< AllocatorType >;
		using Allocator_t			= AllocatorType;
		using Self					= StdPoolAllocator< T, AllocatorType >;

		using UntypedAllocator_t	= UntypedPoolAllocator< AllocatorType >;


	// variables
	private:
		Pool_t &	_pool;


	// methods
	public:
		StdPoolAllocator (Pool_t &pool) noexcept : _pool{pool} {}
		StdPoolAllocator (const UntypedAllocator_t &alloc) noexcept : _pool{alloc.GetPool()} {}

		StdPoolAllocator (Self &&other) noexcept : _pool{other._pool} {}
		StdPoolAllocator (const Self &other) noexcept : _pool{other._pool} {}
		
		template <typename B>
		StdPoolAllocator (const StdPoolAllocator<B,Allocator_t>& other) noexcept : _pool{other.GetPool()} {}

		Self& operator = (const Self &) = delete;

		
		ND_ FG_ALLOCATOR T*  allocate (const size_t count)
		{
			return _pool.Alloc<T>( count );
		}

		void deallocate (T * const, const size_t) noexcept
		{
		}

		ND_ Self  select_on_container_copy_construction() const noexcept
		{
			return Self{ _pool };
		}

		ND_ Pool_t&  GetPool () const
		{
			return _pool;
		}

		ND_ bool operator == (const Self &rhs) const
		{
			return &_pool == &rhs._pool;
		}
	};


}	// FG
