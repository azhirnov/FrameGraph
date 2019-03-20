// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
		
		ImageID			src_image		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{src_dim.x, src_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																				EImageUsage::Transfer }, Default, "SrcImage" );
		ImageID			dst_image		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{dst_dim.x, dst_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
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
		
		CommandBuffer	cmd1 = _frameGraph->Begin( CommandBufferDesc{} );
		CommandBuffer	cmd2 = _frameGraph->Begin( CommandBufferDesc{}, {cmd1} );
		CHECK_ERR( cmd1 and cmd2 );

		// thread 1
		{
			uint2	dim			{ src_dim.x, src_dim.y/2 };
			auto	data		= ArrayView{ src_data.data(), src_data.size()/2 };

			Task	t_fill		= cmd1->AddTask( ClearColorImage{}.SetImage( dst_image ).AddRange( 0_mipmap, 1, 0_layer, 1 ).Clear(RGBA32f{ 1.0f }) );
			Task	t_update	= cmd1->AddTask( UpdateImage{}.SetImage( src_image ).SetData( data, dim ) );
			Task	t_copy		= cmd1->AddTask( CopyImage{}.From( src_image ).To( dst_image ).AddRegion( {}, int2(), {}, img_offset, dim ).DependsOn( t_update, t_fill ) );
			FG_UNUSED( t_copy );

			CHECK_ERR( _frameGraph->Execute( cmd1 ));
		}
		
		// thread 1 again
		{
			uint2	dim			{ src_dim.x, src_dim.y/2 };
			int2	offset		{ 0, int(src_dim.y/2) };
			auto	data		= ArrayView{ src_data.data() + src_data.size()/2, src_data.size()/2 };

			Task	t_update	= cmd2->AddTask( UpdateImage{}.SetImage( src_image, offset ).SetData( data, dim ) );
			Task	t_copy		= cmd2->AddTask( CopyImage{}.From( src_image ).To( dst_image ).AddRegion( {}, offset, {}, offset + img_offset, dim ).DependsOn( t_update ) );
			Task	t_read		= cmd2->AddTask( ReadImage{}.SetImage( dst_image, int2(), dst_dim ).SetCallback( OnLoaded ).DependsOn( t_copy ) );
			FG_UNUSED( t_read );
		
			CHECK_ERR( _frameGraph->Execute( cmd2 ));
		}

		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( not cb_was_called );
		
		CHECK_ERR( _frameGraph->WaitIdle() );

		CHECK_ERR( cb_was_called );
		CHECK_ERR( data_is_correct );

		DeleteResources( src_image, dst_image );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG
