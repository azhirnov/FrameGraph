// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
	};



	//
	// Video Recorder interface
	//

	class IVideoRecorder
	{
	// interface
	public:
		virtual ~IVideoRecorder () {}

		virtual bool Begin (const uint2 &size, uint fps, uint bitrateInKbps, EVideoFormat fmt, StringView filename) = 0;
		virtual bool AddFrame (const ImageView &view) = 0;
		virtual bool End () = 0;
	};


}	// FG
