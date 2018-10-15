// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/PoolAllocator.h"
#include "framegraph/Public/FrameGraph.h"
#include "framegraph/Shared/EnumUtils.h"
#include "VEnumCast.h"
#include "VPipelineResources.h"
#include "VLogicalRenderPass.h"
#include "VPipeline.h"

namespace FG
{
	template <typename TaskType>
	class VFgTask;


	//
	// Task interface
	//

	class IFrameGraphTask
	{
	// types
	protected:
		using Dependencies_t	= FixedArray< Task, FG_MaxTaskDependencies >;
		using Name_t			= SubmitRenderPass::TaskName_t;
		using ProcessFunc_t		= void (*) (void *visitor, const void *taskData);


	// variables
	protected:
		ProcessFunc_t	_processFunc	= null;
		Dependencies_t	_inputs;
		Dependencies_t	_outputs;
		Name_t			_taskName;
		RGBA8u			_debugColor;
		uint			_visitorID		= 0;
		ExeOrderIndex	_exeOrderIdx	= ExeOrderIndex::Initial;


	// methods
	protected:
		IFrameGraphTask ()
		{}

		template <typename T>
		explicit IFrameGraphTask (const _fg_hidden_::BaseTask<T> &task, ProcessFunc_t process) :
			_processFunc{ process },
			_inputs{ task.depends },
			_taskName{ task.taskName },
			_debugColor{ task.debugColor }
		{
			// validate dependencies
			DEBUG_ONLY(
				for (auto& dep : task.depends) {
					ASSERT( dep != null );
				}
			)
		}

	public:
		ND_ StringView							Name ()				const	{ return _taskName; }
		ND_ RGBA8u								DebugColor ()		const	{ return _debugColor; }
		ND_ uint								VisitorID ()		const	{ return _visitorID; }
		ND_ ExeOrderIndex						ExecutionOrder ()	const	{ return _exeOrderIdx; }

		ND_ ArrayView<Task>						Inputs ()			const	{ return _inputs; }
		ND_ ArrayView<Task>						Outputs ()			const	{ return _outputs; }

		ND_ virtual VFgTask<SubmitRenderPass>*	GetSubmitRenderPassTask ()	{ return null; }

		void Attach (Task output)					{ _outputs.push_back( output ); }
		void SetVisitorID (uint id)					{ _visitorID = id; }
		void SetExecutionOrder (ExeOrderIndex idx)	{ _exeOrderIdx = idx; }

		void Process (void *visitor)		const	{ ASSERT( _processFunc );  _processFunc( visitor, this ); }


	// interface
	public:
		virtual ~IFrameGraphTask () {}
	};



	//
	// Empty Task
	//
	class EmptyTask final : public IFrameGraphTask
	{
	// methods
	public:
		EmptyTask () : IFrameGraphTask{}
		{
			_taskName = "Begin";
		}
	};



	//
	// Submit Render Pass
	//
	template <>
	class VFgTask< SubmitRenderPass > final : public IFrameGraphTask
	{
	// types
	private:
		using Self	= VFgTask<SubmitRenderPass>;

	// variables
	private:
		VLogicalRenderPass *	_renderPass		= null;
		Self *					_prevSubpass	= null;
		Self *					_nextSubpass	= null;


	// methods
	public:
		VFgTask (const SubmitRenderPass &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_renderPass{ static_cast<VLogicalRenderPass *>(const_cast<RenderPass>(task.renderPass)) }
		{}

		ND_ VLogicalRenderPass *		GetLogicalPass ()	const	{ return _renderPass; }

		ND_ Self const *				GetPrevSubpass ()	const	{ return _prevSubpass; }
		ND_ Self const *				GetNextSubpass ()	const	{ return _nextSubpass; }

		ND_ bool						IsSubpass ()		const	{ return _prevSubpass != null; }
		ND_ bool						IsLastSubpass ()	const	{ return _nextSubpass == null; }
		
		ND_ VFgTask<SubmitRenderPass>*	GetSubmitRenderPassTask ()	{ return this; }
	};



	//
	// Dispatch Compute
	//
	template <>
	class VFgTask< DispatchCompute > final : public IFrameGraphTask
	{
	// variables
	private:
		VComputePipelinePtr			_pipeline;
		VPipelineResourceSet		_resources;
		uint3						_groupCount;
		Optional< uint3 >			_localGroupSize;


	// methods
	public:
		VFgTask (const DispatchCompute &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_pipeline{ Cast<VComputePipeline>( task.pipeline )},
			_groupCount{ Max( task.groupCount, 1u ) },
			_localGroupSize{ task.localGroupSize }
		{
			for (const auto& res : task.resources) {
				_resources.insert({ res.first, Cast<VPipelineResources>(res.second) });
			}
		}

		ND_ VComputePipelinePtr const&		GetPipeline ()		const	{ return _pipeline; }
		ND_ VPipelineResourceSet const&		GetResources ()		const	{ return _resources; }

		ND_ uint3 const&					GroupCount ()		const	{ return _groupCount; }
		ND_ Optional< uint3 > const&		LocalSize ()		const	{ return _localGroupSize; }
	};



	//
	// Dispatch Indirect Compute
	//
	template <>
	class VFgTask< DispatchIndirectCompute > final : public IFrameGraphTask
	{
	// variables
	private:
		VComputePipelinePtr			_pipeline;
		VPipelineResourceSet		_resources;
		BufferPtr					_indirectBuffer;
		VkDeviceSize				_indirectBufferOffset;
		Optional< uint3 >			_localGroupSize;


	// methods
	public:
		VFgTask (const DispatchIndirectCompute &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_pipeline{ Cast<VComputePipeline>( task.pipeline )},
			_indirectBuffer{ task.indirectBuffer },
			_indirectBufferOffset{ VkDeviceSize(task.indirectBufferOffset) },
			_localGroupSize{ task.localGroupSize }
		{
			for (const auto& res : task.resources) {
				_resources.insert({ res.first, Cast<VPipelineResources>(res.second) });
			}
		}
		
		ND_ VComputePipelinePtr const&		GetPipeline ()			const	{ return _pipeline; }
		ND_ VPipelineResourceSet const&		GetResources ()			const	{ return _resources; }

		ND_ BufferPtr const&				IndirectBuffer ()		const	{ return _indirectBuffer; }
		ND_ VkDeviceSize					IndirectBufferOffset ()	const	{ return _indirectBufferOffset; }

		ND_ Optional< uint3 > const&		LocalSize ()			const	{ return _localGroupSize; }
	};



	//
	// Copy Buffer
	//
	template <>
	class VFgTask< CopyBuffer > final : public IFrameGraphTask
	{
	// types
	private:
		using Regions_t	= FixedArray< VkBufferCopy, FG_MaxCopyRegions >;


	// variables
	private:
		BufferPtr		_srcBuffer;
		BufferPtr		_dstBuffer;
		Regions_t		_regions;


	// methods
	public:
		VFgTask (const CopyBuffer &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_srcBuffer{ task.srcBuffer },
			_dstBuffer{ task.dstBuffer }
		{	
			for (auto& src : task.regions)
			{
				VkBufferCopy	dst = {};

				dst.srcOffset	= VkDeviceSize( src.srcOffset );
				dst.dstOffset	= VkDeviceSize( src.dstOffset );
				dst.size		= VkDeviceSize( src.size );

				_regions.push_back( dst );

				// TODO: check buffer intersection
				ASSERT( src.size + src.srcOffset <= _srcBuffer->Description().size );
				ASSERT( src.size + src.dstOffset <= _dstBuffer->Description().size );
			}
		}

		ND_ BufferPtr const&			SrcBuffer ()	const	{ return _srcBuffer; }
		ND_ BufferPtr const&			DstBuffer ()	const	{ return _dstBuffer; }
		ND_ ArrayView< VkBufferCopy >	Regions ()		const	{ return _regions; }
	};



	//
	// Copy Image
	//
	template <>
	class VFgTask< CopyImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Regions_t	= FixedArray< VkImageCopy, FG_MaxCopyRegions >;


	// variables
	private:
		ImagePtr				_srcImage;
		const VkImageLayout		_srcLayout;
		ImagePtr				_dstImage;
		const VkImageLayout		_dstLayout;
		Regions_t				_regions;


