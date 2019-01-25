// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VTaskProcessor.h"
#include "VFrameGraphThread.h"
#include "VFrameGraphDebugger.h"
#include "VStagingBufferManager.h"
#include "VShaderDebugger.h"
#include "VDrawTask.h"
#include "VEnumCast.h"
#include "FGEnumCast.h"
#include "Shared/EnumUtils.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

	template <typename ID>
	forceinline auto const*  VTaskProcessor::_ToLocal (ID id) const
	{
		return _frameGraph.GetResourceManager()->ToLocal( id );
	}
	
	template <typename ID>
	forceinline auto const*  VTaskProcessor::_GetResource (ID id) const
	{
		return _frameGraph.GetResourceManager()->GetResource( id );
	}
	
	template <typename ResType>
	static void CommitResourceBarrier (const void *res, VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger)
	{
		return static_cast<ResType const*>(res)->CommitBarrier( barrierMngr, debugger );
	}

	inline VTaskProcessor::Statistic_t&  VTaskProcessor::Stat () const
	{
		return _frameGraph.EditStatistic().renderer;
	}
//-----------------------------------------------------------------------------



	//
	// Pipeline Resource Barriers
	//
	class VTaskProcessor::PipelineResourceBarriers
	{
	// variables
	private:
		VTaskProcessor &	_tp;
		ArrayView<uint>		_dynamicOffsets;

	// methods
	public:
		PipelineResourceBarriers (VTaskProcessor &tp, ArrayView<uint> offsets) : _tp{tp}, _dynamicOffsets{offsets} {}

		// ResourceGraph //
		void operator () (const PipelineResources::Buffer &buf);
		void operator () (const PipelineResources::Image &img);
		//void operator () (const PipelineResources::Texture &tex);
		//void operator () (const PipelineResources::SubpassInput &sp);
		void operator () (const PipelineResources::Sampler &) {}
		void operator () (const PipelineResources::RayTracingScene &);
		void operator () (const std::monostate &) {}
	};

	
	
	//
	// Draw Task Barriers
	//
	class VTaskProcessor::DrawTaskBarriers final
	{
	// types
	private:
		using FragmentOutput	= GraphicsPipelineDesc::FragmentOutput;
		using FragOutputs_t		= StaticArray< FragmentOutput, FG_MaxColorBuffers >;
		using UniqFragOutputs_t	= HashSet< const VGraphicsPipeline::FragmentOutputInstance *>;


	// variables
	private:
		VTaskProcessor &			_tp;
		VLogicalRenderPass const&	_logicalRP;

		FragOutputs_t				_fragOutput;
		UniqFragOutputs_t			_uniqueOutputs;
		uint						_maxFragCount;

		bool						_earlyFragmentTests		: 1;
		bool						_lateFragmentTests		: 1;
		bool						_depthWrite				: 1;
		bool						_stencilWrite			: 1;
		bool						_rasterizerDiscard		: 1;
		bool						_compatibleFragOutput	: 1;


	// methods
	public:
		explicit DrawTaskBarriers (VTaskProcessor &tp, const VLogicalRenderPass &);

		void Visit (const VFgDrawTask<DrawVertices> &task);
		void Visit (const VFgDrawTask<DrawIndexed> &task);
		void Visit (const VFgDrawTask<DrawMeshes> &task);
		void Visit (const VFgDrawTask<DrawVerticesIndirect> &task);
		void Visit (const VFgDrawTask<DrawIndexedIndirect> &task);
		void Visit (const VFgDrawTask<DrawMeshesIndirect> &task);

		template <typename PipelineType>
		void _MergePipeline (const _fg_hidden_::DynamicStates &, const PipelineType *);
		
		template <typename DrawTask>
		void _ExtractDescriptorSets (RawPipelineLayoutID layoutId, const DrawTask &task);

		ND_ bool						IsEarlyFragmentTests ()			const	{ return _earlyFragmentTests; }
		ND_ bool						IsLateFragmentTests ()			const	{ return _lateFragmentTests; }
		ND_ bool						IsFragmentOutputCompatible ()	const	{ return _compatibleFragOutput; }
		ND_ bool						HasDepthWriteAccess ()			const	{ return _depthWrite; }
		ND_ bool						HasStencilWriteAccess ()		const	{ return _stencilWrite; }
		ND_ bool						IsRasterizerDiscard ()			const	{ return _rasterizerDiscard; }

		ND_ ArrayView<FragmentOutput>	GetFragOutputs ()				const	{ return ArrayView<FragmentOutput>{ _fragOutput.data(), _maxFragCount }; }
	};


	
	//
	// Draw Task Commands
	//
	class VTaskProcessor::DrawTaskCommands final
	{
	// variables
	private:
		VTaskProcessor &					_tp;
		VDevice const&						_dev;
		VFgTask<SubmitRenderPass> const*	_currTask;
		VkCommandBuffer						_cmdBuffer;


	// methods
	public:
		explicit DrawTaskCommands (VTaskProcessor &tp, VFgTask<SubmitRenderPass> const* task, VkCommandBuffer cmd);

		void Visit (const VFgDrawTask<DrawVertices> &task);
		void Visit (const VFgDrawTask<DrawIndexed> &task);
		void Visit (const VFgDrawTask<DrawMeshes> &task);
		void Visit (const VFgDrawTask<DrawVerticesIndirect> &task);
		void Visit (const VFgDrawTask<DrawIndexedIndirect> &task);
		void Visit (const VFgDrawTask<DrawMeshesIndirect> &task);

	private:
		void _BindVertexBuffers (ArrayView<VLocalBuffer const*> vertexBuffers, ArrayView<VkDeviceSize> vertexOffsets) const;

		template <typename DrawTask>
		void _BindPipelineResources (const VPipelineLayout &layout, const DrawTask &task) const;
	};
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	operator (Buffer)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const PipelineResources::Buffer &buf)
	{
		VLocalBuffer const*	buffer	= _tp._ToLocal( buf.bufferId );
		if ( not buffer )
			return;

		const VkDeviceSize	offset	= VkDeviceSize(buf.offset) + (buf.dynamicOffsetIndex < _dynamicOffsets.size() ? _dynamicOffsets[buf.dynamicOffsetIndex] : 0);		
		const VkDeviceSize	size	= VkDeviceSize( buf.size == ~0_b ? buffer->Size() - offset : buf.size );

		// validation
		{
			ASSERT( offset < buffer->Size() );
			ASSERT( offset + size <= buffer->Size() );

			auto&	limits	= _tp._frameGraph.GetDevice().GetDeviceProperties().limits;

			if ( (buf.state & EResourceState::_StateMask) == EResourceState::UniformRead )
			{
				ASSERT( (offset % limits.minUniformBufferOffsetAlignment) == 0 );
				ASSERT( size <= limits.maxUniformBufferRange );
			}else{
				ASSERT( (offset % limits.minStorageBufferOffsetAlignment) == 0 );
				ASSERT( size <= limits.maxStorageBufferRange );
			}
		}

		_tp._AddBuffer( buffer, buf.state, offset, size );
	}
		
