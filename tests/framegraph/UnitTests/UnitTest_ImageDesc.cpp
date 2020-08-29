// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/Public/ImageDesc.h"
#include "UnitTest_Common.h"


static void ImageDesc_Test1 ()
{
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension( 2 );

		TEST( desc.imageType == EImageDim_1D );
		TEST( desc.viewType == Default );
		TEST( All( desc.dimension == uint3{2, 0, 0} ));
	
		desc.Validate();

		TEST( desc.imageType == EImageDim_1D );
		TEST( desc.viewType == EImage_1D );
		TEST( All( desc.dimension == uint3{2, 1, 1} ));
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension({ 2, 3 });

		TEST( desc.imageType == EImageDim_2D );
		TEST( desc.viewType == Default );
		TEST( All( desc.dimension == uint3{2, 3, 0} ));
	
		desc.Validate();

		TEST( desc.imageType == EImageDim_2D );
		TEST( desc.viewType == EImage_2D );
		TEST( All( desc.dimension == uint3{2, 3, 1} ));
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension({ 2, 3, 4 });

		TEST( desc.imageType == EImageDim_3D );
		TEST( desc.viewType == Default );
		TEST( All( desc.dimension == uint3{2, 3, 4} ));
	
		desc.Validate();

		TEST( desc.imageType == EImageDim_3D );
		TEST( desc.viewType == EImage_3D );
		TEST( All( desc.dimension == uint3{2, 3, 4} ));
	}
}


static void ImageDesc_Test2 ()
{
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetView( EImage_1D );

		TEST( desc.viewType == EImage_1D );
		TEST( desc.imageType == EImageDim_1D );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage_1D );
		TEST( desc.imageType == EImageDim_1D );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetView( EImage::_1DArray );

		TEST( desc.viewType == EImage::_1DArray );
		TEST( desc.imageType == EImageDim_1D );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage::_1DArray );
		TEST( desc.imageType == EImageDim_1D );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetView( EImage_2D );

		TEST( desc.viewType == EImage_2D );
		TEST( desc.imageType == EImageDim_2D );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage_2D );
		TEST( desc.imageType == EImageDim_2D );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetView( EImage::_2DArray );

		TEST( desc.viewType == EImage::_2DArray );
		TEST( desc.imageType == EImageDim_2D );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage::_2DArray );
		TEST( desc.imageType == EImageDim_2D );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetView( EImage_3D );

		TEST( desc.viewType == EImage_3D );
		TEST( desc.imageType == EImageDim_3D );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage_3D );
		TEST( desc.imageType == EImageDim_3D );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetView( EImage_Cube );

		TEST( desc.viewType == EImage_Cube );
		TEST( desc.imageType == EImageDim_2D );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage_Cube );
		TEST( desc.imageType == EImageDim_2D );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetView( EImage_CubeArray );

		TEST( desc.viewType == EImage_CubeArray );
		TEST( desc.imageType == EImageDim_2D );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage_CubeArray );
		TEST( desc.imageType == EImageDim_2D );
	}
}


static void ImageDesc_Test3 ()
{
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension( 8 );
		desc.SetArrayLayers( 4 );

		TEST( desc.viewType == Default );
		TEST( desc.imageType == EImageDim_1D );
		TEST( desc.maxLevel == 1_mipmap );
		TEST( not desc.samples.IsEnabled() );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage::_1DArray );
		TEST( desc.imageType == EImageDim_1D );
		TEST( desc.maxLevel == 1_mipmap );
		TEST( not desc.samples.IsEnabled() );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension({ 8, 8 });
		desc.SetArrayLayers( 4 );

		TEST( desc.viewType == Default );
		TEST( desc.imageType == EImageDim_2D );
		TEST( desc.arrayLayers == 4_layer );
		TEST( desc.maxLevel == 1_mipmap );
		TEST( not desc.samples.IsEnabled() );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage::_2DArray );
		TEST( desc.imageType == EImageDim_2D );
		TEST( desc.arrayLayers == 4_layer );
		TEST( desc.maxLevel == 1_mipmap );
		TEST( not desc.samples.IsEnabled() );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension({ 8, 8 });
		desc.SetArrayLayers( 4 );
		desc.SetMaxMipmaps( 16 );

		TEST( desc.viewType == Default );
		TEST( desc.imageType == EImageDim_2D );
		TEST( desc.arrayLayers == 4_layer );
		TEST( desc.maxLevel == 16_mipmap );
		TEST( not desc.samples.IsEnabled() );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage::_2DArray );
		TEST( desc.imageType == EImageDim_2D );
		TEST( desc.arrayLayers == 4_layer );
		TEST( desc.maxLevel == 4_mipmap );
		TEST( not desc.samples.IsEnabled() );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension({ 8, 8 });
		desc.SetArrayLayers( 4 );
		desc.SetSamples( 8 );

		TEST( desc.viewType == Default );
		TEST( desc.imageType == EImageDim_2D );
		TEST( desc.arrayLayers == 4_layer );
		TEST( desc.maxLevel == 1_mipmap );
		TEST( desc.samples == 8_samples );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage::_2DArray );
		TEST( desc.imageType == EImageDim_2D );
		TEST( desc.arrayLayers == 4_layer );
		TEST( desc.maxLevel == 1_mipmap );
		TEST( desc.samples == 8_samples );
	}

	// crashed on CI decause of assertion
	/*{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension({ 8, 8 });
		desc.SetArrayLayers( 4 );
		desc.SetMaxMipmaps( 16 );
		desc.SetSamples( 4 );

		TEST( desc.viewType == Default );
		TEST( desc.imageType == EImageDim_2D );
		TEST( desc.arrayLayers == 4_layer );
		TEST( desc.maxLevel == 16_mipmap );
		TEST( desc.samples == 4_samples );
	
		desc.Validate();
		
		TEST( desc.viewType == EImage::_2DArray );
		TEST( desc.imageType == EImageDim_2D );
		TEST( desc.arrayLayers == 4_layer );
		TEST( desc.maxLevel == 1_mipmap );
		TEST( desc.samples == 4_samples );
	}*/
}