	// methods
	public:
		VFgTask (const CopyImage &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_srcImage{ task.srcImage },
			_srcLayout{ _srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
			_dstImage{ task.dstImage },
			_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL }
		{
			ASSERT( not _dstImage->IsImmutable() );

			for (auto& src : task.regions)
			{
				VkImageCopy		dst					= {};
				const uint3		image_size			= Max( src.size, 1u );

				dst.srcSubresource.aspectMask		= VEnumCast( src.srcSubresource.aspectMask, _srcImage->PixelFormat() );
				dst.srcSubresource.mipLevel			= src.srcSubresource.mipLevel.Get();
				dst.srcSubresource.baseArrayLayer	= src.srcSubresource.baseLayer.Get();
				dst.srcSubresource.layerCount		= src.srcSubresource.layerCount;
				dst.srcOffset						= VkOffset3D{ src.srcOffset.x, src.srcOffset.y, src.srcOffset.z };

				dst.dstSubresource.aspectMask		= VEnumCast( src.dstSubresource.aspectMask, _dstImage->PixelFormat() );
				dst.dstSubresource.mipLevel			= src.dstSubresource.mipLevel.Get();
				dst.dstSubresource.baseArrayLayer	= src.dstSubresource.baseLayer.Get();
				dst.dstSubresource.layerCount		= src.dstSubresource.layerCount;
				dst.dstOffset						= VkOffset3D{ src.dstOffset.x, src.dstOffset.y, src.dstOffset.z };

				dst.extent							= VkExtent3D{ image_size.x, image_size.y, image_size.z };

				ASSERT( src.srcSubresource.mipLevel < _srcImage->Description().maxLevel );
				ASSERT( src.dstSubresource.mipLevel < _dstImage->Description().maxLevel );
				ASSERT( src.srcSubresource.baseLayer.Get() + src.srcSubresource.layerCount <= _srcImage->ArrayLayers() );
				ASSERT( src.srcSubresource.baseLayer.Get() + src.dstSubresource.layerCount <= _dstImage->ArrayLayers() );
				//ASSERT(All( src.srcOffset + src.size <= Max(1u, _srcImage->Description().dimension.xyz() >> src.srcSubresource.mipLevel.Get()) ));
				//ASSERT(All( src.dstOffset + src.size <= Max(1u, _dstImage->Description().dimension.xyz() >> src.dstSubresource.mipLevel.Get()) ));

				_regions.push_back( dst );
			}
		}

		ND_ ImagePtr const&				SrcImage ()		const	{ return _srcImage; }
		ND_ VkImageLayout				SrcLayout ()	const	{ return _srcLayout; }

		ND_ ImagePtr const&				DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout				DstLayout ()	const	{ return _dstLayout; }

		ND_ ArrayView< VkImageCopy >	Regions ()		const	{ return _regions; }
	};



	//
	// Copy Buffer to Image
	//
	template <>
	class VFgTask< CopyBufferToImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Regions_t	= FixedArray< VkBufferImageCopy, FG_MaxCopyRegions >;


	// variables
	private:
		BufferPtr				_srcBuffer;
		ImagePtr				_dstImage;
		const VkImageLayout		_dstLayout;
		Regions_t				_regions;


	// methods
	public:
		VFgTask (const CopyBufferToImage &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_srcBuffer{ task.srcBuffer },
			_dstImage{ task.dstImage },
			_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL }
		{
			ASSERT( not _dstImage->IsImmutable() );

			for (auto& src : task.regions)
			{
				VkBufferImageCopy	dst			= {};
				const int3			img_offset	= int3(src.imageOffset);
				const uint3			img_size	= Max( src.imageSize, 1u );

				dst.bufferOffset					= VkDeviceSize( src.bufferOffset );
				dst.bufferRowLength					= src.bufferRowLength;
				dst.bufferImageHeight				= src.bufferImageHeight;

				dst.imageSubresource.aspectMask		= VEnumCast( src.imageLayers.aspectMask, _dstImage->PixelFormat() );
				dst.imageSubresource.mipLevel		= src.imageLayers.mipLevel.Get();
				dst.imageSubresource.baseArrayLayer	= src.imageLayers.baseLayer.Get();
				dst.imageSubresource.layerCount		= src.imageLayers.layerCount;
				dst.imageOffset						= VkOffset3D{ img_offset.x, img_offset.y, img_offset.z };
				dst.imageExtent						= VkExtent3D{ img_size.x, img_size.y, img_size.z };

				_regions.push_back( dst );
			}
		}

		ND_ BufferPtr const&				SrcBuffer ()	const	{ return _srcBuffer; }

		ND_ ImagePtr const&					DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout					DstLayout ()	const	{ return _dstLayout; }

