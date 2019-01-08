// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/Math/Bytes.h"
#include "stl/Memory/UntypedAllocator.h"

namespace FG
{
	template <typename AllocatorType>				struct LinearAllocator;
	template <typename T, typename AllocatorType>	struct StdLinearAllocator;
	template <typename AllocatorType>				struct UntypedLinearAllocator;



	//
	// Linear Pool Allocator
	//

	template <typename AllocatorType = UntypedAlignedAllocator>
	struct LinearAllocator final
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
		using Blocks_t		= std::vector< Block >;


	// variables
	private:
		Blocks_t					_blocks;
		BytesU						_blockSize	= 1024_b;
		Allocator_t					_alloc;
		static constexpr BytesU		_ptrAlign	= SizeOf<void *>;


	// methods
	public:
		LinearAllocator () {}
		
		explicit LinearAllocator (const Allocator_t &alloc) : _alloc{alloc}
		{
			_blocks.reserve( 16 );
		}

		LinearAllocator (const LinearAllocator &) = delete;
		LinearAllocator (LinearAllocator &&) = delete;

		LinearAllocator& operator = (const LinearAllocator &) = delete;
		LinearAllocator& operator = (LinearAllocator &&) = delete;

		~LinearAllocator ()
		{
			Release();
		}


		void SetBlockSize (BytesU size) noexcept
		{
			_blockSize = size;
		}


		ND_ FG_ALLOCATOR void*  Alloc (const BytesU size, const BytesU align) noexcept
		{
			for (auto& block : _blocks)
			{
				BytesU	offset	= AlignToLarger( size_t(block.ptr) + block.size, align ) - size_t(block.ptr);

				if ( size <= (block.capacity - offset) )
				{
					block.size = offset + size;
					return block.ptr + offset;
				}
			}

			BytesU	block_size	= size*2 < _blockSize ? _blockSize : size*2;
			auto&	block		= _blocks.emplace_back(Block{ _alloc.Allocate( block_size, _ptrAlign ), 0_b, block_size });	// TODO: check for null
			BytesU	offset		= AlignToLarger( size_t(block.ptr) + block.size, align ) - size_t(block.ptr);

			block.size = offset + size;
			return block.ptr + offset;
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
				_alloc.Deallocate( block.ptr, block.capacity, _ptrAlign );
			}
			_blocks.clear();
		}
	};



	//
	// Untyped Linear Pool Allocator
	//
	
	template <typename AllocatorType = UntypedAlignedAllocator>
	struct UntypedLinearAllocator final
	{
	// types
	public:
		using LinearAllocator_t	= LinearAllocator< AllocatorType >;
		using Allocator_t		= AllocatorType;
		using Self				= UntypedLinearAllocator< AllocatorType >;

		template <typename T>
		using StdAllocator_t	= StdLinearAllocator< T, AllocatorType >;


	// variables
	private:
		LinearAllocator_t&	_alloc;
		

	// methods
	public:
		UntypedLinearAllocator (Self &&other) noexcept : _alloc{other._alloc} {}
		UntypedLinearAllocator (const Self &other) noexcept : _alloc{other._alloc} {}
		UntypedLinearAllocator (LinearAllocator_t &alloc) noexcept : _alloc{alloc} {}


		ND_ FG_ALLOCATOR void*  Allocate (BytesU size, BytesU align)
		{
			return _alloc.Alloc( size, align );
		}

		void  Deallocate (void *, BytesU)
		{}

		void  Deallocate (void *, BytesU, BytesU)
		{}

		ND_ LinearAllocator_t&  GetAllocatorRef () const
		{
			return _alloc;
		}

		ND_ bool operator == (const Self &rhs) const
		{
			return &_alloc == &rhs._alloc;
		}
	};
	


	//
	// Std Linear Pool Allocator
	//

	template <typename T,
			  typename AllocatorType = UntypedAlignedAllocator>
	struct StdLinearAllocator
	{
	// types
	public:
		using value_type			= T;
		using LinearAllocator_t		= LinearAllocator< AllocatorType >;
		using Allocator_t			= AllocatorType;
		using Self					= StdLinearAllocator< T, AllocatorType >;
		using UntypedAllocator_t	= UntypedLinearAllocator< AllocatorType >;


	// variables
	private:
		LinearAllocator_t&	_alloc;


	// methods
	public:
		StdLinearAllocator (LinearAllocator_t &alloc) noexcept : _alloc{alloc} {}
		StdLinearAllocator (const UntypedAllocator_t &alloc) noexcept : _alloc{alloc.GetAllocatorRef()} {}

		StdLinearAllocator (Self &&other) noexcept : _alloc{other._alloc} {}
		StdLinearAllocator (const Self &other) noexcept : _alloc{other._alloc} {}
		
		template <typename B>
		StdLinearAllocator (const StdLinearAllocator<B,Allocator_t>& other) noexcept : _alloc{other.GetAllocatorRef()} {}

		Self& operator = (const Self &) = delete;

		
		ND_ FG_ALLOCATOR T*  allocate (const size_t count)
		{
			return _alloc.template Alloc<T>( count );
		}

		void deallocate (T * const, const size_t) noexcept
		{
		}

		ND_ Self  select_on_container_copy_construction() const noexcept
		{
			return Self{ _alloc };
		}

		ND_ LinearAllocator_t&  GetAllocatorRef () const
		{
			return _alloc;
		}

		ND_ bool operator == (const Self &rhs) const
		{
			return &_alloc == &rhs._alloc;
		}
	};


}	// FG
