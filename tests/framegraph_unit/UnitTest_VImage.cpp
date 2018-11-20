// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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


static void VImage_Test1 ()
{
	VBarrierManager		barrier_mngr;
	
	const auto			tasks		= GenDummyTasks( 30 );
	auto				task_iter	= tasks.begin();

	VImage				global_image;
	VLocalImage			local_image;
	VLocalImage const*	img			= &local_image;

	TEST( VImageUnitTest::Create( global_image,
								  ImageDesc{ EImage::Tex2D, uint3(64, 64, 0), EPixelFormat::RGBA8_UNorm,
											 EImageUsage::ColorAttachment | EImageUsage::Transfer | EImageUsage::Storage | EImageUsage::Sampled,
											 0_layer, 11_mipmap } ));

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
		
		TEST( barriers[1].range.begin == 1 );
		TEST( barriers[1].range.end == 7 );
		TEST( barriers[1].stages == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT );
		TEST( barriers[1].access == 0 );
		TEST( barriers[1].isReadable == false );
		TEST( barriers[1].isWritable == false );
		TEST( barriers[1].layout == VK_IMAGE_LAYOUT_UNDEFINED );
		TEST( barriers[1].index == ExeOrderIndex::Initial );
	}
	
	local_image.ResetState( ExeOrderIndex::Final, barrier_mngr, null );

	Array<UntypedVkResource_t>	ready_to_delete;
	Array<UntypedResourceID_t>	unassign_ids;
	local_image.Destroy( OUT ready_to_delete, OUT unassign_ids );
	global_image.Destroy( OUT ready_to_delete, OUT unassign_ids );
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
								  ImageDesc{ EImage::Tex2DArray, uint3(64, 64, 0), EPixelFormat::RGBA8_UNorm,
											 EImageUsage::ColorAttachment | EImageUsage::Transfer | EImageUsage::Storage | EImageUsage::Sampled,
											 8_layer, 11_mipmap } ));

	TEST( local_image.Create( &global_image ));

	// pass 1
	{
		img->AddPendingState(ImageState{ EResourceState::TransferDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								ImageRange{ 0_layer, 2, 0_mipmap, img->MipmapLevels() },
								VK_IMAGE_ASPECT_COLOR_BIT, (task_iter++)->get() });

		img->CommitBarrier( barrier_mngr, null );

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
		TEST( barriers[1].isReadable == false );
		TEST( barriers[1].isWritable == false );
		TEST( barriers[1].layout == VK_IMAGE_LAYOUT_UNDEFINED );
		TEST( barriers[1].index == ExeOrderIndex::Initial );
	}
	
	local_image.ResetState( ExeOrderIndex::Final, barrier_mngr, null );

	Array<UntypedVkResource_t>	ready_to_delete;
	Array<UntypedResourceID_t>	unassign_ids;
	local_image.Destroy( OUT ready_to_delete, OUT unassign_ids );
	global_image.Destroy( OUT ready_to_delete, OUT unassign_ids );
}


extern void UnitTest_VImage ()
{
	VImage_Test1();
	VImage_Test2();
    FG_LOGI( "UnitTest_VImage - passed" );
}