/*
=================================================
	operator (Image / Texture / SubpassInput)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const PipelineResources::Image &img)
	{
		VLocalImage const*  image = _tp._ToLocal( img.imageId );

		if ( image )
			_tp._AddImage( image, img.state, EResourceState_ToImageLayout( img.state, image->AspectMask() ), img.desc );
	}
		
/*
=================================================
	operator (RayTracingScene)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const PipelineResources::RayTracingScene &rts)
	{
		VLocalRTScene const*	scene = _tp._ToLocal( rts.sceneId );
		if ( not scene )
			return;

		_tp._AddRTScene( scene, EResourceState::RayTracingShaderRead );
		ASSERT( scene->GeometryInstances().size() );

		for (auto& inst : scene->GeometryInstances())
		{
			VLocalRTGeometry const*		geom = _tp._ToLocal( inst );

			if ( geom )
				_tp._AddRTGeometry( geom, EResourceState::RayTracingShaderRead );
		}
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VTaskProcessor::DrawTaskBarriers::DrawTaskBarriers (VTaskProcessor &tp, const VLogicalRenderPass &logicalRP) :
		_tp{ tp },						_logicalRP{ logicalRP },
		_maxFragCount{ 0 },
		_earlyFragmentTests{false},		_lateFragmentTests{false},
		_depthWrite{ _logicalRP.GetDepthState().write },
		_stencilWrite{ false },
		_rasterizerDiscard{ _logicalRP.GetRasterizationState().rasterizerDiscard },
		_compatibleFragOutput{true}
	{
		// invalidate fragment output
		for (auto& frag : _fragOutput)
		{
			frag.index	= UMax;
			frag.type	= EFragOutput::Unknown;
		}

		auto&	stencil_st = _logicalRP.GetStencilState();
		_stencilWrite |= not stencil_st.enabled ? 0 :
						 (stencil_st.front.failOp		!= EStencilOp::Keep) |
						 (stencil_st.front.depthFailOp	!= EStencilOp::Keep) |
						 (stencil_st.front.passOp		!= EStencilOp::Keep) |
						 (stencil_st.back.failOp		!= EStencilOp::Keep) |
						 (stencil_st.back.depthFailOp	!= EStencilOp::Keep) |
						 (stencil_st.back.passOp		!= EStencilOp::Keep);
	}

/*
=================================================
	Visit (DrawVertices)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawVertices> &task)
	{
		// update descriptor sets and add pipeline barriers
		_ExtractDescriptorSets( task.pipeline->GetLayoutID(), task );

		// add vertex buffers
		for (size_t i = 0; i < task.GetVertexBuffers().size(); ++i)
		{
			const VkDeviceSize	vb_offset	= task.GetVBOffsets()[i];
			const VkDeviceSize	stride		= VkDeviceSize(task.GetVBStrides()[i]);

			for (auto& cmd : task.commands)
			{
				VkDeviceSize	offset	= vb_offset + stride * cmd.firstVertex;
				VkDeviceSize	size	= stride * cmd.vertexCount;

				_tp._AddBuffer( task.GetVertexBuffers()[i], EResourceState::VertexBuffer, offset, size );
			}
		}
		
		_MergePipeline( task.dynamicStates, task.pipeline );
	}

/*
=================================================
	Visit (DrawIndexed)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawIndexed> &task)
	{
		// update descriptor sets and add pipeline barriers
		_ExtractDescriptorSets( task.pipeline->GetLayoutID(), task );

		for (auto& cmd : task.commands)
		{
			// add vertex buffers
			for (size_t i = 0; i < task.GetVertexBuffers().size(); ++i)
			{
				_tp._AddBuffer( task.GetVertexBuffers()[i], EResourceState::VertexBuffer, task.GetVBOffsets()[i], VK_WHOLE_SIZE );
			}
		
			// add index buffer
			{
				const VkDeviceSize	index_size	= VkDeviceSize(EIndex_SizeOf( task.indexType ));
				const VkDeviceSize	offset		= VkDeviceSize(task.indexBufferOffset);
				const VkDeviceSize	size		= index_size * cmd.indexCount;

				_tp._AddBuffer( task.indexBuffer, EResourceState::IndexBuffer, offset, size );
			}
		}
		
		_MergePipeline( task.dynamicStates, task.pipeline );
	}
	
/*
=================================================
	Visit (DrawVerticesIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawVerticesIndirect> &task)
	{
		// update descriptor sets and add pipeline barriers
		_ExtractDescriptorSets( task.pipeline->GetLayoutID(), task );

		// add vertex buffers
		for (size_t i = 0; i < task.GetVertexBuffers().size(); ++i)
		{
			_tp._AddBuffer( task.GetVertexBuffers()[i], EResourceState::VertexBuffer, task.GetVBOffsets()[i], VK_WHOLE_SIZE );
		}
		
		_MergePipeline( task.dynamicStates, task.pipeline );
	}
	
/*
=================================================
	Visit (DrawIndexedIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawIndexedIndirect> &task)
	{
		// update descriptor sets and add pipeline barriers
		_ExtractDescriptorSets( task.pipeline->GetLayoutID(), task );
		
		// add vertex buffers
		for (size_t i = 0; i < task.GetVertexBuffers().size(); ++i)
		{
			_tp._AddBuffer( task.GetVertexBuffers()[i], EResourceState::VertexBuffer, task.GetVBOffsets()[i], VK_WHOLE_SIZE );
		}
		
		// add index buffer
		_tp._AddBuffer( task.indexBuffer, EResourceState::IndexBuffer, VkDeviceSize(task.indexBufferOffset), VK_WHOLE_SIZE );

		// add indirect buffer
		for (auto& cmd : task.commands)
		{
			_tp._AddBuffer( task.indirectBuffer, EResourceState::IndirectBuffer, VkDeviceSize(cmd.indirectBufferOffset), VkDeviceSize(cmd.stride) * cmd.drawCount );
		}
		
		_MergePipeline( task.dynamicStates, task.pipeline );
	}
	
/*
=================================================
	Visit (DrawMeshes)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawMeshes> &task)
	{
		// update descriptor sets and add pipeline barriers
		_ExtractDescriptorSets( task.pipeline->GetLayoutID(), task );
		
		_MergePipeline( task.dynamicStates, task.pipeline );
	}
	
/*
=================================================
	Visit (DrawMeshesIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawMeshesIndirect> &task)
	{
		// update descriptor sets and add pipeline barriers
		_ExtractDescriptorSets( task.pipeline->GetLayoutID(), task );
		
		// add indirect buffer
		for (auto& cmd : task.commands)
		{
			_tp._AddBuffer( task.indirectBuffer, EResourceState::IndirectBuffer, VkDeviceSize(cmd.indirectBufferOffset), VkDeviceSize(cmd.stride) * cmd.drawCount );
		}

		_MergePipeline( task.dynamicStates, task.pipeline );
	}
	
/*
=================================================
	_ExtractDescriptorSets
=================================================
*/
	template <typename DrawTask>
	inline void VTaskProcessor::DrawTaskBarriers::_ExtractDescriptorSets (RawPipelineLayoutID layoutId, const DrawTask &task)
	{
		_tp._ExtractDescriptorSets( *_tp._GetResource( layoutId ), task.GetResources(), OUT task.descriptorSets );
	}

/*
=================================================
	_MergePipeline
=================================================
*/
	template <typename PipelineType>
	void VTaskProcessor::DrawTaskBarriers::_MergePipeline (const _fg_hidden_::DynamicStates &ds,const PipelineType* pipeline)
	{
		STATIC_ASSERT(	(IsSameTypes<PipelineType, VGraphicsPipeline>) or
						(IsSameTypes<PipelineType, VMeshPipeline>) );

		// merge fragment output
		auto const*		inst = pipeline->GetFragmentOutput();

		if ( _uniqueOutputs.insert( inst ).second )
		{
			for (const auto& src : inst->Get())
			{
				ASSERT( src.index < _fragOutput.size() );

				auto&	dst = _fragOutput[ src.index ];

				_maxFragCount = Max( _maxFragCount, src.index+1 );

				if ( dst.type == EFragOutput::Unknown and dst.index == UMax )
				{
					dst = src;
					continue;
				}

				if ( src.id   != dst.id	  or
					 src.type != dst.type )
				{
					ASSERT( src.id == dst.id );
					ASSERT( src.type == dst.type );

					_compatibleFragOutput = false;
				}
			}
		}
		
		pipeline->IsEarlyFragmentTests() ? _earlyFragmentTests = true : _lateFragmentTests = true;

		_depthWrite |= (ds.hasDepthWrite & ds.depthWrite);

		_stencilWrite |= not (ds.hasStencilTest or _logicalRP.GetStencilState().enabled) ? false :
						 bool(ds.hasStencilFailOp      & (ds.stencilFailOp      != EStencilOp::Keep)) |
						 bool(ds.hasStencilDepthFailOp & (ds.stencilDepthFailOp != EStencilOp::Keep)) |
						 bool(ds.hasStencilPassOp      & (ds.stencilPassOp      != EStencilOp::Keep));

		_rasterizerDiscard &= not _logicalRP.GetRasterizationState().rasterizerDiscard;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	VTaskProcessor::DrawTaskCommands::DrawTaskCommands (VTaskProcessor &tp, VFgTask<SubmitRenderPass> const* task, VkCommandBuffer cmd) :
		_tp{ tp },	_dev{ tp._dev },	_currTask{ task },	_cmdBuffer{ cmd }
	{
	}

/*
=================================================
	_BindVertexBuffers
=================================================
*/
	void VTaskProcessor::DrawTaskCommands::_BindVertexBuffers (ArrayView<VLocalBuffer const*> vertexBuffers, ArrayView<VkDeviceSize> vertexOffsets) const
	{
		if ( vertexBuffers.empty() )
			return;

		FixedArray<VkBuffer, FG_MaxVertexBuffers>	buffers;	buffers.resize( vertexBuffers.size() );

		for (size_t i = 0; i < vertexBuffers.size(); ++i)
		{
			buffers[i] = vertexBuffers[i]->Handle();
		}

		_dev.vkCmdBindVertexBuffers( _cmdBuffer, 0, uint(buffers.size()), buffers.data(), vertexOffsets.data() );
		_tp.Stat().vertexBufferBindings++;
	}

/*
=================================================
	_BindPipelineResources
=================================================
*/
	template <typename DrawTask>
	void VTaskProcessor::DrawTaskCommands::_BindPipelineResources (const VPipelineLayout &layout, const DrawTask &task) const
	{
		if ( task.descriptorSets.size() )
		{
			_dev.vkCmdBindDescriptorSets( _cmdBuffer,
										  VK_PIPELINE_BIND_POINT_GRAPHICS,
										  layout.Handle(),
										  layout.GetFirstDescriptorSet(),
										  uint(task.descriptorSets.size()),
										  task.descriptorSets.data(),
										  uint(task.GetResources().dynamicOffsets.size()),
										  task.GetResources().dynamicOffsets.data() );
			_tp.Stat().descriptorBinds++;
		}
		
		if ( task.GetDebugModeIndex() != UMax )
		{
			VkDescriptorSet		desc_set;
			uint				offset, binding;
			_tp._frameGraph.GetShaderDebugger()->GetDescriptotSet( task.GetDebugModeIndex(), OUT binding, OUT desc_set, OUT offset );
			
			_dev.vkCmdBindDescriptorSets( _cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout.Handle(), binding, 1, &desc_set, 1, &offset );
			_tp.Stat().descriptorBinds++;
		}
	}

/*
=================================================
	Visit (DrawVertices)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawVertices> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );

		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_tp._SetScissor( _currTask->GetLogicalPass(), task.GetScissors() );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDraw( _cmdBuffer, cmd.vertexCount, cmd.instanceCount, cmd.firstVertex, cmd.firstInstance );
		}
		_tp.Stat().drawCalls += uint(task.commands.size());
	}
	
/*
=================================================
	Visit (DrawIndexed)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawIndexed> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );
		
		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_tp._SetScissor( _currTask->GetLogicalPass(), task.GetScissors() );
		_tp._BindIndexBuffer( task.indexBuffer->Handle(), VkDeviceSize(task.indexBufferOffset), VEnumCast(task.indexType) );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDrawIndexed( _cmdBuffer, cmd.indexCount, cmd.instanceCount,
								   cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance );
		}
		_tp.Stat().drawCalls += uint(task.commands.size());
	}
	
/*
=================================================
	Visit (DrawVerticesIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawVerticesIndirect> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );
		
		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_tp._SetScissor( _currTask->GetLogicalPass(), task.GetScissors() );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDrawIndirect( _cmdBuffer,
									task.indirectBuffer->Handle(),
									VkDeviceSize(cmd.indirectBufferOffset),
								    cmd.drawCount,
								    uint(cmd.stride) );
		}
		_tp.Stat().drawCalls += uint(task.commands.size());
	}
	
/*
=================================================
	Visit (DrawIndexedIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawIndexedIndirect> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );
		
		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_tp._SetScissor( _currTask->GetLogicalPass(), task.GetScissors() );
		_tp._BindIndexBuffer( task.indexBuffer->Handle(), VkDeviceSize(task.indexBufferOffset), VEnumCast(task.indexType) );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDrawIndexedIndirect( _cmdBuffer,
										   task.indirectBuffer->Handle(),
										   VkDeviceSize(cmd.indirectBufferOffset),
										   cmd.drawCount,
										   uint(cmd.stride) );
		}
		_tp.Stat().drawCalls += uint(task.commands.size());
	}
		
/*
=================================================
	Visit (DrawMeshes)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawMeshes> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );
		
		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );
		
		_tp._SetScissor( _currTask->GetLogicalPass(), task.GetScissors() );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDrawMeshTasksNV( _cmdBuffer, cmd.count, cmd.first );
		}
		_tp.Stat().drawCalls += uint(task.commands.size());
	}
	
/*
=================================================
	Visit (DrawMeshesIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawMeshesIndirect> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );
		
		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );
		
		_tp._SetScissor( _currTask->GetLogicalPass(), task.GetScissors() );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDrawMeshTasksIndirectNV( _cmdBuffer,
											   task.indirectBuffer->Handle(),
											   VkDeviceSize(cmd.indirectBufferOffset),
											   cmd.drawCount,
											   uint(cmd.stride) );
		}
		_tp.Stat().drawCalls += uint(task.commands.size());
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VTaskProcessor::VTaskProcessor (VFrameGraphThread &fg, VBarrierManager &barrierMngr,
									VkCommandBuffer cmdbuf, const CommandBatchID &batchId, uint indexInBatch) :
		_frameGraph{ fg },								_dev{ fg.GetDevice() },
		_cmdBuffer{ cmdbuf },							_enableDebugUtils{ false },  //_dev.IsDebugUtilsEnabled() },	// because of crash
		_isDefaultScissor{ false },						_perPassStatesUpdated{ false },
		_pendingResourceBarriers{ fg.GetAllocator() },	_barrierMngr{ barrierMngr }
	{
		ASSERT( _cmdBuffer );

		_CmdPushDebugGroup( "SubBatch: "s << batchId.GetName() << ", index: " << ToString(indexInBatch) );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VTaskProcessor::~VTaskProcessor ()
	{
		_CmdPopDebugGroup();
	}
	
/*
=================================================
	Visit*_DrawVertices
=================================================
*/
	void VTaskProcessor::Visit1_DrawVertices (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawVertices>*>( taskData ) );
	}
	
	void VTaskProcessor::Visit2_DrawVertices (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawVertices>*>( taskData ) );
	}
	
