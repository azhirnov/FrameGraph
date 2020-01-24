// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ImageViewDesc.h"
#include "EnumUtils.h"

namespace FG
{

/*
=================================================
	ImageDesc
=================================================
*/
	ImageDesc::ImageDesc (EImage		imageType,
						  const uint3 &	dimension,
						  EPixelFormat	format,
						  EImageUsage	usage,
						  ImageLayer	arrayLayers,
						  MipmapLevel	maxLevel,
						  MultiSamples	samples,
						  EQueueUsage	queues)  :
		imageType(imageType),		dimension(dimension),
		format(format),				usage(usage),
		arrayLayers{arrayLayers},	maxLevel(maxLevel),
		samples(samples),			queues{queues}
	{}
	
/*
=================================================
	ValidateDimension
=================================================
*/
	static bool ValidateDimension (const EImage imageType, INOUT uint3 &dim, INOUT ImageLayer &layers)
	{
		BEGIN_ENUM_CHECKS();
		switch ( imageType )
		{
			case EImage::Buffer :
			case EImage::Tex1D :
			{
				ASSERT( dim.x > 0 );
				ASSERT( dim.y <= 1 and dim.z <= 1 and layers.Get() <= 1 );
				dim		= Max( uint3( dim.x, 0, 0 ), 1u );
				layers	= 1_layer;
				return true;
			}	
			case EImage::Tex2DMS :
			case EImage::Tex2D :
			{
				ASSERT( dim.x > 0 and dim.y > 0 );
				ASSERT( dim.z <= 1 and layers.Get() <= 1 );
				dim		= Max( uint3( dim.x, dim.y, 0 ), 1u );
				layers	= 1_layer;
				return true;
			}
			case EImage::TexCube :
			{
				ASSERT( dim.x > 0 and dim.y > 0 and dim.z <= 1 );
				ASSERT( layers.Get() == 6 );
				dim		= Max( uint3( dim.x, dim.y, 1 ), 1u );
				layers	= 6_layer;
				return true;
			}
			case EImage::Tex1DArray :
			{
				ASSERT( dim.x > 0 and layers.Get() > 0 );
				[[fallthrough]];
			}
			case EImage::Tex2DMSArray :
			case EImage::Tex2DArray :
			{
				ASSERT( dim.x > 0 and dim.y > 0 and layers.Get() > 0 );
				ASSERT( dim.z <= 1 );
				dim		= Max( uint3( dim.x, dim.y, 0 ), 1u );
				layers	= Max( layers, 1_layer );
				return true;
			}
			case EImage::Tex3D :
			{
				ASSERT( dim.x > 0 and dim.y > 0 and dim.z > 0 );
				ASSERT( layers.Get() <= 1 );
				dim		= Max( uint3( dim.x, dim.y, dim.z ), 1u );
				layers	= 1_layer;
				return true;
			}
			case EImage::TexCubeArray :
			{
				ASSERT( dim.x > 0 and dim.y > 0 and dim.z <= 1 );
				ASSERT( layers.Get() > 0 and layers.Get() % 6 == 0 );
				dim		= Max( uint3( dim.x, dim.y, 0 ), 1u );
				layers	= Max( layers, 6_layer );
				return true;
			}
			case EImage::Unknown :
				break;		// to shutup warnings
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "invalid texture type" );
	}

/*
=================================================
	NumberOfMipmaps
=================================================
*/	
	ND_ static uint  NumberOfMipmaps (const EImage imageType, const uint3 &dim)
	{
		BEGIN_ENUM_CHECKS();
		switch ( imageType )
		{
			case EImage::Buffer :
			case EImage::Tex2DMS :
			case EImage::Tex2DMSArray :		return 1;

			case EImage::Tex2D :
			case EImage::Tex3D :			return std::ilogb( Max(Max( dim.x, dim.y ), dim.z )) + 1;
				
			case EImage::Tex1D :
			case EImage::Tex1DArray :		return std::ilogb( dim.x ) + 1;

			case EImage::TexCube :
			case EImage::TexCubeArray :
			case EImage::Tex2DArray :		return std::ilogb( Max( dim.x, dim.y )) + 1;

			case EImage::Unknown :			break;		// to shutup warnings
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "invalid texture type", 1u );
	}

/*
=================================================
	ValidateDescription
=================================================
*/
	void ImageDesc::Validate ()
	{
		ASSERT( format != EPixelFormat::Unknown );
		ASSERT( imageType != EImage::Unknown );

		ValidateDimension( imageType, INOUT dimension, INOUT arrayLayers );

		if ( EImage_IsMultisampled( imageType ) )
		{
			ASSERT( samples > 1_samples );
			ASSERT( maxLevel <= 1_mipmap );
			maxLevel = 1_mipmap;
		}
		else
		{
			ASSERT( samples <= 1_samples );
			samples  = 1_samples;
			maxLevel = MipmapLevel( Clamp( maxLevel.Get(), 1u, NumberOfMipmaps( imageType, dimension ) ));
		}
	}
//-----------------------------------------------------------------------------


/*
=================================================
	ImageViewDesc
=================================================
*/	
	ImageViewDesc::ImageViewDesc (EImage			viewType,
								  EPixelFormat		format,
								  MipmapLevel		baseLevel,
								  uint				levelCount,
								  ImageLayer		baseLayer,
								  uint				layerCount,
								  ImageSwizzle		swizzle,
								  EImageAspect		aspectMask) :
		viewType{viewType},		format{format},
		baseLevel{baseLevel},	levelCount{levelCount},
		baseLayer{baseLayer},	layerCount{layerCount},
		swizzle{swizzle},		aspectMask{aspectMask}
	{}
	
/*
=================================================
	ImageViewDesc
=================================================
*/
	ImageViewDesc::ImageViewDesc (const ImageDesc &desc) :
		viewType{ desc.imageType },	format{ desc.format },
		baseLevel{},				levelCount{ desc.maxLevel.Get() },
		baseLayer{},				layerCount{ desc.arrayLayers.Get() },
		swizzle{ "RGBA"_swizzle },	aspectMask{EPixelFormat_ToImageAspect(format)}
	{}
	
/*
=================================================
	ImageViewDesc::Validate
=================================================
*/
	void ImageViewDesc::Validate (const ImageDesc &desc)
	{
		if ( viewType == EImage::Unknown )
			viewType = desc.imageType;

		if ( format == EPixelFormat::Unknown )
			format = desc.format;

		uint	max_layers	= desc.imageType == EImage::Tex3D ? 1 : desc.arrayLayers.Get();

		baseLayer	= ImageLayer{Clamp( baseLayer.Get(), 0u, max_layers-1 )};
		layerCount	= Clamp( layerCount, 1u, max_layers - baseLayer.Get() );

		baseLevel	= MipmapLevel{Clamp( baseLevel.Get(), 0u, desc.maxLevel.Get()-1 )};
		levelCount	= Clamp( levelCount, 1u, desc.maxLevel.Get() - baseLevel.Get() );

		auto	mask = EPixelFormat_ToImageAspect( format );
		aspectMask   = (aspectMask == Default ? mask : aspectMask & mask);

		ASSERT( aspectMask != Default );
	}

/*
=================================================
	ImageViewDesc::operator ==
=================================================
*/
	bool ImageViewDesc::operator == (const ImageViewDesc &rhs) const
	{
		return	this->viewType		== rhs.viewType		and
				this->format		== rhs.format		and
				this->baseLevel		== rhs.baseLevel	and
				this->levelCount	== rhs.levelCount	and
				this->baseLayer		== rhs.baseLayer	and
				this->layerCount	== rhs.layerCount	and
				this->aspectMask	== rhs.aspectMask	and
				this->swizzle		== rhs.swizzle;
	}


}	// FG
