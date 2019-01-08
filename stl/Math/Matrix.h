// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"
#include "stl/Algorithms/ArrayUtils.h"

# ifdef COMPILER_MSVC
#	pragma warning (push)
#	pragma warning (disable: 4324)	// structure was padded due to alignment specifier
# endif


namespace FG
{

	enum class EMatrixOrder
	{
		ColumnMajor,
		RowMajor,
	};


	template <typename T, uint Columns, uint Rows, EMatrixOrder Order, size_t Align = alignof(T)>
	struct Matrix;



	//
	// Column-major Matrix
	//

	template <typename T, uint Columns, uint Rows, size_t Align>
	struct Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >
	{
	// types
	public:
		struct alignas(Align) _AlignedVec
		{
			Vec< T, Rows >	data;
		};

		using Self			= Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >;
		using Transposed_t	= Matrix< T, Rows, Columns, EMatrixOrder::ColumnMajor, Align >;
		using Column_t		= Vec< T, Rows >;
		using Row_t			= Vec< T, Columns >;


	// variables
	private:
		StaticArray< _AlignedVec, Columns >		_columns;
		
		//		  c0  c1  c2  c3
		//	r0	| 1 | 2 | 3 | X |	1 - left
		//	r1	|   |   |   | Y |	2 - up
		//	r2	|   |   |   | Z |	3 - forward
		//	r3	| 0 | 0 | 0 | W |


	// methods
	public:
		constexpr Matrix () : _columns{} {}
		
		template <typename Arg0, typename ...Args>
		constexpr explicit Matrix (const Arg0 &arg0, const Args& ...args)
		{
			if constexpr ( CountOf<Arg0, Args...>() == Columns * Rows )
				_CopyScalars<0>( arg0, args... );
			else
			if constexpr ( CountOf<Arg0, Args...>() == Columns )
				_CopyColumns<0>( arg0, args... );
			else
				STATIC_ASSERT(  (CountOf<Arg0, Args...>() == Columns * Rows) or
								(CountOf<Arg0, Args...>() == Columns) );
		}

		template <size_t Align2>
		constexpr explicit Matrix (const Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align2 > &other)
		{
			for (uint c = 0; c < Columns; ++c)
			for (uint r = 0; r < Rows; ++r) {
				(*this)[c][r] = other[c][r];
			}
		}

		template <size_t Align2>
		constexpr explicit Matrix (const Matrix< T, Columns, Rows, EMatrixOrder::RowMajor, Align2 > &other)
		{
			for (uint c = 0; c < Columns; ++c)
			for (uint r = 0; r < Rows; ++r) {
				(*this)[c][r] = other[r][c];
			}
		}

		template <uint Q, size_t Align2>
		ND_ constexpr auto		operator *  (Matrix< T, Q, Columns, EMatrixOrder::ColumnMajor, Align2 > const &right) const
		{
			Matrix< T, Q, Rows, EMatrixOrder::ColumnMajor, Max(Align, Align2) >	result;
			for (uint r = 0; r < Rows; ++r)
			for (uint q = 0; q < Q; ++q)
			{
				//result[q][r] = T(0);
				for (uint c = 0; c < Columns; ++c) {
					result[q][r] += (*this)[c][r] * right[q][c];
				}
			}
			return result;
		}

		ND_ constexpr Column_t		operator *  (const Row_t &v) const
		{
			Column_t	result;
			for (uint r = 0; r < Rows; ++r)
			for (uint c = 0; c < Columns; ++c) {
				result[r] += (*this)[c][r] * v[c];
			}
			return result;
		}

		ND_ friend constexpr Row_t	operator *  (const Column_t &v, const Self &m)
		{
			Row_t	result;
			for (uint c = 0; c < Columns; ++c)
			for (uint r = 0; r < Rows; ++r) {
				result[c] += m[c][r] * v[r];
			}
			return result;
		}
		
		ND_ static Self  Identity ()
		{
			constexpr uint	cnt = Min(Columns, Rows);
			Self			result;

			for (uint i = 0; i < cnt; ++i) {
				result[i][i] = T(1);
			}
			return result;
		}

		ND_ constexpr Column_t const&	operator [] (uint index) const	{ return _columns [index].data; }
		ND_ constexpr Column_t&			operator [] (uint index)		{ return _columns [index].data; }

		ND_ static constexpr size_t		size ()							{ return Columns; }
		ND_ static constexpr uint2		Dimension ()					{ return {Columns, Rows}; }

		ND_ static constexpr bool		IsColumnMajor ()				{ return true; }
		ND_ static constexpr bool		IsRowMajor ()					{ return not IsColumnMajor(); }


	private:
		template <uint I, typename Arg0, typename ...Args>
		constexpr void _CopyScalars (const Arg0 &arg0, const Args& ...args)
		{
			STATIC_ASSERT( IsScalar<Arg0> );
			_columns[I / Rows].data[I % Rows] = arg0;

			if constexpr ( I+1 < Columns * Rows )
				_CopyScalars< I+1 >( args... );
		}

		template <uint I, typename Arg0, typename ...Args>
		constexpr void _CopyColumns (const Arg0 &arg0, const Args& ...args)
		{
			STATIC_ASSERT( IsSameTypes< Arg0, Column_t > );
			_columns[I].data = arg0;

			if constexpr ( I+1 < Columns )
				_CopyColumns< I+1 >( args... );
		}
	};