/*
=================================================
	Visit*_DrawIndexed
=================================================
*/
	void VTaskProcessor::Visit1_DrawIndexed (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawIndexed>*>( taskData ) );
	}

	void VTaskProcessor::Visit2_DrawIndexed (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawIndexed>*>( taskData ) );
	}
	
/*
=================================================
	Visit*_DrawMeshes
=================================================
*/
	void VTaskProcessor::Visit1_DrawMeshes (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawMeshes>*>( taskData ) );
	}

	void VTaskProcessor::Visit2_DrawMeshes (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawMeshes>*>( taskData ) );
	}
	
/*
=================================================
	Visit*_DrawVerticesIndirect
=================================================
*/
	void VTaskProcessor::Visit1_DrawVerticesIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawVerticesIndirect>*>( taskData ) );
	}

	void VTaskProcessor::Visit2_DrawVerticesIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawVerticesIndirect>*>( taskData ) );
	}
	
/*
=================================================
	Visit*_DrawIndexedIndirect
=================================================
*/
	void VTaskProcessor::Visit1_DrawIndexedIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawIndexedIndirect>*>( taskData ) );
	}

	void VTaskProcessor::Visit2_DrawIndexedIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawIndexedIndirect>*>( taskData ) );
	}
	
/*
=================================================
	Visit*_DrawMeshesIndirect
=================================================
*/
	void VTaskProcessor::Visit1_DrawMeshesIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawMeshesIndirect>*>( taskData ) );
	}

	void VTaskProcessor::Visit2_DrawMeshesIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<DrawMeshesIndirect>*>( taskData ) );
	}

/*
=================================================
	_CmdDebugMarker
=================================================
*/
	void VTaskProcessor::_CmdDebugMarker (StringView text) const
	{
		if ( text.size() and _enableDebugUtils )
		{
			VkDebugUtilsLabelEXT	info = {};
			info.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			info.pLabelName	= text.data();
			MemCopy( info.color, _dbgColor );
			
			_dev.vkCmdInsertDebugUtilsLabelEXT( _cmdBuffer, &info );
		}
	}

/*
=================================================
	_CmdPushDebugGroup
=================================================
*/
	void VTaskProcessor::_CmdPushDebugGroup (StringView text) const
	{
		if ( text.size() and _enableDebugUtils )
		{
			VkDebugUtilsLabelEXT	info = {};
			info.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			info.pLabelName	= text.data();
			MemCopy( info.color, _dbgColor );

			_dev.vkCmdBeginDebugUtilsLabelEXT( _cmdBuffer, &info );
		}
	}

/*
=================================================
	_CmdPopDebugGroup
=================================================
*/
	void VTaskProcessor::_CmdPopDebugGroup () const
	{
		if ( _enableDebugUtils )
		{
			_dev.vkCmdEndDebugUtilsLabelEXT( _cmdBuffer );
		}
	}
	
/*
=================================================
	_OnProcessTask
=================================================
*/
	forceinline void  VTaskProcessor::_OnProcessTask () const
	{
		_CmdDebugMarker( _currTask->Name() );
		
		if ( _frameGraph.GetDebugger() )
			_frameGraph.GetDebugger()->RunTask( _currTask );
	}
	
/*
=================================================
	_SetScissor
=================================================
*/
	void VTaskProcessor::_SetScissor (const VLogicalRenderPass *logicalPsss, ArrayView<RectI> srcScissors)
	{
		if ( not srcScissors.empty() )
		{
			FixedArray< VkRect2D, FG_MaxViewports >		vk_scissors;

			for (auto& src : srcScissors)
			{
				VkRect2D		dst = {};
				dst.offset.x		= src.left;
				dst.offset.y		= src.top;
				dst.extent.width	= uint(src.Width());
				dst.extent.height	= uint(src.Height());
				vk_scissors.push_back( dst );
			}

			_dev.vkCmdSetScissor( _cmdBuffer, 0, uint(vk_scissors.size()), vk_scissors.data() );
			_isDefaultScissor = false;
		}
		else
		if ( not _isDefaultScissor )
		{
			const auto&	vk_scissors = logicalPsss->GetScissors();

			_dev.vkCmdSetScissor( _cmdBuffer, 0, uint(vk_scissors.size()), vk_scissors.data() );
			_isDefaultScissor = true;
		}
	}
	
/*
=================================================
	_SetDynamicStates
=================================================
*/
	void VTaskProcessor::_SetDynamicStates (const _fg_hidden_::DynamicStates &state) const
	{
		if ( state.hasStencilTest and state.stencilTest )
		{
			if ( state.hasStencilCompareMask )
				_dev.vkCmdSetStencilCompareMask( _cmdBuffer, VK_STENCIL_FRONT_AND_BACK, state.stencilCompareMask );

			if ( state.hasStencilReference )
				_dev.vkCmdSetStencilReference( _cmdBuffer, VK_STENCIL_FRONT_AND_BACK, state.stencilReference );

			if ( state.hasStencilWriteMask )
				_dev.vkCmdSetStencilWriteMask( _cmdBuffer, VK_STENCIL_FRONT_AND_BACK, state.stencilWriteMask );

			Stat().dynamicStateChanges += uint(state.hasStencilCompareMask) + uint(state.hasStencilReference) + uint(state.hasStencilWriteMask);
		}
	}

/*
=================================================
	_AddRenderTargetBarriers
=================================================
*/
	void VTaskProcessor::_AddRenderTargetBarriers (const VLogicalRenderPass &logicalRP, const DrawTaskBarriers &info)
	{
		CHECK( info.IsFragmentOutputCompatible() );

		if ( logicalRP.GetDepthStencilTarget().IsDefined() )
		{
			auto &			rt		= logicalRP.GetDepthStencilTarget();
			EResourceState	state	= rt.state;

			state |= (info.IsEarlyFragmentTests() ? EResourceState::EarlyFragmentTests : Default);
			state |= (info.IsLateFragmentTests()  ? EResourceState::LateFragmentTests : Default);
			state &= ~(info.HasDepthWriteAccess() ? EResourceState::Unknown : EResourceState::_Write);
			
			VkImageLayout&	layout	= rt._layout;
			layout = EResourceState_ToImageLayout( state, rt.imagePtr->AspectMask() );

			_AddImage( rt.imagePtr, state, layout, rt.desc );
		}

		if ( info.IsRasterizerDiscard() )
			return;

		for (auto& rt : logicalRP.GetColorTargets())
		{
			EResourceState	state	= rt.second.state;
			VkImageLayout&	layout	= rt.second._layout;
			layout = EResourceState_ToImageLayout( state, rt.second.imagePtr->AspectMask() );

			_AddImage( rt.second.imagePtr, state, layout, rt.second.desc );
		}
	}
	
