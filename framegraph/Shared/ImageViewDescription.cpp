// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Public/LowLevel/ImageResource.h"

namespace FG
{

/*
=================================================
	ImageDescription
=================================================
*/
	ImageResource::ImageDescription::ImageDescription (EImage		imageType,
													   const uint3	&dimension,
													   EPixelFormat	format,
													   EImageUsage	usage,
													   ImageLayer	arrayLayers,
													   MipmapLevel	maxLevel,
													   MultiSamples	samples)  :
		imageType(imageType),		dimension(dimension),
		format(format),				usage(usage),
		arrayLayers{arrayLayers},	maxLevel(maxLevel),
		samples(samples)
	{}
		
/*
=================================================
	ImageViewDescription
=================================================
*/	
	ImageResource::ImageViewDescription::ImageViewDescription (EImage			viewType,
															   EPixelFormat		format,
															   MipmapLevel		baseLevel,
															   uint				levelCount,
															   ImageLayer		baseLayer,
															   uint				layerCount,
															   ImageSwizzle		swizzle) :
		viewType{viewType},		format{format},
		baseLevel{baseLevel},	levelCount{levelCount},
		baseLayer{baseLayer},	layerCount{layerCount},
		swizzle{swizzle}
	{}
	
/*
=================================================
	ImageViewDescription
=================================================
*/
	ImageResource::ImageViewDescription::ImageViewDescription (const ImageDescription &desc) :
		viewType{ desc.imageType },		format{ desc.format },
		baseLevel{},					levelCount{ desc.maxLevel.Get() },
		baseLayer{},					layerCount{ desc.arrayLayers.Get() }
		//swizzle{ "RGBA"_Swizzle }
	{}
	
/*
=================================================
	ImageViewDescription::Validate
=================================================
*/
	void ImageResource::ImageViewDescription::Validate (const ImageDescription &desc)
	{
		if ( viewType == EImage::Unknown )
			viewType = desc.imageType;

		if ( format == EPixelFormat::Unknown )
			format = desc.format;

		uint	max_layers	= desc.imageType == EImage::Tex3D ? 1 : desc.arrayLayers.Get();

		baseLayer	= ImageLayer(Clamp( baseLayer.Get(), 0u, max_layers-1 ));
		layerCount	= Clamp( layerCount, 1u, max_layers - baseLayer.Get() );

		baseLevel	= MipmapLevel(Clamp( baseLevel.Get(), 0u, desc.maxLevel.Get()-1 ));
		levelCount	= Clamp( levelCount, 1u, desc.maxLevel.Get() - baseLevel.Get() );
	}

/*
=================================================
	ImageViewDescription::operator ==
=================================================
*/
	bool ImageResource::ImageViewDescription::operator == (const ImageViewDescription &rhs) const
	{
		return	this->viewType		== rhs.viewType		and
				this->format		== rhs.format		and
				this->baseLevel		== rhs.baseLevel	and
				this->levelCount	== rhs.levelCount	and
				this->baseLayer		== rhs.baseLayer	and
				this->layerCount	== rhs.layerCount	and
				this->swizzle		== rhs.swizzle;
	}


}	// FG
