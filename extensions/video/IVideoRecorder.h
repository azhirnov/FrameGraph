// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/FG.h"

namespace FG
{

	enum class EVideoFormat : uint
	{
		YUV_420P,
	};

	enum class EVideoCodec : uint
	{
		H264,
		VP8,
		VP9
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
		Default		= Medium,
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
			EVideoFormat	format		= EVideoFormat::YUV_420P;
			EVideoCodec		codec		= EVideoCodec::H264;
			EVideoPreset	preset		= EVideoPreset::Default;
			uint			fps			= 30;
			uint2			size		= {1920, 1080};
			uint64_t		bitrate		= 50 << 20;		// Mbit/s
			bool			preferGPU	= false;
		};


	// interface
	public:
		virtual ~IVideoRecorder () {}

		virtual bool Begin (const Config &cfg, StringView filename) = 0;
		virtual bool AddFrame (const ImageView &view) = 0;
		virtual bool End () = 0;
	};


}	// FG
