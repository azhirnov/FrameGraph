// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphDrawTask.h"
#include "VCommon.h"

namespace FG
{

	template <typename Task>
	class VFgDrawTask;


	
	//
	// Draw Task interface
	//

	class IDrawTask
	{
	// types
	public:
		using Name_t			= DrawTask::TaskName_t;
		using ProcessFunc_t		= void (*) (void *visitor, void *taskData);
		

	// variables
	private:
		ProcessFunc_t	_pass1	= null;
		ProcessFunc_t	_pass2	= null;
		Name_t			_taskName;
		RGBA8u			_debugColor;


	// interface
	public:
		template <typename TaskType>
		IDrawTask (const TaskType &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
			_pass1{pass1}, _pass2{pass2}, _taskName{task.taskName}, _debugColor{task.debugColor} {}

		virtual ~IDrawTask () {}
		
		ND_ StringView	GetName ()			const	{ return _taskName; }
		ND_ RGBA8u		GetDebugColor ()	const	{ return _debugColor; }
		
		void Process1 (void *visitor)				{ ASSERT( _pass1 );  _pass1( visitor, this ); }
		void Process2 (void *visitor)				{ ASSERT( _pass2 );  _pass2( visitor, this ); }
	};



	//
	// Draw Task
	//

	template <>
	class VFgDrawTask< DrawTask > final : public IDrawTask
	{
	// types
	public:
		using VertexBuffers_t	= StaticArray< VLocalBuffer const*, FG_MaxVertexBuffers >;
		using VertexOffsets_t	= StaticArray< VkDeviceSize, FG_MaxVertexBuffers >;
		using VertexStrides_t	= StaticArray< Bytes<uint>, FG_MaxVertexBuffers >;
		using Scissors_t		= DrawTask::Scissors_t;
		using DrawCmd			= DrawTask::DrawCmd;


	// variables
	private:
		VPipelineResourceSet			_resources;

		VertexBuffers_t					_vertexBuffers;
		VertexOffsets_t					_vbOffsets;
		VertexStrides_t					_vbStrides;
		const uint						_vbCount;

	public:
		const RawGPipelineID			pipeline;

		const RenderState				renderState;
		const EPipelineDynamicState		dynamicStates;

		const VertexInputState			vertexInput;
		const DrawCmd					drawCmd;
		const Scissors_t				scissors;

		mutable VkDescriptorSets_t		descriptorSets;


	// methods
	public:
		VFgDrawTask (VFrameGraphThread *fg, const DrawTask &task, ProcessFunc_t pass1, ProcessFunc_t pass2);
		
		ND_ VPipelineResourceSet const&		GetResources ()		const	{ return _resources; }

		ND_ ArrayView< VLocalBuffer const*>	GetVertexBuffers ()	const	{ return ArrayView{ _vertexBuffers.data(), _vbCount }; }
		ND_ ArrayView< VkDeviceSize >		GetVBOffsets ()		const	{ return ArrayView{ _vbOffsets.data(), _vbCount }; }
		ND_ ArrayView< Bytes<uint> >		GetVBStrides ()		const	{ return ArrayView{ _vbStrides.data(), _vbCount }; }
	};



	//
	// Draw Indexed Task
	//

	template <>
	class VFgDrawTask< DrawIndexedTask > final : public IDrawTask
	{
	// types
	public:
		using VertexBuffers_t	= VFgDrawTask<DrawTask>::VertexBuffers_t;
		using VertexOffsets_t	= VFgDrawTask<DrawTask>::VertexOffsets_t;
		using VertexStrides_t	= VFgDrawTask<DrawTask>::VertexStrides_t;
		using Scissors_t		= DrawIndexedTask::Scissors_t;
		using DrawCmd			= DrawIndexedTask::DrawCmd;


	// variables
	private:
		VPipelineResourceSet			_resources;
		
		VertexBuffers_t					_vertexBuffers;
		VertexOffsets_t					_vbOffsets;
		VertexStrides_t					_vbStrides;
		const uint						_vbCount;

	public:
		const RawGPipelineID			pipeline;

		const RenderState				renderState;
		const EPipelineDynamicState		dynamicStates;
		
		VLocalBuffer const* const		indexBuffer;
		const BytesU					indexBufferOffset;
		const EIndex					indexType;
		
		const VertexInputState			vertexInput;
		const DrawCmd					drawCmd;
		const Scissors_t				scissors;
		
		mutable VkDescriptorSets_t		descriptorSets;


	// methods
	public:
		VFgDrawTask (VFrameGraphThread *fg, const DrawIndexedTask &task, ProcessFunc_t pass1, ProcessFunc_t pass2);
		
		ND_ VPipelineResourceSet const&		GetResources ()		const	{ return _resources; }
		
		ND_ ArrayView< VLocalBuffer const*>	GetVertexBuffers ()	const	{ return ArrayView{ _vertexBuffers.data(), _vbCount }; }
		ND_ ArrayView< VkDeviceSize >		GetVBOffsets ()		const	{ return ArrayView{ _vbOffsets.data(), _vbCount }; }
		ND_ ArrayView< Bytes<uint> >		GetVBStrides ()		const	{ return ArrayView{ _vbStrides.data(), _vbCount }; }
	};
	


	//
	// Draw Mesh Task
	//

	template <>
	class VFgDrawTask< DrawMeshTask > final : public IDrawTask
	{
	// types
	public:
		using Scissors_t	= DrawMeshTask::Scissors_t;
		using DrawCmd		= DrawMeshTask::DrawCmd;


	// variables
	private:
		VPipelineResourceSet			_resources;

	public:
		const RawMPipelineID			pipeline;

		const RenderState				renderState;
		const EPipelineDynamicState		dynamicStates;

		const DrawCmd					drawCmd;
		const Scissors_t				scissors;

		mutable VkDescriptorSets_t		descriptorSets;


	// methods
	public:
		VFgDrawTask (VFrameGraphThread *fg, const DrawMeshTask &task, ProcessFunc_t pass1, ProcessFunc_t pass2);
		
		ND_ VPipelineResourceSet const&		GetResources ()		const	{ return _resources; }
	};


}	// FG
