// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	based on code from:
	https://stackoverflow.com/questions/46444474/c-ffmpeg-create-mp4-file
	https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/encode_video.c
	https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/remuxing.c
*/

#ifdef FG_ENABLE_FFMPEG

#include "video/FFmpegRecorder.h"
#include "stl/Algorithms/StringUtils.h"

#ifdef FG_STD_FILESYSTEM
#	include <filesystem>
	namespace FS = std::filesystem;
#endif

#define FF_CALL( ... ) \
	{ \
		int __ff_err__ = (__VA_ARGS__); \
		FFCheckError( __ff_err__, FG_PRIVATE_TOSTRING( __VA_ARGS__ ), FG_FUNCTION_NAME, __FILE__, __LINE__ ); \
	}

#define FG_PRIVATE_FF_CALL_R( _func_, _ret_, ... ) \
	{ \
		int __ff_err__ = (_func_); \
		if ( not FFCheckError( __ff_err__, FG_PRIVATE_TOSTRING( _func_ ), FG_FUNCTION_NAME, __FILE__, __LINE__ )) \
			return _ret_; \
	}

#define FF_CHECK( ... ) \
	FG_PRIVATE_FF_CALL_R( FG_PRIVATE_GETARG_0( __VA_ARGS__ ), FG_PRIVATE_GETARG_1( __VA_ARGS__, ::FGC::Default ))


namespace
{
	using namespace FG;
	
	FFMpegLoader	ffmpeg;

/*
=================================================
	FFCheckError
=================================================
*/
	bool FFCheckError (int err, const char *ffcall, const char *func, const char *file, int line)
	{
		if ( err == 0 )
			return true;

		String	msg{ "FFmpeg error: " };
		char	buf [AV_ERROR_MAX_STRING_SIZE] = "";

		ffmpeg.av_strerror( err, OUT buf, sizeof(buf) );

		msg = msg + buf + ", in " + ffcall + ", function: " + func;

		FG_LOGE( msg, file, line );
		return false;
	}

/*
=================================================
	EnumCast
=================================================
*/
	ND_ AVPixelFormat  EnumCast (EVideoFormat fmt)
	{
		switch ( fmt )
		{
			case EVideoFormat::YUV_420P : return AV_PIX_FMT_YUV420P;
		}
		RETURN_ERR( "unknown video format", AV_PIX_FMT_NONE );
	}
}
//-----------------------------------------------------------------------------


