// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef FG_ENABLE_DEVIL

#include "scene/Loader/DevIL/DevILLoader.h"
#include "scene/SceneManager/IImageCache.h"
#include "stl/Algorithms/StringUtils.h"

#include "IL/il.h"
#include "IL/ilu.h"

namespace FG
{
/*
=================================================
	InitDevlIL
=================================================
*/
	extern bool InitDevlIL ()
	{
		static bool	isDevILInit = false;

		if ( isDevILInit )
			return true;

		isDevILInit = true;
	
		if (ilGetInteger( IL_VERSION_NUM ) < IL_VERSION)
		{
			RETURN_ERR( "Incorrect DevIL.dll version." );
		}
	
		if (iluGetInteger( ILU_VERSION_NUM ) < ILU_VERSION)
		{
			RETURN_ERR( "Incorrect ILU.dll version." );
		}

		ilInit();
		iluInit();

		ilEnable( IL_KEEP_DXTC_DATA );
		ilSetInteger( IL_KEEP_DXTC_DATA, IL_TRUE );
		return true;
	}
	
namespace {
/*
=================================================
	ConvertDevILFormat
=================================================
*/
	static bool ConvertDevILFormat (ILenum fmt, ILenum type, ILenum dxtc, OUT EPixelFormat &outFmt, OUT bool &isCompressed)
	{
		isCompressed = false;

		// convert and continue
		switch (fmt)
		{
			case IL_BGR :
			case IL_RGB :
			case IL_BGRA : {
				fmt = (type == IL_FLOAT ? IL_RGB : IL_RGBA);
				CHECK_ERR( ilConvertImage( fmt, type ) == IL_TRUE );
				break;
			}
			case IL_COLOR_INDEX : {
				// TODO
				break;
			}
		}

		switch (fmt)
		{
			case IL_RGB : {
				switch (type)
				{
					case IL_FLOAT :				outFmt = EPixelFormat::RGB32F;			return true;
					default :					RETURN_ERR( "unsupported format" );
				}
				break;
			}
			case IL_RGBA : {
				switch (type)
				{
					case IL_UNSIGNED_BYTE : 	outFmt = EPixelFormat::RGBA8_UNorm;		return true;
					case IL_BYTE :				outFmt = EPixelFormat::RGBA8_SNorm;		return true;
					case IL_UNSIGNED_SHORT :	outFmt = EPixelFormat::RGBA16_UNorm;	return true;
					case IL_SHORT :				outFmt = EPixelFormat::RGBA16_SNorm;	return true;
					case IL_HALF :				outFmt = EPixelFormat::RGBA16F;			return true;
					case IL_FLOAT :				outFmt = EPixelFormat::RGBA32F;			return true;
					default :					RETURN_ERR( "unsupported format" );
				}
				break;
			}
			case IL_ALPHA :
			case IL_LUMINANCE : {
				switch (type)
				{
					case IL_UNSIGNED_BYTE : 	outFmt = EPixelFormat::R8_UNorm;		return true;
					case IL_BYTE :				outFmt = EPixelFormat::R8_SNorm;		return true;
					case IL_UNSIGNED_SHORT :	outFmt = EPixelFormat::R16_UNorm;		return true;
					case IL_SHORT :				outFmt = EPixelFormat::R16_SNorm;		return true;
					case IL_HALF :				outFmt = EPixelFormat::R16F;			return true;
					case IL_FLOAT :				outFmt = EPixelFormat::R32F;			return true;
					default :					RETURN_ERR( "unsupported format" );
				}
				break;
			}
			case IL_LUMINANCE_ALPHA : {
				switch (type)
				{
					case IL_UNSIGNED_BYTE : 	outFmt = EPixelFormat::RG8_UNorm;		return true;
					case IL_BYTE :				outFmt = EPixelFormat::RG8_SNorm;		return true;
					case IL_UNSIGNED_SHORT :	outFmt = EPixelFormat::RG16_UNorm;		return true;
					case IL_SHORT :				outFmt = EPixelFormat::RG16_SNorm;		return true;
					case IL_HALF :				outFmt = EPixelFormat::RG16F;			return true;
					case IL_FLOAT :				outFmt = EPixelFormat::RG32F;			return true;
					default :					RETURN_ERR( "unsupported format" );
				}
				break;
			}
		}

		switch ( dxtc )
		{
			case IL_DXT_NO_COMP	:	break;
			case IL_DXT1		:	outFmt = EPixelFormat::BC1_RGB8_UNorm;		isCompressed = true;	return true;
			case IL_DXT1A		:	outFmt = EPixelFormat::BC1_RGB8_A1_UNorm;	isCompressed = true;	return true;
			case IL_DXT3		:	outFmt = EPixelFormat::BC2_RGBA8_UNorm;		isCompressed = true;	return true;
			case IL_DXT5		:	outFmt = EPixelFormat::BC3_RGBA8_UNorm;		isCompressed = true;	return true;
			default				:	RETURN_ERR( "unsupported DXT format" );
		}

		RETURN_ERR( "unknown format" )
	}
	
/*
=================================================
	LoadFromDevIL
=================================================
*/
	static bool LoadFromDevIL (OUT IntermImage::Level &imageLevel)
	{
		const ILint		fmt				= ilGetInteger( IL_IMAGE_FORMAT );
		const ILint		type			= ilGetInteger( IL_IMAGE_TYPE );
		const ILint		dxtc			= ilGetInteger( IL_DXTC_DATA_FORMAT );
		EPixelFormat	format			= EPixelFormat::Unknown;
		bool			is_compressed	= false;

		CHECK_ERR( ConvertDevILFormat( fmt, type, dxtc, OUT format, OUT is_compressed ));
		
		const ILint		width			= Max( 1, ilGetInteger(IL_IMAGE_WIDTH) );
		const ILint		height			= Max( 1, ilGetInteger(IL_IMAGE_HEIGHT) );
		const ILint		depth			= Max( 1, ilGetInteger(IL_IMAGE_DEPTH) );
		const uint		bpp				= ilGetInteger( IL_IMAGE_BITS_PER_PIXEL );
		const BytesU	calc_data_size	= BytesU(width * height * depth * bpp) / 8;

		if ( dxtc != IL_DXT_NO_COMP )
		{
			ILuint	dxt_size = ilGetDXTCData( nullptr, 0, dxtc );
			imageLevel.pixels.resize( dxt_size, false );
			ilGetDXTCData( imageLevel.pixels.data(), dxt_size, dxtc );
		}
		else
		{
			size_t	data_size = Max( 0, ilGetInteger(IL_IMAGE_SIZE_OF_DATA) );
			CHECK_ERR( data_size == calc_data_size );

			imageLevel.pixels.resize( data_size, false );
			std::memcpy( OUT imageLevel.pixels.data(), ilGetData(), data_size );
		}

		imageLevel.dimension	= uint3{int3{ width, height, depth }};
		imageLevel.rowPitch		= BytesU( width * bpp ) / 8;
		imageLevel.slicePitch	= BytesU( width * height * bpp ) / 8;
		imageLevel.format		= format;

		return true;
	}

}	// namespace
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	DevILLoader::DevILLoader ()
	{
		InitDevlIL();
	}

/*
=================================================
	LoadImage
=================================================
*/
	bool DevILLoader::LoadImage (INOUT IntermImagePtr &image, ArrayView<StringView> directories, const ImageCachePtr &imgCache, bool flipY)
	{
		CHECK_ERR( image );

		String	filename;
		CHECK_ERR( _FindImage( image->GetPath(), directories, OUT filename ));

		// search in cache
		if ( imgCache and imgCache->GetImageData( filename, OUT image ))
			return true;


		CHECK_ERR( ilLoadImage( filename.c_str() ) == IL_TRUE );

		const bool	require_flip = (EndsWithIC( filename, ".jpg" ) or EndsWithIC( filename, ".jpeg" ));

		if ( flipY != require_flip )
		{
			CHECK( iluFlipImage() == IL_TRUE );
		}
		
		CHECK( ilActiveImage( 0 ) == IL_TRUE );
		
		const ILint		width	= Max( 1, ilGetInteger(IL_IMAGE_WIDTH) );
		const ILint		height	= Max( 1, ilGetInteger(IL_IMAGE_HEIGHT) );
		const ILint		depth	= Max( 1, ilGetInteger(IL_IMAGE_DEPTH) );
		const ILint		layers	= Max( 1, ilGetInteger(IL_NUM_LAYERS) );
		const ILint		faces	= Max( 1, ilGetInteger(IL_NUM_FACES) );
		EImage			img_type= Default;

		if ( depth > 1 )
			img_type = EImage::Tex3D;
		else
		if ( faces > 1 and layers > 1 )
			img_type = EImage::TexCubeArray;
		else
		if ( faces > 1 )
			img_type = EImage::TexCube;
		else
		if ( height > 1 and layers > 1 )
			img_type = EImage::Tex2DArray;
		else
		if ( layers > 1 )
			img_type = EImage::Tex1DArray;
		else
		if ( height > 1 )
			img_type = EImage::Tex2D;
		else
			img_type = EImage::Tex1D;


		IntermImage::Mipmaps_t	image_data;

		for (int face = 0; face < faces; ++face)
		{
			CHECK( ilActiveFace( face ) == IL_TRUE );

			for (int layer = 0; layer < layers; ++layer)
			{
				CHECK( ilActiveLayer( layer ) == IL_TRUE );

				for (int mm = 0, num_mipmaps = ilGetInteger(IL_NUM_MIPMAPS)+1;
					 mm < num_mipmaps; ++mm)
				{
					CHECK( ilActiveMipmap( mm ) == IL_TRUE );

					IntermImage::Level	image_level;
					CHECK_ERR( LoadFromDevIL( OUT image_level ));

					image_level.mipmap	= MipmapLevel{ uint(mm) };
					image_level.layer	= ImageLayer{ uint(layer) };

					if ( size_t(mm) >= image_data.size() )
						image_data.resize( size_t(mm)+1 );

					if ( size_t(layer) >= image_data[mm].size() )
						image_data[mm].resize( size_t(layer)+1 );

					auto&	curr_mm = image_data[mm][layer];

					CHECK( curr_mm.pixels.empty() );	// warning: previous data will be discarded

					curr_mm = std::move(image_level);
				}
			}
		}

		image->SetData( std::move(image_data), img_type );

		if ( imgCache )
			imgCache->AddImageData( filename, image );

		return true;
	}


}	// FG

#endif	// FG_ENABLE_DEVIL