/*
=================================================
	_SetShadingRateImage
=================================================
*/
	void VTaskProcessor::_SetShadingRateImage (const VLogicalRenderPass &logicalRP, OUT VkImageView &view)
	{
		VLocalImage const*	image;
		ImageViewDesc		desc;

		if ( not logicalRP.GetShadingRateImage( OUT image, OUT desc ) )
			return;

		view = image->GetView( _dev, false, desc );

		_AddImage( image, EResourceState::ShadingRateImageRead, VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV, desc );
	}
	
/*
=================================================
	_BindShadingRateImage
=================================================
*/
	void VTaskProcessor::_BindShadingRateImage (VkImageView view)
	{
		if ( _shadingRateImage != view )
		{
			_shadingRateImage = view;
			_dev.vkCmdBindShadingRateImageNV( _cmdBuffer, _shadingRateImage, VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV );
		}
	}

/*
=================================================
	_ExtractClearValues
=================================================
*/
	void VTaskProcessor::_ExtractClearValues (const VLogicalRenderPass &logicalRP, const VRenderPass *rp, OUT VkClearValues_t &clearValues) const
	{
		const uint	col_offset = uint(logicalRP.GetDepthStencilTarget().IsDefined());

		clearValues.resize( logicalRP.GetColorTargets().size() + col_offset );

		if ( logicalRP.GetDepthStencilTarget().IsDefined() )
		{
			clearValues[0] = logicalRP.GetDepthStencilTarget().clearValue;
		}

		for (const auto& ct : logicalRP.GetColorTargets())
		{
			uint	index;
			CHECK( rp->GetColorAttachmentIndex( ct.first, OUT index ));

			clearValues[index + col_offset] = ct.second.clearValue;
		}
	}

/*
=================================================
	_BeginRenderPass
=================================================
*/
	void VTaskProcessor::_BeginRenderPass (const VFgTask<SubmitRenderPass> &task)
	{
		ASSERT( not task.IsSubpass() );

		FixedArray< VLogicalRenderPass*, 32 >	logical_passes;

		for (auto* iter = &task; iter != null; iter = iter->GetNextSubpass())
		{
			logical_passes.push_back( iter->GetLogicalPass() );
		}

		
		// add barriers
		DrawTaskBarriers	barrier_visitor{ *this, *logical_passes.front() };

		for (auto& pass : logical_passes)
		{
			for (auto& draw : pass->GetDrawTasks())
			{
				draw->Process1( &barrier_visitor );
			}
		}
		
		VkImageView  sri_view = VK_NULL_HANDLE;
		_SetShadingRateImage( *task.GetLogicalPass(), OUT sri_view );

		_AddRenderTargetBarriers( *task.GetLogicalPass(), barrier_visitor );
		_CommitBarriers();


		// create render pass and framebuffer
		CHECK( _frameGraph.GetResourceManager()->GetRenderPassCache()->
					CreateRenderPasses( *_frameGraph.GetResourceManager(), logical_passes, barrier_visitor.GetFragOutputs() ));
		

		// begin render pass
		VFramebuffer const*	framebuffer = _GetResource( task.GetLogicalPass()->GetFramebufferID() );
		VRenderPass const*	render_pass = _GetResource( task.GetLogicalPass()->GetRenderPassID() );
		RectI const&		area		= task.GetLogicalPass()->GetArea();

		VkClearValues_t		clear_values;
		_ExtractClearValues( *task.GetLogicalPass(), render_pass, OUT clear_values );

		VkRenderPassBeginInfo	pass_info = {};
		pass_info.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		pass_info.renderPass				= render_pass->Handle();
		pass_info.renderArea.offset.x		= area.left;
		pass_info.renderArea.offset.y		= area.top;
		pass_info.renderArea.extent.width	= uint(area.Width());
		pass_info.renderArea.extent.height	= uint(area.Height());
		pass_info.clearValueCount			= uint(clear_values.size());
		pass_info.pClearValues				= clear_values.data();
		pass_info.framebuffer				= framebuffer->Handle();
		
		_dev.vkCmdBeginRenderPass( _cmdBuffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE );

		_BindShadingRateImage( sri_view );
	}
	
/*
=================================================
	_BeginSubpass
=================================================
*/
	void VTaskProcessor::_BeginSubpass (const VFgTask<SubmitRenderPass> &task)
	{
		ASSERT( task.IsSubpass() );

		// TODO: barriers for attachments

		_dev.vkCmdNextSubpass( _cmdBuffer, VK_SUBPASS_CONTENTS_INLINE );
		/*
		// TODO
		vkCmdClearAttachments( _cmdBuffer,
							   uint( attachments.Count() ),
							   attachments.ptr(),
							   uint( clear_rects.Count() ),
							   clear_rects.ptr() );*/
	}

/*
=================================================
	Visit (SubmitRenderPass)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<SubmitRenderPass> &task)
	{
		// invalidate some states
		_isDefaultScissor		= false;
		_perPassStatesUpdated	= false;

		if ( not task.IsSubpass() )
		{
			_CmdPushDebugGroup( task.Name() );
			_OnProcessTask();
			_BeginRenderPass( task );
		}
		else
		{
			_OnProcessTask();
			_BeginSubpass( task );
		}


		// draw
		DrawTaskCommands	command_builder{ *this, &task, _cmdBuffer };
		
		for (auto& draw : task.GetLogicalPass()->GetDrawTasks())
		{
			draw->Process2( &command_builder );
		}

		// end render pass
		if ( task.IsLastPass() )
		{
			_dev.vkCmdEndRenderPass( _cmdBuffer );
			_CmdPopDebugGroup();
		}
	}
	
/*
=================================================
	_ExtractDescriptorSets
=================================================
*/
	void VTaskProcessor::_ExtractDescriptorSets (const VPipelineLayout &layout, const VPipelineResourceSet &resourceSet,
												 OUT VkDescriptorSets_t &descriptorSets)
	{
		const FixedArray< uint, FG_MaxBufferDynamicOffsets >	old_offsets = resourceSet.dynamicOffsets;
		StaticArray< Pair<uint, uint>, FG_MaxDescriptorSets >	new_offsets = {};
		const uint												first_ds	= layout.GetFirstDescriptorSet();

		descriptorSets.resize( resourceSet.resources.size() );

		for (size_t i = 0; i < resourceSet.resources.size(); ++i)
		{
			const auto &				res		 = resourceSet.resources[i];
			uint						binding	 = 0;
			RawDescriptorSetLayoutID	ds_layout;

			if ( not layout.GetDescriptorSetLayout( res.descSetId, OUT ds_layout, OUT binding ))
				continue;

			PipelineResourceBarriers	visitor{ *this, resourceSet.dynamicOffsets };

			for (auto& un : res.pplnRes->GetResources()) {
				std::visit( visitor, un.res );
			}

			ASSERT( ds_layout == res.pplnRes->GetLayoutID() );
			ASSERT( binding >= first_ds );
			binding -= first_ds;

			descriptorSets[binding] = res.pplnRes->Handle();
			new_offsets[binding]    = { res.offsetIndex, res.offsetCount };
		}

		// sort dynamic offsets by binding index
		uint	dst = 0;
		for (auto& item : new_offsets)
		{
			for (uint i = item.first; i < item.second; ++i, ++dst)
			{
				resourceSet.dynamicOffsets[dst] = old_offsets[i];
			}
		}
	}
	
/*
=================================================
	_BindPipelineResources
=================================================
*/
	void VTaskProcessor::_BindPipelineResources (const VPipelineLayout &layout, const VPipelineResourceSet &resourceSet,
												 VkPipelineBindPoint bindPoint, uint debugModeIndex)
	{
		// update descriptor sets and add pipeline barriers
		VkDescriptorSets_t	descriptor_sets;
		_ExtractDescriptorSets( layout, resourceSet, OUT descriptor_sets );

		if ( descriptor_sets.size() )
		{
			_dev.vkCmdBindDescriptorSets( _cmdBuffer,
										  bindPoint,
										  layout.Handle(),
										  layout.GetFirstDescriptorSet(),
										  uint(descriptor_sets.size()),
										  descriptor_sets.data(),
										  uint(resourceSet.dynamicOffsets.size()),
										  resourceSet.dynamicOffsets.data() );
			Stat().descriptorBinds++;
		}

		if ( debugModeIndex != UMax )
		{
			VkDescriptorSet		desc_set;
			uint				offset, binding;
			_frameGraph.GetShaderDebugger()->GetDescriptotSet( debugModeIndex, OUT binding, OUT desc_set, OUT offset );

			_dev.vkCmdBindDescriptorSets( _cmdBuffer, bindPoint, layout.Handle(), binding, 1, &desc_set, 1, &offset );
			Stat().descriptorBinds++;
		}
	}

