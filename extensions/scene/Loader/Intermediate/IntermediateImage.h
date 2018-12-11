// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Common.h"

namespace FG
{

	//
	// Intermediate Image
	//

	class IntermediateImage final : public std::enable_shared_from_this<IntermediateImage>
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


	// methods
	public:
		IntermediateImage () {}
		explicit IntermediateImage (StringView path) : _srcPath{path} {}
		explicit IntermediateImage (Mipmaps_t &&data, StringView path = Default) : _srcPath{path}, _data{std::move(data)} {}

		void SetData (Mipmaps_t &&data)		{ _data = std::move(data); }

		ND_ StringView	GetPath ()	const	{ return _srcPath; }
	};
	
	using IntermediateImagePtr = SharedPtr< IntermediateImage >;


}	// FG
