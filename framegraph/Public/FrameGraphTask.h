// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/PipelineResources.h"
#include "framegraph/Public/LowLevel/BufferView.h"
#include "framegraph/Public/LowLevel/ImageView.h"
#include "framegraph/Public/FrameGraphDrawTask.h"

namespace FG
{
	namespace _fg_hidden_
	{
		struct _BaseTask {};


		//
		// Base Task
		//
		template <typename BaseType>
		struct BaseTask : _BaseTask
		{
		// types
			using Tasks_t		= FixedArray< Task, FG_MaxTaskDependencies >;
			using TaskName_t	= StaticString<64>;

		// variables
			Tasks_t					depends;	// current task wiil be executed after dependencies
			TaskName_t				taskName;
			RGBA8u					debugColor;
		
		// methods
			BaseTask () {}
			BaseTask (StringView name, RGBA8u color) : taskName{name}, debugColor{color} {}

			BaseType& SetName (StringView name)					{ taskName = name;  return static_cast<BaseType &>( *this ); }
			BaseType& SetDebugColor (RGBA8u color)				{ debugColor = color;  return static_cast<BaseType &>( *this ); }

			template <typename Arg0, typename ...Args>
            BaseType& DependsOn (Arg0 task0,  Args ...tasks)	{ depends.push_back( task0 );  return DependsOn<Args...>( tasks... ); }
			
			template <typename Arg0>
            BaseType& DependsOn (Arg0 task)						{ depends.push_back( task );  return static_cast<BaseType &>( *this ); }
		};

	}	// _fg_hidden_
	


	//
	// Image Subresource Range
	//
	struct ImageSubresourceRange
	{
	// variables
		EImageAspect	aspectMask;
		MipmapLevel		mipLevel;
		ImageLayer		baseLayer;
		uint			layerCount;

	// methods
		ImageSubresourceRange () :
			aspectMask{EImageAspect::Auto}, layerCount{1} {}
		
		explicit
		ImageSubresourceRange (MipmapLevel	mipLevel,
							   ImageLayer	baseLayer	= Default,
							   uint			layerCount	= 1,
							   EImageAspect	aspectMask	= EImageAspect::Auto) :
			aspectMask(aspectMask), mipLevel(mipLevel), baseLayer(baseLayer), layerCount(layerCount) {}
	};



	//
	// Submit Render Pass
	//
	struct SubmitRenderPass final : _fg_hidden_::BaseTask<SubmitRenderPass>
	{
	// variables
		RenderPass			renderPass = null;		// must be not null

	// methods
		explicit SubmitRenderPass (RenderPass rp) :
			BaseTask<SubmitRenderPass>{ "SubmitRenderPass", HtmlColor::OrangeRed }, renderPass{rp} {}
	};



	//
	// Dispatch Compute
	//
	struct DispatchCompute final : _fg_hidden_::BaseTask<DispatchCompute>
	{
	// variables
		PipelinePtr				pipeline;
		PipelineResourceSet		resources;
		uint3					groupCount;
		Optional< uint3 >		localGroupSize;
		

	// methods
		DispatchCompute () :
			BaseTask<DispatchCompute>{ "DispatchCompute", HtmlColor::MediumBlue } {}
		
		DispatchCompute&  SetPipeline (const PipelinePtr &ppln)										{ pipeline = ppln;  return *this; }
		DispatchCompute&  AddResources (const DescriptorSetID &id, const PipelineResourcesPtr &res)	{ resources.insert({ id, res });  return *this; }

		DispatchCompute&  SetGroupCount (const uint3 &value)										{ groupCount = value;  return *this; }
		DispatchCompute&  SetGroupCount (uint x, uint y = 1, uint z = 1)							{ groupCount = {x, y, z};  return *this; }

		DispatchCompute&  SetLocalSize (const uint3 &value)											{ localGroupSize = value;  return *this; }
		DispatchCompute&  SetLocalSize (uint x, uint y = 1, uint z = 1)								{ localGroupSize = {x, y, z};  return *this; }
	};



