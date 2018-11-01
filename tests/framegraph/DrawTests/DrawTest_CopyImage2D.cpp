// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This test affects:
		- frame graph building and execution
		- tasks: UpdateImage, CopyImage, ReadImage
		- resources: image
		- staging buffers
		- memory managment
*/

#include "../FGApp.h"

namespace FG
{

	bool FGApp::_DrawTest_CopyImage2D ()
	{
		const uint2		src_dim			= {64, 64};
		const uint2		dst_dim			= {128, 128};
		const int2		img_offset		= {16, 27};
		const BytesU	bpp				= 4_b;
		const BytesU	src_row_pitch	= src_dim.x * bpp;

		ImageID			src_image		= _CreateImage2D( src_dim, EPixelFormat::RGBA8_UNorm, "SrcImage" );
		ImageID			dst_image		= _CreateImage2D( dst_dim, EPixelFormat::RGBA8_UNorm, "DstImage" );

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

		const auto	OnLoaded =	[bpp, src_dim, src_row_pitch, img_offset, &src_data, &dst_image, OUT &cb_was_called, OUT &data_is_correct] (const ImageView &imageData)
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
		
		CHECK( _frameGraphInst->Begin() );
		CHECK( _frameGraph->Begin() );

		Task	t_update	= _frameGraph->AddTask( UpdateImage().SetImage( src_image ).SetData( src_data, src_dim ) );
		Task	t_copy		= _frameGraph->AddTask( CopyImage().From( src_image ).To( dst_image ).AddRegion( {}, int2(), {}, img_offset, src_dim ).DependsOn( t_update ) );
		Task	t_read		= _frameGraph->AddTask( ReadImage().SetImage( dst_image, int2(), dst_dim ).SetCallback( OnLoaded ).DependsOn( t_copy ) );
		(void)(t_read);
		
		CHECK( _frameGraph->Compile() );
		CHECK( _frameGraphInst->Execute() );

		// after execution 'src_data' was copied to 'src_image', 'src_image' copied to 'dst_image', 'dst_image' copied to staging buffer...
		CHECK_ERR( not cb_was_called );
		
		// all staging buffers will be synchronized, all 'ReadImage' callbacks will be called.
		_frameGraphInst->WaitIdle();

		CHECK_ERR( cb_was_called );
		CHECK_ERR( data_is_correct );

		_DeleteResources( src_image, dst_image );

        FG_LOGI( "DrawTest_CopyImage2D - passed" );
		return true;
	}

}	// FG
