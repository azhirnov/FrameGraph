// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"
#include "stl/Algorithms/ArrayUtils.h"

namespace FGC
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

		template <uint Columns2, uint Rows2, size_t Align2>
		constexpr explicit Matrix (const Matrix< T, Columns2, Rows2, EMatrixOrder::ColumnMajor, Align2 > &other)
		{
			for (uint c = 0; c < Columns; ++c)
			for (uint r = 0; r < Rows; ++r) {
				(*this)[c][r] = ((c < Columns2) & (r < Rows2)) ? other[c][r] : (c == r ? T(1) : T(0));
			}
		}
		
		template <uint Columns2, uint Rows2, size_t Align2>
		constexpr explicit Matrix (const Matrix< T, Columns2, Rows2, EMatrixOrder::RowMajor, Align2 > &other)
		{
			for (uint c = 0; c < Columns; ++c)
			for (uint r = 0; r < Rows; ++r) {
				(*this)[c][r] = ((r < Columns2) & (c < Rows2)) ? other[r][c] : (c == r ? T(1) : T(0));
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

		ND_ Self  Translate (const Vec<T,Rows> &) const;
		ND_ Self  Translate (const Vec<T,Rows-1> &) const;

		ND_ Self  Inverse () const;

		ND_ Matrix<T, Rows, Columns, EMatrixOrder::ColumnMajor, Align>  Transpose () const;


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
		
		template <uint Columns2, uint Rows2, size_t Align2>
		constexpr explicit Matrix (const Matrix< T, Columns2, Rows2, EMatrixOrder::RowMajor, Align2 > &other)
		{
			for (uint r = 0; r < Rows; ++r)
			for (uint c = 0; c < Columns; ++c) {
				(*this)[r][c] = ((c < Columns2) & (r < Rows2)) ? other[r][c] : (c == r ? T(1) : T(0));
			}
		}
		
		template <uint Columns2, uint Rows2, size_t Align2>
		constexpr explicit Matrix (const Matrix< T, Columns2, Rows2, EMatrixOrder::ColumnMajor, Align2 > &other)
		{
			for (uint r = 0; r < Rows; ++r)
			for (uint c = 0; c < Columns; ++c) {
				(*this)[r][c] = ((r < Columns2) & (c < Rows2)) ? other[c][r] : (c == r ? T(1) : T(0));
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
	
		
/*
=================================================
	Inverse
=================================================
*/
namespace _fgc_hidden_
{
	template <typename T, size_t Align>
	inline auto  Mat44_Inverse (const Matrix< T, 4, 4, EMatrixOrder::ColumnMajor, Align > &m)
	{
		// based on code from GLM https://glm.g-truc.net (MIT license)

		using M = Matrix< T, 4, 4, EMatrixOrder::ColumnMajor, Align >;

		T c00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
		T c02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
		T c03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

		T c04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
		T c06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
		T c07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

		T c08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
		T c10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
		T c11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

		T c12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
		T c14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
		T c15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

		T c16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
		T c18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
		T c19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

		T c20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
		T c22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
		T c23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

		Vec<T,4> f0{ c00, c00, c02, c03 };
		Vec<T,4> f1{ c04, c04, c06, c07 };
		Vec<T,4> f2{ c08, c08, c10, c11 };
		Vec<T,4> f3{ c12, c12, c14, c15 };
		Vec<T,4> f4{ c16, c16, c18, c19 };
		Vec<T,4> f5{ c20, c20, c22, c23 };

		Vec<T,4> v0{ m[1][0], m[0][0], m[0][0], m[0][0] };
		Vec<T,4> v1{ m[1][1], m[0][1], m[0][1], m[0][1] };
		Vec<T,4> v2{ m[1][2], m[0][2], m[0][2], m[0][2] };
		Vec<T,4> v3{ m[1][3], m[0][3], m[0][3], m[0][3] };

		Vec<T,4> i0{ v1 * f0 - v2 * f1 + v3 * f2 };
		Vec<T,4> i1{ v0 * f0 - v2 * f3 + v3 * f4 };
		Vec<T,4> i2{ v0 * f1 - v1 * f3 + v3 * f5 };
		Vec<T,4> i3{ v0 * f2 - v1 * f4 + v2 * f5 };

		Vec<T,4> sign_a{ T(+1), T(-1), T(+1), T(-1) };
		Vec<T,4> sign_b{ T(-1), T(+1), T(-1), T(+1) };
		M        result{ i0 * sign_a, i1 * sign_b, i2 * sign_a, i3 * sign_b };

		Vec<T,4> d0		= m[0] * Vec<T,4>{result[0][0], result[1][0], result[2][0], result[3][0]};
		T        d1		= (d0.x + d0.y) + (d0.z + d0.w);
		T        inv_det= T(1) / d1;
		
		for (uint c = 0; c < 4; ++c)
		for (uint r = 0; r < 4; ++r) {
			result[c][r] *= inv_det;
		}
		return result;
	}

	template <typename T, size_t Align>
	inline auto  Mat33_Inverse (const Matrix< T, 3, 3, EMatrixOrder::ColumnMajor, Align > &m)
	{
		// based on code from GLM https://glm.g-truc.net (MIT license)
		
		using M = Matrix< T, 3, 3, EMatrixOrder::ColumnMajor, Align >;

		const T inv_det = T(1) / (
			+ m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
			- m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
			+ m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]));

		M result;
		result[0][0] = + (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * inv_det;
		result[1][0] = - (m[1][0] * m[2][2] - m[2][0] * m[1][2]) * inv_det;
		result[2][0] = + (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * inv_det;
		result[0][1] = - (m[0][1] * m[2][2] - m[2][1] * m[0][2]) * inv_det;
		result[1][1] = + (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * inv_det;
		result[2][1] = - (m[0][0] * m[2][1] - m[2][0] * m[0][1]) * inv_det;
		result[0][2] = + (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * inv_det;
		result[1][2] = - (m[0][0] * m[1][2] - m[1][0] * m[0][2]) * inv_det;
		result[2][2] = + (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * inv_det;
		return result;
	}
}	// _fgc_hidden_

	template <typename T, uint Columns, uint Rows, size_t Align>
	inline Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >
		Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >::Inverse () const
	{
		if constexpr( Columns == 4 and Rows == 4 )
			return _fgc_hidden_::Mat44_Inverse( *this );
		
		if constexpr( Columns == 3 and Rows == 3 )
			return _fgc_hidden_::Mat33_Inverse( *this );
	}
	
/*
=================================================
	Translate
=================================================
*/
namespace _fgc_hidden_
{
	template <typename T, uint I, size_t Align>
	inline auto  Mat44_Translate (const Matrix< T, 4, 4, EMatrixOrder::ColumnMajor, Align > &m, const Vec<T,I> &v)
	{
		Matrix< T, 4, 4, EMatrixOrder::ColumnMajor, Align >  result{m};
		result[3] = Vec<T,4>{ m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] };
		return result;
	}

}	// _fgc_hidden_

	template <typename T, uint Columns, uint Rows, size_t Align>
	inline Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >
		Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >::Translate (const Vec<T,Rows> &v) const
	{
		if constexpr( Columns == 4 and Rows == 4 )
			return _fgc_hidden_::Mat44_Translate( *this, v );
	}

	template <typename T, uint Columns, uint Rows, size_t Align>
	inline Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >
		Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >::Translate (const Vec<T,Rows-1> &v) const
	{
		if constexpr( Columns == 4 and Rows == 4 )
			return _fgc_hidden_::Mat44_Translate( *this, v );
	}
	
/*
=================================================
	Transpose
=================================================
*/
	template <typename T, uint Columns, uint Rows, size_t Align>
	inline Matrix<T, Rows, Columns, EMatrixOrder::ColumnMajor, Align>
		Matrix< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >::Transpose () const
	{
		Matrix<T, Rows, Columns, EMatrixOrder::ColumnMajor, Align>	result;

		for (uint c = 0; c < Columns; ++c)
		for (uint r = 0; r < Rows; ++r) {
			result[r][c] = (*this)[c][r];
		}
		return result;
	}


}	// FGC