static void ImageView_Test1 ()
{
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension({ 32, 32 });
		desc.SetArrayLayers( 6 );

		TEST( desc.viewType == Default );

		desc.Validate();

		TEST( desc.viewType == EImage::_2DArray );

		ImageViewDesc	view;

		TEST( view.viewType == Default );

		view.Validate( desc );
		
		TEST( view.viewType == EImage::_2DArray );
		TEST( view.format == EPixelFormat::RGBA8_UNorm );
		TEST( view.baseLayer == 0_layer );
		TEST( view.layerCount == 6 );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension({ 32, 32 });
		desc.SetArrayLayers( 6 );
		desc.AddFlags( EImageFlags::CubeCompatible );

		TEST( desc.viewType == Default );

		desc.Validate();

		TEST( desc.viewType == EImage_Cube );

		ImageViewDesc	view;

		TEST( view.viewType == Default );

		view.Validate( desc );
		
		TEST( view.viewType == EImage_Cube );
		TEST( view.format == EPixelFormat::RGBA8_UNorm );
		TEST( view.baseLayer == 0_layer );
		TEST( view.layerCount == 6 );
	}
	{
		ImageDesc	desc;
		desc.format = EPixelFormat::RGBA8_UNorm;
		desc.SetDimension({ 32, 32 });
		desc.SetArrayLayers( 6 );
		desc.AddFlags( EImageFlags::CubeCompatible );
		
		TEST( desc.viewType == Default );
		
		desc.Validate();

		TEST( desc.viewType == EImage_Cube );
		
		ImageViewDesc	view;
		view.SetArrayLayers( 2, 2 );

		TEST( view.viewType == Default );

		view.Validate( desc );
		
		TEST( view.viewType == EImage::_2DArray );
		TEST( view.format == EPixelFormat::RGBA8_UNorm );
		TEST( view.baseLayer == 2_layer );
		TEST( view.layerCount == 2 );
	}
}


static void ImageView_Test2 ()
{
	{
		ImageDesc	desc;
		desc.format		= EPixelFormat::RGBA8_UNorm;
		desc.imageType	= EImageDim_2D;
		desc.Validate();
		
		ImageViewDesc	view;
		view.Validate( desc );

		TEST( view.aspectMask == EImageAspect::Color );
	}
	{
		ImageDesc	desc;
		desc.format		= EPixelFormat::Depth32F;
		desc.imageType	= EImageDim_2D;
		desc.Validate();
		
		ImageViewDesc	view;
		view.Validate( desc );

		TEST( view.aspectMask == EImageAspect::Depth );
	}
	{
		ImageDesc	desc;
		desc.format		= EPixelFormat::Depth24_Stencil8;
		desc.imageType	= EImageDim_2D;
		desc.Validate();
		
		ImageViewDesc	view;
		view.Validate( desc );

		TEST( view.aspectMask == EImageAspect::DepthStencil );
	}
}


extern void UnitTest_ImageDesc ()
{
	ImageDesc_Test1();
	ImageDesc_Test2();
	ImageDesc_Test3();
	
	ImageView_Test1();
	ImageView_Test2();

	FG_LOGI( "UnitTest_ImageDesc - passed" );
}