namespace FG
{
/*
=================================================
	constructor
=================================================
*/
	FFmpegVideoRecorder::FFmpegVideoRecorder () :
		_videoPacket{}
	{
		CHECK( ffmpeg.Load() );

		ffmpeg.av_init_packet( &_videoPacket );
	}
	
/*
=================================================
	destructor
=================================================
*/
	FFmpegVideoRecorder::~FFmpegVideoRecorder ()
	{
		ffmpeg.Unload();
	}

/*
=================================================
	Begin
=================================================
*/
	bool  FFmpegVideoRecorder::Begin (const Config &cfg, StringView filename)
	{
		CHECK_ERR( not _formatCtx and not _videoFrame and not _videoStream );
		
		#ifdef FG_STD_FILESYSTEM
			_tempFile = FS::path{filename}.replace_filename("tmp").replace_extension("video").string();
		#else
			_tempFile = "tmp.video";
		#endif

		Config	config		= cfg;
		bool	initialized = false;

		for (uint i = 0;; ++i)
		{
			if ( _Init( config ))
			{
				initialized = true;
				break;
			}

			_Destroy();

			if ( config.preferGPU )
			{
				config.preferGPU = false;
				continue;
			}
			if ( config.codec != EVideoCodec::H264 )
			{
				config.codec = EVideoCodec::H264;
				continue;
			}
			if ( config.format != EVideoFormat::YUV_420P )
			{
				config.format = EVideoFormat::YUV_420P;
				continue;
			}
			break;
		}
			
		CHECK_ERR( initialized );

		_fps			= cfg.fps;
		_frameCounter	= 0;
		_videoFile		= filename;
		
		FG_LOGD( "Begin recording to temporary: '"s << _tempFile << "', resulting: '" << _videoFile << "'" );
		return true;
	}
	
/*
=================================================
	_GetEncoderInfo
----
	add list of hardware accelerated encoders
	https://trac.ffmpeg.org/wiki/HWAccelIntro
=================================================
*/
	FFmpegVideoRecorder::CodecInfo  FFmpegVideoRecorder::_GetEncoderInfo (const Config &cfg)
	{
		CodecInfo	result;
		
		BEGIN_ENUM_CHECKS();
		switch ( cfg.codec )
		{
			case EVideoCodec::H264 :
			{
				result.format = "h264";

				if ( cfg.preferGPU )
				{
					result.codecs.push_back( "h264_nvenc" );	// nvidia
					result.codecs.push_back( "h264_qsv" );		// intel
				}
				break;
			}
			case EVideoCodec::VP8 :
			{
				result.format = "vp8";
				break;
			}
			case EVideoCodec::VP9 :
			{
				result.format = "vp9";
				break;
			}

			/*
				hevc_nvenc
			*/
		}
		END_ENUM_CHECKS();

		return result;
	}
	
/*
=================================================
	_SetOptions
=================================================
*/
	void  FFmpegVideoRecorder::_SetOptions (INOUT AVDictionary **dict, const Config &cfg) const
	{
		StaticString<64>	preset;
		StringView			codec_name {_codec->name};

		if ( codec_name == "h264" )
		{
			BEGIN_ENUM_CHECKS();
			switch ( cfg.preset )
			{
				case EVideoPreset::UltraFast :	preset = "ultrafast";	break;
				case EVideoPreset::SuperFast :	preset = "superfast";	break;
				case EVideoPreset::VeryFast :	preset = "veryfast";	break;
				case EVideoPreset::Faster :		preset = "faster";		break;
				case EVideoPreset::Fast :		preset = "fast";		break;
				case EVideoPreset::Medium :		preset = "medium";		break;
				case EVideoPreset::Slow :		preset = "slow";		break;
				case EVideoPreset::Slower :		preset = "slower";		break;
				case EVideoPreset::VerySlow :	preset = "veryslow";	break;
			}
			END_ENUM_CHECKS();
		}
		else
		if ( codec_name == "h264_nvec" )
		{
			if ( cfg.preset <= EVideoPreset::Fast )
				preset = "fast";
			else
			if ( cfg.preset >= EVideoPreset::Slow )
				preset = "slow";
			else
				preset = "medium";
		}

		if ( preset.size() )
			ffmpeg.av_dict_set( INOUT dict, "preset", preset.c_str(), 0 );
	}
	
/*
=================================================
	_Init
=================================================
*/
	bool  FFmpegVideoRecorder::_Init (const Config &cfg)
	{
		// create codec
		{
			const CodecInfo	info = _GetEncoderInfo( cfg );

			_format = ffmpeg.av_guess_format( info.format.c_str(), null, null );
			CHECK_ERR( _format );
		
			FF_CHECK( ffmpeg.avformat_alloc_output_context2( OUT &_formatCtx, _format, null, _tempFile.c_str() ));

			for (auto& codec_name : info.codecs)
			{
				if ( _codec = ffmpeg.avcodec_find_encoder_by_name( codec_name.c_str() ); _codec )
					break;
			}

			if ( not _codec )
				_codec = ffmpeg.avcodec_find_encoder( _format->video_codec );

			CHECK_ERR( _codec );
			FG_LOGD( "Used codec: "s << _codec->name );
		
			_codecCtx = ffmpeg.avcodec_alloc_context3( _codec );
			CHECK_ERR( _codecCtx );
		}

		// create video stream
		{
			_videoStream = ffmpeg.avformat_new_stream( _formatCtx, _codec );
			CHECK_ERR( _videoStream );
		
			_videoStream->codecpar->codec_id	= _format->video_codec;
			_videoStream->codecpar->codec_type	= AVMEDIA_TYPE_VIDEO;
			_videoStream->codecpar->width		= int(cfg.size.x);
			_videoStream->codecpar->height		= int(cfg.size.y);
			_videoStream->codecpar->format		= EnumCast( cfg.format );
			_videoStream->codecpar->bit_rate	= int64_t(cfg.bitrate);
			_videoStream->time_base				= { 1, int(cfg.fps) };
		
			FF_CALL( ffmpeg.avcodec_parameters_to_context( _codecCtx, _videoStream->codecpar ));

			_codecCtx->time_base	= { 1, int(cfg.fps) };
			_codecCtx->max_b_frames	= 1;
			_codecCtx->gop_size		= 10;

			if ( _formatCtx->oformat->flags & AVFMT_GLOBALHEADER )
				_codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		
			FF_CALL( ffmpeg.avcodec_parameters_from_context( _videoStream->codecpar, _codecCtx ));
		}

		// create output file
		{
			AVDictionary*	codec_options	= null;
			_SetOptions( INOUT &codec_options, cfg );

			int	err = ffmpeg.avcodec_open2( _codecCtx, _codec, INOUT &codec_options );
			ffmpeg.av_dict_free( &codec_options );
			FF_CHECK( err );
		
			if ( not (_format->flags & AVFMT_NOFILE) )
				FF_CHECK( ffmpeg.avio_open( OUT &_formatCtx->pb, _tempFile.c_str(), AVIO_FLAG_WRITE ));
		
			FF_CHECK( ffmpeg.avformat_write_header( _formatCtx, null ));
		
			ffmpeg.av_dump_format( _formatCtx, 0, _tempFile.c_str(), 1 );

			_videoFrame			= ffmpeg.av_frame_alloc();
			_videoFrame->format	= _codecCtx->pix_fmt;
			_videoFrame->width	= int(cfg.size.x);
			_videoFrame->height	= int(cfg.size.y);
			FF_CHECK( ffmpeg.av_frame_get_buffer( _videoFrame, 32 ));
		}

		// create scaler
		{
			_swsCtx = ffmpeg.sws_getContext( int(cfg.size.x), int(cfg.size.y), AV_PIX_FMT_RGBA, _codecCtx->width, _codecCtx->height, _codecCtx->pix_fmt, SWS_BICUBIC, 0, 0, 0 );
			CHECK_ERR( _swsCtx );
		}

		return true;
	}
	
/*
=================================================
	AddFrame
=================================================
*/
	bool  FFmpegVideoRecorder::AddFrame (const ImageView &view)
	{
		CHECK_ERR( _codecCtx and _formatCtx and _videoFrame );
		CHECK_ERR( view.Parts().size() == 1 );
		CHECK_ERR( int(view.Dimension().x) == _codecCtx->width );
		CHECK_ERR( int(view.Dimension().y) == _codecCtx->height );
		CHECK_ERR( view.Dimension().z == 1 );
		CHECK_ERR( view.Format() == EPixelFormat::RGBA8_UNorm );

		const int		src_stride [1]	= { int(view.RowPitch()) };
		const uint8_t*	data [1]		= { view.data() };

		ffmpeg.sws_scale( _swsCtx, data, src_stride, 0, _codecCtx->height, _videoFrame->data, _videoFrame->linesize );

		_videoFrame->pts = _frameCounter++;

		FF_CHECK( ffmpeg.avcodec_send_frame( _codecCtx, _videoFrame ));

		AVPacket&	packet = _videoPacket;
		for (int err = 0; err >= 0;)
		{
			err = ffmpeg.avcodec_receive_packet( _codecCtx, &packet );
			
			if ( err == AVERROR(EAGAIN) or err == AVERROR_EOF )
				return true;
			
			FF_CHECK( err );

			packet.flags |= AV_PKT_FLAG_KEY;
			FF_CALL( ffmpeg.av_interleaved_write_frame( _formatCtx, &packet ));
			ffmpeg.av_packet_unref( &packet );
		}
		return true;
	}
	
/*
=================================================
	_Finish
=================================================
*/
	bool  FFmpegVideoRecorder::_Finish ()
	{
		CHECK_ERR( _codecCtx and _formatCtx );
		
		AVPacket&	packet = _videoPacket;
			
		FF_CHECK( ffmpeg.avcodec_send_frame( _codecCtx, null ));

		for (int err = 0; err >= 0;)
		{
			err = ffmpeg.avcodec_receive_packet( _codecCtx, &packet );

			if ( err == AVERROR(EAGAIN) or err == AVERROR_EOF )
				break;

			FF_CHECK( err );
				
			FF_CALL( ffmpeg.av_interleaved_write_frame( _formatCtx, &packet ));
			ffmpeg.av_packet_unref( &packet );
		}

		FF_CALL( ffmpeg.av_write_trailer( _formatCtx ));
		return true;
	}
	
/*
=================================================
	End
=================================================
*/
	bool  FFmpegVideoRecorder::End ()
	{
		CHECK( _Finish() );
		
		FG_LOGD( "End recording to: '"s << _tempFile << "', start remuxing to: '" << _videoFile << "'" );

		_Destroy();

		return _Remux();
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void  FFmpegVideoRecorder::_Destroy ()
	{
		if ( _formatCtx and _format and not (_format->flags & AVFMT_NOFILE) )
			FF_CALL( ffmpeg.avio_closep( &_formatCtx->pb ));

		if ( _formatCtx )
			ffmpeg.avformat_free_context( _formatCtx );

		if ( _videoFrame )
			ffmpeg.av_frame_free( &_videoFrame );
		
		if ( _codecCtx )
			ffmpeg.avcodec_free_context( &_codecCtx );
		
		if ( _swsCtx )
			ffmpeg.sws_freeContext( _swsCtx );
		
		_format			= null;
		_formatCtx		= null;
		_videoFrame		= null;
		_codec			= null;
		_codecCtx		= null;
		_swsCtx			= null;
		_frameCounter	= 0;
		_fps			= 0;
	}
	
/*
=================================================
	_Remux
=================================================
*/
	bool  FFmpegVideoRecorder::_Remux ()
	{
		CHECK_ERR( _tempFile.size() and _videoFile.size() );

		AVFormatContext*	ifmt_ctx		= null;
		AVFormatContext*	ofmt_ctx		= null;
		int *				stream_mapping	= null;
		
		const auto	Remux = [&] () -> bool
		{
			AVOutputFormat *ofmt = null;

			FF_CHECK( ffmpeg.avformat_open_input( OUT &ifmt_ctx, _tempFile.c_str(), 0, 0 ));
			FF_CHECK( ffmpeg.avformat_find_stream_info( ifmt_ctx, 0 ));
			ffmpeg.av_dump_format( ifmt_ctx, 0, _tempFile.c_str(), 0 );

			FF_CHECK( ffmpeg.avformat_alloc_output_context2( OUT &ofmt_ctx, null, null, _videoFile.c_str() ));

			int stream_mapping_size	= ifmt_ctx->nb_streams;
			stream_mapping			= Cast<int>( ffmpeg.av_mallocz_array( stream_mapping_size, sizeof(*stream_mapping) ));
			CHECK_ERR( stream_mapping );

			ofmt = ofmt_ctx->oformat;
			
			int stream_index = 0;
			for (uint i = 0; i < ifmt_ctx->nb_streams; ++i)
			{
				AVStream *			out_stream	= null;
				AVStream *			in_stream	= ifmt_ctx->streams[i];
				AVCodecParameters*	in_codecpar	= in_stream->codecpar;

				if ( in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO   and
					 in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO   and
					 in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE )
				{
					stream_mapping[i] = -1;
					continue;
				}

				stream_mapping[i] = stream_index++;

				out_stream = ffmpeg.avformat_new_stream( ofmt_ctx, null );
				CHECK_ERR( out_stream );

				FF_CHECK( ffmpeg.avcodec_parameters_copy( out_stream->codecpar, in_codecpar ));
				out_stream->codecpar->codec_tag = 0;
			}

			ffmpeg.av_dump_format( ofmt_ctx, 0, _videoFile.c_str(), 1 );

			if ( not (ofmt_ctx->oformat->flags & AVFMT_NOFILE) )
				FF_CHECK( ffmpeg.avio_open( &ofmt_ctx->pb, _videoFile.c_str(), AVIO_FLAG_WRITE ));
	
			FF_CHECK( ffmpeg.avformat_write_header( ofmt_ctx, null ));
			
			AVPacket	pkt = {};
			int64_t		ts	= 0;

			for (;;)
			{
				AVStream*	in_stream	= null;
				AVStream*	out_stream	= null;

				if ( ffmpeg.av_read_frame( ifmt_ctx, OUT &pkt ) < 0 )
					break;
				
				in_stream  = ifmt_ctx->streams[ pkt.stream_index ];
				if ( pkt.stream_index >= stream_mapping_size or
					 stream_mapping[ pkt.stream_index ] < 0 )
				{
					ffmpeg.av_packet_unref( &pkt );
					continue;
				}

				pkt.stream_index	= stream_mapping[ pkt.stream_index ];
				out_stream			= ofmt_ctx->streams[ pkt.stream_index ];
				
				pkt.pts		= ts; //av_rescale_q_rnd( pkt.pts, in_stream->time_base, out_stream->time_base, AVRounding(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX) );
				pkt.dts		= ts; //av_rescale_q_rnd( pkt.dts, in_stream->time_base, out_stream->time_base, AVRounding(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX) );
				pkt.duration= ffmpeg.av_rescale_q( pkt.duration, in_stream->time_base, out_stream->time_base );
				pkt.pos		= -1;

				ts += pkt.duration;

				int err = ffmpeg.av_interleaved_write_frame( ofmt_ctx, &pkt );
				if ( err < 0 )
				{
					FF_CALL( err );
					break;
				}

				ffmpeg.av_packet_unref( &pkt );
			}
	
			FF_CALL( ffmpeg.av_write_trailer( ofmt_ctx ));
			return true;
		};

		bool	res = Remux();

		if ( ifmt_ctx )
			ffmpeg.avformat_close_input( &ifmt_ctx );
	
		if ( ofmt_ctx and not (ofmt_ctx->oformat->flags & AVFMT_NOFILE) )
			ffmpeg.avio_closep( &ofmt_ctx->pb );
	
		if ( ofmt_ctx )
			ffmpeg.avformat_free_context( ofmt_ctx );
		
		ffmpeg.av_freep( &stream_mapping );

		std::error_code	err;
		CHECK( FS::remove( FS::path{_tempFile}, OUT err ));
		
		FG_LOGD( "End remuxing: '"s << _videoFile << "'" );

		_tempFile.clear();
		_videoFile.clear();

		return res;
	}


}	// FG

#endif	// FG_ENABLE_FFMPEG
