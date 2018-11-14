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
		ProcessFunc_t		_processFunc	= null;
		Dependencies_t		_inputs;
		Dependencies_t		_outputs;
		Name_t				_taskName;
		RGBA8u				_debugColor;
		uint				_visitorID		= 0;
		ExeOrderIndex		_exeOrderIdx	= ExeOrderIndex::Initial;


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

		ND_ ArrayView< Task >					Inputs ()			const	{ return _inputs; }
		ND_ ArrayView< Task >					Outputs ()			const	{ return _outputs; }

		ND_ virtual VFgTask<SubmitRenderPass>*	GetSubmitRenderPassTask ()	{ return null; }

		void Attach (Task output)					{ _outputs.push_back( output ); }
		void SetVisitorID (uint id)					{ _visitorID = id; }
		void SetExecutionOrder (ExeOrderIndex idx)	{ _exeOrderIdx = idx; }

		void Process (void *visitor)		const	{ ASSERT( _processFunc );  _processFunc( visitor, this ); }


	// interface
	public:
		virtual ~IFrameGraphTask () {}
	};


	/*
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
	*/


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
		VPipelineResourceSet			_resources;
	public:
		const RawCPipelineID			pipeline;
		const uint3						groupCount;
		const Optional< uint3 >			localGroupSize;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const DispatchCompute &task, ProcessFunc_t process);

		ND_ VPipelineResourceSet const&		GetResources ()	const	{ return _resources; }
	};



	//
	// Dispatch Indirect Compute
	//
	template <>
	class VFgTask< DispatchIndirectCompute > final : public IFrameGraphTask
	{
	// variables
	private:
		VPipelineResourceSet		_resources;
	public:
		const RawCPipelineID		pipeline;
		VLocalBuffer const* const	indirectBuffer;
		const VkDeviceSize			indirectBufferOffset;
		const Optional< uint3 >		localGroupSize;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const DispatchIndirectCompute &task, ProcessFunc_t process);
		
		ND_ VPipelineResourceSet const&		GetResources ()	const	{ return _resources; }
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
	public:
		VLocalBuffer const* const	srcBuffer;
		VLocalBuffer const* const	dstBuffer;
		const Regions_t				regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const CopyBuffer &task, ProcessFunc_t process);
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
	public:
		VLocalImage const* const	srcImage;
		const VkImageLayout			srcLayout;
		VLocalImage const* const	dstImage;
		const VkImageLayout			dstLayout;
		const Regions_t				regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const CopyImage &task, ProcessFunc_t process);
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
	public:
		VLocalBuffer const* const	srcBuffer;
		VLocalImage const* const	dstImage;
		const VkImageLayout			dstLayout;
		const Regions_t				regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const CopyBufferToImage &task, ProcessFunc_t process);
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
	public:
		VLocalImage const* const	srcImage;
		const VkImageLayout			srcLayout;
		VLocalBuffer const* const	dstBuffer;
		const Regions_t				regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const CopyImageToBuffer &task, ProcessFunc_t process);
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
	public:
		VLocalImage const* const	srcImage;
		const VkImageLayout			srcLayout;
		VLocalImage const* const	dstImage;
		const VkImageLayout			dstLayout;
		const VkFilter				filter;
		const Regions_t				regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const BlitImage &task, ProcessFunc_t process);
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
	public:
		VLocalImage const* const	srcImage;
		const VkImageLayout			srcLayout;

		VLocalImage const* const	dstImage;
		const VkImageLayout			dstLayout;

		const Regions_t				regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const ResolveImage &task, ProcessFunc_t process);
	};



	//
	// Fill Buffer
	//
	template <>
	class VFgTask< FillBuffer > final : public IFrameGraphTask
	{
	// variables
	public:
		VLocalBuffer const* const	dstBuffer;
		const VkDeviceSize			dstOffset;
		const VkDeviceSize			size;
		const uint					pattern;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const FillBuffer &task, ProcessFunc_t process);
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
	public:
		VLocalImage const* const	dstImage;
		const VkImageLayout			dstLayout;
		const Ranges_t				ranges;
	private:
		VkClearColorValue			_clearValue;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const ClearColorImage &task, ProcessFunc_t process);

		ND_ VkClearColorValue const&	ClearValue ()	const	{ return _clearValue; }
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
	public:
		VLocalImage const* const		dstImage;
		const VkImageLayout				dstLayout;
		const VkClearDepthStencilValue	clearValue;
		const Ranges_t					ranges;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const ClearDepthStencilImage &task, ProcessFunc_t process);
	};



	//
	// Update Buffer
	//
	template <>
	class VFgTask< UpdateBuffer > final : public IFrameGraphTask
	{
	// types
	private:
		using Data_t	= std::vector< uint8_t, StdLinearAllocator<uint8_t> >;

	// variables
	public:
		VLocalBuffer const* const	dstBuffer;		// local in gpu only
		const VkDeviceSize			dstOffset;
	private:
		Data_t						_data;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const UpdateBuffer &task, ProcessFunc_t process);

		ND_ VkDeviceSize	DataSize ()		const	{ return VkDeviceSize(ArraySizeOf( _data )); }
		ND_ void const*		DataPtr ()		const	{ return _data.data(); }
	};



	//
	// Present
	//
	template <>
	class VFgTask< Present > final : public IFrameGraphTask
	{
	// variables
	public:
		VLocalImage const* const	image;
		const ImageLayer			layer;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const Present &task, ProcessFunc_t process);
	};



	//
	// PresentVR
	//
	template <>
	class VFgTask< PresentVR > final : public IFrameGraphTask
	{
	// variables
	public:
		VLocalImage const* const	leftEyeImage;
		const ImageLayer			leftEyeLayer;

		VLocalImage const* const	rightEyeImage;
		const ImageLayer			rightEyeLayer;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const PresentVR &task, ProcessFunc_t process);
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
		using SearchableNodes_t		= std::unordered_set< Task, std::hash<Task>, std::equal_to<Task>, StdLinearAllocator<Task> >;
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
