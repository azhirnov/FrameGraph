// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This test affects:
		- ...
*/

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_CopyImage3 ()
	{
		const uint2		src_dim			= {64, 64};
		const uint2		dst_dim			= {128, 128};
		const int2		img_offset		= {16, 27};
		const BytesU	bpp				= 4_b;
		const BytesU	src_row_pitch	= src_dim.x * bpp;
		
		auto			frame_graph1	= _fgThreads[0];
		auto			frame_graph2	= _fgThreads[1];

		ImageID			src_image		= frame_graph1->CreateImage( ImageDesc{ EImage::Tex2D, uint3{src_dim.x, src_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																				EImageUsage::Transfer }, Default, "SrcImage" );
		ImageID			dst_image		= frame_graph1->CreateImage( ImageDesc{ EImage::Tex2D, uint3{dst_dim.x, dst_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																				EImageUsage::Transfer }, Default, "DstImage" );

		Array<uint8_t>	src_data;		src_data.resize( size_t(src_row_pitch * src_dim.y) );

		for (uint y = 0; y < src_dim.y; ++y)
		{
			for (uint x = 0; x < src_dim.x; ++x)
			{
				uint8_t*	ptr = &src_data[ size_t(x * bpp + y * src_row_pitch) ];

				ptr[0] = uint8_t(x);
				ptr[1] = uint8_t(y);
				ptr[2] = uint8_t(Max( x, y ));
				ptr[3] = 0;
			}
		}


		bool	cb_was_called	= false;
		bool	data_is_correct	= false;

		const auto	OnLoaded =	[bpp, src_dim, src_row_pitch, img_offset, &src_data, OUT &cb_was_called, OUT &data_is_correct] (const ImageView &imageData)
		{
			cb_was_called	= true;
			data_is_correct	= true;

			for (uint y = 0; y < src_dim.y; ++y)
			{
				ArrayView<uint8_t>	dst_row = imageData.GetRow( y + img_offset.y );

				for (uint x = 0; x < src_dim.x; ++x)
				{
					uint8_t const*	src		= &src_data[ size_t(x * bpp + y * src_row_pitch) ];
					uint8_t const*	dst		= &dst_row[ size_t((x + img_offset.x) * bpp) ];

					const bool		is_equal = (src[0] == dst[0] and
												src[1] == dst[1] and
												src[2] == dst[2] and
												src[3] == dst[3]);

					ASSERT( is_equal );
					data_is_correct &= is_equal;
				}
			}
		};
		
		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id, 3 );
		
		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));

		// thread 1
		{
			CHECK_ERR( frame_graph1->Begin( batch_id, 0, EQueueUsage::Graphics ));

			uint2	dim			{ src_dim.x, src_dim.y/2 };
			auto	data		= ArrayView{ src_data.data(), src_data.size()/2 };

			Task	t_update	= frame_graph1->AddTask( UpdateImage().SetImage( src_image ).SetData( data, dim ) );
			Task	t_copy		= frame_graph1->AddTask( CopyImage().From( src_image ).To( dst_image ).AddRegion( {}, int2(), {}, img_offset, dim ).DependsOn( t_update ) );
			FG_UNUSED( t_copy );

			CHECK_ERR( frame_graph1->Execute() );
		}

		// thread 2
		{
			CHECK_ERR( frame_graph2->Begin( batch_id, 1, EQueueUsage::Graphics ));
			
			uint2	dim			{ src_dim.x, src_dim.y/2 };
			int2	offset		{ 0, int(src_dim.y/2) };
			auto	data		= ArrayView{ src_data.data() + src_data.size()/2, src_data.size()/2 };

			Task	t_update	= frame_graph2->AddTask( UpdateImage().SetImage( src_image, offset ).SetData( data, dim ) );
			Task	t_copy		= frame_graph2->AddTask( CopyImage().From( src_image ).To( dst_image ).AddRegion( {}, offset, {}, offset + img_offset, dim ).DependsOn( t_update ) );
			Task	t_read		= frame_graph2->AddTask( ReadImage().SetImage( dst_image, int2(), dst_dim ).SetCallback( OnLoaded ).DependsOn( t_copy ) );
			FG_UNUSED( t_read );
		
			CHECK_ERR( frame_graph2->Execute() );
		}

		// thread 3 (unused)
		{
			CHECK_ERR( _fgInstance->SkipBatch( batch_id, 2 ));
		}

		CHECK_ERR( _fgInstance->EndFrame() );

		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( not cb_was_called );
		
		CHECK_ERR( _fgInstance->WaitIdle() );

		CHECK_ERR( cb_was_called );
		CHECK_ERR( data_is_correct );

		DeleteResources( src_image, dst_image );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG
