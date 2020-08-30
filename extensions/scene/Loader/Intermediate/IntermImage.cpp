// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/Loader/Intermediate/IntermImage.h"

namespace FG
{
	
/*
=================================================
	constructor
=================================================
*/
	IntermImage::IntermImage (const ImageView &view)
	{
		_imageType = view.Dimension().z > 1 ? EImage_3D :
					 view.Dimension().y > 1 ? EImage_2D :
											  EImage_1D;

		_data.resize( 1 );
		_data[0].resize( 1 );

		auto&	level	= _data[0][0];
		level.dimension		= Max( 1u, view.Dimension() );
		level.format		= view.Format();
		level.layer			= 0_layer;
		level.mipmap		= 0_mipmap;
		level.rowPitch		= view.RowPitch();
		level.slicePitch	= view.SlicePitch();
		level.pixels.resize( size_t(level.slicePitch * level.dimension.z) );

		BytesU	offset = 0_b;
		for (auto& part : view.Parts())
		{
			BytesU	size = ArraySizeOf(part);
			std::memcpy( level.pixels.data() + offset, part.data(), size_t(size) );
			offset += size;
		}

		ASSERT( offset == ArraySizeOf(level.pixels) );
		STATIC_ASSERT( sizeof(level.pixels[0]) == sizeof(view.Parts()[0][0]) );
	}
	
/*
=================================================
	SetData
=================================================
*/
	void  IntermImage::SetData (Mipmaps_t &&data, EImage type)
	{
		ASSERT( not _immutable );
		_data = std::move(data);
		_imageType = type;
	}
	
/*
=================================================
	GetImageDim
=================================================
*/
	EImageDim  IntermImage::GetImageDim () const
	{
		switch ( _imageType )
		{
			case EImage_1D :
			case EImage_1DArray :		return EImageDim_1D;
			case EImage_2D :
			case EImage_2DArray :
			case EImage_Cube :
			case EImage_CubeArray :	return EImageDim_2D;
			case EImage_3D :			return EImageDim_3D;
		}
		return Default;
	}

}	// FG
