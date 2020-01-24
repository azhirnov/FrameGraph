// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_FFMPEG

#include "stl/Common.h"

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
#	define FG_FFMPEG_AVCODEC_FUNCS( _builder_ ) \
		_builder_( avcodec_get_hw_config ) \
		_builder_( av_codec_iterate ) \
		_builder_( avcodec_version ) \
		_builder_( avcodec_configuration ) \
		_builder_( avcodec_license ) \
		_builder_( avcodec_alloc_context3 ) \
		_builder_( avcodec_free_context ) \
		_builder_( avcodec_get_context_defaults3 ) \
		_builder_( avcodec_get_class ) \
		_builder_( avcodec_get_frame_class ) \
		_builder_( avcodec_get_subtitle_rect_class ) \
		_builder_( avcodec_parameters_alloc ) \
		_builder_( avcodec_parameters_free ) \
		_builder_( avcodec_parameters_copy ) \
		_builder_( avcodec_parameters_from_context ) \
		_builder_( avcodec_parameters_to_context ) \
		_builder_( avcodec_open2 ) \
		_builder_( avcodec_close ) \
		_builder_( avsubtitle_free ) \
		_builder_( av_packet_alloc ) \
		_builder_( av_packet_clone ) \
		_builder_( av_packet_free ) \
		_builder_( av_init_packet ) \
		_builder_( av_new_packet ) \
		_builder_( av_shrink_packet ) \
		_builder_( av_grow_packet ) \
		_builder_( av_packet_from_data ) \
		_builder_( av_packet_new_side_data ) \
		_builder_( av_packet_add_side_data ) \
		_builder_( av_packet_shrink_side_data ) \
		_builder_( av_packet_get_side_data ) \
		_builder_( av_packet_side_data_name ) \
		_builder_( av_packet_pack_dictionary ) \
		_builder_( av_packet_unpack_dictionary ) \
		_builder_( av_packet_free_side_data ) \
		_builder_( av_packet_ref ) \
		_builder_( av_packet_unref ) \
		_builder_( av_packet_move_ref ) \
		_builder_( av_packet_copy_props ) \
		_builder_( av_packet_make_refcounted ) \
		_builder_( av_packet_make_writable ) \
		_builder_( av_packet_rescale_ts ) \
		_builder_( avcodec_find_decoder ) \
		_builder_( avcodec_find_decoder_by_name ) \
		_builder_( avcodec_default_get_buffer2 ) \
		_builder_( avcodec_align_dimensions ) \
		_builder_( avcodec_align_dimensions2 ) \
		_builder_( avcodec_enum_to_chroma_pos ) \
		_builder_( avcodec_chroma_pos_to_enum ) \
		_builder_( avcodec_decode_subtitle2 ) \
		_builder_( avcodec_send_packet ) \
		_builder_( avcodec_receive_frame ) \
		_builder_( avcodec_send_frame ) \
		_builder_( avcodec_receive_packet ) \
		_builder_( avcodec_get_hw_frames_parameters ) \
		_builder_( av_parser_iterate ) \
		_builder_( av_parser_parse2 ) \
		_builder_( av_parser_change ) \
		_builder_( av_parser_close ) \
		_builder_( avcodec_find_encoder ) \
		_builder_( avcodec_find_encoder_by_name ) \
		_builder_( avcodec_encode_subtitle ) \
		_builder_( avcodec_pix_fmt_to_codec_tag ) \
		_builder_( avcodec_get_pix_fmt_loss ) \
		_builder_( avcodec_find_best_pix_fmt_of_list ) \
		_builder_( avcodec_find_best_pix_fmt_of_2 ) \
		_builder_( avcodec_default_get_format ) \
		_builder_( avcodec_string ) \
		_builder_( av_get_profile_name ) \
		_builder_( avcodec_profile_name ) \
		_builder_( avcodec_default_execute ) \
		_builder_( avcodec_default_execute2 ) \
		_builder_( avcodec_fill_audio_frame ) \
		_builder_( avcodec_flush_buffers ) \
		_builder_( av_get_bits_per_sample ) \
		_builder_( av_get_pcm_codec ) \
		_builder_( av_get_exact_bits_per_sample ) \
		_builder_( av_get_audio_frame_duration ) \
		_builder_( av_get_audio_frame_duration2 ) \
		_builder_( av_bsf_get_by_name ) \
		_builder_( av_bsf_iterate ) \
		_builder_( av_bsf_alloc ) \
		_builder_( av_bsf_init ) \
		_builder_( av_bsf_send_packet ) \
		_builder_( av_bsf_receive_packet ) \
		_builder_( av_bsf_flush ) \
		_builder_( av_bsf_free ) \
		_builder_( av_bsf_get_class ) \
		_builder_( av_bsf_list_alloc ) \
		_builder_( av_bsf_list_free ) \
		_builder_( av_bsf_list_append ) \
		_builder_( av_bsf_list_append2 ) \
		_builder_( av_bsf_list_finalize ) \
		_builder_( av_bsf_list_parse_str ) \
		_builder_( av_bsf_get_null_filter ) \
		_builder_( av_fast_padded_malloc ) \
		_builder_( av_fast_padded_mallocz ) \
		_builder_( av_xiphlacing ) \
		_builder_( avcodec_get_type ) \
		_builder_( avcodec_get_name ) \
		_builder_( avcodec_is_open ) \
		_builder_( av_codec_is_encoder ) \
		_builder_( av_codec_is_decoder ) \
		_builder_( avcodec_descriptor_get ) \
		_builder_( avcodec_descriptor_next ) \
		_builder_( avcodec_descriptor_get_by_name ) \
		_builder_( av_cpb_properties_alloc ) \