	//
	// Dispatch Indirect Compute
	//
	struct DispatchIndirectCompute final : _fg_hidden_::BaseTask<DispatchIndirectCompute>
	{
	// variables
		PipelinePtr				pipeline;
		PipelineResourceSet		resources;
		BufferPtr				indirectBuffer;
		BytesU					indirectBufferOffset;
		Optional< uint3 >		localGroupSize;
		

	// methods
		DispatchIndirectCompute () :
			BaseTask<DispatchIndirectCompute>{ "DispatchIndirectCompute", HtmlColor::MediumBlue } {}
		
		DispatchIndirectCompute&  SetPipeline (const PipelinePtr &ppln)		{ pipeline = ppln;  return *this; }
	};



	//
	// Copy Buffer
	//
	struct CopyBuffer final : _fg_hidden_::BaseTask<CopyBuffer>
	{
	// types
		struct Region
		{
			BytesU		srcOffset;
			BytesU		dstOffset;
			BytesU		size;
		};
		using Regions_t	= FixedArray< Region, FG_MaxCopyRegions >;


	// variables
		BufferPtr	srcBuffer;
		BufferPtr	dstBuffer;
		Regions_t	regions;


	// methods
		CopyBuffer () :
			BaseTask<CopyBuffer>{ "CopyBuffer", HtmlColor::Green } {}

		CopyBuffer&  From (const BufferPtr &buf)		{ srcBuffer = buf;  return *this; }
		CopyBuffer&  To   (const BufferPtr &buf)		{ dstBuffer = buf;  return *this; }
		
		CopyBuffer&  AddRegion (BytesU srcOffset, BytesU dstOffset, BytesU size)
		{
			regions.push_back(Region{ srcOffset, dstOffset, size });
			return *this;
		}
	};



	//
	// Copy Image
	//
	struct CopyImage final : _fg_hidden_::BaseTask<CopyImage>
	{
	// types
		struct Region
		{
			ImageSubresourceRange	srcSubresource;
			int3					srcOffset;
			ImageSubresourceRange	dstSubresource;
			int3					dstOffset;
			uint3					size;
		};
		using Regions_t		= FixedArray< Region, FG_MaxCopyRegions >;


	// variables
		ImagePtr		srcImage;
		ImagePtr		dstImage;
		Regions_t		regions;


	// methods
		CopyImage () :
			BaseTask<CopyImage>{ "CopyImage", HtmlColor::Green } {}

		CopyImage&  From (const ImagePtr &img)		{ srcImage = img;  return *this; }
		CopyImage&  To   (const ImagePtr &img)		{ dstImage = img;  return *this; }

		CopyImage&  AddRegion (const ImageSubresourceRange &srcSubresource, const int3 &srcOffset,
							   const ImageSubresourceRange &dstSubresource, const int3 &dstOffset,
							   const uint3 &size)
		{
			regions.push_back(Region{ srcSubresource, srcOffset, dstSubresource, dstOffset, size });
			return *this;
		}

		CopyImage&  AddRegion (const ImageSubresourceRange &srcSubresource, const int2 &srcOffset,
							   const ImageSubresourceRange &dstSubresource, const int2 &dstOffset,
							   const uint2 &size)
		{
			regions.push_back(Region{ srcSubresource, int3{ srcOffset.x, srcOffset.y, 0 },
									  dstSubresource, int3{ dstOffset.x, dstOffset.y, 0 },
									  uint3{ size.x, size.y, 0 } });
			return *this;
		}
	};



	//
	// Copy Buffer to Image
	//
	struct CopyBufferToImage final : _fg_hidden_::BaseTask<CopyBufferToImage>
	{
	// types
		struct Region
		{
			BytesU					bufferOffset;
			uint					bufferRowLength;	// pixels
			uint					bufferImageHeight;	// pixels
			ImageSubresourceRange	imageLayers;
			int3					imageOffset;
			uint3					imageSize;
		};
		using Regions_t	= FixedArray< Region, FG_MaxCopyRegions >;


	// variables
		BufferPtr		srcBuffer;
		ImagePtr		dstImage;
		Regions_t		regions;


	// methods
		CopyBufferToImage () :
			BaseTask<CopyBufferToImage>{ "CopyBufferToImage", HtmlColor::Green } {}

		CopyBufferToImage&  From (const BufferPtr &buf)		{ srcBuffer = buf;  return *this; }
		CopyBufferToImage&  To   (const ImagePtr &img)		{ dstImage = img;  return *this; }

		CopyBufferToImage&  AddRegion (BytesU bufferOffset, uint bufferRowLength, uint bufferImageHeight,
									   const ImageSubresourceRange &imageLayers, const int3 &imageOffset, const uint3 &imageSize)
		{
			regions.push_back(Region{ bufferOffset, bufferRowLength, bufferImageHeight,
									  imageLayers, imageOffset, imageSize });
			return *this;
		}
	};



	//
	// Copy Image to Buffer
	//
	struct CopyImageToBuffer final : _fg_hidden_::BaseTask<CopyImageToBuffer>
	{
	// types
		using Region	= CopyBufferToImage::Region;
		using Regions_t	= CopyBufferToImage::Regions_t;


	// variables
		ImagePtr		srcImage;
		BufferPtr		dstBuffer;
		Regions_t		regions;


	// methods
		CopyImageToBuffer () :
			BaseTask<CopyImageToBuffer>{ "CopyImageToBuffer", HtmlColor::Green } {}

		CopyImageToBuffer&  From (const ImagePtr &img)		{ srcImage = img;  return *this; }
		CopyImageToBuffer&  To   (const BufferPtr &buf)		{ dstBuffer = buf;  return *this; }

		CopyImageToBuffer&  AddRegion (const ImageSubresourceRange &imageLayers, const int3 &imageOffset, const uint3 &imageSize,
									   BytesU bufferOffset, uint bufferRowLength, uint bufferImageHeight)
		{
			regions.push_back(Region{ bufferOffset, bufferRowLength, bufferImageHeight,
									  imageLayers, imageOffset, imageSize });
			return *this;
		}
	};



	//
	// Blit Image
	//
	struct BlitImage final : _fg_hidden_::BaseTask<BlitImage>
	{
	// types
		struct Region
		{
			ImageSubresourceRange	srcSubresource;
			int3					srcOffset0;		// start offset
			int3					srcOffset1;		// end offset
			ImageSubresourceRange	dstSubresource;
			int3					dstOffset0;		// start offset
			int3					dstOffset1;		// end offset
		};
		using Regions_t = FixedArray< Region, FG_MaxBlitRegions >;


	// variables
		ImagePtr		srcImage;
		ImagePtr		dstImage;
		EFilter			filter		= EFilter::Nearest;
		Regions_t		regions;


	// methods
		BlitImage () :
			BaseTask<BlitImage>{ "BlitImage", HtmlColor::Green } {}

		BlitImage&  From (const ImagePtr &img)		{ srcImage = img;  return *this; }
		BlitImage&  To   (const ImagePtr &img)		{ dstImage = img;  return *this; }
		
		BlitImage&  SetFilter (EFilter value)		{ filter = value;  return *this; }

		BlitImage&  AddRegion (const ImageSubresourceRange &srcSubresource, const int3 &srcOffset0, const int3 &srcOffset1,
							   const ImageSubresourceRange &dstSubresource, const int3 &dstOffset0, const int3 &dstOffset1)
		{
			regions.push_back(Region{ srcSubresource, srcOffset0, srcOffset1,
									  dstSubresource, dstOffset0, dstOffset1 });
			return *this;
		}
	};



	//
	// Resolve Image
	//
	struct ResolveImage final : _fg_hidden_::BaseTask<ResolveImage>
	{
	// types
		struct Region
		{
			ImageSubresourceRange	srcSubresource;
			int3					srcOffset;
			ImageSubresourceRange	dstSubresource;
			int3					dstOffset;
			uint3					extent;
		};
		using Regions_t = FixedArray< Region, FG_MaxResolveRegions >;


