// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/MipmapLevel.h"
#include "framegraph/Public/LowLevel/MultiSamples.h"
#include "framegraph/Public/LowLevel/ImageLayer.h"
#include "framegraph/Public/LowLevel/ImageSwizzle.h"
#include "framegraph/Public/LowLevel/ResourceEnums.h"
#include "framegraph/Public/LowLevel/EResourceState.h"

namespace FG
{

	//
	// Image Resource
	//

	class ImageResource : public std::enable_shared_from_this<ImageResource>
	{
	// types
	public:
		struct ImageDescription
		{
		// variables
			EImage			imageType	= EImage::Unknown;
			uint3			dimension;	// width, height, depth, layers
			EPixelFormat	format		= EPixelFormat::Unknown;
			EImageUsage		usage;
			ImageLayer		arrayLayers;
			MipmapLevel		maxLevel;
			MultiSamples	samples;	// if > 1 then enabled multisampling

		// methods
			ImageDescription () {}
		
			ImageDescription (EImage		imageType,
							  const uint3	&dimension,
							  EPixelFormat	format,
							  EImageUsage	usage,
							  ImageLayer	arrayLayers	= Default,
							  MipmapLevel	maxLevel	= Default,
							  MultiSamples	samples		= Default);
		};
		

		struct ImageViewDescription
		{
		// variables
			EImage				viewType	= EImage::Unknown;
			EPixelFormat		format		= EPixelFormat::Unknown;
			MipmapLevel			baseLevel;
			uint				levelCount	= 1;
			ImageLayer			baseLayer;
			uint				layerCount	= 1;
			ImageSwizzle		swizzle;

		// methods
			ImageViewDescription () {}

			ImageViewDescription (EImage			viewType,
								  EPixelFormat		format,
								  MipmapLevel		baseLevel	= Default,
								  uint				levelCount	= 1,
								  ImageLayer		baseLayer	= Default,
								  uint				layerCount	= 1,
								  ImageSwizzle		swizzle		= Default);

			explicit
			ImageViewDescription (const ImageDescription &desc);

			void Validate (const ImageDescription &desc);

			ND_ bool operator == (const ImageViewDescription &rhs) const;
		};

		using Description_t	= ImageDescription;


	// variables
	protected:
		EImageUsage			_currentUsage	= Default;
		Description_t		_descr;

		bool				_isLogical   : 1;
		bool				_isExternal	 : 1;
		bool				_isImmutable : 1;


	// methods
	protected:
		ImageResource () : _isLogical{false}, _isExternal{false}, _isImmutable{false} {}

	public:
		virtual ~ImageResource () {}
		
		ND_ virtual ImagePtr		GetReal (Task task, EResourceState rs) = 0;
			virtual void			SetDebugName (StringView name) = 0;
			virtual bool			MakeImmutable (bool immutable) = 0;

			void					AddUsage (EImageUsage value)		{ _currentUsage |= value; }

		ND_ Description_t const&	Description ()		const			{ return _descr; }
		ND_ uint3 const&			Dimension ()		const			{ return _descr.dimension; }
		ND_ uint const				Width ()			const			{ return _descr.dimension.x; }
		ND_ uint const				Height ()			const			{ return _descr.dimension.y; }
		ND_ uint const				Depth ()			const			{ return _descr.dimension.z; }
		ND_ uint const				ArrayLayers ()		const			{ return _descr.arrayLayers.Get(); }
		ND_ uint const				MipmapLevels ()		const			{ return _descr.maxLevel.Get(); }
		ND_ EPixelFormat			PixelFormat ()		const			{ return _descr.format; }
		ND_ EImage					ImageType ()		const			{ return _descr.imageType; }
		ND_ uint const				Samples ()			const			{ return _descr.samples.Get(); }

		ND_ bool					IsLogical ()		const			{ return _isLogical; }
		ND_ bool					IsExternal ()		const			{ return _isExternal; }
		ND_ bool					IsImmutable ()		const			{ return _isImmutable; }
	};


	using ImageViewDesc = ImageResource::ImageViewDescription;

}	// FG


namespace std
{
	template <>
	struct hash< FG::ImageResource::ImageDescription >
	{
		ND_ size_t  operator () (const FG::ImageResource::ImageDescription &value) const noexcept
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
	struct hash< FG::ImageResource::ImageViewDescription >
	{
		ND_ size_t  operator () (const FG::ImageResource::ImageViewDescription &value) const noexcept
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

