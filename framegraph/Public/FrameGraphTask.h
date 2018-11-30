// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/PipelineResources.h"
#include "framegraph/Public/BufferView.h"
#include "framegraph/Public/ImageView.h"
#include "framegraph/Public/SamplerEnums.h"
#include "framegraph/Public/RayTracingGeometryDesc.h"
#include "framegraph/Public/RayTracingSceneDesc.h"
#include "framegraph/Public/FrameGraphDrawTask.h"

namespace FG
{
	namespace _fg_hidden_
	{
		//
		// Base Task
		//
		template <typename BaseType>
		struct BaseTask
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
			BaseType& DependsOn (Arg0 task0, Args ...tasks)		{ if ( task0 ) depends.push_back( task0 );  return DependsOn<Args...>( tasks... ); }
			
			template <typename Arg0>
			BaseType& DependsOn (Arg0 task)						{ if ( task ) depends.push_back( task );  return static_cast<BaseType &>( *this ); }
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
		LogicalPassID		renderPassId;

	// methods
		explicit SubmitRenderPass (LogicalPassID rp) :
			BaseTask<SubmitRenderPass>{ "SubmitRenderPass", HtmlColor::OrangeRed }, renderPassId{rp} {}
	};



	//
	// Dispatch Compute
	//
	struct DispatchCompute final : _fg_hidden_::BaseTask<DispatchCompute>
	{
	// types
		using PushConstants_t	= _fg_hidden_::PushConstants_t;


	// variables
		RawCPipelineID			pipeline;
		PipelineResourceSet		resources;
		uint3					groupCount;
		Optional< uint3 >		localGroupSize;
		PushConstants_t			pushConstants;
		

	// methods
		DispatchCompute () :
			BaseTask<DispatchCompute>{ "DispatchCompute", HtmlColor::MediumBlue } {}
		
		DispatchCompute&  SetPipeline (const CPipelineID &ppln)								{ ASSERT( ppln );  pipeline = ppln.Get();  return *this; }
		DispatchCompute&  AddResources (uint bindingIndex, const PipelineResources *res)	{ resources[bindingIndex] = res;  return *this; }

		DispatchCompute&  SetGroupCount (const uint3 &value)								{ groupCount = value;  return *this; }
		DispatchCompute&  SetGroupCount (uint x, uint y = 1, uint z = 1)					{ groupCount = {x, y, z};  return *this; }

		DispatchCompute&  SetLocalSize (const uint3 &value)									{ localGroupSize = value;  return *this; }
		DispatchCompute&  SetLocalSize (uint x, uint y = 1, uint z = 1)						{ localGroupSize = {x, y, z};  return *this; }
		
		template <typename ValueType>
		DispatchCompute&  AddPushConstant (const PushConstantID &id, const ValueType &value){ return AddPushConstant( id, AddressOf(value), SizeOf<ValueType> ); }

		DispatchCompute&  AddPushConstant (const PushConstantID &id, const void *ptr, BytesU size)
		{
			ASSERT( id.IsDefined() );
			pushConstants.emplace_back( id, Bytes<uint16_t>(size) );
			MemCopy( pushConstants.back().data, BytesU::SizeOf(pushConstants.back().data), ptr, size );
			return *this;
		}
	};



	//
	// Dispatch Compute Indirect
	//
	struct DispatchComputeIndirect final : _fg_hidden_::BaseTask<DispatchComputeIndirect>
	{
	// types
		using PushConstants_t	= _fg_hidden_::PushConstants_t;


	// variables
		RawCPipelineID			pipeline;
		PipelineResourceSet		resources;
		RawBufferID				indirectBuffer;
		BytesU					indirectBufferOffset;
		Optional< uint3 >		localGroupSize;
		PushConstants_t			pushConstants;
		

	// methods
		DispatchComputeIndirect () :
			BaseTask<DispatchComputeIndirect>{ "DispatchComputeIndirect", HtmlColor::MediumBlue } {}
		
		DispatchComputeIndirect&  AddResources (uint bindingIndex, const PipelineResources *res)	{ resources[bindingIndex] = res;  return *this; }

		DispatchComputeIndirect&  SetLocalSize (const uint3 &value)									{ localGroupSize = value;  return *this; }
		DispatchComputeIndirect&  SetLocalSize (uint x, uint y = 1, uint z = 1)						{ localGroupSize = {x, y, z};  return *this; }
		
		DispatchComputeIndirect&  SetPipeline (const CPipelineID &ppln)
		{
			ASSERT( ppln );
			pipeline = ppln.Get();
			return *this;
		}
		
