// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VImage.h"
#include "VBarrierManager.h"
#include "framegraph/Public/FrameGraph.h"
#include "UnitTestCommon.h"
#include "DummyTask.h"


namespace FG
{
	class VImageUnitTest
	{
	public:
		using Barrier = VImage::ImageBarrier;

		static ArrayView<Barrier>  GetRWBarriers (const VImagePtr &img) {
			return img->_readWriteBarriers;
		}
	};

	using ImageState	= VImage::ImageState;
	using ImageRange	= VImage::ImageRange;
}	// FG


static void VImage_Test1 (const FrameGraphPtr &fg)
{
	VBarrierManager	barrier_mngr;
	
	const auto	tasks		= GenDummyTasks( 30 );
	auto		task_iter	= tasks.begin();

	VImagePtr	img = Cast<VImage>( fg->CreateImage( EMemoryType::Default, EImage::Tex2D, uint3(64, 64, 0), EPixelFormat::RGBA8_UNorm,
													 EImageUsage::ColorAttachment | EImageUsage::Transfer | EImageUsage::Storage | EImageUsage::Sampled,
													 0_layer, 11_mipmap ));
	
	// pass 1
	{
		img->AddPendingState(ImageState{ EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								ImageRange{ 0_layer, 1, 0_mipmap, 1 },
								VK_IMAGE_ASPECT_COLOR_BIT, (task_iter++)->get() });

		img->CommitBarrier( barrier_mngr );

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
		
		TEST( barriers[1].range.begin == 1 );
		TEST( barriers[1].range.end == 7 );
		TEST( barriers[1].stages == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
		TEST( barriers[1].access == 0 );
		TEST( barriers[1].isReadable == true );
		TEST( barriers[1].isWritable == true );
		TEST( barriers[1].layout == VK_IMAGE_LAYOUT_UNDEFINED );
		TEST( barriers[1].index == ExeOrderIndex::Initial );
	}
}


static void VImage_Test2 (const FrameGraphPtr &fg)
{
	VBarrierManager	barrier_mngr;
	
	const auto	tasks		= GenDummyTasks( 30 );
	auto		task_iter	= tasks.begin();

	VImagePtr	img = Cast<VImage>( fg->CreateImage( EMemoryType::Default, EImage::Tex2DArray, uint3(64, 64, 0), EPixelFormat::RGBA8_UNorm,
													 EImageUsage::ColorAttachment | EImageUsage::Transfer | EImageUsage::Storage | EImageUsage::Sampled,
													 8_layer, 11_mipmap ));
	
	// pass 1
	{
		img->AddPendingState(ImageState{ EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								ImageRange{ 0_layer, 2, 0_mipmap, img->MipmapLevels() },
								VK_IMAGE_ASPECT_COLOR_BIT, (task_iter++)->get() });

		img->CommitBarrier( barrier_mngr );

		auto	barriers = VImageUnitTest::GetRWBarriers( img );
		
		TEST( barriers.size() == 14 );

		TEST( barriers[0].range.begin == 0 );
		TEST( barriers[0].range.end == 2 );
		TEST( barriers[0].stages == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( barriers[0].access == VK_ACCESS_TRANSFER_WRITE_BIT );
		TEST( barriers[0].isReadable == false );
		TEST( barriers[0].isWritable == true );
		TEST( barriers[0].layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		TEST( barriers[0].index == ExeOrderIndex(1) );
		
		TEST( barriers[1].range.begin == 2 );
		TEST( barriers[1].range.end == 8 );
		TEST( barriers[1].stages == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
		TEST( barriers[1].access == 0 );
		TEST( barriers[1].isReadable == true );
		TEST( barriers[1].isWritable == true );
		TEST( barriers[1].layout == VK_IMAGE_LAYOUT_UNDEFINED );
		TEST( barriers[1].index == ExeOrderIndex::Initial );
	}
}


extern void UnitTest_VImage (const FrameGraphPtr &fg)
{
	VImage_Test1( fg );
	VImage_Test2( fg );
    FG_LOGI( "UnitTest_VImage - passed" );
}
