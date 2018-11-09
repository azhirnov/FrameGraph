// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Shared/ResourceDataRange.h"
#include "framegraph/Public/ImageLayer.h"
#include "framegraph/Public/MipmapLevel.h"

namespace FG
{

	//
	// Image Data Range
	//

	struct ImageDataRange
	{
	// types
	public:
		using Self			= ImageDataRange;
		using Value_t		= uint;
		using SubRange_t	= ResourceDataRange< Value_t >;

	private:
		using RangeU		= ResourceDataRange< uint >;


	// variables
	private:
		RangeU		_layers;
		RangeU		_mipmaps;


	// methods
	private:
		explicit ImageDataRange (const RangeU &layers, const RangeU &mipmaps) :
			_layers{ layers }, _mipmaps{ mipmaps } {}

	public:
		ImageDataRange () {}

		ImageDataRange (ImageLayer baseLayer, uint layerCount, MipmapLevel baseLevel, uint levelCount)
		{
			_layers  = RangeU{ 0, layerCount } + baseLayer.Get();
			_mipmaps = RangeU{ 0, levelCount } + baseLevel.Get();
		}

		ND_ RangeU const&	Layers ()						const	{ return _layers; }
		ND_ RangeU const&	Mipmaps ()						const	{ return _mipmaps; }
		
		ND_ bool		IsWholeLayers ()					const	{ return _layers.IsWhole(); }
		ND_ bool		IsWholeMipmaps ()					const	{ return _mipmaps.IsWhole(); }
		
		ND_ bool		IsEmpty ()							const	{ return _layers.IsEmpty() or _mipmaps.IsEmpty(); }
		
		ND_ bool		operator == (const Self &rhs)		const	{ return _layers == rhs._layers and _mipmaps == rhs._mipmaps; }
		ND_ bool		operator != (const Self &rhs)		const	{ return not (*this == rhs); }


		ND_ Self		Intersect (const Self &other)		const
		{
			RangeU	layers = _layers.Intersect( other._layers );
			return Self{ layers, layers.IsEmpty() ? RangeU() : _mipmaps.Intersect( other._mipmaps ) };
		}


		ND_ bool		IsIntersects (const Self &other)	const
		{
			return	_layers.IsIntersects( other._layers ) and
					_mipmaps.IsIntersects( other._mipmaps );
		}


		/*Self &			Merge (const Self &other)
		{
			_layers.Merge( other._layers );
			_mipmaps.Merge( other._mipmaps );	// TODO ?
			return*this;
		}*/
	};

}	// FG
