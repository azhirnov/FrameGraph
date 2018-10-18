// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"

namespace FG
{
namespace _fg_hidden_
{

	template <typename IndexType, typename BitType, IndexType MaxSize, uint Level>
	struct BitTreeImpl;


	//
	// Bit Tree implementation for level == 0
	//
	template <typename IndexType, typename BitType, IndexType MaxSize>
	struct BitTreeImpl< IndexType, BitType, MaxSize, 0 >
	{
	// types
		static constexpr uint	BitCount	= sizeof(BitType)*8;
		static constexpr uint	Size		= Max( 1u, uint((MaxSize + BitCount-1) / BitCount) );

		STATIC_ASSERT( Size <= BitCount );
		STATIC_ASSERT( MaxSize <= (Size * BitCount) );
		
		using BitArray_t	= StaticArray< BitType, Size >;
		using Index_t		= IndexType;


	// variables
		BitType			_highLevel;
		BitArray_t		_bits;			// 1 - is unassigned bit, 0 - assigned bit
		Index_t			_indexOffset;


	// methods
		BitTreeImpl ()
		{}
		
		void Initialize (const Index_t offset, const Index_t totalSize)
		{
			_indexOffset = offset;

			if constexpr (Size != BitCount)
				_highLevel = ~(((BitType(1) << (BitCount - Size)) - 1) << Size);
			else
				_highLevel = ~BitType(0);

			for (uint i = 0; i < Size; ++i)
			{
				const Index_t	off = offset + i * BitCount;

				if ( off < totalSize )
				{
					const Index_t	size = totalSize - off;

					_bits[i]    = ~(BitCount < size ? BitType(0) : ((BitType(1) << (BitCount - size)) - 1) << size);
					_highLevel &= ~(BitType(_bits[i] == 0) << i);
				}
				else
				{
					_bits[i]    = 0;
					_highLevel &= ~(BitType(1) << i);
				}
			}
		}


		ND_ bool Assign (OUT Index_t &index)
		{
			int  idx1 = BitScanForward( _highLevel );
			if ( idx1 >= 0 )
			{
				int  idx0 = BitScanForward( _bits[idx1] );
				ASSERT( idx0 >= 0 );

				_bits[idx1] ^= (BitType(1) << idx0);	// 1 -> 0

				_highLevel ^= (BitType(_bits[idx1] == 0) << idx1);	// change 1 to 0 if all bits are assigned

				index = _indexOffset + idx1 * BitCount + idx0;
				return true;
			}
			return false;
		}


		ND_ bool IsAssigned (Index_t index) const
		{
			ASSERT( index >= _indexOffset and index < _indexOffset + Capacity() );
			index -= _indexOffset;

			return ( _bits[ index / BitCount ] & (BitType(1) << (index % BitCount)) ) == 0;
		}


		void Unassign (Index_t index)
		{
			ASSERT( index >= _indexOffset and index < _indexOffset + Capacity() );
			ASSERT( IsAssigned( index ) );
			index -= _indexOffset;

			uint  idx1 = index / BitCount;
			uint  idx0 = index % BitCount;

			_bits[idx1] |= (BitType(1) << idx0);	// 0 -> 1

			_highLevel |= (BitType(1) << idx1);		// change 0 to 1 if any bit is unassigned
		}


		ND_ static constexpr Index_t	Capacity ()				{ return Size * BitCount; }

		ND_ constexpr bool				Available ()	const	{ return _highLevel != 0; }		// returns 'true' if has unassigned bits
	};



	//
	// Bit Tree implementation
	//
	template <typename IndexType, typename BitType, IndexType MaxSize, uint Level>
	struct BitTreeImpl
	{
	// types
		static constexpr uint	BitCount	= sizeof(BitType)*8;

		template <uint N>
		static constexpr IndexType  _CalcLevelSize ()
		{
			if constexpr (N) {
				constexpr IndexType  base = _CalcLevelSize<N-1>();
				return Min( AlignToLarger( MaxSize, IndexType(base) ), BitCount*base );
			}else
				return Min( AlignToLarger( MaxSize, IndexType(BitCount) ), BitCount*BitCount );
		}