		DispatchComputeIndirect&  SetIndirectBuffer (const BufferID &buffer)
		{
			ASSERT( buffer );
			indirectBuffer = buffer.Get();
			return *this;
		}

		template <typename ValueType>
		DispatchComputeIndirect&  AddPushConstant (const PushConstantID &id, const ValueType &value)
		{
			return AddPushConstant( id, AddressOf(value), SizeOf<ValueType> );
		}

		DispatchComputeIndirect&  AddPushConstant (const PushConstantID &id, const void *ptr, BytesU size)
		{
			ASSERT( id.IsDefined() );
			pushConstants.emplace_back( id, Bytes<uint16_t>(size) );
			MemCopy( pushConstants.back().data, BytesU::SizeOf(pushConstants.back().data), ptr, size );
			return *this;
		}
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
		RawBufferID		srcBuffer;
		RawBufferID		dstBuffer;
		Regions_t		regions;


	// methods
		CopyBuffer () :
			BaseTask<CopyBuffer>{ "CopyBuffer", HtmlColor::Green } {}

		CopyBuffer&  From (const BufferID &buf)		{ ASSERT( buf );  srcBuffer = buf.Get();  return *this; }
		CopyBuffer&  To   (const BufferID &buf)		{ ASSERT( buf );  dstBuffer = buf.Get();  return *this; }
		
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
		RawImageID		srcImage;
		RawImageID		dstImage;
		Regions_t		regions;


	// methods
		CopyImage () :
			BaseTask<CopyImage>{ "CopyImage", HtmlColor::Green } {}

		CopyImage&  From (const ImageID &img)		{ ASSERT( img );  srcImage = img.Get();  return *this; }
		CopyImage&  To   (const ImageID &img)		{ ASSERT( img );  dstImage = img.Get();  return *this; }

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
		RawBufferID		srcBuffer;
		RawImageID		dstImage;
		Regions_t		regions;


	// methods
		CopyBufferToImage () :
			BaseTask<CopyBufferToImage>{ "CopyBufferToImage", HtmlColor::Green } {}

		CopyBufferToImage&  From (const BufferID &buf)		{ ASSERT( buf );  srcBuffer = buf.Get();  return *this; }
		CopyBufferToImage&  To   (const ImageID &img)		{ ASSERT( img );  dstImage  = img.Get();  return *this; }

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
		RawImageID		srcImage;
		RawBufferID		dstBuffer;
		Regions_t		regions;


	// methods
		CopyImageToBuffer () :
			BaseTask<CopyImageToBuffer>{ "CopyImageToBuffer", HtmlColor::Green } {}

		CopyImageToBuffer&  From (const ImageID &img)		{ ASSERT( img );  srcImage  = img.Get();  return *this; }
		CopyImageToBuffer&  To   (const BufferID &buf)		{ ASSERT( buf );  dstBuffer = buf.Get();  return *this; }

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
		RawImageID		srcImage;
		RawImageID		dstImage;
		EFilter			filter		= EFilter::Nearest;
		Regions_t		regions;


	// methods
		BlitImage () :
			BaseTask<BlitImage>{ "BlitImage", HtmlColor::Green } {}

		BlitImage&  From (const ImageID &img)		{ ASSERT( img );  srcImage = img.Get();  return *this; }
		BlitImage&  To   (const ImageID &img)		{ ASSERT( img );  dstImage = img.Get();  return *this; }
		
		BlitImage&  SetFilter (EFilter value)		{ filter = value;  return *this; }
		
		BlitImage&  AddRegion (const ImageSubresourceRange &srcSubresource, const int2 &srcOffset0, const int2 &srcOffset1,
							   const ImageSubresourceRange &dstSubresource, const int2 &dstOffset0, const int2 &dstOffset1)
		{
			regions.push_back(Region{ srcSubresource, int3{srcOffset0.x, srcOffset0.y, 0}, int3{srcOffset1.x, srcOffset1.y, 1},
									  dstSubresource, int3{dstOffset0.x, dstOffset0.y, 0}, int3{dstOffset1.x, dstOffset1.y, 1} });
			return *this;
		}

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
		RawImageID		srcImage;
		RawImageID		dstImage;
		Regions_t		regions;


	// methods
		ResolveImage () :
			BaseTask<ResolveImage>{ "ResolveImage", HtmlColor::Green } {}

		ResolveImage&  From (const ImageID &img)		{ ASSERT( img );  srcImage = img.Get();  return *this; }
		ResolveImage&  To   (const ImageID &img)		{ ASSERT( img );  dstImage = img.Get();  return *this; }
		
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
		RawBufferID		dstBuffer;
		BytesU			dstOffset;
		BytesU			size;
		uint			pattern		= 0;


