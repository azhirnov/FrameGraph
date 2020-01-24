// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Common.h"

namespace FG
{

	//
	// Intermediate Image
	//

	class IntermImage final : public std::enable_shared_from_this<IntermImage>
	{
	// types
	public:
		struct Level
		{
			uint3			dimension;
			EPixelFormat	format;
			ImageLayer		layer;
			MipmapLevel		mipmap;
			BytesU			rowPitch;
			BytesU			slicePitch;
			Array<uint8_t>	pixels;
		};

		using ArrayLayers_t		= Array< Level >;			// size == 1 for non-array images
		using Mipmaps_t			= Array< ArrayLayers_t >;


	// variables
	private:
		String		_srcPath;

		Mipmaps_t	_data;		// mipmaps[] { layers[] { level{} } }

		bool		_immutable	= false;


	// methods
	public:
		IntermImage () {}
		explicit IntermImage (StringView path) : _srcPath{path} {}
		explicit IntermImage (Mipmaps_t &&data, StringView path = Default) : _srcPath{path}, _data{std::move(data)} {}

		void  MakeImmutable ()							{ _immutable = true; }
		void  SetData (Mipmaps_t &&data)				{ ASSERT( not _immutable );  _data = std::move(data); }
		void  ReleaseData ()							{ Mipmaps_t temp;  std::swap( temp, _data ); }

		ND_ StringView			GetPath ()		const	{ return _srcPath; }
		ND_ bool				IsImmutable ()	const	{ return _immutable; }
		ND_ Mipmaps_t const&	GetData ()		const	{ return _data; }
	};


}	// FG
