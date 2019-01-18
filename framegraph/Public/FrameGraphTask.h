// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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


		//
		// Compute Shader Debug Mode
		//
		struct ComputeShaderDebugMode
		{
			EShaderDebugMode	mode	= Default;
			uint3				globalID;
		};

		//
		// Ray Tracing Shader Debug Mode
		//
		struct RayTracingShaderDebugMode
		{
		// types
			struct RayGen {
				uint	launchX;	// gl_LaunchIDNV.x
				uint	launchY;	// gl_LaunchIDNV.y
				uint	launchZ;	// gl_LaunchIDNV.z
			};

		// variables
			EShaderDebugMode	mode	= Default;
			EShader				shader	= Default;
			union {
				RayGen		raygen;
			};
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
		using DebugMode			= _fg_hidden_::ComputeShaderDebugMode;
		
		struct ComputeCmd
		{
			uint3	baseGroup;
			uint3	groupCount;
		};
		using ComputeCmds_t		= FixedArray< ComputeCmd, 16 >;


	// variables
		RawCPipelineID			pipeline;
		PipelineResourceSet		resources;
		ComputeCmds_t			commands;
		Optional< uint3 >		localGroupSize;
		PushConstants_t			pushConstants;
		DebugMode				debugMode;
		

	// methods
		DispatchCompute () :
			BaseTask<DispatchCompute>{ "DispatchCompute", HtmlColor::MediumBlue } {}
		
		DispatchCompute&  SetPipeline (const CPipelineID &ppln)				{ ASSERT( ppln );  pipeline = ppln.Get();  return *this; }
		
		DispatchCompute&  Dispatch (const uint2 &off, const uint2 &count)	{ commands.emplace_back(uint3{off, 0}, uint3{count, 1});  return *this; }
		DispatchCompute&  Dispatch (const uint3 &off, const uint3 &count)	{ commands.emplace_back(off, count);  return *this; }
		DispatchCompute&  Dispatch (const uint2 &count)						{ commands.emplace_back(uint3{0}, uint3{count, 1});  return *this; }
		DispatchCompute&  Dispatch (const uint3 &count)						{ commands.emplace_back(uint3{0}, count);  return *this; }
		
		DispatchCompute&  SetLocalSize (const uint2 &value)					{ localGroupSize = {value.x, value.y, 1};  return *this; }
		DispatchCompute&  SetLocalSize (const uint3 &value)					{ localGroupSize = value;  return *this; }
		DispatchCompute&  SetLocalSize (uint x, uint y = 1, uint z = 1)		{ localGroupSize = {x, y, z};  return *this; }

		DispatchCompute&  EnableDebugTrace (const uint3 &globalID)
		{
			debugMode.mode		= EShaderDebugMode::Trace;
			debugMode.globalID	= globalID;
			return *this;
		}
		
		DispatchCompute&  AddResources (const DescriptorSetID &id, const PipelineResources *res)
		{
			ASSERT( id.IsDefined() and res );
			resources.insert({ id, res });
			return *this;
		}

		template <typename ValueType>
		DispatchCompute&  AddPushConstant (const PushConstantID &id, const ValueType &value) { return AddPushConstant( id, AddressOf(value), SizeOf<ValueType> ); }

		DispatchCompute&  AddPushConstant (const PushConstantID &id, const void *ptr, BytesU size)
		{
			ASSERT( id.IsDefined() );
			auto&	pc = pushConstants.emplace_back();
			pc.id = id;
			pc.size = Bytes<uint16_t>{size};
			MemCopy( pc.data, BytesU::SizeOf(pc.data), ptr, size );
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
		using DebugMode			= _fg_hidden_::ComputeShaderDebugMode;
		
		struct ComputeCmd
		{
			BytesU		indirectBufferOffset;
		};
		using ComputeCmds_t		= FixedArray< ComputeCmd, 16 >;

		struct DispatchIndirectCommand
		{
			uint3		groudCount;
		};


	// variables
		RawCPipelineID			pipeline;
		PipelineResourceSet		resources;
		ComputeCmds_t			commands;
		RawBufferID				indirectBuffer;
		Optional< uint3 >		localGroupSize;
		PushConstants_t			pushConstants;
		DebugMode				debugMode;
		

	// methods
		DispatchComputeIndirect () :
			BaseTask<DispatchComputeIndirect>{ "DispatchComputeIndirect", HtmlColor::MediumBlue } {}
		
		DispatchComputeIndirect&  SetLocalSize (const uint3 &value)					{ localGroupSize = value;  return *this; }
		DispatchComputeIndirect&  SetLocalSize (uint x, uint y = 1, uint z = 1)		{ localGroupSize = {x, y, z};  return *this; }
		
		DispatchComputeIndirect&  Dispatch (BytesU offset)							{ commands.push_back({ offset });  return *this; }

		DispatchComputeIndirect&  EnableDebugTrace (const uint3 &globalID)
		{
			debugMode.mode		= EShaderDebugMode::Trace;
			debugMode.globalID	= globalID;
			return *this;
		}

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
		
		DispatchComputeIndirect&  AddResources (const DescriptorSetID &id, const PipelineResources *res)
		{
			ASSERT( id.IsDefined() and res );
			resources.insert({ id, res });
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
			auto&	pc = pushConstants.emplace_back();
			pc.id = id;
			pc.size = Bytes<uint16_t>{size};
			MemCopy( pc.data, BytesU::SizeOf(pc.data), ptr, size );
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
		
		FillBuffer&  SetBuffer (const BufferID &buf)
		{
			return SetBuffer( buf, 0_b, ~0_b );
		}

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
		using ClearColor_t	= Union< std::monostate, RGBA32f, RGBA32u, RGBA32i >;


	// variables
		RawImageID			dstImage;
		Ranges_t			ranges;
		ClearColor_t		clearValue;
		
		
	// methods
		ClearColorImage () :
			BaseTask<ClearColorImage>{ "ClearColorImage", HtmlColor::Green } {}

		ClearColorImage&  SetImage (const ImageID &img)		{ ASSERT( img );  dstImage = img.Get();  return *this; }

		ClearColorImage&  Clear (const RGBA32f &value)		{ clearValue = value;  return *this; }
		ClearColorImage&  Clear (const RGBA32u &value)		{ clearValue = value;  return *this; }
		ClearColorImage&  Clear (const RGBA32i &value)		{ clearValue = value;  return *this; }
		
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

		ClearDepthStencilImage&  Clear (float depth, uint stencil = 0)	{ clearValue = DepthStencil{ depth, stencil };  return *this; }
		
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
	// types
		struct Region
		{
			BytesU				offset;
			ArrayView<uint8_t>	data;
		};
		using Regions_t	= FixedArray< Region, FG_MaxCopyRegions >;


	// variables
		RawBufferID		dstBuffer;		// if buffer has gpu local memory, then staging buffer will be used
		Regions_t		regions;


	// methods
		UpdateBuffer () :
			BaseTask<UpdateBuffer>{ "UpdateBuffer", HtmlColor::BlueViolet } {}

		UpdateBuffer (const BufferID &buf, BytesU off, ArrayView<uint8_t> data) :
			UpdateBuffer() { SetBuffer( buf ).AddData( data, off ); }

		UpdateBuffer&  SetBuffer (const BufferID &buf)
		{
			ASSERT( buf );
			dstBuffer = buf.Get();
			return *this;
		}

		template <typename T>
		UpdateBuffer&  AddData (const Array<T> &value, BytesU bufferOffset = 0_b)
		{
			regions.emplace_back( bufferOffset, ArrayView{ Cast<uint8_t>(value.data()), value.size()*sizeof(T) });
			return *this;
		}

		template <typename T>
		UpdateBuffer&  AddData (ArrayView<T> value, BytesU bufferOffset = 0_b)
		{
			regions.emplace_back( bufferOffset, ArrayView{ Cast<uint8_t>(value.data()), value.size()*sizeof(T) });
			return *this;
		}

		template <typename T>
		UpdateBuffer&  AddData (const T* ptr, size_t count, BytesU bufferOffset = 0_b)
		{
			regions.emplace_back( bufferOffset, ArrayView{ Cast<uint8_t>(ptr), count*sizeof(T) });
			return *this;
		}

		template <typename T>
		UpdateBuffer&  AddData (const void* ptr, BytesU size, BytesU bufferOffset = 0_b)
		{
			regions.emplace_back( bufferOffset, ArrayView{ Cast<uint8_t>(ptr), size_t(size) });
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
			return SetData( ArrayView<uint8_t>{ Cast<uint8_t>(ptr), count*sizeof(T) }, dimension, rowPitch, slicePitch );
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
			imageSize	= uint3( size.x, size.y, 0 );
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
			imageSize	= uint3( size.x, size.y, 0 );
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
	// Build Ray Tracing Geometry
	//
	struct BuildRayTracingGeometry final : _fg_hidden_::BaseTask<BuildRayTracingGeometry>
	{
	// types
		using EFlags	= ERayTracingGeometryFlags;
		using Matrix3x4	= Matrix< float, 3, 4, EMatrixOrder::RowMajor >;

		struct Triangles
		{
		// variables
			GeometryID			geometryId;
			EFlags				flags				= Default;

			ArrayView<uint8_t>	vertexData;
			RawBufferID			vertexBuffer;
			BytesU				vertexOffset;
			Bytes<uint>			vertexStride;
			uint				vertexCount			= 0;
			EVertexType			vertexFormat		= Default;

			// optional:
			ArrayView<uint8_t>	indexData;
			RawBufferID			indexBuffer;
			BytesU				indexOffset;
			uint				indexCount			= 0;
			EIndex				indexType			= Default;

			// optional:
			Optional<Matrix3x4>	transformData;
			RawBufferID			transformBuffer;	// 3x4 row major affine transformation matrix for this geometry.
			BytesU				transformOffset;

		// methods
			Triangles () {}
			explicit Triangles (const GeometryID &id) : geometryId{id} {}

			template <typename T, typename Idx>
			Triangles&  SetVertices (Idx count, BytesU stride = 0_b);
			template <typename Idx>
			Triangles&  SetVertices (Idx count, EVertexType format, BytesU stride = 0_b);
			Triangles&  SetVertexBuffer (const BufferID &id, BytesU offset = 0_b);
			Triangles&  SetVertexData (ArrayView<uint8_t> data);
			template <typename VertexT>
			Triangles&  SetVertices (ArrayView<VertexT> vertices);
			template <typename Idx>
			Triangles&  SetIndices (Idx count, EIndex type);
			template <typename IndexT>
			Triangles&  SetIndices (ArrayView<IndexT> indices);
			Triangles&  SetIndexBuffer (const BufferID &id, BytesU offset = 0_b);
			Triangles&  SetIndexData (ArrayView<uint8_t> data);
			Triangles&  SetTransform (const BufferID &id, BytesU offset = 0_b);
			Triangles&  SetTransform (const Matrix3x4 &mat);
			Triangles&  AddFlags (EFlags value)					{ flags |= value;  return *this; }
			Triangles&  SetID (const GeometryID &id)			{ geometryId = id;  return *this; }
		};


		struct AABBData
		{
			float3		min;
			float3		max;
		};

		struct AABB
		{
		// variables
			GeometryID			geometryId;
			EFlags				flags			= Default;

			ArrayView<uint8_t>	aabbData;
			RawBufferID			aabbBuffer;		// array of 'AABBData'
			BytesU				aabbOffset;
			Bytes<uint>			aabbStride;
			uint				aabbCount		= 0;

		// methods
			AABB () {}
			explicit AABB (const GeometryID &id) : geometryId{id} {}

			template <typename Idx>
			AABB&  SetCount (Idx count, BytesU stride = 0_b);
			AABB&  SetBuffer (const BufferID &id, BytesU offset = 0_b);
			AABB&  SetData (ArrayView<uint8_t> data);
			AABB&  AddFlags (EFlags value)						{ flags |= value;  return *this; }
			AABB&  SetID (const GeometryID &id)					{ geometryId = id;  return *this; }
		};


	// variables
		RawRTGeometryID			rtGeometry;
		Array< Triangles >		triangles;
		Array< AABB >			aabbs;


	// methods
		BuildRayTracingGeometry () :
			BaseTask<BuildRayTracingGeometry>{ "BuildRayTracingGeometry", HtmlColor::Lime } {}

		BuildRayTracingGeometry&  SetTarget (const RTGeometryID &id)	{ ASSERT( id );  rtGeometry = id.Get();  return *this; }

		BuildRayTracingGeometry&  Add (const Triangles &value)			{ triangles.push_back( value );  return *this; }
		BuildRayTracingGeometry&  Add (const AABB &value)				{ aabbs.push_back( value );  return *this; }
	};



	//
	// Build Ray Tracing Scene
	//
	struct BuildRayTracingScene final : _fg_hidden_::BaseTask<BuildRayTracingScene>
	{
	// types
		using Matrix4x3		= Matrix< float, 4, 3, EMatrixOrder::RowMajor >;
		using EFlags		= ERayTracingInstanceFlags;

		struct Instance
		{
		// variables
			RawRTGeometryID		geometryId;
			Matrix4x3			transform		= Matrix4x3::Identity();
			uint				instanceID		= 0;
			EFlags				flags			= Default;
			uint8_t				mask			= 0xFF;

		// methods
			Instance () {}

			Instance&  SetGeometry (const RTGeometryID &id);
			Instance&  SetInstance (const Matrix4x3 &transform, uint id);
			Instance&  AddFlags (EFlags value)			{ flags |= value;  return *this; }
			Instance&  SetMask (uint8_t value)			{ mask   = value;  return *this; }
		};


	// variables
		RawRTSceneID		rtScene;
		Array< Instance >	instances;
		uint				hitShadersPerGeometry = 1;		// same as 'sbtRecordStride' in ray gen shader


	// methods
		BuildRayTracingScene () :
			BaseTask<BuildRayTracingScene>{ "BuildRayTracingScene", HtmlColor::Lime } {}

		BuildRayTracingScene&  SetTarget (const RTSceneID &id)			{ ASSERT( id );  rtScene = id.Get();  return *this; }
		BuildRayTracingScene&  SetHitShadersPerGeometry (uint count)	{ ASSERT( count > 0 );  hitShadersPerGeometry = count;  return *this; }
		BuildRayTracingScene&  Add (const Instance &value)				{ instances.push_back( value );  return *this; }
	};



	//
	// Update Ray Tracing Shader Table
	//
	struct UpdateRayTracingShaderTable final : _fg_hidden_::BaseTask<UpdateRayTracingShaderTable>
	{
	// types
		struct ShaderTable
		{
			RawBufferID			buffer;
			//BytesU			bufferSize;
			
			BytesU				rayGenOffset;
			BytesU				rayMissOffset;
			BytesU				rayHitOffset;
			BytesU				callableOffset;

			Bytes<uint16_t>		rayMissStride;
			Bytes<uint16_t>		rayHitStride;
			Bytes<uint16_t>		callableStride;

			uint16_t			maxMissShaders;
			//uint16_t			hitShadersPerGeometry;	// in ray gen shader the 'sbtRecordStride' parameter of 'traceNV()' must equal to 'hitShadersPerGeometry'
			//											// and 'sbtRecordOffset' must be less then 'hitShadersPerGeometry'
		};
		
		struct RayGenShaderGroup
		{
			RTShaderGroupID		groupId;
		};

		struct MissShaderGroup
		{
			RTShaderGroupID		groupId;
		};

		struct HitShaderGroup
		{
			RTShaderGroupID		groupId;
			GeometryID			geometryId;
			uint				instance	= 0;
		};
		
		using MissShaderGroups_t	= Array< MissShaderGroup >;
		using HitShaderGroups_t		= Array< HitShaderGroup >;


	// variables
		ShaderTable &			result;
		RawRTPipelineID			pipeline;
		RawRTSceneID			rtScene;
		RawBufferID				dstBuffer;
		BytesU					dstOffset;
		RayGenShaderGroup		rayGenShader;
		MissShaderGroups_t		missShaders;
		HitShaderGroups_t		hitShaders;
		//ShaderGroups_t		callableShaders;


	// methods
		UpdateRayTracingShaderTable (OUT ShaderTable &result) :
			BaseTask<UpdateRayTracingShaderTable>{ "UpdateRayTracingShaderTable", HtmlColor::BlueViolet },
			result{result} {}

		UpdateRayTracingShaderTable&  SetPipeline (const RTPipelineID &ppln)
		{
			ASSERT( ppln );
			pipeline = ppln.Get();
			return *this;
		}

		UpdateRayTracingShaderTable&  SetScene (const RTSceneID &scene)
		{
			ASSERT( scene );
			rtScene = scene.Get();
			return *this;
		}
		
		UpdateRayTracingShaderTable&  SetBuffer (const BufferID &buffer, BytesU offset = 0_b)
		{
			ASSERT( buffer );
			dstBuffer	= buffer.Get();
			dstOffset	= offset;
			return *this;
		}

		UpdateRayTracingShaderTable&  SetRayGenShader (const RTShaderGroupID &group)
		{
			ASSERT( group.IsDefined() );
			rayGenShader = RayGenShaderGroup{ group };
			return *this;
		}

		UpdateRayTracingShaderTable&  AddMissShader (const RTShaderGroupID &group)
		{
			ASSERT( group.IsDefined() );
			missShaders.push_back(MissShaderGroup{ group });
			return *this;
		}

		UpdateRayTracingShaderTable&  AddHitShader (const GeometryID &geom, const RTShaderGroupID &group, uint instance = 0)
		{
			ASSERT( geom.IsDefined() );
			ASSERT( group.IsDefined() );
			hitShaders.push_back(HitShaderGroup{ group, geom, instance });
			return *this;
		}
	};



	//
	// Trace Rays
	//
	struct TraceRays final : _fg_hidden_::BaseTask<TraceRays>
	{
	// types
		using PushConstants_t	= _fg_hidden_::PushConstants_t;
		using ShaderTable		= UpdateRayTracingShaderTable::ShaderTable;
		using DebugMode			= _fg_hidden_::RayTracingShaderDebugMode;


	// variables
		RawRTPipelineID			pipeline;
		PipelineResourceSet		resources;
		uint3					groupCount;
		PushConstants_t			pushConstants;
		ShaderTable				shaderTable;
		DebugMode				debugMode;


	// methods
		TraceRays () :
			BaseTask<TraceRays>{ "TraceRays", HtmlColor::Lime } {}
		
		TraceRays&  SetPipeline (const RTPipelineID &ppln)							{ ASSERT( ppln );  pipeline = ppln.Get();  return *this; }
		
		TraceRays&  SetGroupCount (const uint3 &value)								{ groupCount = value;  return *this; }
		TraceRays&  SetGroupCount (uint x, uint y = 1, uint z = 1)					{ groupCount = {x, y, z};  return *this; }
		
		TraceRays&  AddResources (const DescriptorSetID &id, const PipelineResources *res)
		{
			ASSERT( id.IsDefined() and res );
			resources.insert({ id, res });
			return *this;
		}

		TraceRays&  SetShaderTable (const ShaderTable &value)
		{
			ASSERT( value.buffer );
			ASSERT( value.rayMissStride > 0 and value.rayHitStride > 0 );
			shaderTable = value;
			return *this;
		}

		template <typename ValueType>
		TraceRays&  AddPushConstant (const PushConstantID &id, const ValueType &value)
		{
			return AddPushConstant( id, AddressOf(value), SizeOf<ValueType> );
		}

		TraceRays&  AddPushConstant (const PushConstantID &id, const void *ptr, BytesU size);

		TraceRays&  EnableRayGenDebugTrace (const uint3 &launchID);
	};
//-----------------------------------------------------------------------------

	
	
	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetVertexBuffer (const BufferID &id, BytesU offset)
	{
		ASSERT( vertexData.empty() );
		vertexBuffer	= id.Get();
		vertexOffset	= offset;
		return *this;
	}
	
	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetVertexData (ArrayView<uint8_t> data)
	{
		ASSERT( not vertexBuffer );
		vertexData	= data;
		return *this;
	}
	
	template <typename VertexT>
	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetVertices (ArrayView<VertexT> vertices)
	{
		ASSERT( vertices.size() );
		vertexBuffer	= Default;
		vertexData		= ArrayView<uint8_t>{ Cast<uint8_t>(vertices.data()), vertices.size() * sizeof(VertexT) };
		vertexCount		= uint(vertices.size());
		vertexFormat	= VertexDesc<VertexT>::attrib;
		vertexStride	= Bytes<uint>::SizeOf<VertexT>();
		return *this;
	}

	template <typename T, typename Idx>
	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetVertices (Idx count, BytesU stride)
	{
		STATIC_ASSERT( IsInteger<Idx> );
		vertexCount		= uint(count);
		vertexStride	= Bytes<uint>{ uint(stride) };
		vertexFormat	= VertexDesc<T>::attrib;
		return *this;
	}
	
	template <typename Idx>
	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetVertices (Idx count, EVertexType format, BytesU stride)
	{
		STATIC_ASSERT( IsInteger<Idx> );
		vertexCount		= uint(count);
		vertexStride	= Bytes<uint>{ uint(stride) };
		vertexFormat	= format;
		return *this;
	}

	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetIndexBuffer (const BufferID &id, BytesU offset)
	{
		ASSERT( indexData.empty() );
		indexBuffer	= id.Get();
		indexOffset	= offset;
		return *this;
	}
	
	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetIndexData (ArrayView<uint8_t> data)
	{
		ASSERT( not indexBuffer );
		indexData	= data;
		return *this;
	}

	template <typename Idx>
	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetIndices (Idx count, EIndex type)
	{
		STATIC_ASSERT( IsInteger<Idx> );
		indexCount	= uint(count);
		indexType	= type;
		return *this;
	}
	
	template <typename IndexT>
	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetIndices (ArrayView<IndexT> indices)
	{
		ASSERT( indices.size() );
		indexBuffer	= Default;
		indexData	= ArrayView<uint8_t>{ Cast<uint8_t>(indices.data()), indices.size() * sizeof(IndexT) };
		indexCount	= uint(indices.size());
		indexType	= (IsSameTypes<IndexT, uint32_t> ? EIndex::UInt : (IsSameTypes<IndexT, uint16_t> ? EIndex::UShort : EIndex::Unknown));
		return *this;
	}

	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetTransform (const BufferID &id, BytesU offset)
	{
		ASSERT( not transformData.has_value() );
		transformBuffer	= id.Get();
		transformOffset	= offset;
		return *this;
	}

	inline BuildRayTracingGeometry::Triangles&
		BuildRayTracingGeometry::Triangles::SetTransform (const Matrix3x4 &mat)
	{
		ASSERT( not transformBuffer );
		transformData	= mat;
		return *this;
	}
//-----------------------------------------------------------------------------

	
	inline BuildRayTracingGeometry::AABB&
		BuildRayTracingGeometry::AABB::SetBuffer (const BufferID &id, BytesU offset)
	{
		ASSERT( aabbData.empty() );
		aabbBuffer	= id.Get();
		aabbOffset	= offset;
		return *this;
	}
	
	inline BuildRayTracingGeometry::AABB&
		BuildRayTracingGeometry::AABB::SetData (ArrayView<uint8_t> data)
	{
		ASSERT( not aabbBuffer );
		aabbData	= data;
		return *this;
	}

	template <typename Idx>
	inline BuildRayTracingGeometry::AABB&
		BuildRayTracingGeometry::AABB::SetCount (Idx count, BytesU stride)
	{
		STATIC_ASSERT( IsInteger<Idx> );
		aabbCount	= uint(count);
		aabbStride	= Bytes<uint>{ uint(stride) };
		return *this;
	}
//-----------------------------------------------------------------------------
	

	inline BuildRayTracingScene::Instance&
		BuildRayTracingScene::Instance::SetGeometry (const RTGeometryID &id)
	{
		geometryId = id.Get();
		return *this;
	}

	inline BuildRayTracingScene::Instance&
		BuildRayTracingScene::Instance::SetInstance (const Matrix4x3 &mat, uint id)
	{
		this->transform	 = mat;
		this->instanceID = id;
		return *this;
	}
//-----------------------------------------------------------------------------
	

	inline TraceRays&  TraceRays::AddPushConstant (const PushConstantID &id, const void *ptr, BytesU size)
	{
		ASSERT( id.IsDefined() );
		auto&	pc = pushConstants.emplace_back();
		pc.id = id;
		pc.size = Bytes<uint16_t>{size};
		MemCopy( pc.data, BytesU::SizeOf(pc.data), ptr, size );
		return *this;
	}

	inline TraceRays&  TraceRays::EnableRayGenDebugTrace (const uint3 &launchID)
	{
		debugMode.mode		= EShaderDebugMode::Trace;
		debugMode.shader	= EShader::RayGen;
		debugMode.raygen	= { launchID.x, launchID.y, launchID.z };
		return *this;
	}


}	// FG
