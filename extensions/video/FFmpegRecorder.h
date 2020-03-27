// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_FFMPEG

#include "video/IVideoRecorder.h"
#include "stl/Stream/Stream.h"
#include "video/FFMpegLoader.h"

namespace FG
{

	//
	// FFmpeg Video Recorder
	//

	class FFmpegVideoRecorder final : public IVideoRecorder
	{
	// types
	private:
		struct CodecInfo
		{
			using Name_t = StaticString<64>;
			
			Name_t					format;
			FixedArray<Name_t, 8>	codecs;
			bool					remux		= true;
			bool					hasBFrames	= true;
		};


	// variables
	private:
		AVOutputFormat *	_format			= null;
		AVFormatContext *	_formatCtx		= null;

		AVStream *			_videoStream	= null;
		AVFrame *			_videoFrame		= null;

		AVCodec *			_codec			= null;
		AVCodecContext *	_codecCtx		= null;

		SwsContext *		_swsCtx			= null;

		AVPacket			_videoPacket;

		uint				_frameCounter	= 0;
		uint				_fps			= 0;

		String				_tempFile;
		String				_videoFile;
		bool				_remuxRequired	= false;

		Config				_config;



	// methods
	public:
		FFmpegVideoRecorder ();
		~FFmpegVideoRecorder () override;
		
		bool Begin (const Config &cfg, StringView filename) override;
		bool AddFrame (const ImageView &view) override;
		bool End () override;
		
		Config		GetConfig () const override;
		StringView	GetExtension (EVideoCodec codec) const override;

	private:
		bool _Init (const Config &cfg);
		bool _Remux ();
		bool _Finish ();
		void _Destroy ();

		void _SetOptions (INOUT AVDictionary **dict, const Config &cfg) const;

		static ND_ CodecInfo  _GetEncoderInfo (const Config &cfg);
	};


}	// FG

#endif	// FG_ENABLE_FFMPEG
