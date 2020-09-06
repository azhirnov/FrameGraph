// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef FG_ENABLE_VULKAN

#include "VLocalImage.h"
#include "VBarrierManager.h"
#include "framegraph/Public/FrameGraph.h"
#include "UnitTest_Common.h"
#include "DummyTask.h"


namespace FG
{
	class VImageUnitTest
	{
	public:
		using Barrier = VLocalImage::ImageAccess;

		static bool Create (VImage &img, const ImageDesc &desc)
		{
			img._desc	= desc;
			img._desc.Validate();

			img._defaultLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			return true;
		}

		static ArrayView<Barrier>  GetRWBarriers (const VLocalImage *img) {
			return img->_accessForReadWrite;
		}
	};

	using ImageState	= VLocalImage::ImageState;
	using ImageRange	= VLocalImage::ImageRange;
}	// FG


static void CheckLayers (const VImageUnitTest::Barrier &barrier, uint arrayLayers,
						 uint baseMipLevel, uint levelCount, uint baseArrayLayer, uint layerCount)
{
	uint	base_mip_level		= (barrier.range.begin / arrayLayers);
	uint	level_count			= (barrier.range.end - barrier.range.begin - 1) / arrayLayers + 1;
	uint	base_array_layer	= (barrier.range.begin % arrayLayers);
	uint	layer_count			= (barrier.range.end - barrier.range.begin - 1) % arrayLayers + 1;

	TEST( base_mip_level == baseMipLevel );
	TEST( level_count == levelCount );
	TEST( base_array_layer == baseArrayLayer );
	TEST( layer_count == layerCount );
}