		static constexpr IndexType	MaxLevelSize = _CalcLevelSize< Level-1 >();
		static constexpr uint		Size		 = Max( 1u, uint((MaxSize + MaxLevelSize-1) / MaxLevelSize) );

		using LowLevel_t	= BitTreeImpl< IndexType, BitType, MaxLevelSize, Level-1 >;
		using BitArray_t	= StaticArray< LowLevel_t, Size >;
		using Index_t		= IndexType;
		
		STATIC_ASSERT( Size <= BitCount );
		STATIC_ASSERT( MaxSize <= (Size * LowLevel_t::Capacity()) );


	// variables
		BitType			_highLevel;
		BitArray_t		_level;
		

	// methods
		BitTreeImpl ()
		{}

		
		void Initialize (Index_t offset, const Index_t totalSize)
		{
			if constexpr (Size != BitCount)
				_highLevel = ~(((BitType(1) << (BitCount - Size)) - 1) << Size);
			else
				_highLevel = ~BitType(0);

			for (uint i = 0; i < Size; ++i)
			{
				_level[i].Initialize( offset, totalSize );
				
				_highLevel &= ~(BitType(not _level[i].Available()) << i);

				offset += _level[i].Capacity();
			}
		}

		
		ND_ bool Assign (OUT Index_t &index)
		{
			int  idx = BitScanForward( _highLevel );
			if ( idx >= 0 )
			{
				bool	result = _level[idx].Assign( OUT index );
				ASSERT( result );
				
				_highLevel ^= (BitType(not _level[idx].Available()) << idx);	// change 1 to 0 if all bits are assigned
				return result;
			}
			return false;
		}


		ND_ bool IsAssigned (Index_t index) const
		{
			return _level[ index / LowLevel_t::Capacity() ].IsAssigned( index );
		}


		void Unassign (Index_t index)
		{
			uint  idx = index / LowLevel_t::Capacity();

			_level[idx].Unassign( index );
			
			_highLevel |= (BitType(1) << idx);		// change 0 to 1 if any bit is unassigned
		}


		ND_ static constexpr Index_t	Capacity ()				{ return Size * LowLevel_t::Capacity(); }

		ND_ constexpr bool				Available ()	const	{ return _highLevel != 0; }		// returns 'true' if has unassigned bits
	};
	
}	// _fg_hidden_



	//
	// Bit Tree
	//

	template <typename IndexType, typename BitType, IndexType MaxSize>
	struct BitTree
	{
	// types
	public:
		using Self		= BitTree< IndexType, BitType, MaxSize >;
		using Index_t	= IndexType;
		using Bit_t		= BitType;

	private:
		static constexpr uint	BitCount	= sizeof(BitType)*8;

		static constexpr uint  _CalcLevel (IndexType size)
		{
			const IndexType  cnt = (size + BitCount-1) / BitCount;
			return (cnt > 1 ? 1 + _CalcLevel( cnt ) : 0);
		}

		static constexpr uint	LevelCount = _CalcLevel( MaxSize );

		using BitTreeHighLevel_t = _fg_hidden_::BitTreeImpl< IndexType, BitType, MaxSize, LevelCount >;


	// variables
	private:
		BitTreeHighLevel_t		_bits;


	// methods
	public:
		explicit BitTree (IndexType size = MaxSize)
		{
			_bits.Initialize( 0, size );
		}

		BitTree (const Self &) = default;
		BitTree (Self &&) = default;

		Self&  operator = (const Self &) = default;
		Self&  operator = (Self &&) = default;
		

		ND_ bool Assign (OUT Index_t &index)
		{
			return _bits.Assign( OUT index );
		}

		ND_ bool IsAssigned (Index_t index) const
		{
			ASSERT( index < MaxSize );
			return _bits.IsAssigned( index );
		}

		void Unassign (Index_t index)
		{
			ASSERT( index < MaxSize );
			return _bits.Unassign( index );
		}

		void Resize (IndexType size)
		{
			_bits.Initialize( 0, size );
		}

		ND_ constexpr IndexType  Capacity () const
		{
			return _bits.Capacity();
		}
	};


}	// FG
