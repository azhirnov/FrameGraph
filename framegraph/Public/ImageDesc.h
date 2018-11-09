// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/MipmapLevel.h"
#include "framegraph/Public/MultiSamples.h"
#include "framegraph/Public/ImageLayer.h"
#include "framegraph/Public/ImageSwizzle.h"
#include "framegraph/Public/ResourceEnums.h"

namespace FG
{

	//
	// Image Description
	//

	struct ImageDesc
	{
	// variables
		EImage			imageType	= Default;
		uint3			dimension;	// width, height, depth, layers
		EPixelFormat	format		= Default;
		EImageUsage		usage		= Default;
		ImageLayer		arrayLayers;
		MipmapLevel		maxLevel;
		MultiSamples	samples;	// if > 1 then enabled multisampling
		//bool			isLogical	= false;
		//bool			isExternal	= false;

	// methods
		ImageDesc () {}
		
		ImageDesc (EImage		imageType,
				   const uint3	&dimension,
				   EPixelFormat	format,
				   EImageUsage	usage,
				   ImageLayer	arrayLayers	= Default,
				   MipmapLevel	maxLevel	= Default,
				   MultiSamples	samples		= Default);

		void Validate ();
	};
		


	//
	// Image View Description
	//

	struct ImageViewDesc
	{
	// variables
		EImage				viewType	= Default;
		EPixelFormat		format		= Default;
		MipmapLevel			baseLevel;
		uint				levelCount	= 1;
		ImageLayer			baseLayer;
		uint				layerCount	= 1;
		ImageSwizzle		swizzle;

	// methods
		ImageViewDesc () {}

		ImageViewDesc (EImage			viewType,
					   EPixelFormat		format,
					   MipmapLevel		baseLevel	= Default,
					   uint				levelCount	= 1,
					   ImageLayer		baseLayer	= Default,
					   uint				layerCount	= 1,
					   ImageSwizzle		swizzle		= Default);

		explicit ImageViewDesc (const ImageDesc &desc);

		void Validate (const ImageDesc &desc);

		ND_ bool operator == (const ImageViewDesc &rhs) const;
	};


}	// FG


namespace std
{
	template <>
	struct hash< FG::ImageDesc >
	{
		ND_ size_t  operator () (const FG::ImageDesc &value) const noexcept
		{
		#if FG_FAST_HASH
			return size_t(FG::HashOf( AddressOf(value), sizeof(value) ));
		#else
			FG::HashVal	result;
			result << FG::HashOf( value.imageType );
			result << FG::HashOf( value.dimension );
			result << FG::HashOf( value.format );
			result << FG::HashOf( value.usage );
			result << FG::HashOf( value.maxLevel );
			result << FG::HashOf( value.samples );
			return size_t(result);
		#endif
		}
	};
	

	template <>
	struct hash< FG::ImageViewDesc >
	{
		ND_ size_t  operator () (const FG::ImageViewDesc &value) const noexcept
		{
		#if FG_FAST_HASH
			return size_t(FG::HashOf( AddressOf(value), sizeof(value) ));
		#else
			FG::HashVal	result;
			result << FG::HashOf( value.viewType );
			result << FG::HashOf( value.format );
			result << FG::HashOf( value.baseLevel );
			result << FG::HashOf( value.levelCount );
			result << FG::HashOf( value.baseLayer );
			result << FG::HashOf( value.layerCount );
			result << FG::HashOf( value.swizzle );
			return size_t(result);
		#endif
		}
	};

}	// std

