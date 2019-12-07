// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	based on code from:
	https://stackoverflow.com/questions/46444474/c-ffmpeg-create-mp4-file
	https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/encode_video.c
	https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/remuxing.c
*/

#ifdef FG_ENABLE_FFMPEG

#include "video/FFmpegRecorder.h"

#ifdef FG_STD_FILESYSTEM
#	include <filesystem>
	namespace FS = std::filesystem;
#endif

namespace
{
	using namespace FG;

	bool FFCheckError (int err, const char *ffcall, const char *func, const char *file, int line)
	{
		if ( err == 0 )
			return true;
		
		/*switch ( err )
		{
			case AVERROR(EPERM) :
			case AVERROR(ENOENT) :
			case AVERROR(ESRCH) :
			case AVERROR(EINTR) :
			case AVERROR(EIO) :
			case AVERROR(ENXIO) :
			case AVERROR(E2BIG) :
			case AVERROR(ENOEXEC) :
			case AVERROR(EBADF) :
			case AVERROR(ECHILD) :
			case AVERROR(EAGAIN) :
			case AVERROR(ENOMEM) :
			case AVERROR(EACCES) :
			case AVERROR(EFAULT) :
			case AVERROR(EBUSY) :
			case AVERROR(EEXIST) :
			case AVERROR(EXDEV) :
			case AVERROR(ENODEV) :
			case AVERROR(ENOTDIR) :
			case AVERROR(EISDIR) :
			case AVERROR(ENFILE) :
			case AVERROR(EMFILE) :
			case AVERROR(ENOTTY) :
			case AVERROR(EFBIG) :
			case AVERROR(ENOSPC) :
			case AVERROR(ESPIPE) :
			case AVERROR(EROFS) :
			case AVERROR(EMLINK) :
			case AVERROR(EPIPE) :
			case AVERROR(EDOM) :
			case AVERROR(EDEADLK) :
			case AVERROR(ENAMETOOLONG) :
			case AVERROR(ENOLCK) :
			case AVERROR(ENOSYS) :
			case AVERROR(ENOTEMPTY) :
			case AVERROR_BSF_NOT_FOUND :
			case AVERROR_BUG :
			case AVERROR_BUFFER_TOO_SMALL :
			case AVERROR_DECODER_NOT_FOUND :
			case AVERROR_DEMUXER_NOT_FOUND :
			case AVERROR_ENCODER_NOT_FOUND :
			case AVERROR_EOF :
			case AVERROR_EXIT :
			case AVERROR_EXTERNAL :
			case AVERROR_FILTER_NOT_FOUND :
			case AVERROR_INVALIDDATA :
			case AVERROR_MUXER_NOT_FOUND :
			case AVERROR_OPTION_NOT_FOUND :
			case AVERROR_PATCHWELCOME :
			case AVERROR_PROTOCOL_NOT_FOUND :
			case AVERROR_STREAM_NOT_FOUND :
			case AVERROR_BUG2 :
			case AVERROR_UNKNOWN :
			case AVERROR_EXPERIMENTAL :
			case AVERROR_INPUT_CHANGED :
			case AVERROR_OUTPUT_CHANGED :
			case AVERROR_HTTP_BAD_REQUEST :
			case AVERROR_HTTP_UNAUTHORIZED :
			case AVERROR_HTTP_FORBIDDEN :
			case AVERROR_HTTP_NOT_FOUND :
			case AVERROR_HTTP_OTHER_4XX :
			case AVERROR_HTTP_SERVER_ERROR :
				break;
			default :
				ASSERT(!"unknown error code");
				break;
		}*/

		String	msg{ "FFmpeg error: " };
		char	buf [AV_ERROR_MAX_STRING_SIZE] = "";

		av_strerror( err, OUT buf, sizeof(buf) );

		msg = msg + buf + ", in " + ffcall + ", function: " + func;

		FG_LOGE( msg, file, line );
		return false;
	}
}

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