	// variables
		ImagePtr		srcImage;
		ImagePtr		dstImage;
		Regions_t		regions;


	// methods
		ResolveImage () :
			BaseTask<ResolveImage>{ "ResolveImage", HtmlColor::Green } {}

		ResolveImage&  From (const ImagePtr &img)		{ srcImage = img;  return *this; }
		ResolveImage&  To   (const ImagePtr &img)		{ dstImage = img;  return *this; }
		
		ResolveImage&  AddRegion (const ImageSubresourceRange &srcSubresource, const int3 &srcOffset,
								  const ImageSubresourceRange &dstSubresource, const int3 &dstOffset,
								  const uint3 &extent)
		{
			regions.push_back(Region{ srcSubresource, srcOffset, dstSubresource, dstOffset, extent });
			return *this;
		}
	};



	//
	// Fill Buffer
	//
	struct FillBuffer final : _fg_hidden_::BaseTask<FillBuffer>
	{
	// variables
		BufferPtr		dstBuffer;
		BytesU			dstOffset;
		BytesU			size;
		uint			pattern		= 0;


	// methods
		FillBuffer () :
			BaseTask<FillBuffer>{ "FillBuffer", HtmlColor::Green } {}

		FillBuffer&  SetBuffer (const BufferPtr &buf, BytesU off, BytesU bufSize)
		{
			dstBuffer	= buf;
			dstOffset	= off;
			size		= bufSize;
			return *this;
		}

		FillBuffer&  SetPattern (uint value)
		{
			pattern = value;
			return *this;
		}
	};



	//
	// Clear Color Image
	//
	struct ClearColorImage final : _fg_hidden_::BaseTask<ClearColorImage>
	{
	// types
		struct Range
		{
			EImageAspect	aspectMask;
			MipmapLevel		baseMipLevel;
			uint			levelCount;
			ImageLayer		baseLayer;
			uint			layerCount;
		};
		using Ranges_t		= FixedArray< Range, FG_MaxClearRects >;
		using ClearColor_t	= Union< float4, uint4, int4 >;


	// variables
		ImagePtr			dstImage;
		Ranges_t			ranges;
		ClearColor_t		clearValue;
		
		
	// methods
		ClearColorImage () :
			BaseTask<ClearColorImage>{ "ClearColorImage", HtmlColor::Green } {}

		ClearColorImage&  SetImage (const ImagePtr &img)	{ dstImage = img;  return *this; }

		ClearColorImage&  Clear (const float4 &value)		{ clearValue = value;  return *this; }
		ClearColorImage&  Clear (const uint4 &value)		{ clearValue = value;  return *this; }
		ClearColorImage&  Clear (const int4 &value)			{ clearValue = value;  return *this; }
		
		ClearColorImage&  AddRange (MipmapLevel baseMipLevel, uint levelCount,
								    ImageLayer baseLayer, uint layerCount)
		{
			ranges.push_back(Range{ EImageAspect::Color, baseMipLevel, levelCount, baseLayer, layerCount });
			return *this;
		}
	};



	//
	// Clear Depth Stencil Image
	//
	struct ClearDepthStencilImage final : _fg_hidden_::BaseTask<ClearDepthStencilImage>
	{
	// types
		using Range		= ClearColorImage::Range;
		using Ranges_t	= FixedArray< Range, FG_MaxClearRects >;


	// variables
		ImagePtr			dstImage;
		Ranges_t			ranges;
		DepthStencil		clearValue;
		

	// methods
		ClearDepthStencilImage () :
			BaseTask<ClearDepthStencilImage>{ "ClearDepthStencilImage", HtmlColor::Green } {}
		
		ClearDepthStencilImage&  SetImage (const ImagePtr &img)			{ dstImage = img;  return *this; }