static void VImage_Test1 ()
{
	VBarrierManager		barrier_mngr;
	
	const auto			tasks		= GenDummyTasks( 30 );
	auto				task_iter	= tasks.begin();

	VImage				global_image;
	VLocalImage			local_image;
	VLocalImage const*	img			= &local_image;

	TEST( VImageUnitTest::Create( global_image,
								  ImageDesc{}.SetDimension({ 64, 64 }).SetFormat( EPixelFormat::RGBA8_UNorm )
											.SetUsage( EImageUsage::ColorAttachment | EImageUsage::Transfer | EImageUsage::Storage | EImageUsage::Sampled )
											.SetMaxMipmaps( 11 )));

	TEST( local_image.Create( &global_image ));

	
	// pass 1
	{
		img->AddPendingState(ImageState{ EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								ImageRange{ 0_layer, 1, 0_mipmap, 1 },
								VK_IMAGE_ASPECT_COLOR_BIT, (task_iter++)->get() });

		img->CommitBarrier( barrier_mngr, null );

		auto	barriers = VImageUnitTest::GetRWBarriers( img );

		TEST( barriers.size() == 2 );

		TEST( barriers[0].range.begin == 0 );
		TEST( barriers[0].range.end == 1 );
		TEST( barriers[0].stages == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( barriers[0].access == VK_ACCESS_TRANSFER_WRITE_BIT );
		TEST( barriers[0].isReadable == false );
		TEST( barriers[0].isWritable == true );
		TEST( barriers[0].layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		TEST( barriers[0].index == ExeOrderIndex(1) );

		CheckLayers( barriers[0], img->ArrayLayers(), 0, 1, 0, img->ArrayLayers() );
		
		TEST( barriers[1].range.begin == 1 );
		TEST( barriers[1].range.end == 7 );
		TEST( barriers[1].stages == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
		TEST( barriers[1].access == 0 );
		TEST( barriers[1].isReadable == false );
		TEST( barriers[1].isWritable == false );
		TEST( barriers[1].layout == VK_IMAGE_LAYOUT_UNDEFINED );
		TEST( barriers[1].index == ExeOrderIndex::Initial );

		CheckLayers( barriers[1], img->ArrayLayers(), 1, img->MipmapLevels()-1, 0, img->ArrayLayers() );
	}
	
	local_image.ResetState( ExeOrderIndex::Final, barrier_mngr, null );

	local_image.Destroy();
	//global_image.Destroy();
}


static void VImage_Test2 ()
{
	VBarrierManager		barrier_mngr;
	
	const auto			tasks		= GenDummyTasks( 30 );
	auto				task_iter	= tasks.begin();
	
	VImage				global_image;
	VLocalImage			local_image;
	VLocalImage const*	img			= &local_image;

	TEST( VImageUnitTest::Create( global_image,
								  ImageDesc{}.SetDimension({ 64, 64 }).SetFormat( EPixelFormat::RGBA8_UNorm )
											.SetUsage( EImageUsage::ColorAttachment | EImageUsage::Transfer | EImageUsage::Storage | EImageUsage::Sampled )
											.SetMaxMipmaps( 11 ).SetArrayLayers( 8 )));

	TEST( local_image.Create( &global_image ));

	// pass 1
	{
		img->AddPendingState( ImageState{ EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
										  ImageRange{ 0_layer, 2, 0_mipmap, img->MipmapLevels() },
										  VK_IMAGE_ASPECT_COLOR_BIT, (task_iter++)->get() });

		img->CommitBarrier( barrier_mngr, null );

		auto	barriers = VImageUnitTest::GetRWBarriers( img );
		TEST( barriers.size() == 14 );

		for (size_t i = 0; i < barriers.size(); ++i)
		{
			const uint	j = uint(i/2);

			TEST( barriers[i].range.begin == j*8 );
			TEST( barriers[i].range.end == 2 + j*8 );
			TEST( barriers[i].stages == VK_PIPELINE_STAGE_TRANSFER_BIT );
			TEST( barriers[i].access == VK_ACCESS_TRANSFER_WRITE_BIT );
			TEST( barriers[i].isReadable == false );
			TEST( barriers[i].isWritable == true );
			TEST( barriers[i].layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
			TEST( barriers[i].index == ExeOrderIndex(1) );
		
			CheckLayers( barriers[i], img->ArrayLayers(), j, 1, 0, 2 );

			++i;
			TEST( barriers[i].range.begin == 2 + j*8 );
			TEST( barriers[i].range.end == 8 + j*8 );
			TEST( barriers[i].stages == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
			TEST( barriers[i].access == 0 );
			TEST( barriers[i].isReadable == false );
			TEST( barriers[i].isWritable == false );
			TEST( barriers[i].layout == VK_IMAGE_LAYOUT_UNDEFINED );
			TEST( barriers[i].index == ExeOrderIndex::Initial );

			CheckLayers( barriers[i], img->ArrayLayers(), j, 1, 2, img->ArrayLayers()-2 );
		}
	}
	
	// pass 2
	{
		img->AddPendingState( ImageState{ EResourceState::ShaderSample | EResourceState::_FragmentShader, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
										  ImageRange{ 0_layer, img->ArrayLayers(), 0_mipmap, 2 },
										  VK_IMAGE_ASPECT_COLOR_BIT, (task_iter++)->get() });

		img->CommitBarrier( barrier_mngr, null );

		auto	barriers = VImageUnitTest::GetRWBarriers( img );
		TEST( barriers.size() == 12 );
		
		for (size_t i = 0; i < 2; ++i)
		{
			TEST( barriers[i].range.begin == 8*i + 0 );
			TEST( barriers[i].range.end == 8*i + 8 );
			TEST( barriers[i].stages == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT );
			TEST( barriers[i].access == VK_ACCESS_SHADER_READ_BIT );
			TEST( barriers[i].isReadable == true );
			TEST( barriers[i].isWritable == false );
			TEST( barriers[i].layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			TEST( barriers[i].index == ExeOrderIndex(2) );

			CheckLayers( barriers[i], img->ArrayLayers(), uint(i), 1, 0, img->ArrayLayers() );
		}

		// from previous pass
		for (size_t i = 2; i < barriers.size(); ++i)
		{
			const uint	j = uint(i/2) + 1;

			TEST( barriers[i].range.begin == j*8 );
			TEST( barriers[i].range.end == 2 + j*8 );
			TEST( barriers[i].stages == VK_PIPELINE_STAGE_TRANSFER_BIT );
			TEST( barriers[i].access == VK_ACCESS_TRANSFER_WRITE_BIT );
			TEST( barriers[i].isReadable == false );
			TEST( barriers[i].isWritable == true );
			TEST( barriers[i].layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
			TEST( barriers[i].index == ExeOrderIndex(1) );
		
			CheckLayers( barriers[i], img->ArrayLayers(), j, 1, 0, 2 );

			++i;
			TEST( barriers[i].range.begin == 2 + j*8 );
			TEST( barriers[i].range.end == 8 + j*8 );
			TEST( barriers[i].stages == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
			TEST( barriers[i].access == 0 );
			TEST( barriers[i].isReadable == false );
			TEST( barriers[i].isWritable == false );
			TEST( barriers[i].layout == VK_IMAGE_LAYOUT_UNDEFINED );
			TEST( barriers[i].index == ExeOrderIndex::Initial );

			CheckLayers( barriers[i], img->ArrayLayers(), j, 1, 2, img->ArrayLayers()-2 );
		}
	}
	
	// pass 3
	{
		img->AddPendingState( ImageState{ EResourceState::ShaderWrite | EResourceState::_ComputeShader, VK_IMAGE_LAYOUT_GENERAL,
										  ImageRange{ 0_layer, img->ArrayLayers(), 0_mipmap, img->MipmapLevels() },
										  VK_IMAGE_ASPECT_COLOR_BIT, (task_iter++)->get() });

		img->CommitBarrier( barrier_mngr, null );

		auto	barriers = VImageUnitTest::GetRWBarriers( img );
		TEST( barriers.size() == 7 );
		
		for (size_t i = 0; i < barriers.size(); ++i)
		{
			TEST( barriers[i].range.begin == i*8 );
			TEST( barriers[i].range.end == (i+1)*8 );
			TEST( barriers[i].stages == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
			TEST( barriers[i].access == VK_ACCESS_SHADER_WRITE_BIT );
			TEST( barriers[i].isReadable == false );
			TEST( barriers[i].isWritable == true );
			TEST( barriers[i].layout == VK_IMAGE_LAYOUT_GENERAL );
			TEST( barriers[i].index == ExeOrderIndex(3) );

			CheckLayers( barriers[i], img->ArrayLayers(), uint(i), 1, 0, img->ArrayLayers() );
		}
	}

	local_image.ResetState( ExeOrderIndex::Final, barrier_mngr, null );

	local_image.Destroy();
	//global_image.Destroy();
}


extern void UnitTest_VImage ()
{
	VImage_Test1();
	VImage_Test2();
	FG_LOGI( "UnitTest_VImage - passed" );
}

#endif	// FG_ENABLE_VULKAN
