// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
		EImage				imageType	= Default;
		uint3				dimension;	// width, height, depth, layers
		EPixelFormat		format		= Default;
		EImageUsage			usage		= Default;
		ImageLayer			arrayLayers;
		MipmapLevel			maxLevel;
		MultiSamples		samples;	// if > 1 then enabled multisampling
		EQueueUsageBits		queues		= Default;
		//bool				isLogical	= false;
		bool				isExternal	= false;

	// methods
		ImageDesc () {}
		
		ImageDesc (EImage			imageType,
				   const uint3 &	dimension,
				   EPixelFormat		format,
				   EImageUsage		usage,
				   ImageLayer		arrayLayers	= Default,
				   MipmapLevel		maxLevel	= Default,
				   MultiSamples		samples		= Default,
				   EQueueUsageBits	queues		= Default);

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
		EImageAspect		aspectMask	= Default;

	// methods
		ImageViewDesc () {}

		ImageViewDesc (EImage			viewType,
					   EPixelFormat		format,
					   MipmapLevel		baseLevel	= Default,
					   uint				levelCount	= 1,
					   ImageLayer		baseLayer	= Default,
					   uint				layerCount	= 1,
					   ImageSwizzle		swizzle		= Default,
					   EImageAspect		aspectMask	= Default);

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
			return size_t(FGC::HashOf( AddressOf(value), sizeof(value) ));
		#else
			FG::HashVal	result;
			result << FGC::HashOf( value.imageType );
			result << FGC::HashOf( value.dimension );
			result << FGC::HashOf( value.format );
			result << FGC::HashOf( value.usage );
			result << FGC::HashOf( value.maxLevel );
			result << FGC::HashOf( value.samples );
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
	};

}	// std