namespace FG
{
namespace
{
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


/*
=================================================
	constructor
=================================================
*/
	FFmpegVideoRecorder::FFmpegVideoRecorder () :
		_videoPacket{}
	{
		av_init_packet( &_videoPacket );
	}
	
/*
=================================================
	destructor
=================================================
*/
	FFmpegVideoRecorder::~FFmpegVideoRecorder ()
	{}
	
/*
=================================================
	Begin
=================================================
*/
	bool  FFmpegVideoRecorder::Begin (const uint2 &size, uint fps, uint bitrateInKbps, EVideoFormat fmt, StringView filename)
	{
		CHECK_ERR( not _formatCtx and not _videoFrame and not _videoStream );
		
		#ifdef FG_STD_FILESYSTEM
			_tempFile = FS::path{filename}.replace_filename("tmp").replace_extension("h264").string();
		#else
			_tempFile = "tmp.h264";
		#endif

		_format = av_guess_format( null, _tempFile.c_str(), null );
		CHECK_ERR( _format );
		
		FF_CHECK( avformat_alloc_output_context2( OUT &_formatCtx, _format, null, _tempFile.c_str() ));

		_codec = avcodec_find_encoder( _format->video_codec );
		CHECK_ERR( _codec );
		
		_videoStream = avformat_new_stream( _formatCtx, _codec );
		CHECK_ERR( _videoStream );
		
		_codecCtx = avcodec_alloc_context3( _codec );
		CHECK_ERR( _codecCtx );
		
		_videoStream->codecpar->codec_id	= _format->video_codec;
		_videoStream->codecpar->codec_type	= AVMEDIA_TYPE_VIDEO;
		_videoStream->codecpar->width		= int(size.x);
		_videoStream->codecpar->height		= int(size.y);
		_videoStream->codecpar->format		= EnumCast( fmt );
		_videoStream->codecpar->bit_rate	= int64_t(bitrateInKbps) << 10;
		_videoStream->time_base				= { 1, int(fps) };
		
		avcodec_parameters_to_context( _codecCtx, _videoStream->codecpar );
		_codecCtx->time_base	= { 1, int(fps) };
		_codecCtx->max_b_frames	= 1;
		_codecCtx->gop_size		= 10;

		if ( _videoStream->codecpar->codec_id == AV_CODEC_ID_H264 )
			av_opt_set( _codecCtx, "preset", "ultrafast", 0 );

		if ( _formatCtx->oformat->flags & AVFMT_GLOBALHEADER )
			_codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		
		avcodec_parameters_from_context( _videoStream->codecpar, _codecCtx );
		
		FF_CHECK( avcodec_open2( _codecCtx, _codec, null ));
		
		if ( not (_format->flags & AVFMT_NOFILE) )
			FF_CHECK( avio_open( OUT &_formatCtx->pb, _tempFile.c_str(), AVIO_FLAG_WRITE ));
		
		FF_CHECK( avformat_write_header( _formatCtx, null ));
		
		av_dump_format( _formatCtx, 0, _tempFile.c_str(), 1 );

		_videoFrame			= av_frame_alloc();
		_videoFrame->format	= _codecCtx->pix_fmt;
		_videoFrame->width	= int(size.x);
		_videoFrame->height	= int(size.y);
		FF_CHECK( av_frame_get_buffer( _videoFrame, 32 ));

		_swsCtx = sws_getContext( int(size.x), int(size.y), AV_PIX_FMT_RGBA, int(size.x), int(size.y), _codecCtx->pix_fmt, SWS_BICUBIC, 0, 0, 0 );
		CHECK_ERR( _swsCtx );

		_fps			= fps;
		_frameCounter	= 0;
		_videoFile		= filename;
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

		sws_scale( _swsCtx, data, src_stride, 0, _codecCtx->height, _videoFrame->data, _videoFrame->linesize );

		_videoFrame->pts = _frameCounter++;

		FF_CHECK( avcodec_send_frame( _codecCtx, _videoFrame ));

		AVPacket&	packet = _videoPacket;
		for (int err = 0; err >= 0;)
		{
			err = avcodec_receive_packet( _codecCtx, &packet );
			
			if ( err == AVERROR(EAGAIN) or err == AVERROR_EOF )
				return true;
			
			FF_CHECK( err );

			packet.flags |= AV_PKT_FLAG_KEY;
			FF_CALL( av_interleaved_write_frame( _formatCtx, &packet ));
			av_packet_unref( &packet );
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
			
		FF_CHECK( avcodec_send_frame( _codecCtx, null ));

		for (int err = 0; err >= 0;)
		{
			err = avcodec_receive_packet( _codecCtx, &packet );

			if ( err == AVERROR(EAGAIN) or err == AVERROR_EOF )
				break;

			FF_CHECK( err );
				
			FF_CALL( av_interleaved_write_frame( _formatCtx, &packet ));
			av_packet_unref( &packet );
		}

		FF_CALL( av_write_trailer( _formatCtx ));
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
			FF_CALL( avio_closep( &_formatCtx->pb ));

		if ( _formatCtx )
			avformat_free_context( _formatCtx );

		if ( _videoFrame )
			av_frame_free( &_videoFrame );
		
		if ( _codecCtx )
			avcodec_free_context( &_codecCtx );
		
		if ( _swsCtx )
			sws_freeContext( _swsCtx );
		
		_format		= null;
		_formatCtx	= null;
		_videoFrame	= null;
		_codec		= null;
		_codecCtx	= null;
		_swsCtx		= null;
		_frameCounter= 0;
		_fps		= 0;
	}
	
/*
=================================================
	_Remux
=================================================
*/
	bool  FFmpegVideoRecorder::_Remux ()
	{
		CHECK_ERR( not _videoFile.empty() );

		AVFormatContext*	ifmt_ctx		= null;
		AVFormatContext*	ofmt_ctx		= null;
		int *				stream_mapping	= null;
		
		const auto	Remux = [&] () -> bool
		{
			AVOutputFormat *ofmt = null;

			FF_CHECK( avformat_open_input( OUT &ifmt_ctx, _tempFile.c_str(), 0, 0 ));
			FF_CHECK( avformat_find_stream_info( ifmt_ctx, 0 ));
			av_dump_format( ifmt_ctx, 0, _tempFile.c_str(), 0 );

			FF_CHECK( avformat_alloc_output_context2( OUT &ofmt_ctx, null, null, _videoFile.c_str() ));

			int stream_mapping_size	= ifmt_ctx->nb_streams;
			stream_mapping			= Cast<int>( av_mallocz_array( stream_mapping_size, sizeof(*stream_mapping) ));
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

				out_stream = avformat_new_stream( ofmt_ctx, null );
				CHECK_ERR( out_stream );

				FF_CHECK( avcodec_parameters_copy( out_stream->codecpar, in_codecpar ));
				out_stream->codecpar->codec_tag = 0;
			}

			av_dump_format( ofmt_ctx, 0, _videoFile.c_str(), 1 );

			if ( not (ofmt_ctx->oformat->flags & AVFMT_NOFILE) )
				FF_CHECK( avio_open( &ofmt_ctx->pb, _videoFile.c_str(), AVIO_FLAG_WRITE ));
	
			FF_CHECK( avformat_write_header( ofmt_ctx, null ));
			
			AVPacket	pkt = {};
			int64_t		ts	= 0;

			for (;;)
			{
				AVStream*	in_stream	= null;
				AVStream*	out_stream	= null;

				if ( av_read_frame( ifmt_ctx, OUT &pkt ) < 0 )
					break;
				
				in_stream  = ifmt_ctx->streams[ pkt.stream_index ];
				if ( pkt.stream_index >= stream_mapping_size or
					 stream_mapping[ pkt.stream_index ] < 0 )
				{
					av_packet_unref( &pkt );
					continue;
				}

				pkt.stream_index	= stream_mapping[ pkt.stream_index ];
				out_stream			= ofmt_ctx->streams[ pkt.stream_index ];
				
				pkt.pts		= ts; //av_rescale_q_rnd( pkt.pts, in_stream->time_base, out_stream->time_base, AVRounding(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX) );
				pkt.dts		= ts; //av_rescale_q_rnd( pkt.dts, in_stream->time_base, out_stream->time_base, AVRounding(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX) );
				pkt.duration= av_rescale_q( pkt.duration, in_stream->time_base, out_stream->time_base );
				pkt.pos		= -1;

				ts += pkt.duration;

				int err = av_interleaved_write_frame( ofmt_ctx, &pkt );
				if ( err < 0 )
				{
					FF_CALL( err );
					break;
				}

				av_packet_unref( &pkt );
			}
	
			FF_CALL( av_write_trailer( ofmt_ctx ));
			return true;
		};

		bool	res = Remux();

		if ( ifmt_ctx )
			avformat_close_input( &ifmt_ctx );
	
		if ( ofmt_ctx and not (ofmt_ctx->oformat->flags & AVFMT_NOFILE) )
			avio_closep( &ofmt_ctx->pb );
	
		if ( ofmt_ctx )
			avformat_free_context( ofmt_ctx );
		
		av_freep( &stream_mapping );

		std::error_code	err;
		CHECK( FS::remove( FS::path{_tempFile}, OUT err ));

		return res;
	}


}	// FG

#endif	// FG_ENABLE_FFMPEG
