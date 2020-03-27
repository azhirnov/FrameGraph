// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/FG.h"

namespace FG
{

	enum class EVideoFormat : uint
	{
		YUV420P,	// 12bpp
		YUV422P,	// 16bpp
		YUV444P,	// 24bpp
	};

	enum class EVideoCodec : uint
	{
		H264,		// HW accelerated
		H265,		// HW accelerated
		WEBP,		// web animation
	};

	enum class EVideoPreset : uint
	{
		UltraFast,
		SuperFast,
		VeryFast,
		Faster,
		Fast,
		Medium,
		Slow,
		Slower,
		VerySlow,
		Default,
	};



	//
	// Video Recorder interface
	//

	class IVideoRecorder
	{
	// types
	public:
		struct Config
		{
			EVideoFormat	format			= EVideoFormat::YUV420P;
			EVideoCodec		codec			= EVideoCodec::H264;
			EVideoPreset	preset			= EVideoPreset::Default;
			uint			fps				= 30;
			uint2			size			= {1920, 1080};
			uint64_t		bitrate			= 50 << 20;		// Mbit/s
			bool			hwAccelerated	= false;		// use hardware acceleration on GPU or CPU
		};


	// interface
	public:
		virtual ~IVideoRecorder () {}

		virtual bool Begin (const Config &cfg, StringView filename) = 0;
		virtual bool AddFrame (const ImageView &view) = 0;
		virtual bool End () = 0;

		ND_ virtual Config		GetConfig () const = 0;
		ND_ virtual StringView	GetExtension (EVideoCodec codec) const = 0;
	};


}	// FG