	//
	// Row-major Matrix
	//

	template <typename T, uint Columns, uint Rows, size_t Align>
	struct Matrix< T, Columns, Rows, EMatrixOrder::RowMajor, Align >
	{
	// types
	public:
		struct alignas(Align) _AlignedVec
		{
			Vec< T, Columns >	data;
		};

		using Self			= Matrix< T, Columns, Rows, EMatrixOrder::RowMajor, Align >;
		using Transposed_t	= Matrix< T, Rows, Columns, EMatrixOrder::RowMajor, Align >;
		using Row_t			= Vec< T, Columns >;
		using Column_t		= Vec< T, Rows >;


	// variables
	private:
		StaticArray< _AlignedVec, Rows >	_rows;
		
		//		  c0  c1  c2  c3
		//	r0	| 1         | 0 |	1 - left
		//      |---------------|
		//	r1	| 2         | 0 |	2 - up
		//      |---------------|
		//	r2	| 3         | 0 |	3 - forward
		//      |---------------|
		//	r3	| X | Y | Z | W |
		

	// methods
	public:
		constexpr Matrix () : _rows{} {}

		template <typename Arg0, typename ...Args>
		constexpr explicit Matrix (const Arg0 &arg0, const Args& ...args)
		{
			if constexpr ( CountOf<Arg0, Args...>() == Columns * Rows )
				_CopyScalars<0>( arg0, args... );
			else
			if constexpr ( CountOf<Arg0, Args...>() == Rows )
				_CopyRows<0>( arg0, args... );
			else
				STATIC_ASSERT(  (CountOf<Arg0, Args...>() == Columns * Rows) or
								(CountOf<Arg0, Args...>() == Rows) );
		}
		
		template <size_t Align2>
		constexpr explicit Matrix (const Matrix< T, Columns, Rows, EMatrixOrder::RowMajor, Align2 > &other)
		{
			for (uint r = 0; r < Rows; ++r)
			for (uint c = 0; c < Columns; ++c) {
				(*this)[r][c] = other[r][c];
			}
		}

		template <size_t Align2>
		constexpr explicit Matrix (const Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align2 > &other)
		{
			for (uint r = 0; r < Rows; ++r)
			for (uint c = 0; c < Columns; ++c) {
				(*this)[r][c] = other[c][r];
			}
		}

		template <uint Q, size_t Align2>
		ND_ constexpr auto		operator *  (Matrix< T, Q, Columns, EMatrixOrder::RowMajor, Align2 > const &right) const
		{
			Matrix< T, Q, Rows, EMatrixOrder::RowMajor, Max(Align, Align2) >	result;
			for (uint r = 0; r < Rows; ++r)
			for (uint q = 0; q < Q; ++q)
			{
				//result[r][q] = T(0);
				for (uint c = 0; c < Columns; ++c) {
					result[r][q] += (*this)[r][c] * right[c][q];
				}
			}
			return result;
		}

		ND_ constexpr Row_t		operator *  (const Column_t &v) const
		{
			Row_t	result;
			for (uint c = 0; c < Columns; ++c)
			for (uint r = 0; r < Rows; ++r) {
				result[c] += (*this)[r][c] * v[r];
			}
			return result;
		}

		ND_ friend constexpr Column_t	operator *  (const Row_t &v, const Self &m)
		{
			Column_t	result;
			for (uint r = 0; r < Rows; ++r)
			for (uint c = 0; c < Columns; ++c) {
				result[r] += m[r][c] * v[c];
			}
			return result;
		}

		ND_ static Self  Identity ()
		{
			constexpr uint	cnt = Min(Columns, Rows);
			Self			result;

			for (uint i = 0; i < cnt; ++i) {
				result[i][i] = T(1);
			}
			return result;
		}

		ND_ constexpr Row_t const&		operator [] (uint index) const	{ return _rows [index].data; }
		ND_ constexpr Row_t&			operator [] (uint index)		{ return _rows [index].data; }

		ND_ static constexpr size_t		size ()							{ return Rows; }
		ND_ static constexpr uint2		Dimension ()					{ return {Columns, Rows}; }
		
		ND_ static constexpr bool		IsColumnMajor ()				{ return false; }
		ND_ static constexpr bool		IsRowMajor ()					{ return not IsColumnMajor(); }


	private:
		template <uint I, typename Arg0, typename ...Args>
		constexpr void _CopyScalars (const Arg0 &arg0, const Args& ...args)
		{
			STATIC_ASSERT( IsScalar<Arg0> );
			_rows[I / Columns].data[I % Columns] = arg0;

			if constexpr ( I+1 < Columns * Rows )
				_CopyScalars< I+1 >( args... );
		}

		template <uint I, typename Arg0, typename ...Args>
		constexpr void _CopyRows (const Arg0 &arg0, const Args& ...args)
		{
			STATIC_ASSERT( IsSameTypes< Arg0, Row_t > );
			_rows[I].data = arg0;

			if constexpr ( I+1 < Rows )
				_CopyRows< I+1 >( args... );
		}
	};

}	// FG


# ifdef COMPILER_MSVC
#	pragma warning (pop)
# endif
