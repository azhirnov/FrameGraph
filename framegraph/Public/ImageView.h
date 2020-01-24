// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/Math/Vec.h"
#include "stl/Math/Color.h"
#include "framegraph/Public/BufferView.h"
#include "framegraph/Public/ResourceEnums.h"

namespace FG
{

	//
	// Image View
	//

	struct ImageView
	{
	// types
	public:
		using T					= BufferView::value_type;
		using value_type		= T;
		using Iterator			= typename BufferView::Iterator;

		using LoadRGBA32fFun_t	= void (*) (ArrayView<T>, OUT RGBA32f &);
		using LoadRGBA32uFun_t	= void (*) (ArrayView<T>, OUT RGBA32u &);
		using LoadRGBA32iFun_t	= void (*) (ArrayView<T>, OUT RGBA32i &);


	// variables
	private:
		BufferView			_parts;
		uint3				_dimension;
		size_t				_rowPitch		= 0;
		size_t				_slicePitch		= 0;
		uint				_bitsPerPixel	= 0;
		EPixelFormat		_format			= Default;
		LoadRGBA32fFun_t	_loadF4			= null;
		LoadRGBA32uFun_t	_loadU4			= null;
		LoadRGBA32iFun_t	_loadI4			= null;


	// methods
	public:
		ImageView ()
		{}

		ImageView (ArrayView<ArrayView<T>> parts, const uint3 &dim, BytesU rowPitch, BytesU slicePitch, EPixelFormat format, EImageAspect aspect);

		ND_ Iterator				begin ()		const	{ return _parts.begin(); }
		ND_ Iterator				end ()			const	{ return _parts.end(); }
		ND_ bool					empty ()		const	{ return _parts.empty(); }
		ND_ size_t					size ()			const	{ return _parts.size(); }
		ND_ T const *				data ()			const	{ return _parts.data(); }

		ND_ ArrayView<ArrayView<T>>	Parts ()		const	{ return _parts.Parts(); }
		ND_ uint3 const&			Dimension ()	const	{ return _dimension; }
		ND_ BytesU					RowPitch ()		const	{ return BytesU(_rowPitch); }
		ND_ BytesU					SlicePitch ()	const	{ return BytesU(_slicePitch); }
		ND_ uint					BitsPerPixel ()	const	{ return _bitsPerPixel; }
		ND_ BytesU					RowSize ()		const	{ return BytesU(_dimension.x * _bitsPerPixel) / 8; }
		ND_ BytesU					SliseSize ()	const	{ return BytesU(_rowPitch * _dimension.y); }
		ND_ EPixelFormat			Format ()		const	{ return _format; }


		// implementation garanties that single row completely placed to solid memory block.
		ND_ ArrayView<T>  GetRow (uint y, uint z = 0) const
		{
			ASSERT( y < _dimension.y and z < _dimension.z );

			const size_t	row_size	= size_t(RowSize()) / sizeof(T);
			const auto		res			= _parts.section( _slicePitch * z + _rowPitch * y, row_size );

			ASSERT( res.size() == row_size );
			return res;
		}


		// implementation doesn't garanties that single slice compilete placed to solid memory block,
		// so check result size with 'SliseSize()'.
		ND_ ArrayView<T>  GetSlice (uint z) const
		{
			ASSERT( z < _dimension.z );

			const size_t	slice_size = size_t(SliseSize()) / sizeof(T);

			return _parts.section( _slicePitch * z, slice_size );
		}


		ND_ ArrayView<T>  GetPixel (const uint3 &point) const
		{
			ASSERT(All( point < _dimension ));

			return GetRow( point.y, point.z ).section( (point.x * _bitsPerPixel) / 8, (_bitsPerPixel + 7) / 8 );
		}


		void Load (const uint3 &point, OUT RGBA32f &col) const
		{
			ASSERT( _loadF4 );
			return _loadF4( GetPixel( point ), OUT col );
		}

		void Load (const uint3 &point, OUT RGBA32u &col) const
		{
			ASSERT( _loadU4 );
			return _loadU4( GetPixel( point ), OUT col );
		}

		void Load (const uint3 &point, OUT RGBA32i &col) const
		{
			ASSERT( _loadI4 );
			return _loadI4( GetPixel( point ), OUT col );
		}

		/*void Load (const uint3 &point, OUT RGBA32f &col) const
		{
			ASSERT( _isFloatFormat );
			
			auto	pixel = GetPixel( point );

			if constexpr ( IsInteger<T> )
			{
				ASSERT( _isNormalized );

			}
			else
			{
				ASSERT( not _isNormalized );
				
			}
		}*/
		
		/*
		void Load (const uint3 &point, OUT RGBA32u &col) const
		{
			ASSERT( not _isFloatFormat );
			//ASSERT( not _isNormalized );	// it is OK to read integer normalized as unnormalized value
			
			const auto	pixel			= GetPixel( point );
			const uint	channel_size	= (_bitsPerPixel + 7) / (_numChannels * 8);

			for (uint i = 0, cnt = NumChannels(); i < cnt; ++i)
			{
				std::memcpy( col.data() + i, row.data() + i * channel_size, channel_size );
			}
		}
		*/

		/*void Load (const uint3 &point, OUT RGBA32i &col) const
		{
			ASSERT( not _isFloatFormat );
			//ASSERT( not _isNormalized );	// it is OK to read integer normalized as unnormalized value

			const auto	pixel			= GetPixel( point );
			const uint	channel_size	= (_bitsPerPixel + 7) / (_numChannels * 8);

			for (uint i = 0, cnt = NumChannels(); i < cnt; ++i)
			{
				std::memcpy( col.data() + i, row.data() + i * channel_size, channel_size );
			}
		}*/
	};


}	// FG
