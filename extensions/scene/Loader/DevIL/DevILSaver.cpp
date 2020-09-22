// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef FG_ENABLE_DEVIL

#include "scene/Loader/DevIL/DevILSaver.h"
#include "scene/Loader/Intermediate/IntermImage.h"
#include "stl/Stream/FileStream.h"

#include "IL/il.h"
#include "IL/ilu.h"

namespace FG
{
	extern bool InitDevlIL ();

namespace
{
/*
=================================================
	GetImageFormat
=================================================
*/
	static bool  GetImageFormat (EPixelFormat fmt, OUT ILint &format, OUT ILint &type, OUT ILint &channels)
	{
		switch ( fmt )
		{
			case EPixelFormat::RGBA8_UNorm :	format = IL_RGBA;				type = IL_UNSIGNED_BYTE;	channels = 4;	return true;
			case EPixelFormat::RGBA8_SNorm :	format = IL_RGBA;				type = IL_BYTE;				channels = 4;	return true;
			case EPixelFormat::RGBA16_UNorm :	format = IL_RGBA;				type = IL_UNSIGNED_SHORT;	channels = 4;	return true;
			case EPixelFormat::RGBA16_SNorm :	format = IL_RGBA;				type = IL_SHORT;			channels = 4;	return true;
			case EPixelFormat::RGBA16F :		format = IL_RGBA;				type = IL_HALF;				channels = 4;	return true;
			case EPixelFormat::RGBA32F :		format = IL_RGBA;				type = IL_FLOAT;			channels = 4;	return true;
			
			case EPixelFormat::R8_UNorm :		format = IL_ALPHA;				type = IL_UNSIGNED_BYTE;	channels = 1;	return true;
			case EPixelFormat::R8_SNorm :		format = IL_ALPHA;				type = IL_BYTE;				channels = 1;	return true;
			case EPixelFormat::R16_UNorm :		format = IL_ALPHA;				type = IL_UNSIGNED_SHORT;	channels = 1;	return true;
			case EPixelFormat::R16_SNorm :		format = IL_ALPHA;				type = IL_SHORT;			channels = 1;	return true;
			case EPixelFormat::R16F :			format = IL_ALPHA;				type = IL_HALF;				channels = 1;	return true;
			case EPixelFormat::R32F :			format = IL_ALPHA;				type = IL_FLOAT;			channels = 1;	return true;
				
			case EPixelFormat::RGB32F :			format = IL_RGB;				type = IL_FLOAT;			channels = 3;	return true;
				
			case EPixelFormat::RG8_UNorm :		format = IL_LUMINANCE_ALPHA;	type = IL_UNSIGNED_BYTE;	channels = 2;	return true;
			case EPixelFormat::RG8_SNorm :		format = IL_LUMINANCE_ALPHA;	type = IL_BYTE;				channels = 2;	return true;
			case EPixelFormat::RG16_UNorm :		format = IL_LUMINANCE_ALPHA;	type = IL_UNSIGNED_SHORT;	channels = 2;	return true;
			case EPixelFormat::RG16_SNorm :		format = IL_LUMINANCE_ALPHA;	type = IL_SHORT;			channels = 2;	return true;
			case EPixelFormat::RG16F :			format = IL_LUMINANCE_ALPHA;	type = IL_HALF;				channels = 2;	return true;
			case EPixelFormat::RG32F :			format = IL_LUMINANCE_ALPHA;	type = IL_FLOAT;			channels = 2;	return true;
		}
		return false;
	}
}

/*
=================================================
	SaveImage
=================================================
*/
	bool  DevILSaver::SaveImage (StringView filename, const IntermImagePtr &image)
	{
		CHECK_ERR( image );
		CHECK_ERR( image->GetData().size() );
		CHECK_ERR( image->GetData()[0].size() );

		uint	arr_layers	= uint(image->GetData()[0].size());
		uint	cube_faces	= 1;

		InitDevlIL();
		
		ILint	img = ilGenImage();
		ilBindImage( img );

		
		switch ( image->GetType() )
		{
			case EImage_1D :
			case EImage_2D :
			case EImage_3D :
				CHECK_ERR( arr_layers == 1 );
				break;

			case EImage_Cube :
				CHECK_ERR( arr_layers == 6 );
				cube_faces = 6;
				arr_layers = 1;
				break;

			case EImage_CubeArray :
				CHECK_ERR( arr_layers % 6 == 0 );
				arr_layers /= 6;
				cube_faces  = 6;
				ilSetInteger( IL_NUM_LAYERS, arr_layers );
				break;

			case EImage_1DArray :
			case EImage_2DArray :
				ilSetInteger( IL_NUM_LAYERS, arr_layers );
				break;

			default :
				RETURN_ERR( "unsupported image type" );
		}
		
		for (uint layer = 0; layer < arr_layers; ++layer)
		{
			CHECK_ERR( ilActiveLayer( layer ) == IL_TRUE );

			for (uint face = 0; face < cube_faces; ++face)
			{
				CHECK_ERR( ilActiveFace( face ) == IL_TRUE );
				
				const uint	mipmap_count= uint(image->GetData().size());
				
				ilSetInteger( IL_NUM_MIPMAPS, Max( 1u, mipmap_count ) - 1 );

				for (uint mm = 0; mm < mipmap_count; ++mm)
				{
					CHECK_ERR( ilActiveMipmap( mm ) == IL_TRUE );

					const auto&		level	= image->GetData()[mm][layer + face];
					const uint3		dim		= level.dimension;
					ILint			format, type, channels;

					CHECK_ERR( GetImageFormat( level.format, OUT format, OUT type, OUT channels ));

					CHECK_ERR( ilTexImage( dim.x, dim.y, dim.z, ILubyte(channels), format, type, const_cast<uint8_t*>(level.pixels.data()) ) == IL_TRUE );
				}
			}
		}

		ilEnable( IL_FILE_OVERWRITE );
		
		CHECK( iluFlipImage() == IL_TRUE );

		CHECK_ERR( ilSaveImage( String(filename).c_str() ) == IL_TRUE );

		ilDeleteImage( img );
		return true;
	}

}	// FG

#endif	// FG_ENABLE_DEVIL
