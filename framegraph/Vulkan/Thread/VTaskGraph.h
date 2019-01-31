// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphInstance.h"
#include "framegraph/Shared/EnumUtils.h"
#include "VCommon.h"

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
		VLogicalRenderPass *	_logicalPass	= null;
		Self *					_prevSubpass	= null;
		Self *					_nextSubpass	= null;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const SubmitRenderPass &task, ProcessFunc_t process);

		ND_ bool  IsValid () const;

		ND_ VLogicalRenderPass *		GetLogicalPass ()	const	{ return _logicalPass; }

		ND_ Self const *				GetPrevSubpass ()	const	{ return _prevSubpass; }
		ND_ Self const *				GetNextSubpass ()	const	{ return _nextSubpass; }

		ND_ bool						IsSubpass ()		const	{ return _prevSubpass != null; }
		ND_ bool						IsLastPass ()		const	{ return _nextSubpass == null; }
		
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
		VPipelineResourceSet					_resources;
		uint									_debugModeIndex	= UMax;
	public:
		VComputePipeline const* const			pipeline;
		const _fg_hidden_::PushConstants_t		pushConstants;
		
		const DispatchCompute::ComputeCmds_t	commands;
		const Optional< uint3 >					localGroupSize;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const DispatchCompute &task, ProcessFunc_t process);
		
		ND_ bool  IsValid () const;

		ND_ VPipelineResourceSet const&		GetResources ()			const	{ return _resources; }
		ND_ uint							GetDebugModeIndex ()	const	{ return _debugModeIndex; }
	};



	//
	// Dispatch Indirect Compute
	//
	template <>
	class VFgTask< DispatchComputeIndirect > final : public IFrameGraphTask
	{
	// variables
	private:
		VPipelineResourceSet					_resources;
		uint									_debugModeIndex	= UMax;
	public:
		VComputePipeline const* const			pipeline;
		const _fg_hidden_::PushConstants_t		pushConstants;
		
		const DispatchComputeIndirect::ComputeCmds_t	commands;

		VLocalBuffer const* const				indirectBuffer;
		const Optional< uint3 >					localGroupSize;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const DispatchComputeIndirect &task, ProcessFunc_t process);
		
		ND_ bool  IsValid () const;

		ND_ VPipelineResourceSet const&		GetResources ()			const	{ return _resources; }
		ND_ uint							GetDebugModeIndex ()	const	{ return _debugModeIndex; }
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
		
		ND_ bool  IsValid () const;
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
		
		ND_ bool  IsValid () const;
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
		
		ND_ bool  IsValid () const;
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
		
		ND_ bool  IsValid () const;
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
		
		ND_ bool  IsValid () const;
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
		
		ND_ bool  IsValid () const;
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
		
		ND_ bool  IsValid () const;
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
		
		ND_ bool  IsValid () const;

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
		
		ND_ bool  IsValid () const;
	};



	//
	// Update Buffer
	//
	template <>
	class VFgTask< UpdateBuffer > final : public IFrameGraphTask
	{
	// types
	private:
		struct Region
		{
			void *			dataPtr;
			VkDeviceSize	dataSize;
			VkDeviceSize	bufferOffset;
		};


	// variables
	public:
		VLocalBuffer const* const	dstBuffer;		// local in gpu only
	private:
		ArrayView<Region>			_regions;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const UpdateBuffer &task, ProcessFunc_t process);
		
		ND_ bool  IsValid () const;

		ND_ ArrayView<Region>	Regions ()	const	{ return _regions; }
	};



	//
	// Present
	//
	template <>
	class VFgTask< Present > final : public IFrameGraphTask
	{
	// variables
	public:
		VLocalImage const* const	srcImage;
		const ImageLayer			layer;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const Present &task, ProcessFunc_t process);
		
		ND_ bool  IsValid () const;
	};



	//
	// Present VR
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
		
		ND_ bool  IsValid () const;
	};



	//
	// Update Ray Tracing Shader Table
	//
	template <>
	class VFgTask< UpdateRayTracingShaderTable > final : public IFrameGraphTask
	{
		friend class VFrameGraphThread;
		
		using RayGenShaderGroup		= UpdateRayTracingShaderTable::RayGenShaderGroup;
		using MissShaderGroups_t	= Array< UpdateRayTracingShaderTable::MissShaderGroup >;
		using HitShaderGroups_t		= Array< UpdateRayTracingShaderTable::HitShaderGroup >;

	// variables
	public:
		VRayTracingPipeline const* const	pipeline;
		VLocalRTScene const* const			rtScene;
		VLocalBuffer const* const			dstBuffer;
		VkDeviceSize						dstOffset;
		const RayGenShaderGroup				rayGenShader;
		const MissShaderGroups_t			missShaders;
		const HitShaderGroups_t				hitShaders;
		//const ShaderGroups_t				callableShaders;

	// methods
	public:
		VFgTask (VFrameGraphThread *, const UpdateRayTracingShaderTable &task, ProcessFunc_t process);
		
		ND_ bool  IsValid () const;
	};



	//
	// Build Ray Tracing Geometry
	//
	template <>
	class VFgTask< BuildRayTracingGeometry > final : public IFrameGraphTask
	{
		friend class VFrameGraphThread;

	// types
	private:
		using UsableBuffers_t = std::unordered_set< VLocalBuffer const*, std::hash<VLocalBuffer const*>, std::equal_to<VLocalBuffer const*>,
													StdLinearAllocator<VLocalBuffer const*> >;


	// variables
	private:
		VLocalRTGeometry const*		_rtGeometry				= null;
		VLocalBuffer const*			_scratchBuffer			= null;
		VkDeviceSize				_scratchBufferOffset	= 0;
		VkGeometryNV *				_geometry				= null;
		size_t						_geometryCount			= 0;
		UsableBuffers_t				_usableBuffers;


	// methods
	public:
		VFgTask (VFrameGraphThread *, const BuildRayTracingGeometry &task, ProcessFunc_t process);
		
		ND_ bool  IsValid () const	{ return true; }

		ND_ VLocalRTGeometry const*		RTGeometry ()			const	{ return _rtGeometry; }
		ND_ VLocalBuffer const*			ScratchBuffer ()		const	{ return _scratchBuffer; }
		ND_ VkDeviceSize				ScratchBufferOffset ()	const	{ return _scratchBufferOffset; }
		ND_ ArrayView<VkGeometryNV>		GetGeometry ()			const	{ return ArrayView{ _geometry, _geometryCount }; }
		ND_ UsableBuffers_t const&		GetBuffers ()			const	{ return _usableBuffers; }
	};



	//
	// Build Ray Tracing Scene
	//
	template <>
	class VFgTask< BuildRayTracingScene > final : public IFrameGraphTask
	{
		friend class VFrameGraphThread;

	// variables
	private:
		VLocalRTScene const*		_rtScene				= null;
		VLocalBuffer const*			_scratchBuffer			= null;
		VkDeviceSize				_scratchBufferOffset	= 0;
		VLocalBuffer const*			_instanceBuffer			= null;
		VkDeviceSize				_instanceBufferOffset	= 0;
		VLocalRTGeometry const**	_rtGeometries			= null;
		RawRTGeometryID *			_rtGeometryIDs			= null;		// strong references
		size_t						_rtGeometryCount		= 0;
		uint						_instanceCount			= 0;
		uint						_hitShadersPerInstance	= 0;
		uint						_maxHitShaderCount		= 0;


	// methods
	public:
		VFgTask (VFrameGraphThread *, const BuildRayTracingScene &task, ProcessFunc_t process) : IFrameGraphTask{task, process} {}
		
		ND_ bool  IsValid () const	{ return true; }

		ND_ VLocalRTScene const*				RTScene ()				const	{ return _rtScene; }
		ND_ VLocalBuffer const*					ScratchBuffer ()		const	{ return _scratchBuffer; }
		ND_ VkDeviceSize						ScratchBufferOffset ()	const	{ return _scratchBufferOffset; }
		ND_ VLocalBuffer const*					InstanceBuffer ()		const	{ return _instanceBuffer; }
		ND_ VkDeviceSize						InstanceBufferOffset ()	const	{ return _instanceBufferOffset; }
		ND_ uint								InstanceCount ()		const	{ return _instanceCount; }
		ND_ ArrayView<VLocalRTGeometry const*>	Geometries ()			const	{ return ArrayView{ _rtGeometries, _rtGeometryCount }; }
		ND_ ArrayView<RawRTGeometryID>			GeometryIDs ()			const	{ return ArrayView{ _rtGeometryIDs, _rtGeometryCount }; }
		ND_ uint								HitShadersPerInstance()	const	{ return _hitShadersPerInstance; }
		ND_ uint								MaxHitShaderCount ()	const	{ return _maxHitShaderCount; }
	};



	//
	// Trace Rays
	//
	template <>
	class VFgTask< TraceRays > final : public IFrameGraphTask
	{
	// variables
	private:
		VPipelineResourceSet				_resources;
		uint								_debugModeIndex	= UMax;
	public:
		VRayTracingPipeline const* const	pipeline;
		const _fg_hidden_::PushConstants_t	pushConstants;

		const uint3							groupCount;

		VLocalBuffer const* const			sbtBuffer;
		const VkDeviceSize					rayGenOffset;
		const VkDeviceSize					rayMissOffset;
		const VkDeviceSize					rayHitOffset;
		const VkDeviceSize					callableOffset;
		const uint16_t						rayMissStride;
		const uint16_t						rayHitStride;
		const uint16_t						callableStride;


	// methods
	public:
		VFgTask (VFrameGraphThread *fg, const TraceRays &task, ProcessFunc_t process);
		
		ND_ bool  IsValid () const;

		ND_ VPipelineResourceSet const&		GetResources ()			const	{ return _resources; }
		ND_ uint							GetDebugModeIndex ()	const	{ return _debugModeIndex; }
	};
	


	//
	// Task Graph
	//
	template <typename VisitorT>
	class VTaskGraph
	{
	// types
	private:
		using Self					= VTaskGraph< VisitorT >;
		using Allocator_t			= LinearAllocator<>;
		using SearchableNodes_t		= std::unordered_set< Task, std::hash<Task>, std::equal_to<Task>, StdLinearAllocator<Task> >;
		using Entries_t				= std::vector< Task, StdLinearAllocator<Task> >;


	// variables
	private:
		InPlace<SearchableNodes_t>	_nodes;
		InPlace<Entries_t>			_entries;


	// methods
	public:
		VTaskGraph () {}
		~VTaskGraph () {}

		template <typename T>
		ND_ VFgTask<T>*  Add (VFrameGraphThread *fg, const T &task);

		void OnStart (LinearAllocator<> &);
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


	/*
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
	};*/

	
	
/*
=================================================
	OnStart
=================================================
*/
	template <typename VisitorT>
	inline void  VTaskGraph<VisitorT>::OnStart (LinearAllocator<> &alloc)
	{
		_nodes.Create( alloc );
		_entries.Create( alloc );
		_entries->reserve( 64 );
	}
	
/*
=================================================
	OnDiscardMemory
=================================================
*/
	template <typename VisitorT>
	inline void  VTaskGraph<VisitorT>::OnDiscardMemory ()
	{
		_nodes.Destroy();
		_entries.Destroy();
	}


}	// FG