/*
=================================================
	_PushConstants
=================================================
*/
	void VTaskProcessor::_PushConstants (const VPipelineLayout &layout, const _fg_hidden_::PushConstants_t &pushConstants) const
	{
		auto const&		pc_map = layout.GetPushConstants();
			
		ASSERT( pushConstants.size() == pc_map.size() );	// used push constants from previous draw/dispatch calls or may contains undefined values

		for (auto& pc : pushConstants)
		{
			auto	iter = pc_map.find( pc.id );
			ASSERT( iter != pc_map.end() );

			if ( iter != pc_map.end() )
			{
				ASSERT( pc.size == iter->second.size );

				_dev.vkCmdPushConstants( _cmdBuffer, layout.Handle(), VEnumCast( iter->second.stageFlags ), uint(iter->second.offset),
										 uint(iter->second.size), pc.data );
				Stat().pushConstants++;
			}
		}
	}

/*
=================================================
	_BindPipeline2
=================================================
*/
	inline void VTaskProcessor::_BindPipeline2 (const VLogicalRenderPass &logicalRP, VkPipeline pipelineId)
	{
		if ( _graphicsPipeline.pipeline != pipelineId )
		{
			_graphicsPipeline.pipeline = pipelineId;
			_dev.vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineId );
			Stat().graphicsPipelineBindings++;
		}
		
		// all pipelines in current render pass have same viewport count and same dynamic states, so this values should not be invalidated.
		if ( _perPassStatesUpdated )
			return;

		_perPassStatesUpdated = true;
		
		if ( auto viewports = logicalRP.GetViewports(); viewports.size() )
			_dev.vkCmdSetViewport( _cmdBuffer, 0, uint(viewports.size()), viewports.data() );

		if ( auto palette = logicalRP.GetShadingRatePalette(); palette.size() )
			_dev.vkCmdSetViewportShadingRatePaletteNV( _cmdBuffer, 0, uint(palette.size()), palette.data() );
	}
	
/*
=================================================
	_BindPipeline
=================================================
*/
	inline void VTaskProcessor::_BindPipeline (const VLogicalRenderPass &logicalRP, const VBaseDrawVerticesTask &task, VPipelineLayout const* &pplnLayout)
	{
		VkPipeline	ppln_id;
		_frameGraph.GetPipelineCache()->CreatePipelineInstance(
									*_frameGraph.GetResourceManager(),
									*_frameGraph.GetShaderDebugger(),
									logicalRP, task,
									OUT ppln_id, OUT pplnLayout );

		_BindPipeline2( logicalRP, ppln_id );
	}
	
	inline void VTaskProcessor::_BindPipeline (const VLogicalRenderPass &logicalRP, const VBaseDrawMeshes &task, VPipelineLayout const* &pplnLayout)
	{
		VkPipeline	ppln_id;
		_frameGraph.GetPipelineCache()->CreatePipelineInstance(
									*_frameGraph.GetResourceManager(),
									*_frameGraph.GetShaderDebugger(),
									logicalRP, task,
									OUT ppln_id, OUT pplnLayout );
		
		_BindPipeline2( logicalRP, ppln_id );
	}

/*
=================================================
	_BindPipeline
=================================================
*/
	inline void VTaskProcessor::_BindPipeline (const VComputePipeline* pipeline, const Optional<uint3> &localSize,
											   uint debugModeIndex, VkPipelineCreateFlags flags, OUT VPipelineLayout const* &pplnLayout)
	{
		VkPipeline	ppln_id;
		_frameGraph.GetPipelineCache()->CreatePipelineInstance(
										*_frameGraph.GetResourceManager(),
										*_frameGraph.GetShaderDebugger(),
										*pipeline, localSize,
										flags,
										debugModeIndex,
										OUT ppln_id, OUT pplnLayout );
		
		if ( _computePipeline.pipeline != ppln_id )
		{
			_computePipeline.pipeline = ppln_id;
			_dev.vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ppln_id );
			Stat().computePipelineBindings++;
		}
	}

/*
=================================================
	Visit (DispatchCompute)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<DispatchCompute> &task)
	{
		_OnProcessTask();

		VPipelineLayout const*	layout = null;

		_BindPipeline( task.pipeline, task.localGroupSize, task.GetDebugModeIndex(), VK_PIPELINE_CREATE_DISPATCH_BASE, OUT layout );
		_BindPipelineResources( *layout, task.GetResources(), VK_PIPELINE_BIND_POINT_COMPUTE, task.GetDebugModeIndex() );
		_PushConstants( *layout, task.pushConstants );

		_CommitBarriers();

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDispatchBase( _cmdBuffer, cmd.baseGroup.x, cmd.baseGroup.y, cmd.baseGroup.z,
								    cmd.groupCount.x, cmd.groupCount.y, cmd.groupCount.z );
		}
		Stat().dispatchCalls += uint(task.commands.size());
	}
	
/*
=================================================
	Visit (DispatchComputeIndirect)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<DispatchComputeIndirect> &task)
	{
		_OnProcessTask();
		
		VPipelineLayout const*	layout = null;

		_BindPipeline( task.pipeline, task.localGroupSize, task.GetDebugModeIndex(), 0, OUT layout );
		_BindPipelineResources( *layout, task.GetResources(), VK_PIPELINE_BIND_POINT_COMPUTE, task.GetDebugModeIndex() );
		_PushConstants( *layout, task.pushConstants );
		
		for (auto& cmd : task.commands)
		{
			_AddBuffer( task.indirectBuffer, EResourceState::IndirectBuffer, VkDeviceSize(cmd.indirectBufferOffset),
					    sizeof(DispatchComputeIndirect::DispatchIndirectCommand) );
		}
		_CommitBarriers();
		
		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDispatchIndirect( _cmdBuffer,
									    task.indirectBuffer->Handle(),
										VkDeviceSize(cmd.indirectBufferOffset) );
		}
		Stat().dispatchCalls += uint(task.commands.size());
	}

/*
=================================================
	Visit (CopyBuffer)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<CopyBuffer> &task)
	{
		_OnProcessTask();

		VLocalBuffer const *	src_buffer	= task.srcBuffer;
		VLocalBuffer const *	dst_buffer	= task.dstBuffer;
		BufferCopyRegions_t		regions;	regions.resize( task.regions.size() );

		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&	src = task.regions[i];
			auto&		dst = regions[i];

			dst.srcOffset	= VkDeviceSize( src.srcOffset );
			dst.dstOffset	= VkDeviceSize( src.dstOffset );
			dst.size		= VkDeviceSize( src.size );

			ASSERT( src.size + src.srcOffset <= src_buffer->Size() );
			ASSERT( src.size + src.dstOffset <= dst_buffer->Size() );

			if ( src_buffer == dst_buffer ) {
				ASSERT( IsIntersects( dst.srcOffset, dst.size == VK_WHOLE_SIZE ? dst.size : dst.srcOffset + dst.size,
									  dst.dstOffset, dst.size == VK_WHOLE_SIZE ? dst.size : dst.dstOffset + dst.size ));
			}

			_AddBuffer( src_buffer, EResourceState::TransferSrc, dst.srcOffset, dst.size );
			_AddBuffer( dst_buffer, EResourceState::TransferDst, dst.dstOffset, dst.size );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdCopyBuffer( _cmdBuffer,
							  src_buffer->Handle(),
							  dst_buffer->Handle(),
							  uint(regions.size()),
							  regions.data() );

		Stat().transferOps++;
	}
	
/*
=================================================
	Visit (CopyImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<CopyImage> &task)
	{
		_OnProcessTask();

		VLocalImage const *		src_image	= task.srcImage;
		VLocalImage const *		dst_image	= task.dstImage;
		ImageCopyRegions_t		regions;	regions.resize( task.regions.size() );

		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&		src					= task.regions[i];
			auto&			dst					= regions[i];
			const uint3		image_size			= Max( src.size, 1u );

			dst.srcSubresource.aspectMask		= VEnumCast( src.srcSubresource.aspectMask, src_image->PixelFormat() );
			dst.srcSubresource.mipLevel			= src.srcSubresource.mipLevel.Get();
			dst.srcSubresource.baseArrayLayer	= src.srcSubresource.baseLayer.Get();
			dst.srcSubresource.layerCount		= src.srcSubresource.layerCount;
			dst.srcOffset						= VkOffset3D{ src.srcOffset.x, src.srcOffset.y, src.srcOffset.z };

			dst.dstSubresource.aspectMask		= VEnumCast( src.dstSubresource.aspectMask, dst_image->PixelFormat() );
			dst.dstSubresource.mipLevel			= src.dstSubresource.mipLevel.Get();
			dst.dstSubresource.baseArrayLayer	= src.dstSubresource.baseLayer.Get();
			dst.dstSubresource.layerCount		= src.dstSubresource.layerCount;
			dst.dstOffset						= VkOffset3D{ src.dstOffset.x, src.dstOffset.y, src.dstOffset.z };

			dst.extent							= VkExtent3D{ image_size.x, image_size.y, image_size.z };

			ASSERT( src.srcSubresource.mipLevel < src_image->Description().maxLevel );
			ASSERT( src.dstSubresource.mipLevel < dst_image->Description().maxLevel );
			ASSERT( src.srcSubresource.baseLayer.Get() + src.srcSubresource.layerCount <= src_image->ArrayLayers() );
			ASSERT( src.srcSubresource.baseLayer.Get() + src.dstSubresource.layerCount <= dst_image->ArrayLayers() );
			//ASSERT(All( src.srcOffset + src.size <= Max(1u, src_image.Dimension().xyz() >> src.srcSubresource.mipLevel.Get()) ));
			//ASSERT(All( src.dstOffset + src.size <= Max(1u, dst_image.Dimension().xyz() >> src.dstSubresource.mipLevel.Get()) ));

			_AddImage( src_image, EResourceState::TransferSrc, task.srcLayout, dst.srcSubresource );
			_AddImage( dst_image, EResourceState::TransferDst, task.dstLayout, dst.dstSubresource );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdCopyImage( _cmdBuffer,
							 src_image->Handle(),
							 task.srcLayout,
							 dst_image->Handle(),
							 task.dstLayout,
							 uint(regions.size()),
							 regions.data() );
		
		Stat().transferOps++;
	}
	
/*
=================================================
	Visit (CopyBufferToImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<CopyBufferToImage> &task)
	{
		_OnProcessTask();

		VLocalBuffer const *		src_buffer	= task.srcBuffer;
		VLocalImage const *			dst_image	= task.dstImage;
		BufferImageCopyRegions_t	regions;	regions.resize( task.regions.size() );
		
		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&			src			= task.regions[i];
			auto&				dst			= regions[i];
			const int3			img_offset	= int3(src.imageOffset);
			const uint3			img_size	= Max( src.imageSize, 1u );

			dst.bufferOffset					= VkDeviceSize( src.bufferOffset );
			dst.bufferRowLength					= src.bufferRowLength;
			dst.bufferImageHeight				= src.bufferImageHeight;

			dst.imageSubresource.aspectMask		= VEnumCast( src.imageLayers.aspectMask, dst_image->PixelFormat() );
			dst.imageSubresource.mipLevel		= src.imageLayers.mipLevel.Get();
			dst.imageSubresource.baseArrayLayer	= src.imageLayers.baseLayer.Get();
			dst.imageSubresource.layerCount		= src.imageLayers.layerCount;
			dst.imageOffset						= VkOffset3D{ img_offset.x, img_offset.y, img_offset.z };
			dst.imageExtent						= VkExtent3D{ img_size.x, img_size.y, img_size.z };

			_AddBuffer( src_buffer, EResourceState::TransferSrc, dst, dst_image );
			_AddImage(  dst_image,  EResourceState::TransferDst, task.dstLayout, dst.imageSubresource );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdCopyBufferToImage( _cmdBuffer,
									 src_buffer->Handle(),
									 dst_image->Handle(),
									 task.dstLayout,
									 uint(regions.size()),
									 regions.data() );
		
		Stat().transferOps++;
	}
	
/*
=================================================
	Visit (CopyImageToBuffer)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<CopyImageToBuffer> &task)
	{
		_OnProcessTask();

		VLocalImage const *			src_image	= task.srcImage;
		VLocalBuffer const *		dst_buffer	= task.dstBuffer;
		BufferImageCopyRegions_t	regions;	regions.resize( task.regions.size() );
		
		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&			src			= task.regions[i];
			auto&				dst			= regions[i];
			const uint3			image_size	= Max( src.imageSize, 1u );

			dst.bufferOffset					= VkDeviceSize( src.bufferOffset );
			dst.bufferRowLength					= src.bufferRowLength;
			dst.bufferImageHeight				= src.bufferImageHeight;

			dst.imageSubresource.aspectMask		= VEnumCast( src.imageLayers.aspectMask, src_image->PixelFormat() );
			dst.imageSubresource.mipLevel		= src.imageLayers.mipLevel.Get();
			dst.imageSubresource.baseArrayLayer	= src.imageLayers.baseLayer.Get();
			dst.imageSubresource.layerCount		= src.imageLayers.layerCount;
			dst.imageOffset						= VkOffset3D{ src.imageOffset.x, src.imageOffset.y, src.imageOffset.z };
			dst.imageExtent						= VkExtent3D{ image_size.x, image_size.y, image_size.z };

			_AddImage(  src_image,  EResourceState::TransferSrc, task.srcLayout, dst.imageSubresource );
			_AddBuffer( dst_buffer, EResourceState::TransferDst, dst, src_image );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdCopyImageToBuffer( _cmdBuffer,
									 src_image->Handle(),
									 task.srcLayout,
									 dst_buffer->Handle(),
									 uint(regions.size()),
									 regions.data() );
		
		Stat().transferOps++;
	}
	
/*
=================================================
	Visit (BlitImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<BlitImage> &task)
	{
		_OnProcessTask();

		VLocalImage const *		src_image	= task.srcImage;
		VLocalImage const *		dst_image	= task.dstImage;
		BlitRegions_t			regions;	regions.resize( task.regions.size() );
		
		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&		src	= task.regions[i];
			auto&			dst	= regions[i];
				
			dst.srcSubresource.aspectMask		= VEnumCast( src.srcSubresource.aspectMask, src_image->PixelFormat() );
			dst.srcSubresource.mipLevel			= src.srcSubresource.mipLevel.Get();
			dst.srcSubresource.baseArrayLayer	= src.srcSubresource.baseLayer.Get();
			dst.srcSubresource.layerCount		= src.srcSubresource.layerCount;
			dst.srcOffsets[0]					= VkOffset3D{ src.srcOffset0.x, src.srcOffset0.y, src.srcOffset0.z };
			dst.srcOffsets[1]					= VkOffset3D{ src.srcOffset1.x, src.srcOffset1.y, src.srcOffset1.z };
			
			dst.dstSubresource.aspectMask		= VEnumCast( src.dstSubresource.aspectMask, dst_image->PixelFormat() );
			dst.dstSubresource.mipLevel			= src.dstSubresource.mipLevel.Get();
			dst.dstSubresource.baseArrayLayer	= src.dstSubresource.baseLayer.Get();
			dst.dstSubresource.layerCount		= src.dstSubresource.layerCount;
			dst.dstOffsets[0]					= VkOffset3D{ src.dstOffset0.x, src.dstOffset0.y, src.dstOffset0.z };
			dst.dstOffsets[1]					= VkOffset3D{ src.dstOffset1.x, src.dstOffset1.y, src.dstOffset1.z };

			_AddImage( src_image, EResourceState::TransferSrc, task.srcLayout, dst.srcSubresource );
			_AddImage( dst_image, EResourceState::TransferDst, task.dstLayout, dst.dstSubresource );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdBlitImage( _cmdBuffer,
							 src_image->Handle(),
							 task.srcLayout,
							 dst_image->Handle(),
							 task.dstLayout,
							 uint(regions.size()),
							 regions.data(),
							 task.filter );
		
		Stat().transferOps++;
	}
	
/*
=================================================
	Visit (ResolveImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<ResolveImage> &task)
	{
		_OnProcessTask();
		
		VLocalImage	const *		src_image	= task.srcImage;
		VLocalImage const *		dst_image	= task.dstImage;
		ResolveRegions_t		regions;	regions.resize( task.regions.size() );
		
		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&			src				= task.regions[i];
			auto&				dst				= regions[i];
			const uint3			image_size		= Max( src.extent, 1u );

			dst.srcSubresource.aspectMask		= VEnumCast( src.srcSubresource.aspectMask, src_image->PixelFormat() );
			dst.srcSubresource.mipLevel			= src.srcSubresource.mipLevel.Get();
			dst.srcSubresource.baseArrayLayer	= src.srcSubresource.baseLayer.Get();
			dst.srcSubresource.layerCount		= src.srcSubresource.layerCount;
			dst.srcOffset						= VkOffset3D{ src.srcOffset.x, src.srcOffset.y, src.srcOffset.z };
			
			dst.dstSubresource.aspectMask		= VEnumCast( src.dstSubresource.aspectMask, dst_image->PixelFormat() );
			dst.dstSubresource.mipLevel			= src.dstSubresource.mipLevel.Get();
			dst.dstSubresource.baseArrayLayer	= src.dstSubresource.baseLayer.Get();
			dst.dstSubresource.layerCount		= src.dstSubresource.layerCount;
			dst.dstOffset						= VkOffset3D{ src.dstOffset.x, src.dstOffset.y, src.dstOffset.z };
			
			dst.extent							= VkExtent3D{ image_size.x, image_size.y, image_size.z };

			_AddImage( src_image, EResourceState::TransferSrc, task.srcLayout, dst.srcSubresource );
			_AddImage( dst_image, EResourceState::TransferDst, task.dstLayout, dst.dstSubresource );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdResolveImage(	_cmdBuffer,
								src_image->Handle(),
								task.srcLayout,
								dst_image->Handle(),
								task.dstLayout,
								uint(regions.size()),
								regions.data() );
		
		Stat().transferOps++;
	}
	
/*
=================================================
	Visit (FillBuffer)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<FillBuffer> &task)
	{
		_OnProcessTask();

		VLocalBuffer const *	dst_buffer = task.dstBuffer;

		_AddBuffer( dst_buffer, EResourceState::TransferDst, task.dstOffset, task.size );
		
		_CommitBarriers();
		
		_dev.vkCmdFillBuffer( _cmdBuffer,
							  dst_buffer->Handle(),
							  task.dstOffset,
							  task.size,
							  task.pattern );
		
		Stat().transferOps++;
	}
	
/*
=================================================
	Visit (ClearColorImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<ClearColorImage> &task)
	{
		_OnProcessTask();

		VLocalImage const *		dst_image	= task.dstImage;
		ImageClearRanges_t		ranges;		ranges.resize( task.ranges.size() );
		
		for (size_t i = 0, count = ranges.size(); i < count; ++i)
		{
			const auto&		src	= task.ranges[i];
			auto&			dst	= ranges[i];
			
			dst.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			dst.baseMipLevel	= src.baseMipLevel.Get();
			dst.levelCount		= src.levelCount;
			dst.baseArrayLayer	= src.baseLayer.Get();
			dst.layerCount		= src.layerCount;

			_AddImage( dst_image, EResourceState::TransferDst, task.dstLayout, dst );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdClearColorImage( _cmdBuffer,
								   dst_image->Handle(),
								   task.dstLayout,
								   &task.ClearValue(),
								   uint(ranges.size()),
								   ranges.data() );
		
		Stat().transferOps++;
	}
	
/*
=================================================
	Visit (ClearDepthStencilImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<ClearDepthStencilImage> &task)
	{
		_OnProcessTask();
		
		VLocalImage const *		dst_image	= task.dstImage;
		ImageClearRanges_t		ranges;		ranges.resize( task.ranges.size() );
		
		for (size_t i = 0, count = ranges.size(); i < count; ++i)
		{
			const auto&		src	= task.ranges[i];
			auto&			dst	= ranges[i];
			
			dst.aspectMask		= VEnumCast( src.aspectMask, dst_image->PixelFormat() );
			dst.baseMipLevel	= src.baseMipLevel.Get();
			dst.levelCount		= src.levelCount;
			dst.baseArrayLayer	= src.baseLayer.Get();
			dst.layerCount		= src.layerCount;

			_AddImage( dst_image, EResourceState::TransferDst, task.dstLayout, dst );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdClearDepthStencilImage( _cmdBuffer,
										  dst_image->Handle(),
										  task.dstLayout,
										  &task.clearValue,
										  uint(ranges.size()),
										  ranges.data() );
		
		Stat().transferOps++;
	}

/*
=================================================
	Visit (UpdateBuffer)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<UpdateBuffer> &task)
	{
		_OnProcessTask();
		
		VLocalBuffer const *	dst_buffer = task.dstBuffer;

		for (auto& reg : task.Regions()) {
			_AddBuffer( dst_buffer, EResourceState::TransferDst, reg.bufferOffset, reg.dataSize );
		}	
		_CommitBarriers();
		
		for (auto& reg : task.Regions()) {
			_dev.vkCmdUpdateBuffer( _cmdBuffer, dst_buffer->Handle(), reg.bufferOffset, reg.dataSize, reg.dataPtr );
		}

		Stat().transferOps += uint(task.Regions().size());
	}

/*
=================================================
	Visit (Present)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<Present> &task)
	{
		_OnProcessTask();
		
		RawImageID	swapchain_image;
		CHECK( _frameGraph.GetSwapchain()->Acquire( ESwapchainImage::Primary, OUT swapchain_image ));

		VLocalImage const *		src_image	= task.srcImage;
		VLocalImage const *		dst_image	= _ToLocal( swapchain_image );
		VkImageBlit				region;
		
		region.srcSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel			= 0;
		region.srcSubresource.baseArrayLayer	= task.layer.Get();
		region.srcSubresource.layerCount		= 1;
		region.srcOffsets[0]					= VkOffset3D{ 0, 0, 0 };
		region.srcOffsets[1]					= VkOffset3D{ int(src_image->Width()), int(src_image->Height()), 1 };
			
		region.dstSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel			= 0;
		region.dstSubresource.baseArrayLayer	= 0;
		region.dstSubresource.layerCount		= 1;
		region.dstOffsets[0]					= VkOffset3D{ 0, 0, 0 };
		region.dstOffsets[1]					= VkOffset3D{ int(dst_image->Width()), int(dst_image->Height()), 1 };

		_AddImage( src_image, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, region.srcSubresource );

		VkImageMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask		= 0;
		barrier.dstAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.image				= dst_image->Handle();
		barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		_barrierMngr.AddImageBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier );

		_CommitBarriers();
		
		_dev.vkCmdBlitImage( _cmdBuffer,
							 src_image->Handle(),
							 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							 dst_image->Handle(),
							 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							 1, &region,
							 VK_FILTER_NEAREST );
		
		barrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask	= 0;
		barrier.oldLayout		= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		_barrierMngr.AddImageBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, barrier );
	}

/*
=================================================
	Visit (PresentVR)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<PresentVR> &task)
	{
		_OnProcessTask();

		/*_AddImage( task.GetLeftEyeImage(), EResourceState::PresentImage | EResourceState::_InvalidateAfter, EImageLayout::PresentSrc,
				   ImageRange{ task.GetLeftEyeLayer(), 1, 0_mipmap, 1 }, EImageAspect::Color );
		
		_AddImage( task.GetRightEyeImage(), EResourceState::PresentImage | EResourceState::_InvalidateAfter, EImageLayout::PresentSrc,
				   ImageRange{ task.GetRightEyeLayer(), 1, 0_mipmap, 1 }, EImageAspect::Color );

		_CommitBarriers();*/
		
		// TODO
	}
	