#	define FG_FFMPEG_AVFORMAT_FUNCS( _builder_ ) \
		_builder_( av_stream_get_parser ) \
		_builder_( av_stream_get_end_pts ) \
		_builder_( av_format_inject_global_side_data ) \
		_builder_( av_fmt_ctx_get_duration_estimation_method ) \
		_builder_( avformat_version ) \
		_builder_( avformat_configuration ) \
		_builder_( avformat_license ) \
		_builder_( avformat_network_init ) \
		_builder_( avformat_network_deinit ) \
		_builder_( av_muxer_iterate ) \
		_builder_( av_demuxer_iterate ) \
		_builder_( avformat_alloc_context ) \
		_builder_( avformat_free_context ) \
		_builder_( avformat_get_class ) \
		_builder_( avformat_new_stream ) \
		_builder_( av_stream_add_side_data ) \
		_builder_( av_stream_new_side_data ) \
		_builder_( av_stream_get_side_data ) \
		_builder_( av_new_program ) \
		_builder_( avformat_alloc_output_context2 ) \
		_builder_( av_find_input_format ) \
		_builder_( av_probe_input_format ) \
		_builder_( av_probe_input_format2 ) \
		_builder_( av_probe_input_format3 ) \
		_builder_( av_probe_input_buffer2 ) \
		_builder_( av_probe_input_buffer ) \
		_builder_( avformat_open_input ) \
		_builder_( avformat_find_stream_info ) \
		_builder_( av_find_program_from_stream ) \
		_builder_( av_program_add_stream_index ) \
		_builder_( av_find_best_stream ) \
		_builder_( av_read_frame ) \
		_builder_( av_seek_frame ) \
		_builder_( avformat_seek_file ) \
		_builder_( avformat_flush ) \
		_builder_( av_read_play ) \
		_builder_( av_read_pause ) \
		_builder_( avformat_close_input ) \
		_builder_( avformat_write_header ) \
		_builder_( avformat_init_output ) \
		_builder_( av_write_frame ) \
		_builder_( av_interleaved_write_frame ) \
		_builder_( av_write_uncoded_frame ) \
		_builder_( av_interleaved_write_uncoded_frame ) \
		_builder_( av_write_uncoded_frame_query ) \
		_builder_( av_write_trailer ) \
		_builder_( av_guess_format ) \
		_builder_( av_guess_codec ) \
		_builder_( av_get_output_timestamp ) \
		_builder_( av_hex_dump ) \
		_builder_( av_hex_dump_log ) \
		_builder_( av_pkt_dump2 ) \
		_builder_( av_pkt_dump_log2 ) \
		_builder_( av_codec_get_id ) \
		_builder_( av_codec_get_tag ) \
		_builder_( av_codec_get_tag2 ) \
		_builder_( av_find_default_stream_index ) \
		_builder_( av_index_search_timestamp ) \
		_builder_( av_add_index_entry ) \
		_builder_( av_url_split ) \
		_builder_( av_dump_format ) \
		_builder_( av_get_frame_filename2 ) \
		_builder_( av_get_frame_filename ) \
		_builder_( av_filename_number_test ) \
		_builder_( av_sdp_create ) \
		_builder_( av_match_ext ) \
		_builder_( avformat_query_codec ) \
		_builder_( avformat_get_riff_video_tags ) \
		_builder_( avformat_get_riff_audio_tags ) \
		_builder_( avformat_get_mov_video_tags ) \
		_builder_( avformat_get_mov_audio_tags ) \
		_builder_( av_guess_sample_aspect_ratio ) \
		_builder_( av_guess_frame_rate ) \
		_builder_( avformat_match_stream_specifier ) \
		_builder_( avformat_queue_attached_pictures ) \
		_builder_( avformat_transfer_internal_stream_timing_info ) \
		_builder_( av_stream_get_codec_timebase ) \
		_builder_( avio_find_protocol_name ) \
		_builder_( avio_check ) \
		_builder_( avpriv_io_move ) \
		_builder_( avpriv_io_delete ) \
		_builder_( avio_open_dir ) \
		_builder_( avio_read_dir ) \
		_builder_( avio_close_dir ) \
		_builder_( avio_free_directory_entry ) \
		_builder_( avio_alloc_context ) \
		_builder_( avio_context_free ) \
		_builder_( avio_w8 ) \
		_builder_( avio_write ) \
		_builder_( avio_wl64 ) \
		_builder_( avio_wb64 ) \
		_builder_( avio_wl32 ) \
		_builder_( avio_wb32 ) \
		_builder_( avio_wl24 ) \
		_builder_( avio_wb24 ) \
		_builder_( avio_wl16 ) \
		_builder_( avio_wb16 ) \
		_builder_( avio_put_str ) \
		_builder_( avio_put_str16le ) \
		_builder_( avio_put_str16be ) \
		_builder_( avio_write_marker ) \
		_builder_( avio_seek ) \
		_builder_( avio_skip ) \
		_builder_( avio_size ) \
		_builder_( avio_feof ) \
		_builder_( avio_printf ) \
		_builder_( avio_flush ) \
		_builder_( avio_read ) \
		_builder_( avio_read_partial ) \
		_builder_( avio_r8 ) \
		_builder_( avio_rl16 ) \
		_builder_( avio_rl24 ) \
		_builder_( avio_rl32 ) \
		_builder_( avio_rl64 ) \
		_builder_( avio_rb16 ) \
		_builder_( avio_rb24 ) \
		_builder_( avio_rb32 ) \
		_builder_( avio_rb64 ) \
		_builder_( avio_get_str ) \
		_builder_( avio_get_str16le ) \
		_builder_( avio_get_str16be ) \
		_builder_( avio_open ) \
		_builder_( avio_open2 ) \
		_builder_( avio_close ) \
		_builder_( avio_closep ) \
		_builder_( avio_open_dyn_buf ) \
		_builder_( avio_get_dyn_buf ) \
		_builder_( avio_close_dyn_buf ) \
		_builder_( avio_enum_protocols ) \
		_builder_( avio_pause ) \
		_builder_( avio_seek_time ) \
		_builder_( avio_read_to_bprint ) \
		_builder_( avio_accept ) \
		_builder_( avio_handshake ) \

#	define FG_FFMPEG_AVUTIL_FUNCS( _builder_ ) \
		_builder_( av_strerror ) \
		_builder_( av_dict_get ) \
		_builder_( av_dict_count ) \
		_builder_( av_dict_set ) \
		_builder_( av_dict_set_int ) \
		_builder_( av_dict_parse_string ) \
		_builder_( av_dict_copy ) \
		_builder_( av_dict_free ) \
		_builder_( av_dict_get_string ) \
		_builder_( av_get_colorspace_name ) \
		_builder_( av_frame_alloc ) \
		_builder_( av_frame_free ) \
		_builder_( av_frame_ref ) \
		_builder_( av_frame_clone ) \
		_builder_( av_frame_unref ) \
		_builder_( av_frame_move_ref ) \
		_builder_( av_frame_get_buffer ) \
		_builder_( av_frame_is_writable ) \
		_builder_( av_frame_make_writable ) \
		_builder_( av_frame_copy ) \
		_builder_( av_frame_copy_props ) \
		_builder_( av_frame_get_plane_buffer ) \
		_builder_( av_frame_new_side_data ) \
		_builder_( av_frame_new_side_data_from_buf ) \
		_builder_( av_frame_get_side_data ) \
		_builder_( av_frame_remove_side_data ) \
		_builder_( av_frame_apply_cropping ) \
		_builder_( av_frame_side_data_name ) \
		_builder_( av_malloc ) \
		_builder_( av_mallocz ) \
		_builder_( av_malloc_array ) \
		_builder_( av_mallocz_array ) \
		_builder_( av_calloc ) \
		_builder_( av_realloc ) \
		_builder_( av_reallocp ) \
		_builder_( av_realloc_f ) \
		_builder_( av_realloc_array ) \
		_builder_( av_reallocp_array ) \
		_builder_( av_fast_realloc ) \
		_builder_( av_fast_malloc ) \
		_builder_( av_fast_mallocz ) \
		_builder_( av_free ) \
		_builder_( av_freep ) \
		_builder_( av_strdup ) \
		_builder_( av_strndup ) \
		_builder_( av_memdup ) \
		_builder_( av_memcpy_backptr ) \
		_builder_( av_dynarray_add ) \
		_builder_( av_dynarray_add_nofree ) \
		_builder_( av_dynarray2_add ) \
		_builder_( av_max_alloc ) \
		_builder_( av_gcd ) \
		_builder_( av_rescale ) \
		_builder_( av_rescale_rnd ) \
		_builder_( av_rescale_q ) \
		_builder_( av_rescale_q_rnd ) \
		_builder_( av_compare_ts ) \
		_builder_( av_compare_mod ) \
		_builder_( av_rescale_delta ) \
		_builder_( av_add_stable ) \

