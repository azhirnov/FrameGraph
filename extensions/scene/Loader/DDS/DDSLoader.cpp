// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	DDS specs:
	https://docs.microsoft.com/en-us/windows/desktop/direct3ddds/dx-graphics-dds-pguide
*/

#include "scene/Loader/DDS/DDSLoader.h"
#include "scene/Loader/Intermediate/IntermImage.h"
#include "scene/SceneManager/IImageCache.h"
#include "scene/Loader/DDS/DDSUtils.h"

#include "framegraph/Shared/EnumUtils.h"
#include "stl/Stream/FileStream.h"

namespace FG
{
namespace
{

/*
=================================================
	LoadDX10Image
=================================================
*/
	static bool  LoadDX10Image (IntermImagePtr &image, const DDS_HEADER &header, const DDS_HEADER_DXT10 &headerDX10, FileRStream &file)
	{
		CHECK_ERR( EnumEq( header.dwFlags, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT ));

		EPixelFormat	format = DDSFormatToPixelFormat( headerDX10.dxgiFormat );
		CHECK_ERR( format != Default );
		
		const bool		is_cube			= EnumEq( headerDX10.miscFlag, D3D11_RESOURCE_MISC_TEXTURECUBE );
		const uint		mipmap_count	= EnumEq( header.dwFlags, DDSD_MIPMAPCOUNT ) ? header.dwMipMapCount : 1;
		const uint		array_layers	= (is_cube ? 6 : 1) * headerDX10.arraySize;
		uint3			dim				= { header.dwWidth, header.dwHeight, 1 };
		EImage			img_type		= Default;

		BEGIN_ENUM_CHECKS();
		switch ( headerDX10.resourceDimension )
		{
			case D3D11_RESOURCE_DIMENSION_BUFFER :
				RETURN_ERR( "buffer is not supported" );
				break;

			case D3D11_RESOURCE_DIMENSION_TEXTURE1D :
				img_type = array_layers > 1 ? EImage::Tex1DArray : EImage::Tex1D;
				break;

			case D3D11_RESOURCE_DIMENSION_TEXTURE2D :
				if ( is_cube )
					img_type = array_layers > 6 ? EImage::TexCubeArray : EImage::TexCube;
				else
					img_type = array_layers > 1 ? EImage::Tex2DArray : EImage::Tex2D;
				break;

			case D3D11_RESOURCE_DIMENSION_TEXTURE3D :
				CHECK_ERR( EnumEq( header.dwFlags, DDSD_DEPTH ));
				CHECK_ERR( array_layers == 1 );
				dim.z	 = header.dwDepth;
				img_type = EImage::Tex3D;
				break;

			case D3D11_RESOURCE_DIMENSION_UNKNOWN :
			default :
				RETURN_ERR( "unknown dimension" );
		}
		END_ENUM_CHECKS();
		
		CHECK_ERR( img_type != Default );


		const auto&		info	= EPixelFormat_GetInfo( format );
		size_t			pitch	= 0;
				
		if ( All( info.blockSize == uint2(1) ))
		{
			// uncompressed texture
			if ( EnumEq( header.dwFlags, DDSD_PITCH ))
				pitch = header.dwPitchOrLinearSize;
			else
				pitch = (dim.x * info.bitsPerBlock + 7) / 8;
		}
		else
		{
			// compressed texture
			if ( EnumEq( header.dwFlags, DDSD_LINEARSIZE ))
				pitch = header.dwPitchOrLinearSize;
			else
				pitch = Max( 1u, (dim.x + 3) / 4 ) * (info.bitsPerBlock / 8);
		}


		IntermImage::Mipmaps_t	image_data;
		const uint3				block_dim	{ (dim.x + info.blockSize.x-1) / info.blockSize.x, (dim.y + info.blockSize.y-1) / info.blockSize.y, dim.z };
		
		for (uint layer = 0; layer < array_layers; ++layer)
		{
			for (uint mm = 0; mm < mipmap_count; ++mm)
			{
				IntermImage::Level	image_level;
				image_level.pixels.resize( block_dim.y * pitch * block_dim.z );

				CHECK_ERR( file.Read( image_level.pixels.data(), ArraySizeOf(image_level.pixels) ));

				image_level.format		= format;
				image_level.dimension	= dim;
				image_level.mipmap		= MipmapLevel{ uint(mm) };
				image_level.layer		= ImageLayer{ uint(layer) };
				image_level.rowPitch	= BytesU{pitch};
				image_level.slicePitch	= image_level.rowPitch * dim.z;
			
				if ( size_t(mm) >= image_data.size() )
					image_data.resize( mm + 1 );

				if ( size_t(layer) >= image_data[mm].size() )
					image_data[mm].resize( layer + 1 );
			
				auto&	curr_mm = image_data[mm][layer];

				CHECK( curr_mm.pixels.empty() );	// warning: previous data will be discarded

				curr_mm = std::move(image_level);
			}
		}

		image->SetData( std::move(image_data), img_type );
		return true;
	}
	
/*
=================================================
	LoadDDSImage
=================================================
*/
	static bool  LoadDDSImage (IntermImagePtr &image, const DDS_HEADER &header, FileRStream &file)
	{
		FG_UNUSED( image, header, file );
		// TODO
		return false;
	}

}	// namespace
//-----------------------------------------------------------------------------


/*
=================================================
	LoadImage
=================================================
*/
	bool  DDSLoader::LoadImage (INOUT IntermImagePtr &image, ArrayView<StringView> directories, const ImageCachePtr &imgCache, bool flipY)
	{
		CHECK_ERR( image );
		CHECK_ERR( not flipY );

		String	filename;
		CHECK_ERR( _FindImage( image->GetPath(), directories, OUT filename ));

		// search in cache
		if ( imgCache and imgCache->GetImageData( filename, OUT image ))
			return true;
		
		// load DDS header
		FileRStream		file{ filename };
		CHECK_ERR( file.IsOpen() );

		DDS_HEADER					header		= {};
		Optional<DDS_HEADER_DXT10>	header_10;
		bool						result		= true;

		result &= file.Read( OUT header );
		CHECK_ERR( result );
		CHECK_ERR( header.dwMagic == MakeFourCC('D','D','S',' '));
		CHECK_ERR( header.dwSize == sizeof(header) - sizeof(header.dwMagic) );
		CHECK_ERR( header.ddspf.dwSize == sizeof(header.ddspf) );

		if ( EnumEq( header.ddspf.dwFlags, DDPF_FOURCC ) and header.ddspf.dwFourCC == MakeFourCC('D','X','1','0') )
		{
			header_10  = DDS_HEADER_DXT10{};
			result    &= file.Read( OUT *header_10 );

			CHECK_ERR( result and LoadDX10Image( image, header, *header_10, file ));
		}
		else
		{
			CHECK_ERR( LoadDDSImage( image, header, file ));
		}

		if ( imgCache )
			imgCache->AddImageData( filename, image );

		return true;
	}

}	// FG