/*
=================================================
	Visit (UpdateRayTracingShaderTable)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<UpdateRayTracingShaderTable> &task)
	{
		_OnProcessTask();

		const uint		geom_stride		= task.rtScene->HitShadersPerGeometry();
		const uint		max_hit_shaders	= task.rtScene->MaxHitShaderCount();
		const auto		instances		= task.rtScene->GeometryInstances();
		const BytesU	sh_size			= task.pipeline->ShaderHandleSize();
		const BytesU	req_size		= sh_size * (1 + task.missShaders.size() + max_hit_shaders);

		RawBufferID		staging_buffer;
		BytesU			buf_offset, buf_size, ptr_offset;
		void *			mapped_ptr = null;
		
		CHECK_ERR( task.dstBuffer->Size() >= task.dstOffset + req_size, void());
		CHECK_ERR( _frameGraph.GetStagingBufferManager()->GetWritableBuffer( req_size, req_size, OUT staging_buffer, OUT buf_offset, OUT buf_size, OUT mapped_ptr ), void());
		
		const auto	CopyShaderGroupHandle = [&ptr_offset, mapped_ptr, buf_size, pipeline = task.pipeline] (const RTShaderGroupID &id) -> bool
											{
												auto	sh_handle = pipeline->GetShaderGroupHandle( id );
												CHECK_ERR( not sh_handle.empty() );	// group is not exist in pipeline

												MemCopy( mapped_ptr + ptr_offset, buf_size - ptr_offset, sh_handle.data(), ArraySizeOf(sh_handle) );
												ptr_offset += ArraySizeOf(sh_handle);
												return true;
											};

		CHECK( CopyShaderGroupHandle( task.rayGenShader.groupId ));

		for (auto& shader : task.missShaders)
		{
			CHECK( CopyShaderGroupHandle( shader.groupId ));
		}

		for (auto& shader : task.hitShaders)
		{
			CHECK_ERR( shader.instance < instances.size(), void());

			RawRTGeometryID				geom_id	= instances[ shader.instance ];
			VRayTracingGeometry const*	geom	= _GetResource( geom_id );
			size_t						index	= geom->GetGeometryIndex( shader.geometryId );

			CHECK( index * geom_stride < max_hit_shaders );
			
			auto	sh_handle = task.pipeline->GetShaderGroupHandle( shader.groupId );
			CHECK( not sh_handle.empty() );	// group is not exist in pipeline
			
			BytesU	offset = ptr_offset + sh_size * index * geom_stride;
			MemCopy( mapped_ptr + offset, buf_size - offset, sh_handle.data(), ArraySizeOf(sh_handle) );
		}


		VkBufferCopy	region = {};
		region.srcOffset	= VkDeviceSize( buf_offset );
		region.dstOffset	= task.dstOffset;
		region.size			= VkDeviceSize( req_size );

		VLocalBuffer const*		src_buffer = _ToLocal( staging_buffer );
		_AddBuffer( src_buffer, EResourceState::TransferSrc, region.srcOffset, region.size );
		_AddBuffer( task.dstBuffer, EResourceState::TransferDst, region.dstOffset, region.size );
		
		_CommitBarriers();
		
		_dev.vkCmdCopyBuffer( _cmdBuffer, src_buffer->Handle(), task.dstBuffer->Handle(), 1, &region );
		
		Stat().transferOps++;
	}

/*
=================================================
	Visit (BuildRayTracingGeometry)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<BuildRayTracingGeometry> &task)
	{
		_OnProcessTask();
		
		_AddRTGeometry( task.RTGeometry(), EResourceState::BuildRayTracingStructWrite );
		_AddBuffer( task.ScratchBuffer(), EResourceState::RTASBuildingBufferReadWrite, 0, VK_WHOLE_SIZE );
		
		for (auto& buf : task.GetBuffers())
		{
			// resource state doesn't matter
			_AddBuffer( buf, EResourceState::TransferSrc, 0, VK_WHOLE_SIZE );
		}

		_CommitBarriers();

		VkAccelerationStructureInfoNV	info = {};
		info.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		info.type			= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
		info.geometryCount	= uint(task.GetGeometry().size());
		info.pGeometries	= task.GetGeometry().data();
		info.flags			= VEnumCast( task.RTGeometry()->GetFlags() );

		_dev.vkCmdBuildAccelerationStructureNV( _cmdBuffer, &info,
												VK_NULL_HANDLE, 0,
												VK_FALSE,
												task.RTGeometry()->Handle(), VK_NULL_HANDLE,
												task.ScratchBuffer()->Handle(),
												task.ScratchBufferOffset()
											);
		Stat().buildASCalls++;
	}
	
/*
=================================================
	Visit (BuildRayTracingScene)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<BuildRayTracingScene> &task)
	{
		_OnProcessTask();
		
		task.RTScene()->SetGeometryInstances( task.GeometryIDs(), task.HitShadersPerGeometry(), task.MaxHitShaderCount() );

		_AddRTScene( task.RTScene(), EResourceState::BuildRayTracingStructWrite );
		_AddBuffer( task.ScratchBuffer(), EResourceState::RTASBuildingBufferReadWrite, 0, VK_WHOLE_SIZE );
		_AddBuffer( task.InstanceBuffer(), EResourceState::RTASBuildingBufferRead, 0, VK_WHOLE_SIZE );

		for (auto& blas : task.Geometries()) {
			_AddRTGeometry( blas, EResourceState::BuildRayTracingStructRead );
		}

		_CommitBarriers();
		
		VkAccelerationStructureInfoNV	info = {};
		info.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		info.type			= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		info.flags			= VEnumCast( task.RTScene()->GetFlags() );
		info.instanceCount	= task.InstanceCount();

		_dev.vkCmdBuildAccelerationStructureNV( _cmdBuffer, &info,
												task.InstanceBuffer()->Handle(), task.InstanceBufferOffset(),
												VK_FALSE,
												task.RTScene()->Handle(), VK_NULL_HANDLE,
												task.ScratchBuffer()->Handle(),
												task.ScratchBufferOffset()
											);
		Stat().buildASCalls++;
	}
	
/*
=================================================
	_BindPipeline
=================================================
*/
	inline void VTaskProcessor::_BindPipeline (const VRayTracingPipeline* pipeline)
	{
		VkPipeline	ppln_id = pipeline->Handle();
		
		if ( _rayTracingPipeline.pipeline != ppln_id )
		{
			_rayTracingPipeline.pipeline = ppln_id;
			_dev.vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, ppln_id );
			Stat().rayTracingPipelineBindings++;
		}
	}
	