#	define FG_FFMPEG_SWSCALE_FUNCS( _builder_ ) \
		_builder_( swscale_version ) \
		_builder_( swscale_configuration ) \
		_builder_( swscale_license ) \
		_builder_( sws_getCoefficients ) \
		_builder_( sws_isSupportedInput ) \
		_builder_( sws_isSupportedOutput ) \
		_builder_( sws_isSupportedEndiannessConversion ) \
		_builder_( sws_alloc_context ) \
		_builder_( sws_init_context ) \
		_builder_( sws_freeContext ) \
		_builder_( sws_getContext ) \
		_builder_( sws_scale ) \
		_builder_( sws_setColorspaceDetails ) \
		_builder_( sws_getColorspaceDetails ) \
		_builder_( sws_allocVec ) \
		_builder_( sws_getGaussianVec ) \
		_builder_( sws_scaleVec ) \
		_builder_( sws_normalizeVec ) \
		_builder_( sws_freeVec ) \
		_builder_( sws_getDefaultFilter ) \
		_builder_( sws_freeFilter ) \
		_builder_( sws_getCachedContext ) \
		_builder_( sws_convertPalette8ToPacked32 ) \
		_builder_( sws_convertPalette8ToPacked24 ) \
		_builder_( sws_get_class )


	//
	// ffmpeg loader
	//

	struct FFMpegLoader
	{
		#define FFMPEG_DECL_FN( _name_ )	static inline decltype(&::_name_)  _name_ = null;
		FG_FFMPEG_AVCODEC_FUNCS( FFMPEG_DECL_FN )
		FG_FFMPEG_AVFORMAT_FUNCS( FFMPEG_DECL_FN )
		FG_FFMPEG_AVUTIL_FUNCS( FFMPEG_DECL_FN )
		FG_FFMPEG_SWSCALE_FUNCS( FFMPEG_DECL_FN )
		#undef FFMPEG_DECL_FN

		static bool Load ();
		static void Unload ();
	};

}	// FG

#endif	// FG_ENABLE_FFMPEG