	// methods
		FillBuffer () :
			BaseTask<FillBuffer>{ "FillBuffer", HtmlColor::Green } {}

		FillBuffer&  SetBuffer (const BufferID &buf, BytesU off, BytesU bufSize)
		{
			ASSERT( buf );
			dstBuffer	= buf.Get();
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
		using Ranges_t		= FixedArray< Range, FG_MaxClearRanges >;
		using ClearColor_t	= Union< float4, uint4, int4 >;


	// variables
		RawImageID			dstImage;
		Ranges_t			ranges;
		ClearColor_t		clearValue;
		
		
	// methods
		ClearColorImage () :
			BaseTask<ClearColorImage>{ "ClearColorImage", HtmlColor::Green } {}

		ClearColorImage&  SetImage (const ImageID &img)		{ ASSERT( img );  dstImage = img.Get();  return *this; }

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
		using Ranges_t	= FixedArray< Range, FG_MaxClearRanges >;


	// variables
		RawImageID			dstImage;
		Ranges_t			ranges;
		DepthStencil		clearValue;
		

	// methods
		ClearDepthStencilImage () :
			BaseTask<ClearDepthStencilImage>{ "ClearDepthStencilImage", HtmlColor::Green } {}
		
		ClearDepthStencilImage&  SetImage (const ImageID &img)			{ ASSERT( img );  dstImage = img.Get();  return *this; }

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
		RawBufferID			dstBuffer;		// if buffer has gpu local memory, then staging buffer will be used
		BytesU				offset;
		ArrayView<uint8_t>	data;


	// methods
		UpdateBuffer () :
			BaseTask<UpdateBuffer>{ "UpdateBuffer", HtmlColor::BlueViolet } {}

		UpdateBuffer (const BufferID &buf, BytesU off, ArrayView<uint8_t> data) :
			UpdateBuffer() { SetBuffer( buf, off ).SetData( data ); }

		UpdateBuffer&  SetBuffer (const BufferID &buf, BytesU off = 0_b)
		{
			ASSERT( buf );
			dstBuffer	= buf.Get();
			offset		= off;
			return *this;
		}

		template <typename T>
		UpdateBuffer&  SetData (const Array<T> &value)
		{
			data = ArrayView{ Cast<uint8_t>(value.data()), value.size()*sizeof(T) };
			return *this;
		}

		template <typename T>
		UpdateBuffer&  SetData (ArrayView<T> value)
		{
			data = ArrayView{ Cast<uint8_t>(value.data()), value.size()*sizeof(T) };
			return *this;
		}

		template <typename T>
		UpdateBuffer&  SetData (const T* ptr, size_t count)
		{
			data = ArrayView{ Cast<uint8_t>(ptr), count*sizeof(T) };
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
		RawBufferID		srcBuffer;
		BytesU			offset;
		BytesU			size;
		Callback_t		callback;

	// methods
		ReadBuffer () :
			BaseTask<ReadBuffer>{ "ReadBuffer", HtmlColor::BlueViolet } {}

		ReadBuffer&  SetBuffer (const BufferID &buf, const BytesU off, const BytesU dataSize)
		{
			ASSERT( buf );
			srcBuffer	= buf.Get();
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
		RawImageID			dstImage;
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
		
		UpdateImage&  SetImage (const ImageID &img, const int2 &offset, MipmapLevel mipmap = Default)
		{
			ASSERT( img );
			dstImage		= img.Get();
			imageOffset		= int3( offset.x, offset.y, 0 );
			mipmapLevel		= mipmap;
			return *this;
		}

		UpdateImage&  SetImage (const ImageID &img, const int3 &offset = Default, MipmapLevel mipmap = Default)
		{
			ASSERT( img );
			dstImage		= img.Get();
			imageOffset		= offset;
			mipmapLevel		= mipmap;
			return *this;
		}

		UpdateImage&  SetImage (const ImageID &img, const int2 &offset, ImageLayer layer, MipmapLevel mipmap)
		{
			ASSERT( img );
			dstImage		= img.Get();
			imageOffset		= int3( offset.x, offset.y, 0 );
			arrayLayer		= layer;
			mipmapLevel		= mipmap;
			return *this;
		}

		UpdateImage&  SetData (ArrayView<uint8_t> value, const uint2 &dimension, BytesU rowPitch = 0_b)
		{
			data			= value;
			imageSize		= uint3( dimension.x, dimension.y, 0 );
			dataRowPitch	= rowPitch;
			return *this;
		}
		
		template <typename T>
		UpdateImage&  SetData (const T* ptr, size_t count, const uint2 &dimension, BytesU rowPitch = 0_b)
		{
			return SetData( ArrayView<uint8_t>{ Cast<uint8_t>(ptr), count*sizeof(T) }, dimension, rowPitch );
		}

		UpdateImage&  SetData (ArrayView<uint8_t> value, const uint3 &dimension, BytesU rowPitch = 0_b, BytesU slicePitch = 0_b)
		{
			data			= value;
			imageSize		= dimension;
			dataRowPitch	= rowPitch;
			dataSlicePitch	= slicePitch;
			return *this;
		}
		
		template <typename T>
		UpdateImage&  SetData (const T* ptr, size_t count, const uint3 &dimension, BytesU rowPitch = 0_b, BytesU slicePitch = 0_b)
		{
			return SetData( ArrayView<uint8_t>{ Cast<uint8_t>(ptr), count*sizeof(T) }, dimension, rowPitch );
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
		RawImageID		srcImage;
		int3			imageOffset;
		uint3			imageSize;
		ImageLayer		arrayLayer;
		MipmapLevel		mipmapLevel;
		EImageAspect	aspectMask	= EImageAspect::Color;	// must only have a single bit set
		Callback_t		callback;

		
	// methods
		ReadImage () :
			BaseTask<ReadImage>{ "ReadImage", HtmlColor::BlueViolet } {}
		
		ReadImage&  SetImage (const ImageID &img, const int2 &offset, const uint2 &size, MipmapLevel mipmap = Default)
		{
			ASSERT( img );
			srcImage	= img.Get();
			imageOffset	= int3( offset.x, offset.y, 0 );
			imageSize	= int3( size.x, size.y, 0 );
			mipmapLevel	= mipmap;
			return *this;
		}

		ReadImage&  SetImage (const ImageID &img, const int3 &offset, const uint3 &size, MipmapLevel mipmap = Default)
		{
			ASSERT( img );
			srcImage	= img.Get();
			imageOffset	= offset;
			imageSize	= size;
			mipmapLevel	= mipmap;
			return *this;
		}
		
		ReadImage&  SetImage (const ImageID &img, const int2 &offset, const uint2 &size, ImageLayer layer, MipmapLevel mipmap = Default)
		{
			ASSERT( img );
			srcImage	= img.Get();
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
		RawImageID		srcImage;
		ImageLayer		layer;


	// methods
		Present () :
			BaseTask<Present>{ "Present", HtmlColor::Red } {}

		explicit Present (const ImageID &img) :
			Present() { srcImage = img.Get(); }

		Present&  SetImage (const ImageID &img, ImageLayer imgLayer = Default)
		{
			ASSERT( img );
			srcImage	= img.Get();
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
		RawImageID		leftEye;
		ImageLayer		leftEyeLayer;

		RawImageID		rightEye;
		ImageLayer		rightEyeLayer;


	// methods
		PresentVR () :
			BaseTask<Present>{ "PresentVR", HtmlColor::Red } {}

		PresentVR&  SetLeftEye (const ImageID &img, ImageLayer imgLayer = Default)
		{
			ASSERT( img );
			leftEye			= img.Get();
			leftEyeLayer	= imgLayer;
			return *this;
		}
		
		PresentVR&  SetRightEye (const ImageID &img, ImageLayer imgLayer = Default)
		{
			ASSERT( img );
			rightEye		= img.Get();
			rightEyeLayer	= imgLayer;
			return *this;
		}
	};



	//
	// Build Ray Tracing Geometry (experimental)
	//
	struct BuildRayTracingGeometry final : _fg_hidden_::BaseTask<BuildRayTracingGeometry>
	{
	// variables

	// methods
		BuildRayTracingGeometry () :
			BaseTask<BuildRayTracingGeometry>{ "BuildRayTracingGeometry", HtmlColor::Lime } {}
	};



	//
	// Build Ray Tracing Scene (experimental)
	//
	struct BuildRayTracingScene final : _fg_hidden_::BaseTask<BuildRayTracingScene>
	{
	// variables

	// methods
		BuildRayTracingScene () :
			BaseTask<BuildRayTracingScene>{ "BuildRayTracingScene", HtmlColor::Lime } {}
	};



	//
	// Trace Rays (experimental)
	//
	struct TraceRays final : _fg_hidden_::BaseTask<TraceRays>
	{
	// variables
		RawRTPipelineID			pipeline;
		PipelineResourceSet		resources;
		uint3					groupCount;


	// methods
		TraceRays () :
			BaseTask<TraceRays>{ "TraceRays", HtmlColor::Lime } {}
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
