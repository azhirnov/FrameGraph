// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraph.h"
#include "framegraph/Shared/EnumUtils.h"
#include "VEnumCast.h"

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
		VFgTask (VFrameGraphThread *fg, const SubmitRenderPass &task, ProcessFunc_t process);

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
		RawCPipelineID				_pipeline;
		VPipelineResourceSet		_resources;
		uint3						_groupCount;
		Optional< uint3 >			_localGroupSize;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const DispatchCompute &task, ProcessFunc_t process);

		ND_ RawCPipelineID					GetPipeline ()		const	{ return _pipeline; }
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
		RawCPipelineID				_pipeline;
		VPipelineResourceSet		_resources;
		LocalBufferID				_indirectBuffer;
		VkDeviceSize				_indirectBufferOffset;
		Optional< uint3 >			_localGroupSize;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const DispatchIndirectCompute &task, ProcessFunc_t process);
		
		ND_ RawCPipelineID					GetPipeline ()			const	{ return _pipeline; }
		ND_ VPipelineResourceSet const&		GetResources ()			const	{ return _resources; }

		ND_ LocalBufferID					IndirectBuffer ()		const	{ return _indirectBuffer; }
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
		using Region	= CopyBuffer::Region;
		using Regions_t = CopyBuffer::Regions_t;


	// variables
	private:
		LocalBufferID	_srcBuffer;
		LocalBufferID	_dstBuffer;
		Regions_t		_regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const CopyBuffer &task, ProcessFunc_t process);

		ND_ LocalBufferID			SrcBuffer ()	const	{ return _srcBuffer; }
		ND_ LocalBufferID			DstBuffer ()	const	{ return _dstBuffer; }
		ND_ ArrayView< Region >		Regions ()		const	{ return _regions; }
	};



	//
	// Copy Image
	//
	template <>
	class VFgTask< CopyImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Region	= CopyImage::Region;
		using Regions_t = CopyImage::Regions_t;


	// variables
	private:
		LocalImageID			_srcImage;
		const VkImageLayout		_srcLayout;
		LocalImageID			_dstImage;
		const VkImageLayout		_dstLayout;
		Regions_t				_regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const CopyImage &task, ProcessFunc_t process);

		ND_ LocalImageID			SrcImage ()		const	{ return _srcImage; }
		ND_ VkImageLayout			SrcLayout ()	const	{ return _srcLayout; }

		ND_ LocalImageID			DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout			DstLayout ()	const	{ return _dstLayout; }

		ND_ ArrayView< Region >		Regions ()		const	{ return _regions; }
	};



	//
	// Copy Buffer to Image
	//
	template <>
	class VFgTask< CopyBufferToImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Region	= CopyBufferToImage::Region;
		using Regions_t = CopyBufferToImage::Regions_t;


	// variables
	private:
		LocalBufferID			_srcBuffer;
		LocalImageID			_dstImage;
		const VkImageLayout		_dstLayout;
		Regions_t				_regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const CopyBufferToImage &task, ProcessFunc_t process);

		ND_ LocalBufferID			SrcBuffer ()	const	{ return _srcBuffer; }

		ND_ LocalImageID			DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout			DstLayout ()	const	{ return _dstLayout; }

		ND_ ArrayView< Region >		Regions ()		const	{ return _regions; }
	};



	//
	// Copy Image to Buffer
	//
	template <>
	class VFgTask< CopyImageToBuffer > final : public IFrameGraphTask
	{
	// types
	private:
		using Region	= CopyImageToBuffer::Region;
		using Regions_t = CopyImageToBuffer::Regions_t;


	// variables
	private:
		LocalImageID			_srcImage;
		const VkImageLayout		_srcLayout;
		LocalBufferID			_dstBuffer;
		Regions_t				_regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const CopyImageToBuffer &task, ProcessFunc_t process);

		ND_ LocalImageID			SrcImage ()		const	{ return _srcImage; }
		ND_ VkImageLayout			SrcLayout ()	const	{ return _srcLayout; }

		ND_ LocalBufferID			DstBuffer ()	const	{ return _dstBuffer; }

		ND_ ArrayView< Region >		Regions ()		const	{ return _regions; }
	};



	//
	// Blit Image
	//
	template <>
	class VFgTask< BlitImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Region	= BlitImage::Region;
		using Regions_t = BlitImage::Regions_t;


	// variables
	private:
		LocalImageID			_srcImage;
		const VkImageLayout		_srcLayout;
		LocalImageID			_dstImage;
		const VkImageLayout		_dstLayout;
		VkFilter				_filter;
		Regions_t				_regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const BlitImage &task, ProcessFunc_t process);

		ND_ LocalImageID			SrcImage ()		const	{ return _srcImage; }
		ND_ VkImageLayout			SrcLayout ()	const	{ return _srcLayout; }

		ND_ LocalImageID			DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout			DstLayout ()	const	{ return _dstLayout; }

		ND_ ArrayView< Region >		Regions ()		const	{ return _regions; }
		ND_ VkFilter				Filter ()		const	{ return _filter; }
	};



	//
	// Resolve Image
	//
	template <>
	class VFgTask< ResolveImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Region	= ResolveImage::Region;
		using Regions_t = ResolveImage::Regions_t;


	// variables
	private:
		LocalImageID			_srcImage;
		const VkImageLayout		_srcLayout;
		LocalImageID			_dstImage;
		const VkImageLayout		_dstLayout;
		Regions_t				_regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const ResolveImage &task, ProcessFunc_t process);

		ND_ LocalImageID			SrcImage ()		const	{ return _srcImage; }
		ND_ VkImageLayout			SrcLayout ()	const	{ return _srcLayout; }

		ND_ LocalImageID			DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout			DstLayout ()	const	{ return _dstLayout; }

		ND_ ArrayView< Region >		Regions ()		const	{ return _regions; }
	};



	//
	// Fill Buffer
	//
	template <>
	class VFgTask< FillBuffer > final : public IFrameGraphTask
	{
	// variables
	private:
		LocalBufferID	_dstBuffer;
		VkDeviceSize	_dstOffset;
		VkDeviceSize	_size;
		uint			_pattern	= 0;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const FillBuffer &task, ProcessFunc_t process);

		ND_ LocalBufferID		DstBuffer ()	const	{ return _dstBuffer; }
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
		using Range		= ClearColorImage::Range;
		using Ranges_t	= ClearColorImage::Ranges_t;
	

	// variables
	private:
		LocalImageID			_dstImage;
		const VkImageLayout		_dstLayout;
		VkClearColorValue		_clearValue;
		Ranges_t				_ranges;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const ClearColorImage &task, ProcessFunc_t process);

		ND_ LocalImageID				DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout				DstLayout ()	const	{ return _dstLayout; }
		ND_ VkClearColorValue const&	ClearValue ()	const	{ return _clearValue; }
		ND_ ArrayView< Range >			Ranges ()		const	{ return _ranges; }
	};



	//
	// Clear Depth Stencil Image
	//
	template <>
	class VFgTask< ClearDepthStencilImage > final : public IFrameGraphTask
	{
	// types
	private:
		using Range		= ClearDepthStencilImage::Range;
		using Ranges_t	= ClearDepthStencilImage::Ranges_t;
	

	// variables
	private:
		LocalImageID					_dstImage;
		const VkImageLayout				_dstLayout;
		const VkClearDepthStencilValue	_clearValue;
		Ranges_t						_ranges;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const ClearDepthStencilImage &task, ProcessFunc_t process);
		
		ND_ LocalImageID					DstImage ()		const	{ return _dstImage; }
		ND_ VkImageLayout					DstLayout ()	const	{ return _dstLayout; }
		ND_ VkClearDepthStencilValue const&	ClearValue ()	const	{ return _clearValue; }
		ND_ ArrayView< Range >				Ranges ()		const	{ return _ranges; }
	};



	//
	// Update Buffer
	//
	template <>
	class VFgTask< UpdateBuffer > final : public IFrameGraphTask
	{
	// variables
	private:
		LocalBufferID	_buffer;	// local in gpu only
		VkDeviceSize	_offset;
		Array<char>		_data;		// TODO: optimize


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const UpdateBuffer &task, ProcessFunc_t process);

		ND_ LocalBufferID		DstBuffer ()		const	{ return _buffer; }
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
		LocalImageID	_image;
		ImageLayer		_layer;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const Present &task, ProcessFunc_t process);

		ND_ LocalImageID		SrcImage ()			const	{ return _image; }
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
		LocalImageID	_leftEyeImage;
		ImageLayer		_leftEyeLayer;

		LocalImageID	_rightEyeImage;
		ImageLayer		_rightEyeLayer;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const PresentVR &task, ProcessFunc_t process);

		ND_ LocalImageID	LeftEyeImage ()		const	{ return _leftEyeImage; }
		ND_ ImageLayer		LeftEyeLayer ()		const	{ return _leftEyeLayer; }
		
		ND_ LocalImageID	RightEyeImage ()	const	{ return _rightEyeImage; }
		ND_ ImageLayer		RightEyeLayer ()	const	{ return _rightEyeLayer; }
	};



	//
	// Task Group Synchronization
	//
	template <>
	class VFgTask< TaskGroupSync > final : public IFrameGraphTask
	{
	// methods
	public:
		VFgTask (VFrameGraphThread *, const TaskGroupSync &task, ProcessFunc_t process) :
			IFrameGraphTask{ task, process } {}
	};
	


	//
	// Task Graph
	//
	template <typename VisitorT>
	class VTaskGraph
	{
	// types
	public:
		static constexpr BytesU		BlockSize	{ Max(	sizeof(VFgTask<SubmitRenderPass>),		sizeof(VFgTask<DispatchCompute>),
														sizeof(VFgTask<DispatchIndirectCompute>),sizeof(VFgTask<CopyBuffer>),
														sizeof(VFgTask<CopyImage>),				sizeof(VFgTask<CopyBufferToImage>),
														sizeof(VFgTask<CopyImageToBuffer>),		sizeof(VFgTask<BlitImage>),
														sizeof(VFgTask<ResolveImage>),			sizeof(VFgTask<FillBuffer>),
														sizeof(VFgTask<ClearColorImage>),		sizeof(VFgTask<ClearDepthStencilImage>),
														sizeof(VFgTask<UpdateBuffer>),			sizeof(VFgTask<Present>),
														sizeof(VFgTask<PresentVR>),				sizeof(VFgTask<TaskGroupSync>) )};

	private:
		using Self					= VTaskGraph< VisitorT >;
		using Allocator_t			= LinearAllocator<>;
		using SearchableNodes_t		= std::unordered_set< IFrameGraphTask*, std::hash<IFrameGraphTask*>, std::equal_to<IFrameGraphTask*>, StdLinearAllocator<IFrameGraphTask*> >;
		using Entries_t				= std::vector< Task, StdLinearAllocator<Task> >;


	// variables
	private:
		Storage<SearchableNodes_t>	_nodes;
		Storage<Entries_t>			_entries;


	// methods
	public:
		VTaskGraph () {}
		~VTaskGraph () {}

		template <typename T>
		ND_ Task  Add (VFrameGraphThread *fg, const T &task);

		void OnStart (VFrameGraphThread *fg);
		void OnDiscardMemory ();

		ND_ ArrayView<Task>		Entries ()		const	{ return *_entries; }
		ND_ size_t				Count ()		const	{ return _nodes->size(); }
		ND_ bool				Empty ()		const	{ return _nodes->empty(); }


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