		ClearDepthStencilImage&  Clear (float depth, uint stencil)		{ clearValue = DepthStencil{ depth, stencil };  return *this; }
		
		ClearDepthStencilImage&  AddRange (MipmapLevel baseMipLevel, uint levelCount,
										   ImageLayer baseLayer, uint layerCount)
		{
			ranges.push_back(Range{ EImageAspect::DepthStencil, baseMipLevel, levelCount, baseLayer, layerCount });
			return *this;
		}
	};



	//
	// Update Buffer
	//
	struct UpdateBuffer final : _fg_hidden_::BaseTask<UpdateBuffer>
	{
	// variables
		BufferPtr			dstBuffer;		// if buffer has gpu local memory, then staging buffer will be used
		BytesU				offset;
		ArrayView<uint8_t>	data;


	// methods
		UpdateBuffer () :
			BaseTask<UpdateBuffer>{ "UpdateBuffer", HtmlColor::BlueViolet } {}

		UpdateBuffer (const BufferPtr &buf, BytesU off, ArrayView<uint8_t> data) :
			UpdateBuffer() { SetBuffer( buf, off ).SetData( data ); }

		UpdateBuffer&  SetBuffer (const BufferPtr &buf, BytesU off)
		{
			dstBuffer	= buf;
			offset		= off;
			return *this;
		}

		UpdateBuffer&  SetData (ArrayView<uint8_t> value)
		{
			data = value;
			return *this;
		}
	};



	//
	// Read Buffer
	//
	struct ReadBuffer final : _fg_hidden_::BaseTask<ReadBuffer>
	{
	// types
		using Callback_t	= std::function< void (BufferView) >;

	// variables
		BufferPtr		srcBuffer;
		BytesU			offset;
		BytesU			size;
		Callback_t		callback;

	// methods
		ReadBuffer () :
			BaseTask<ReadBuffer>{ "ReadBuffer", HtmlColor::BlueViolet } {}

		ReadBuffer&  SetBuffer (const BufferPtr &buf, const BytesU off, const BytesU dataSize)
		{
			srcBuffer	= buf;
			offset		= off;
			size		= dataSize;
			return *this;
		}

		template <typename FN>
		ReadBuffer&  SetCallback (FN &&value)
		{
			callback = std::move(value);
			return *this;
		}
	};



	//
	// Update Image
	//
	struct UpdateImage final : _fg_hidden_::BaseTask<UpdateImage>
	{
	// variables
		ImagePtr			dstImage;
		int3				imageOffset;
		uint3				imageSize;
		ImageLayer			arrayLayer;
		MipmapLevel			mipmapLevel;
		BytesU				dataRowPitch;
		BytesU				dataSlicePitch;
		EImageAspect		aspectMask	= EImageAspect::Color;	// must only have a single bit set
		ArrayView<uint8_t>	data;

		
	// methods
		UpdateImage () :
			BaseTask<UpdateImage>{ "UpdateImage", HtmlColor::BlueViolet } {}
		
		UpdateImage&  SetImage (const ImagePtr &img, const int2 &offset, MipmapLevel mipmap = Default)
		{
			dstImage		= img;
			imageOffset		= int3( offset.x, offset.y, 0 );
			mipmapLevel		= mipmap;
			return *this;
		}

		UpdateImage&  SetImage (const ImagePtr &img, const int3 &offset = Default, MipmapLevel mipmap = Default)
		{
			dstImage		= img;
			imageOffset		= offset;
			mipmapLevel		= mipmap;
			return *this;
		}

		UpdateImage&  SetImage (const ImagePtr &img, const int2 &offset, ImageLayer layer, MipmapLevel mipmap)
		{
			dstImage		= img;
			imageOffset		= int3( offset.x, offset.y, 0 );
			arrayLayer		= layer;
			mipmapLevel		= mipmap;
			return *this;
		}

		UpdateImage&  SetData (ArrayView<uint8_t> value, const uint2 &size, BytesU rowPitch = 0_b)
		{
			data			= value;
			imageSize		= uint3( size.x, size.y, 0 );
			dataRowPitch	= rowPitch;
			return *this;
		}

