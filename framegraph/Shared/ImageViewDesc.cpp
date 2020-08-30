// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ImageViewDesc.h"
#include "EnumUtils.h"

namespace FG
{
	
/*
=================================================
	ImageDesc::SetDimension
=================================================
*/
	ImageDesc&  ImageDesc::SetDimension (const uint value)
	{
		dimension = uint3{ value, 1, 1 };
		imageType = imageType == Default ? EImageDim_1D : imageType;
		return *this;
	}

	ImageDesc&  ImageDesc::SetDimension (const uint2 &value)
	{
		dimension = uint3{ value, 1 };
		imageType = imageType == Default ? EImageDim_2D : imageType;
		return *this;
	}

	ImageDesc&  ImageDesc::SetDimension (const uint3 &value)
	{
		dimension = value;
		imageType = imageType == Default ? EImageDim_3D : imageType;
		return *this;
	}
	
/*
=================================================
	SetView
=================================================
*/	
	ImageDesc&  ImageDesc::SetView (EImage value)
	{
		viewType = value;

		BEGIN_ENUM_CHECKS();
		switch ( viewType )
		{
			case EImage_1D :
			case EImage_1DArray :
				imageType = EImageDim_1D;
				break;

			case EImage_2D :
			case EImage_2DArray :
				imageType = EImageDim_2D;
				break;

			case EImage_Cube :
			case EImage_CubeArray :
				imageType = EImageDim_2D;
				flags    |= EImageFlags::CubeCompatible;
				break;
				
			case EImage_3D :
				imageType = EImageDim_3D;
				flags    |= EImageFlags::Array2DCompatible;
				break;

			case EImage::Unknown :
			default :
				ASSERT( !"unknown image view type" );
		}
		END_ENUM_CHECKS();

		return *this;
	}

/*
=================================================
	NumberOfMipmaps
=================================================
*/	
namespace {
	ND_ inline uint  NumberOfMipmaps (const uint3 &dim)
	{
		return IntLog2( Max(Max( dim.x, dim.y ), dim.z )) + 1;
	}
}
/*
=================================================
	Validate
=================================================
*/
	void ImageDesc::Validate ()
	{
		ASSERT( format != Default );
		ASSERT( imageType != Default );
		
		dimension	= Max( dimension, uint3{1} );
		arrayLayers	= Max( arrayLayers, 1_layer );

		BEGIN_ENUM_CHECKS();
		switch ( imageType )
		{
			case EImageDim_1D :
				ASSERT( not samples.IsEnabled() );
				ASSERT( dimension.y == 1 and dimension.z == 1 );
				ASSERT( not AnyBits( flags, EImageFlags::Array2DCompatible | EImageFlags::CubeCompatible ));	// this flags are not supported for 1D

				flags		&= ~(EImageFlags::Array2DCompatible | EImageFlags::CubeCompatible);
				samples		= 1_samples;
				dimension	= uint3{ dimension.x, 1, 1 };
				break;

			case EImageDim_2D :
				ASSERT( dimension.z == 1 );
				if ( AllBits( flags, EImageFlags::CubeCompatible ) and (arrayLayers.Get() % 6 != 0) )
					flags &= ~EImageFlags::CubeCompatible;
				
				dimension.z	= 1;
				break;

			case EImageDim_3D :
				ASSERT( not samples.IsEnabled() );
				ASSERT( arrayLayers == 1_layer );
				ASSERT( not AnyBits( flags, EImageFlags::CubeCompatible ));	// flags are not supported for 1D

				flags		&= ~EImageFlags::CubeCompatible;
				samples		= 1_samples;
				arrayLayers	= 1_layer;
				break;

			case EImageDim::Unknown :
				break;
		}
		END_ENUM_CHECKS();

		// validate samples and mipmaps
		if ( samples.IsEnabled() )
		{
			ASSERT( maxLevel <= 1_mipmap );
			maxLevel = 1_mipmap;
		}
		else
		{
			samples  = 1_samples;
			maxLevel = MipmapLevel( Clamp( maxLevel.Get(), 1u, NumberOfMipmaps( dimension )));
		}
		
		// set default view
		if ( viewType == Default )
		{
			BEGIN_ENUM_CHECKS();
			switch ( imageType )
			{
				case EImageDim_1D :
					if ( arrayLayers > 1_layer )
						viewType = EImage_1DArray;
					else
						viewType = EImage_1D;
					break;

				case EImageDim_2D :
					if ( arrayLayers > 6_layer and AllBits( flags, EImageFlags::CubeCompatible ))
						viewType = EImage_CubeArray;
					else
					if ( arrayLayers == 6_layer and AllBits( flags, EImageFlags::CubeCompatible ))
						viewType = EImage_Cube;
					else
					if ( arrayLayers > 1_layer )
						viewType = EImage_2DArray;
					else
						viewType = EImage_2D;
					break;

				case EImageDim_3D :
					viewType = EImage_3D;
					break;

				case EImageDim::Unknown :
				default :
					ASSERT( !"unknown image dimension" );
					break;
			}
			END_ENUM_CHECKS();
		}
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	ImageViewDesc
=================================================
*/	
	ImageViewDesc::ImageViewDesc (EImage		viewType,
								  EPixelFormat		format,
								  MipmapLevel		baseLevel,
								  uint				levelCount,
								  ImageLayer		baseLayer,
								  uint				layerCount,
								  ImageSwizzle		swizzle,
								  EImageAspect		aspectMask) :
		viewType{ viewType },		format{ format },
		baseLevel{ baseLevel },		levelCount{ levelCount },
		baseLayer{ baseLayer },		layerCount{ layerCount },
		swizzle{ swizzle },			aspectMask{ aspectMask }
	{}
	
/*
=================================================
	ImageViewDesc
=================================================
*/
	ImageViewDesc::ImageViewDesc (const ImageDesc &desc) :
		viewType{ desc.viewType },	format{ desc.format },
		baseLevel{},				levelCount{ desc.maxLevel.Get() },
		baseLayer{},				layerCount{ desc.arrayLayers.Get() },
		swizzle{ "RGBA"_swizzle },	aspectMask{ EPixelFormat_ToImageAspect( format )}
	{
	}
	
/*
=================================================
	ImageViewDesc::Validate
=================================================
*/
	void ImageViewDesc::Validate (const ImageDesc &desc)
	{
		baseLevel	= MipmapLevel{Clamp( baseLevel.Get(), 0u, desc.maxLevel.Get()-1 )};
		levelCount	= Clamp( levelCount, 1u, desc.maxLevel.Get() - baseLevel.Get() );

		// validate format
		if ( format == Default )
		{
			format = desc.format;
		}
		else
		if ( format != desc.format and not AllBits( desc.flags, EImageFlags::MutableFormat ))
		{
			ASSERT( !"can't change format for immutable image" );
			format = desc.format;
		}

		// validate aspect mask
		EImageAspect	mask = EPixelFormat_ToImageAspect( format );
		aspectMask			 = (aspectMask == Default ? mask : (aspectMask & mask));
		ASSERT( aspectMask != Default );


		// choose view type
		if ( viewType == Default )
		{
			const uint	max_layers	= desc.arrayLayers.Get();

			baseLayer	= ImageLayer{Clamp( baseLayer.Get(), 0u, max_layers-1 )};
			layerCount	= Clamp( layerCount, 1u, max_layers - baseLayer.Get() );

			BEGIN_ENUM_CHECKS();
			switch ( desc.imageType )
			{
				case EImageDim_1D :
					if ( layerCount > 1 )
						viewType = EImage_1DArray;
					else
						viewType = EImage_1D;
					break;

				case EImageDim_2D :
					if ( layerCount > 6 and AllBits( desc.flags, EImageFlags::CubeCompatible ))
						viewType = EImage_CubeArray;
					else
					if ( layerCount == 6 and AllBits( desc.flags, EImageFlags::CubeCompatible ))
						viewType = EImage_Cube;
					else
					if ( layerCount > 1 )
						viewType = EImage_2DArray;
					else
						viewType = EImage_2D;
					break;

				case EImageDim_3D :
					viewType = EImage_3D;
					break;

				case EImageDim::Unknown :
					break;
			}
			END_ENUM_CHECKS();
		}
		else
		// validate view type
		{
			const uint	max_layers	= (desc.imageType == EImageDim_3D and viewType != EImage_3D ? desc.dimension.z : desc.arrayLayers.Get());

			baseLayer = ImageLayer{Clamp( baseLayer.Get(), 0u, max_layers-1 )};

			BEGIN_ENUM_CHECKS();
			switch ( viewType )
			{
				case EImage_1D :
					ASSERT( desc.imageType == EImageDim_1D );
					ASSERT( layerCount == UMax or layerCount == 1 );
					layerCount = 1;
					break;

				case EImage_1DArray :
					ASSERT( desc.imageType == EImageDim_1D );
					layerCount = Clamp( layerCount, 1u, max_layers - baseLayer.Get() );
					break;

				case EImage_2D :
					ASSERT( desc.imageType == EImageDim_2D or
						    (desc.imageType == EImageDim_3D and AllBits( desc.flags, EImageFlags::Array2DCompatible )));
					ASSERT( layerCount == UMax or layerCount == 1 );
					layerCount = 1;
					break;

				case EImage_2DArray :
					ASSERT( desc.imageType == EImageDim_2D or
						    (desc.imageType == EImageDim_3D and AllBits( desc.flags, EImageFlags::Array2DCompatible )));
					layerCount = Clamp( layerCount, 1u, max_layers - baseLayer.Get() );
					break;

				case EImage_Cube :
					ASSERT( desc.imageType == EImageDim_2D or
						    (desc.imageType == EImageDim_3D and AllBits( desc.flags, EImageFlags::Array2DCompatible )));
					ASSERT( AllBits( desc.flags, EImageFlags::CubeCompatible ));
					ASSERT( layerCount == UMax or layerCount == 6 );
					layerCount = 6;
					break;

				case EImage_CubeArray :
					ASSERT( desc.imageType == EImageDim_2D or
						    (desc.imageType == EImageDim_3D and AllBits( desc.flags, EImageFlags::Array2DCompatible )));
					ASSERT( AllBits( desc.flags, EImageFlags::CubeCompatible ));
					ASSERT( layerCount == UMax or layerCount % 6 == 0 );
					layerCount = Max( 1u, ((max_layers - baseLayer.Get()) / 6) ) * 6;
					break;
				
				case EImage_3D :
					ASSERT( desc.imageType == EImageDim_3D );
					ASSERT( layerCount == UMax or layerCount == 1 );
					layerCount = 1;
					break;

				case EImage::Unknown :
				default :
					ASSERT( !"unknown image view type" );
			}
			END_ENUM_CHECKS();
		}
	}

/*
=================================================
	ImageViewDesc::operator ==
=================================================
*/
	bool ImageViewDesc::operator == (const ImageViewDesc &rhs) const
	{
		return	(this->viewType		== rhs.viewType)	&
				(this->format		== rhs.format)		&
				(this->baseLevel	== rhs.baseLevel)	&
				(this->levelCount	== rhs.levelCount)	&
				(this->baseLayer	== rhs.baseLayer)	&
				(this->layerCount	== rhs.layerCount)	&
				(this->aspectMask	== rhs.aspectMask)	&
				(this->swizzle		== rhs.swizzle);
	}

}	// FG

namespace std
{
/*
=================================================
	hash<ImageViewDesc>::operator ()
=================================================
*/
	size_t  hash< FG::ImageViewDesc >::operator () (const FG::ImageViewDesc &value) const
	{
	#if FG_FAST_HASH
		return size_t(FGC::HashOf( AddressOf(value), sizeof(value) ));
	#else
		FG::HashVal	result;
		result << FGC::HashOf( value.viewType );
		result << FGC::HashOf( value.format );
		result << FGC::HashOf( value.baseLevel );
		result << FGC::HashOf( value.levelCount );
		result << FGC::HashOf( value.baseLayer );
		result << FGC::HashOf( value.layerCount );
		result << FGC::HashOf( value.swizzle );
		return size_t(result);
	#endif
	}

}	// std
