// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_FFMPEG

#include "video/IVideoRecorder.h"
#include "stl/Stream/Stream.h"

# ifdef COMPILER_MSVC
#	pragma warning (push)
#	pragma warning (disable: 4244)	// conversion from '...' to '...', possible loss of data
# endif

extern "C"
{
# include <libavcodec/avcodec.h>
# include <libavcodec/avfft.h>

# include <libavdevice/avdevice.h>

# include <libavfilter/avfilter.h>
# include <libavfilter/buffersink.h>
# include <libavfilter/buffersrc.h>

# include <libavformat/avformat.h>
# include <libavformat/avio.h>
	
# include <libswscale/swscale.h>

}	// extern "C"

# ifdef COMPILER_MSVC
#	pragma warning (pop)
# endif


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



	// methods
	public:
		FFmpegVideoRecorder ();
		~FFmpegVideoRecorder () override;
		
		bool Begin (const Config &cfg, StringView filename) override;
		bool AddFrame (const ImageView &view) override;
		bool End () override;

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