		UpdateImage&  SetData (ArrayView<uint8_t> value, const uint3 &size, BytesU rowPitch = 0_b, BytesU slicePitch = 0_b)
		{
			data			= value;
			imageSize		= size;
			dataRowPitch	= rowPitch;
			dataSlicePitch	= slicePitch;
			return *this;
		}
	};



	//
	// Read Image
	//
	struct ReadImage final : _fg_hidden_::BaseTask<ReadImage>
	{
	// types
		using Callback_t	= std::function< void (const ImageView &) >;

		
	// variables
		ImagePtr		srcImage;
		int3			imageOffset;
		uint3			imageSize;
		ImageLayer		arrayLayer;
		MipmapLevel		mipmapLevel;
		EImageAspect	aspectMask	= EImageAspect::Color;	// must only have a single bit set
		Callback_t		callback;

		
	// methods
		ReadImage () :
			BaseTask<ReadImage>{ "ReadImage", HtmlColor::BlueViolet } {}
		
		ReadImage&  SetImage (const ImagePtr &img, const int2 &offset, const uint2 &size, MipmapLevel mipmap = Default)
		{
			srcImage	= img;
			imageOffset	= int3( offset.x, offset.y, 0 );
			imageSize	= int3( size.x, size.y, 0 );
			mipmapLevel	= mipmap;
			return *this;
		}

		ReadImage&  SetImage (const ImagePtr &img, const int3 &offset, const uint3 &size, MipmapLevel mipmap = Default)
		{
			srcImage	= img;
			imageOffset	= offset;
			imageSize	= size;
			mipmapLevel	= mipmap;
			return *this;
		}
		
		ReadImage&  SetImage (const ImagePtr &img, const int2 &offset, const uint2 &size, ImageLayer layer, MipmapLevel mipmap = Default)
		{
			srcImage	= img;
			imageOffset	= int3( offset.x, offset.y, 0 );
			imageSize	= int3( size.x, size.y, 0 );
			arrayLayer	= layer;
			mipmapLevel	= mipmap;
			return *this;
		}

		template <typename FN>
		ReadImage&  SetCallback (FN &&value)
		{
			callback = std::move(value);
			return *this;
		}
	};



	//
	// Present
	//
	struct Present final : _fg_hidden_::BaseTask<Present>
	{
	// variables
		ImagePtr			srcImage;
		ImageLayer			layer;


	// methods
		Present () :
			BaseTask<Present>{ "Present", HtmlColor::Red } {}

		explicit Present (const ImagePtr &img) :
			Present() { srcImage = img; }

		Present&  SetImage (const ImagePtr &img, ImageLayer imgLayer = ImageLayer())
		{
			srcImage	= img;
			layer		= imgLayer;
			return *this;
		}
	};



	//
	// Present VR
	//
	struct PresentVR final : _fg_hidden_::BaseTask<Present>
	{
	// variables
		ImagePtr		leftEye;
		ImageLayer		leftEyeLayer;

		ImagePtr		rightEye;
		ImageLayer		rightEyeLayer;


	// methods
		PresentVR () :
			BaseTask<Present>{ "PresentVR", HtmlColor::Red } {}
		
		PresentVR&  SetLeftEye (const ImagePtr &img, ImageLayer imgLayer = ImageLayer())
		{
			leftEye			= img;
			leftEyeLayer	= imgLayer;
			return *this;
		}
		
		PresentVR&  SetRightEye (const ImagePtr &img, ImageLayer imgLayer = ImageLayer())
		{
			rightEye		= img;
			rightEyeLayer	= imgLayer;
			return *this;
		}
	};



	//
	// Synchronization Point
	//
	struct TaskGroupSync final : _fg_hidden_::BaseTask<TaskGroupSync>
	{
		TaskGroupSync () :
			BaseTask<TaskGroupSync>{ "TaskGroupSync", HtmlColor::Teal } {}
	};


}	// FG
