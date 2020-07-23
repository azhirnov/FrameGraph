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
			EPixelFormat	format		= Default;
			ImageLayer		layer		= 0_layer;
			MipmapLevel		mipmap		= 0_mipmap;
			BytesU			rowPitch;
			BytesU			slicePitch;
			Array<uint8_t>	pixels;
		};

		using ArrayLayers_t		= Array< Level >;			// size == 1 for non-array images
		using Mipmaps_t			= Array< ArrayLayers_t >;


	// variables
	private:
		String		_srcPath;

		Mipmaps_t	_data;					// mipmaps[] { layers[] { level } }
		EImage		_imageType	= Default;

		bool		_immutable	= false;


	// methods
	public:
		IntermImage () {}
		explicit IntermImage (const ImageView &view);
		explicit IntermImage (StringView path) : _srcPath{path} {}
		IntermImage (Mipmaps_t &&data, EImage type, StringView path = Default) : _srcPath{path}, _data{std::move(data)}, _imageType{type} {}

		void  MakeImmutable ()							{ _immutable = true; }
		void  SetData (Mipmaps_t &&data, EImage type);
		void  ReleaseData ()							{ Mipmaps_t temp;  std::swap( temp, _data ); }

		ND_ StringView			GetPath ()		const	{ return _srcPath; }
		ND_ bool				IsImmutable ()	const	{ return _immutable; }
		ND_ Mipmaps_t const&	GetData ()		const	{ return _data; }
		ND_ EImage				GetType ()		const	{ return _imageType; }
	};


}	// FG