/*
=================================================
	Visit (TraceRays)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<TraceRays> &task)
	{
		_OnProcessTask();
		
		auto*	layout = _GetResource( task.pipeline->GetLayoutID() );

		_BindPipeline( task.pipeline );
		_BindPipelineResources( *layout, task.GetResources(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, task.GetDebugModeIndex() );
		_PushConstants( *layout, task.pushConstants );

		_AddBuffer( task.sbtBuffer, EResourceState::UniformRead | EResourceState::_RayTracingShader,
				    Min( task.rayGenOffset, task.rayMissOffset, task.rayHitOffset ), VK_WHOLE_SIZE );

		_CommitBarriers();
		
		_dev.vkCmdTraceRaysNV( _cmdBuffer, 
								task.sbtBuffer->Handle(), task.rayGenOffset,
								task.sbtBuffer->Handle(), task.rayMissOffset, task.rayMissStride,
								task.sbtBuffer->Handle(), task.rayHitOffset,  task.rayHitStride,
								VK_NULL_HANDLE, 0, 0,
								task.groupCount.x, task.groupCount.y, task.groupCount.z );

		Stat().traceRaysCalls++;
	}

/*
=================================================
	_AddImageState
=================================================
*/
	inline void VTaskProcessor::_AddImageState (const VLocalImage *img, const ImageState &state)
	{
		ASSERT( img );
		ASSERT( not state.range.IsEmpty() );

		_pendingResourceBarriers.insert({ img, &CommitResourceBarrier<VLocalImage> });

		img->AddPendingState( state );

		if ( _frameGraph.GetDebugger() )
			_frameGraph.GetDebugger()->AddImageUsage( img->ToGlobal(), state );
	}
	
/*
=================================================
	_AddImage
=================================================
*/
	inline void VTaskProcessor::_AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, const ImageViewDesc &desc)
	{
		ASSERT( desc.layerCount > 0 and desc.levelCount > 0 );

		_AddImageState( img,
						ImageState{
							state, layout,
							ImageRange{ desc.baseLayer, desc.layerCount, desc.baseLevel, desc.levelCount },
							(EPixelFormat_HasDepth( desc.format )   ? VK_IMAGE_ASPECT_DEPTH_BIT   :
							 EPixelFormat_HasStencil( desc.format ) ? VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT) | 0u,
							_currTask
						});
	}

/*
=================================================
	_AddImage
=================================================
*/
	inline void VTaskProcessor::_AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, const VkImageSubresourceLayers &subres)
	{
		_AddImageState( img,
						ImageState{
							state, layout,
							ImageRange{ ImageLayer(subres.baseArrayLayer), subres.layerCount, MipmapLevel(subres.mipLevel), 1 },
							subres.aspectMask,
							_currTask
						});
	}
	
/*
=================================================
	_AddImage
=================================================
*/
	inline void VTaskProcessor::_AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, const VkImageSubresourceRange &subres)
	{
		_AddImageState( img,
						ImageState{
							state, layout,
							ImageRange{ ImageLayer(subres.baseArrayLayer), subres.layerCount, MipmapLevel(subres.baseMipLevel), subres.levelCount },
							subres.aspectMask,
							_currTask
						});
	}

/*
=================================================
	_AddBufferState
=================================================
*/
	inline void VTaskProcessor::_AddBufferState (const VLocalBuffer *buf, const BufferState &state)
	{
		ASSERT( buf );
		_pendingResourceBarriers.insert({ buf, &CommitResourceBarrier<VLocalBuffer> });

		buf->AddPendingState( state );
		
		if ( _frameGraph.GetDebugger() )
			_frameGraph.GetDebugger()->AddBufferUsage( buf->ToGlobal(), state );
	}

/*
=================================================
	_AddBuffer
=================================================
*/
	inline void VTaskProcessor::_AddBuffer (const VLocalBuffer *buf, EResourceState state, VkDeviceSize offset, VkDeviceSize size)
	{
		ASSERT( buf );
		ASSERT( size > 0 );

		//if ( buf->IsImmutable() )
		//	return;

		const VkDeviceSize	buf_size = VkDeviceSize(buf->Size());
		
		size = Min( buf_size, (size == VK_WHOLE_SIZE ? buf_size - offset : offset + size) );

		_AddBufferState( buf, BufferState{ state, offset, size, _currTask });
	}
	
/*
=================================================
	_AddBuffer
=================================================
*/
	inline void VTaskProcessor::_AddBuffer (const VLocalBuffer *buf, EResourceState state,
											const VkBufferImageCopy &reg, const VLocalImage *img)
	{
		ASSERT( img );

		//if ( buf->IsImmutable() )
		//	return;

		const uint			bpp			= EPixelFormat_BitPerPixel( img->PixelFormat(), FGEnumCast( VkImageAspectFlagBits(reg.imageSubresource.aspectMask) ));
		const VkDeviceSize	row_pitch	= (reg.bufferRowLength * bpp) / 8;
		const VkDeviceSize	slice_pitch	= reg.bufferImageHeight * row_pitch;
		const uint			dim_z		= Max( reg.imageSubresource.layerCount, reg.imageExtent.depth );
		BufferState			buf_state	{ state, 0, slice_pitch * dim_z, _currTask };

#if 1
		// one big barrier
		buf_state.range += reg.bufferOffset;
		
		_AddBufferState( buf, buf_state );

#else
		const VkDeviceSize	row_size	= (reg.imageExtent.width * bpp) / 8;

		for (uint z = 0; z < dim_z; ++z)
		{
			for (uint y = 0; y < reg.imageExtent.height; ++y)
			{
				buf_state.range = BufferRange{0, row_size} + (reg.bufferOffset + z * slice_pitch + y * row_pitch);
				
				_AddBufferState( buf, buf_state );
			}
		}
#endif
	}
	
/*
=================================================
	_AddRTGeometry
=================================================
*/
	void VTaskProcessor::_AddRTGeometry (const VLocalRTGeometry *geom, EResourceState state)
	{
		ASSERT( geom );
		_pendingResourceBarriers.insert({ geom, &CommitResourceBarrier<VLocalRTGeometry> });

		geom->AddPendingState(RTGeometryState{ state, _currTask });

		if ( _frameGraph.GetDebugger() )
			_frameGraph.GetDebugger()->AddRTGeometryUsage( geom->ToGlobal(), RTGeometryState{ state, _currTask });
	}
	
/*
=================================================
	_AddRTScene
=================================================
*/
	void VTaskProcessor::_AddRTScene (const VLocalRTScene *scene, EResourceState state)
	{
		ASSERT( scene );
		_pendingResourceBarriers.insert({ scene, &CommitResourceBarrier<VLocalRTScene> });

		scene->AddPendingState(RTSceneState{ state, _currTask });

		if ( _frameGraph.GetDebugger() )
			_frameGraph.GetDebugger()->AddRTSceneUsage( scene->ToGlobal(), RTSceneState{ state, _currTask });
	}

/*
=================================================
	_CommitBarriers
=================================================
*/
	inline void VTaskProcessor::_CommitBarriers ()
	{
		for (auto& res : _pendingResourceBarriers)
		{
			res.second( res.first, _barrierMngr, _frameGraph.GetDebugger() );
		}

		_pendingResourceBarriers.clear();

		_barrierMngr.Commit( _dev, _cmdBuffer );
	}
	
/*
=================================================
	_BindIndexBuffer
=================================================
*/
	inline void VTaskProcessor::_BindIndexBuffer (VkBuffer indexBuffer, VkDeviceSize indexOffset, VkIndexType indexType)
	{
		if ( _indexBuffer		!= indexBuffer	or
			 _indexBufferOffset	!= indexOffset	or
			 _indexType			!= indexType )
		{
			_indexBuffer		= indexBuffer;
			_indexBufferOffset	= indexOffset;
			_indexType			= indexType;

			_dev.vkCmdBindIndexBuffer( _cmdBuffer, _indexBuffer, _indexBufferOffset, _indexType );
			Stat().indexBufferBindings++;
		}
	}
	

}	// FG