		ND_ ArrayView< VkBufferImageCopy >	Regions ()		const	{ return _regions; }
	};



	//
	// Copy Image to Buffer
	//
	template <>
	class VFgTask< CopyImageToBuffer > final : public IFrameGraphTask
	{
	// types
	private:
		using Regions_t	= FixedArray< VkBufferImageCopy, FG_MaxCopyRegions >;


	// variables
	private:
		ImagePtr				_srcImage;
		const VkImageLayout		_srcLayout;
		BufferPtr				_dstBuffer;
		Regions_t				_regions;


	// methods
	public:
		VFgTask (const CopyImageToBuffer &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_srcImage{ task.srcImage },
			_srcLayout{ _srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
			_dstBuffer{ task.dstBuffer }
		{
			for (auto& src : task.regions)
			{
				VkBufferImageCopy	dst				= {};
				const uint3			image_size		= Max( src.imageSize, 1u );

				dst.bufferOffset					= VkDeviceSize( src.bufferOffset );
				dst.bufferRowLength					= src.bufferRowLength;
				dst.bufferImageHeight				= src.bufferImageHeight;

				dst.imageSubresource.aspectMask		= VEnumCast( src.imageLayers.aspectMask, _srcImage->PixelFormat() );
				dst.imageSubresource.mipLevel		= src.imageLayers.mipLevel.Get();
				dst.imageSubresource.baseArrayLayer	= src.imageLayers.baseLayer.Get();
				dst.imageSubresource.layerCount		= src.imageLayers.layerCount;
				dst.imageOffset						= VkOffset3D{ src.imageOffset.x, src.imageOffset.y, src.imageOffset.z };
				dst.imageExtent						= VkExtent3D{ image_size.x, image_size.y, image_size.z };

				_regions.push_back( dst );
			}
		}

		ND_ ImagePtr const&					SrcImage ()		const	{ return _srcImage; }
		ND_ VkImageLayout					SrcLayout ()	const	{ return _srcLayout; }

		ND_ BufferPtr const&				DstBuffer ()	const	{ return _dstBuffer; }

		ND_ ArrayView< VkBufferImageCopy >	Regions ()		const	{ return _regions; }
	};



	//
	// Blit Image
	//
	template <>
	class VFgTask< BlitImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Regions_t = FixedArray< VkImageBlit, FG_MaxBlitRegions >;


	// variables
	private:
		ImagePtr				_srcImage;
		const VkImageLayout		_srcLayout;
		ImagePtr				_dstImage;
		const VkImageLayout		_dstLayout;
		VkFilter				_filter;
		Regions_t				_regions;


	// methods
	public:
		VFgTask (const BlitImage &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_srcImage{ task.srcImage },
			_srcLayout{ _srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
			_dstImage{ task.dstImage },
			_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
			_filter{ VEnumCast( task.filter ) }
		{
			ASSERT( not _dstImage->IsImmutable() );

			if ( EPixelFormat_HasDepthOrStencil( _srcImage->PixelFormat() ) )
			{
				ASSERT( _filter != VK_FILTER_NEAREST );
				_filter = VK_FILTER_NEAREST;
			}

			for (auto& src : task.regions)
			{
				VkImageBlit		dst = {};
				
				dst.srcSubresource.aspectMask		= VEnumCast( src.srcSubresource.aspectMask, _srcImage->PixelFormat() );
				dst.srcSubresource.mipLevel			= src.srcSubresource.mipLevel.Get();
				dst.srcSubresource.baseArrayLayer	= src.srcSubresource.baseLayer.Get();
				dst.srcSubresource.layerCount		= src.srcSubresource.layerCount;
				dst.srcOffsets[0]					= VkOffset3D{ src.srcOffset0.x, src.srcOffset0.y, src.srcOffset0.z };
				dst.srcOffsets[1]					= VkOffset3D{ src.srcOffset1.x, src.srcOffset1.y, src.srcOffset1.z };
			
				dst.dstSubresource.aspectMask		= VEnumCast( src.dstSubresource.aspectMask, _dstImage->PixelFormat() );
				dst.dstSubresource.mipLevel			= src.dstSubresource.mipLevel.Get();
				dst.dstSubresource.baseArrayLayer	= src.dstSubresource.baseLayer.Get();
				dst.dstSubresource.layerCount		= src.dstSubresource.layerCount;
				dst.dstOffsets[0]					= VkOffset3D{ src.dstOffset0.x, src.dstOffset0.y, src.dstOffset0.z };
				dst.dstOffsets[1]					= VkOffset3D{ src.dstOffset1.x, src.dstOffset1.y, src.dstOffset1.z };

				_regions.push_back( dst );
			}
		}

		ND_ ImagePtr const&				SrcImage ()		const	{ return _srcImage; }
		ND_ VkImageLayout				SrcLayout ()	const	{ return _srcLayout; }

		ND_ ImagePtr const&				DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout				DstLayout ()	const	{ return _dstLayout; }

		ND_ ArrayView< VkImageBlit >	Regions ()		const	{ return _regions; }
		ND_ VkFilter					Filter ()		const	{ return _filter; }
	};



	//
	// Resolve Image
	//
	template <>
	class VFgTask< ResolveImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Regions_t = FixedArray< VkImageResolve, FG_MaxResolveRegions >;


	// variables
	private:
		ImagePtr				_srcImage;
		const VkImageLayout		_srcLayout;
		ImagePtr				_dstImage;
		const VkImageLayout		_dstLayout;
		Regions_t				_regions;


	// methods
	public:
		VFgTask (const ResolveImage &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_srcImage{ task.srcImage },
			_srcLayout{ _srcImage->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
			_dstImage{ task.dstImage },
			_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL }
		{
			ASSERT( not _dstImage->IsImmutable() );

			for (auto& src : task.regions)
			{
				VkImageResolve		dst				= {};
				const uint3			image_size		= Max( src.extent, 1u );

				dst.srcSubresource.aspectMask		= VEnumCast( src.srcSubresource.aspectMask, _srcImage->PixelFormat() );
				dst.srcSubresource.mipLevel			= src.srcSubresource.mipLevel.Get();
				dst.srcSubresource.baseArrayLayer	= src.srcSubresource.baseLayer.Get();
				dst.srcSubresource.layerCount		= src.srcSubresource.layerCount;
				dst.srcOffset						= VkOffset3D{ src.srcOffset.x, src.srcOffset.y, src.srcOffset.z };
			
				dst.dstSubresource.aspectMask		= VEnumCast( src.dstSubresource.aspectMask, _dstImage->PixelFormat() );
				dst.dstSubresource.mipLevel			= src.dstSubresource.mipLevel.Get();
				dst.dstSubresource.baseArrayLayer	= src.dstSubresource.baseLayer.Get();
				dst.dstSubresource.layerCount		= src.dstSubresource.layerCount;
				dst.dstOffset						= VkOffset3D{ src.dstOffset.x, src.dstOffset.y, src.dstOffset.z };
			
				dst.extent							= VkExtent3D{ image_size.x, image_size.y, image_size.z };

				_regions.push_back( dst );
			}
		}

		ND_ ImagePtr const&				SrcImage ()		const	{ return _srcImage; }
		ND_ VkImageLayout				SrcLayout ()	const	{ return _srcLayout; }

		ND_ ImagePtr const&				DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout				DstLayout ()	const	{ return _dstLayout; }

		ND_ ArrayView< VkImageResolve >	Regions ()		const	{ return _regions; }
	};



	//
	// Fill Buffer
	//
	template <>
	class VFgTask< FillBuffer > final : public IFrameGraphTask
	{
	// variables
	private:
		BufferPtr		_dstBuffer;
		VkDeviceSize	_dstOffset;
		VkDeviceSize	_size;
		uint			_pattern	= 0;


	// methods
	public:
		VFgTask (const FillBuffer &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_dstBuffer{ task.dstBuffer },
			_dstOffset{ VkDeviceSize(task.dstOffset) },
			_size{ VkDeviceSize(task.size) },
			_pattern{ task.pattern }
		{}

		ND_ BufferPtr const&	DstBuffer ()	const	{ return _dstBuffer; }
		ND_ VkDeviceSize		DstOffset ()	const	{ return _dstOffset; }
		ND_ VkDeviceSize		Size ()			const	{ return _size; }
		ND_ uint				Pattern ()		const	{ return _pattern; }
	};



	//
	// Clear Color Image
	//
	template <>
	class VFgTask< ClearColorImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Ranges_t	= FixedArray< VkImageSubresourceRange, FG_MaxClearRects >;
	

	// variables
	private:
		ImagePtr				_dstImage;
		const VkImageLayout		_dstLayout;
		VkClearColorValue		_clearValue;
		Ranges_t				_ranges;


	// methods
	public:
		VFgTask (const ClearColorImage &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_dstImage{ task.dstImage },
			_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
			_clearValue{}
		{
			ASSERT( not _dstImage->IsImmutable() );

			if ( auto* fval = std::get_if<float4>( &task.clearValue ) )
				::memcpy( _clearValue.float32, fval, sizeof(_clearValue.float32) );
			else
			if ( auto* ival = std::get_if<int4>( &task.clearValue ) )
				::memcpy( _clearValue.int32, ival, sizeof(_clearValue.int32) );
			else
			if ( auto* uval = std::get_if<uint4>( &task.clearValue ) )
				::memcpy( _clearValue.uint32, uval, sizeof(_clearValue.uint32) );
			
			for (auto& src : task.ranges)
			{
				VkImageSubresourceRange		dst = {};
				dst.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
				dst.baseMipLevel	= src.baseMipLevel.Get();
				dst.levelCount		= src.levelCount;
				dst.baseArrayLayer	= src.baseLayer.Get();
				dst.layerCount		= src.layerCount;
				_ranges.push_back( dst );
			}
		}

		ND_ ImagePtr const&							DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout							DstLayout ()	const	{ return _dstLayout; }
		ND_ VkClearColorValue const&				ClearValue ()	const	{ return _clearValue; }
		ND_ ArrayView< VkImageSubresourceRange >	Ranges ()		const	{ return _ranges; }
	};



	//
	// Clear Depth Stencil Image
	//
	template <>
	class VFgTask< ClearDepthStencilImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Ranges_t	= FixedArray< VkImageSubresourceRange, FG_MaxClearRects >;
	

	// variables
	private:
		ImagePtr						_dstImage;
		const VkImageLayout				_dstLayout;
		const VkClearDepthStencilValue	_clearValue;
		Ranges_t						_ranges;


	// methods
	public:
		VFgTask (const ClearDepthStencilImage &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_dstImage{ task.dstImage },
			_dstLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
			_clearValue{ task.clearValue.depth, task.clearValue.stencil }
		{
			ASSERT( not _dstImage->IsImmutable() );

			for (auto& src : task.ranges)
			{
				VkImageSubresourceRange		dst = {};
				dst.aspectMask		= VEnumCast( src.aspectMask, _dstImage->PixelFormat() );
				dst.baseMipLevel	= src.baseMipLevel.Get();
				dst.levelCount		= src.levelCount;
				dst.baseArrayLayer	= src.baseLayer.Get();
				dst.layerCount		= src.layerCount;
				_ranges.push_back( dst );
			}
		}
		
		ND_ ImagePtr const&							DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout							DstLayout ()	const	{ return _dstLayout; }
		ND_ VkClearDepthStencilValue const&			ClearValue ()	const	{ return _clearValue; }
		ND_ ArrayView< VkImageSubresourceRange >	Ranges ()		const	{ return _ranges; }
	};



	//
	// Update Buffer
	//
	template <>
	class VFgTask< UpdateBuffer > final : public IFrameGraphTask
	{
	// variables
	private:
		BufferPtr		_buffer;	// local in gpu only
		VkDeviceSize	_offset;
		Array<char>		_data;		// TODO: optimize


	// methods
	public:
		VFgTask (const UpdateBuffer &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_buffer{ task.dstBuffer },
			_offset{ VkDeviceSize(task.offset) },
			_data{ task.data.begin(), task.data.end() }
		{}

		ND_ BufferPtr const&	DstBuffer ()		const	{ return _buffer; }
		ND_ VkDeviceSize		DstBufferOffset ()	const	{ return _offset; }
		ND_ VkDeviceSize		DataSize ()			const	{ return VkDeviceSize(ArraySizeOf(_data)); }
		ND_ void const*			DataPtr ()			const	{ return _data.data(); }
	};



	//
	// Present
	//
	template <>
	class VFgTask< Present > final : public IFrameGraphTask
	{
	// variables
	private:
		ImagePtr		_image;
		ImageLayer		_layer;


	// methods
	public:
		VFgTask (const Present &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_image{ task.srcImage },
			_layer{ task.layer }
		{}

		ND_ ImagePtr const&		SrcImage ()			const	{ return _image; }
		ND_ ImageLayer			SrcImageLayer ()	const	{ return _layer; }
	};



	//
	// PresentVR
	//
	template <>
	class VFgTask< PresentVR > final : public IFrameGraphTask
	{
	// variables
	private:
		ImagePtr		_leftEyeImage;
		ImageLayer		_leftEyeLayer;

		ImagePtr		_rightEyeImage;
		ImageLayer		_rightEyeLayer;


	// methods
	public:
		VFgTask (const PresentVR &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process },
			_leftEyeImage{ task.leftEye },
			_leftEyeLayer{ task.leftEyeLayer },
			_rightEyeImage{ task.rightEye },
			_rightEyeLayer{ task.rightEyeLayer }
		{}

		ND_ ImagePtr const&		LeftEyeImage ()		const	{ return _leftEyeImage; }
		ND_ ImageLayer			LeftEyeLayer ()		const	{ return _leftEyeLayer; }
		
		ND_ ImagePtr const&		RightEyeImage ()	const	{ return _rightEyeImage; }
		ND_ ImageLayer			RightEyeLayer ()	const	{ return _rightEyeLayer; }
	};



	//
	// Task Group Synchronization
	//
	template <>
	class VFgTask< TaskGroupSync > final : public IFrameGraphTask
	{
	// methods
	public:
		VFgTask (const TaskGroupSync &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process } {}
	};



	//
	// Task Graph
	//
	template <typename VisitorT>
	class VTaskGraph
	{
	// types
	private:
		static constexpr BytesU		BlockSize	{ Max(Max(Max(Max( sizeof(VFgTask<SubmitRenderPass>),		sizeof(VFgTask<DispatchCompute>) ),
															  Max( sizeof(VFgTask<DispatchIndirectCompute>),sizeof(VFgTask<CopyBuffer>) )),
														  Max(Max( sizeof(VFgTask<CopyImage>),				sizeof(VFgTask<CopyBufferToImage>) ),
															  Max( sizeof(VFgTask<CopyImageToBuffer>),		sizeof(VFgTask<BlitImage>) ))),
													  Max(Max(Max( sizeof(VFgTask<ResolveImage>),			sizeof(VFgTask<FillBuffer>) ),
															  Max( sizeof(VFgTask<ClearColorImage>),		sizeof(VFgTask<ClearDepthStencilImage>) )),
														  Max(Max( sizeof(VFgTask<UpdateBuffer>),			sizeof(VFgTask<Present>) ),
															  Max( sizeof(VFgTask<PresentVR>),				sizeof(VFgTask<TaskGroupSync>) )))) };


	// variables
	private:
		HashSet< IFrameGraphTask *>		_nodes;
		Array< Task >					_entries;
		PoolAllocator					_allocator;


	// methods
	public:
		VTaskGraph ()
		{
			_allocator.SetBlockSize( AlignToLarger( BlockSize * 100, 4_Kb ));
		}


		template <typename T>
		ND_ Task  Add (const T &task)
		{
			void *	ptr  = _allocator.Alloc< VFgTask<T> >();
			auto	iter = *_nodes.insert( PlacementNew< VFgTask<T> >( ptr, task, &_Visitor<T> )).first;

			if ( iter->Inputs().empty() )
				_entries.push_back( iter );

			for (auto in_node : iter->Inputs())
			{
				ASSERT( !!_nodes.count( in_node ) );

				in_node->Attach( iter );
			}
			return iter;
		}


		void Clear ()
		{
			for (auto& node : _nodes) {
				node->~IFrameGraphTask();
			}

			_nodes.clear();
			_entries.clear();
			_allocator.Clear();
		}


		ND_ Array<Task> const&	Entries ()		const	{ return _entries; }
		ND_ size_t				Count ()		const	{ return _nodes.size(); }
		ND_ bool				Empty ()		const	{ return _nodes.empty(); }


	private:
		template <typename T>
		static void _Visitor (void *p, const void *task)
		{
			static_cast< VisitorT *>(p)->Visit( *static_cast< VFgTask<T> const *>(task) );
		}
	};



	//
	// Render Pass Graph
	//
	class VRenderPassGraph
	{
	// methods
	public:
		void Add (const VFgTask<SubmitRenderPass> *)
		{
		}

		void Clear ()
		{
		}
	};


}	// FG
