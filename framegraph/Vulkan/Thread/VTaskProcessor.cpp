// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VTaskProcessor.h"
#include "VFrameGraphThread.h"
#include "VFrameGraphDebugger.h"
#include "VStagingBufferManager.h"
#include "VDrawTask.h"
#include "VEnumCast.h"
#include "FGEnumCast.h"
#include "Shared/EnumUtils.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

	template <typename ID>
	forceinline auto const*  VTaskProcessor::_GetState (ID id) const
	{
		return _frameGraph.GetResourceManager()->GetState( id );
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
//-----------------------------------------------------------------------------



	//
	// Pipeline Resource Barriers
	//
	class VTaskProcessor::PipelineResourceBarriers
	{
	// types
	private:
		using ResourceSet_t	= PipelineResources::ResourceSet_t;


	// variables
	private:
		VTaskProcessor &	_tp;
		const Task			_currTask;


	// methods
	public:
		PipelineResourceBarriers (VTaskProcessor &tp, Task task) : _tp{tp}, _currTask{task} {}

		// ResourceGraph //
		void operator () (const PipelineResources::Buffer &buf);
		void operator () (const PipelineResources::Image &img);
		//void operator () (const PipelineResources::Texture &tex);
		//void operator () (const PipelineResources::SubpassInput &sp);
		void operator () (const PipelineResources::Sampler &) {}
		void operator () (const PipelineResources::RayTracingScene &) {}	// TODO
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
		bool						_depthRead				: 1;
		bool						_depthWrite				: 1;
		bool						_stencilReadWrite		: 1;
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
		void _MergePipeline (const PipelineType* ppln);

		ND_ bool						IsEarlyFragmentTests () const	{ return _earlyFragmentTests; }
		ND_ bool						IsLateFragmentTests ()	const	{ return _lateFragmentTests; }

		ND_ ArrayView<FragmentOutput>	GetFragOutputs ()		const	{ return ArrayView<FragmentOutput>{ _fragOutput.data(), _maxFragCount }; }
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

		void _BindVertexBuffers (ArrayView<VLocalBuffer const*> vertexBuffers, ArrayView<VkDeviceSize> vertexOffsets) const;
		void _SetScissor (const _fg_hidden_::Scissors_t &sc) const;

		void _BindPipeline (const VLogicalRenderPass &logicalRP, const VBaseDrawVerticesTask &task) const;
		void _BindPipeline (const VLogicalRenderPass &logicalRP, const VBaseDrawMeshes &task) const;
		void _BindPipelineResources (const VPipelineLayout &layout, const VkDescriptorSets_t &descriptorSets, ArrayView<uint> dynamicOffsets) const;
	};
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	operator (Buffer)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const PipelineResources::Buffer &buf)
	{
		if ( not buf.bufferId )
			return;

		VLocalBuffer const *	buffer	= _tp._GetState( buf.bufferId );
		VkDeviceSize const		size	= VkDeviceSize( buf.size == ~0_b ? buffer->Size() - buf.offset : buf.size );

		// validation
		{
			ASSERT( buf.offset < buffer->Size() );
			ASSERT( buf.offset + size <= buffer->Size() );

			auto&	limits	= _tp._frameGraph.GetDevice().GetDeviceProperties().limits;

			if ( (buf.state & EResourceState::_StateMask) == EResourceState::UniformRead )
			{
				ASSERT( (buf.offset % limits.minUniformBufferOffsetAlignment) == 0 );
				ASSERT( size <= limits.maxUniformBufferRange );
			}else{
				ASSERT( (buf.offset % limits.minStorageBufferOffsetAlignment) == 0 );
				ASSERT( size <= limits.maxStorageBufferRange );
			}
		}

		_tp._AddBuffer( buffer, buf.state, VkDeviceSize(buf.offset), VkDeviceSize(buf.size) );
	}
		
/*
=================================================
	operator (Image / Texture / SubpassInput)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const PipelineResources::Image &img)
	{
		if ( not img.imageId )
			return;

		VLocalImage const*  image = _tp._GetState( img.imageId );

		_tp._AddImage( image, img.state, EResourceState_ToImageLayout( img.state ), *img.desc );
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
		_depthRead{false},				_depthWrite{false},
		_stencilReadWrite{false},		_rasterizerDiscard{false},
		_compatibleFragOutput{true}
	{
		// invalidate fragment output
		for (auto& frag : _fragOutput)
		{
			frag.index	= ~0u;
			frag.type	= EFragOutput::Unknown;
		}
	}

/*
=================================================
	Visit (DrawVertices)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawVertices> &task)
	{
		// update descriptor sets and add pipeline barriers
		_tp._ExtractDescriptorSets( task.GetResources(), OUT task.descriptorSets );

		// add vertex buffers
		for (size_t i = 0; i < task.GetVertexBuffers().size(); ++i)
		{
			const VkDeviceSize	vb_offset	= task.GetVBOffsets()[i];
			const VkDeviceSize	stride		= VkDeviceSize(task.GetVBStrides()[i]);

			for (auto& cmd : task.commands)
			{
				VkDeviceSize	offset	= vb_offset + stride * cmd.firstVertex;
				VkDeviceSize	size	= stride * cmd.vertexCount;			// TODO: instance

				_tp._AddBuffer( task.GetVertexBuffers()[i], EResourceState::VertexBuffer, offset, size );
			}
		}
		
		_MergePipeline( task.pipeline );
	}

/*
=================================================
	Visit (DrawIndexed)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawIndexed> &task)
	{
		// update descriptor sets and add pipeline barriers
		_tp._ExtractDescriptorSets( task.GetResources(), OUT task.descriptorSets );

		for (auto& cmd : task.commands)
		{
			// add vertex buffers
			for (size_t i = 0; i < task.GetVertexBuffers().size(); ++i)
			{
				VkDeviceSize		offset	= task.GetVBOffsets()[i];
				VkDeviceSize		size	= VK_WHOLE_SIZE;
				const VkDeviceSize	stride	= VkDeviceSize(task.GetVBStrides()[i]);
			
				offset += VkDeviceSize(stride) * cmd.vertexOffset;
				//size    = VkDeviceSize(stride) * EIndex::MaxValue( task.indexBuffer.indexType );	// TODO: instance

				_tp._AddBuffer( task.GetVertexBuffers()[i], EResourceState::VertexBuffer, offset, size );
			}
		
			// add index buffer
			{
				const VkDeviceSize	index_size	= VkDeviceSize(EIndex_SizeOf( task.indexType ));
				const VkDeviceSize	offset		= VkDeviceSize(task.indexBufferOffset);
				const VkDeviceSize	size		= index_size * cmd.indexCount;

				_tp._AddBuffer( task.indexBuffer, EResourceState::IndexBuffer, offset, size );
			}
		}
		
		_MergePipeline( task.pipeline );
	}
	
/*
=================================================
	Visit (DrawVerticesIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawVerticesIndirect> &task)
	{
		// update descriptor sets and add pipeline barriers
		_tp._ExtractDescriptorSets( task.GetResources(), OUT task.descriptorSets );

		// add vertex buffers
		for (size_t i = 0; i < task.GetVertexBuffers().size(); ++i)
		{
			_tp._AddBuffer( task.GetVertexBuffers()[i], EResourceState::VertexBuffer, task.GetVBOffsets()[i], VK_WHOLE_SIZE );
		}
		
		_MergePipeline( task.pipeline );
	}
	
/*
=================================================
	Visit (DrawIndexedIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawIndexedIndirect> &task)
	{
		// update descriptor sets and add pipeline barriers
		_tp._ExtractDescriptorSets( task.GetResources(), OUT task.descriptorSets );
		
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
		
		_MergePipeline( task.pipeline );
	}
	
/*
=================================================
	Visit (DrawMeshes)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawMeshes> &task)
	{
		// update descriptor sets and add pipeline barriers
		_tp._ExtractDescriptorSets( task.GetResources(), OUT task.descriptorSets );
		
		_MergePipeline( task.pipeline );
	}
	
/*
=================================================
	Visit (DrawMeshesIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<DrawMeshesIndirect> &task)
	{
		// update descriptor sets and add pipeline barriers
		_tp._ExtractDescriptorSets( task.GetResources(), OUT task.descriptorSets );
		
		// add indirect buffer
		for (auto& cmd : task.commands)
		{
			_tp._AddBuffer( task.indirectBuffer, EResourceState::IndirectBuffer, VkDeviceSize(cmd.indirectBufferOffset), VkDeviceSize(cmd.stride) * cmd.drawCount );
		}

		_MergePipeline( task.pipeline );
	}

/*
=================================================
	_MergePipeline
=================================================
*/
	template <typename PipelineType>
	void VTaskProcessor::DrawTaskBarriers::_MergePipeline (const PipelineType* pipeline)
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

				if ( dst.type == EFragOutput::Unknown and dst.index == ~0u )
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
		

		if ( pipeline->IsEarlyFragmentTests() )
			_earlyFragmentTests = true;
		else
			_lateFragmentTests = true;

		if ( _logicalRP.GetDepthState().test )
			_depthRead = true;

		if ( _logicalRP.GetDepthState().write )
			_depthWrite = true;

		if ( _logicalRP.GetStencilState().enabled )
			_stencilReadWrite = true;

		if ( _logicalRP.GetRasterizationState().rasterizerDiscard )
			_rasterizerDiscard = true;
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
	}
	
/*
=================================================
	_BindPipeline
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::_BindPipeline (const VLogicalRenderPass &logicalRP, const VBaseDrawVerticesTask &task) const
	{
		VkPipeline	ppln_id = _tp._frameGraph.GetPipelineCache()->CreatePipelineInstance( *_tp._frameGraph.GetResourceManager(), logicalRP, task );
		
		if ( _tp._graphicsPipeline.pipeline != ppln_id )
		{
			_tp._graphicsPipeline.pipeline = ppln_id;
			_dev.vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln_id );
		}
	}
	
	inline void VTaskProcessor::DrawTaskCommands::_BindPipeline (const VLogicalRenderPass &logicalRP, const VBaseDrawMeshes &task) const
	{
		VkPipeline	ppln_id = _tp._frameGraph.GetPipelineCache()->CreatePipelineInstance( *_tp._frameGraph.GetResourceManager(), logicalRP, task );
		
		if ( _tp._graphicsPipeline.pipeline != ppln_id )
		{
			_tp._graphicsPipeline.pipeline = ppln_id;
			_dev.vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln_id );
		}
	}

/*
=================================================
	_BindPipelineResources
=================================================
*/
	void VTaskProcessor::DrawTaskCommands::_BindPipelineResources (const VPipelineLayout &layout, const VkDescriptorSets_t &descriptorSets, ArrayView<uint> dynamicOffsets) const
	{
		if ( descriptorSets.empty() )
			return;

		_dev.vkCmdBindDescriptorSets( _cmdBuffer,
									  VK_PIPELINE_BIND_POINT_GRAPHICS,
									  layout.Handle(),
									  0,
									  uint(descriptorSets.size()), descriptorSets.data(),
									  uint(dynamicOffsets.size()), dynamicOffsets.data() );
	}

/*
=================================================
	_SetScissor
=================================================
*/
	void VTaskProcessor::DrawTaskCommands::_SetScissor (const _fg_hidden_::Scissors_t &srcScissors) const
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
		}
		else
		{
			const auto&	vk_scissors = _currTask->GetLogicalPass()->GetScissors();

			_dev.vkCmdSetScissor( _cmdBuffer, 0, uint(vk_scissors.size()), vk_scissors.data() );
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

		auto*	layout = _tp._GetResource( task.pipeline->GetLayoutID() );

		_BindPipeline( *_currTask->GetLogicalPass(), task );
		_BindPipelineResources( *layout, task.descriptorSets, task.GetResources().dynamicOffsets );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_SetScissor( task.scissors );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDraw( _cmdBuffer, cmd.vertexCount, cmd.instanceCount, cmd.firstVertex, cmd.firstInstance );
		}
	}
	
/*
=================================================
	Visit (DrawIndexed)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawIndexed> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );
		
		auto*	layout = _tp._GetResource( task.pipeline->GetLayoutID() );

		_BindPipeline( *_currTask->GetLogicalPass(), task );
		_BindPipelineResources( *layout, task.descriptorSets, task.GetResources().dynamicOffsets );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_SetScissor( task.scissors );

		_tp._BindIndexBuffer( task.indexBuffer->Handle(), VkDeviceSize(task.indexBufferOffset), VEnumCast(task.indexType) );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDrawIndexed( _cmdBuffer, cmd.indexCount, cmd.instanceCount,
								   cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance );
		}
	}
	
/*
=================================================
	Visit (DrawVerticesIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawVerticesIndirect> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );

		auto*	layout = _tp._GetResource( task.pipeline->GetLayoutID() );

		_BindPipeline( *_currTask->GetLogicalPass(), task );
		_BindPipelineResources( *layout, task.descriptorSets, task.GetResources().dynamicOffsets );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_SetScissor( task.scissors );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDrawIndirect( _cmdBuffer,
									task.indirectBuffer->Handle(),
									VkDeviceSize(cmd.indirectBufferOffset),
								    cmd.drawCount,
								    uint(cmd.stride) );
		}
	}
	
/*
=================================================
	Visit (DrawIndexedIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawIndexedIndirect> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );
		
		auto*	layout = _tp._GetResource( task.pipeline->GetLayoutID() );

		_BindPipeline( *_currTask->GetLogicalPass(), task );
		_BindPipelineResources( *layout, task.descriptorSets, task.GetResources().dynamicOffsets );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_SetScissor( task.scissors );

		_tp._BindIndexBuffer( task.indexBuffer->Handle(), VkDeviceSize(task.indexBufferOffset), VEnumCast(task.indexType) );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDrawIndexedIndirect( _cmdBuffer,
										   task.indirectBuffer->Handle(),
										   VkDeviceSize(cmd.indirectBufferOffset),
										   cmd.drawCount,
										   uint(cmd.stride) );
		}
	}
		
/*
=================================================
	Visit (DrawMeshes)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawMeshes> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );
		
		auto*	layout = _tp._GetResource( task.pipeline->GetLayoutID() );

		_BindPipeline( *_currTask->GetLogicalPass(), task );
		_BindPipelineResources( *layout, task.descriptorSets, task.GetResources().dynamicOffsets );
		_tp._PushConstants( *layout, task.pushConstants );
		
		_SetScissor( task.scissors );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDrawMeshTasksNV( _cmdBuffer, cmd.count, cmd.first );
		}
	}
	
/*
=================================================
	Visit (DrawMeshesIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<DrawMeshesIndirect> &task)
	{
		_tp._CmdDebugMarker( task.GetName() );
		
		auto*	layout = _tp._GetResource( task.pipeline->GetLayoutID() );

		_BindPipeline( *_currTask->GetLogicalPass(), task );
		_BindPipelineResources( *layout, task.descriptorSets, task.GetResources().dynamicOffsets );
		_tp._PushConstants( *layout, task.pushConstants );
		
		_SetScissor( task.scissors );

		for (auto& cmd : task.commands)
		{
			_dev.vkCmdDrawMeshTasksIndirectNV( _cmdBuffer,
											   task.indirectBuffer->Handle(),
											   VkDeviceSize(cmd.indirectBufferOffset),
											   cmd.drawCount,
											   uint(cmd.stride) );
		}
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VTaskProcessor::VTaskProcessor (VFrameGraphThread &fg, VBarrierManager &barrierMngr, VkCommandBuffer cmdbuf, const CommandBatchID &batchId, uint indexInBatch) :
		_frameGraph{ fg },								_dev{ fg.GetDevice() },
		_cmdBuffer{ cmdbuf },							_enableDebugUtils{ _dev.IsDebugUtilsEnabled() },
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
		if ( text.empty() )
			return;

		/*if ( _enableDebugUtils )
		{
			VkDebugUtilsLabelEXT	info = {};
			info.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			info.pLabelName	= text.data();
			//MemCopy( info.color, _dbgColor );
			
			_dev.vkCmdInsertDebugUtilsLabelEXT( _cmdBuffer, &info );
		}*/
	}

/*
=================================================
	_CmdPushDebugGroup
=================================================
*/
	void VTaskProcessor::_CmdPushDebugGroup (StringView text) const
	{
		ASSERT( not text.empty() );

		/*if ( _enableDebugUtils )
		{
			VkDebugUtilsLabelEXT	info = {};
			info.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			info.pLabelName	= text.data();
			//MemCopy( info.color, _dbgColor );

			_dev.vkCmdBeginDebugUtilsLabelEXT( _cmdBuffer, &info );
		}*/
	}

/*
=================================================
	_CmdPopDebugGroup
=================================================
*/
	void VTaskProcessor::_CmdPopDebugGroup () const
	{
		/*if ( _enableDebugUtils )
		{
			_dev.vkCmdEndDebugUtilsLabelEXT( _cmdBuffer );
		}*/
	}
	
/*
=================================================
	_OnRunTask
=================================================
*/
	forceinline void  VTaskProcessor::_OnRunTask () const
	{
		_CmdDebugMarker( _currTask->Name() );
		
		if ( _frameGraph.GetDebugger() )
			_frameGraph.GetDebugger()->RunTask( _currTask );
	}

/*
=================================================
	_AddRenderTargetBarriers
=================================================
*/
	void VTaskProcessor::_AddRenderTargetBarriers (const VLogicalRenderPass &logicalRP, const DrawTaskBarriers &info)
	{
		if ( logicalRP.GetDepthStencilTarget().IsDefined() )
		{
			const auto &	rt		= logicalRP.GetDepthStencilTarget();
			EResourceState	state	= rt.state;
			VkImageLayout	layout	= EResourceState_ToImageLayout( rt.state );

			if ( info.IsEarlyFragmentTests() )
				state |= EResourceState::EarlyFragmentTests;

			if ( info.IsLateFragmentTests() )
				state |= EResourceState::LateFragmentTests;
			
			_AddImage( rt.imagePtr, state, layout, rt.desc );
		}

		for (const auto& rt : logicalRP.GetColorTargets())
		{
			VkImageLayout	layout = EResourceState_ToImageLayout( rt.second.state );

			_AddImage( rt.second.imagePtr, rt.second.state, layout, rt.second.desc );
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


		// create render pass and framebuffer
		CHECK( _frameGraph.GetResourceManager()->GetRenderPassCache()->
					CreateRenderPasses( *_frameGraph.GetResourceManager(), logical_passes, barrier_visitor.GetFragOutputs() ));
		
		VFramebuffer const*	framebuffer = _GetResource( task.GetLogicalPass()->GetFramebufferID() );

		_AddRenderTargetBarriers( *task.GetLogicalPass(), barrier_visitor );
		_CommitBarriers();


		// begin render pass
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
		if ( not task.IsSubpass() )
		{
			_CmdPushDebugGroup( "renderpass" );		// TODO
			_OnRunTask();
			_BeginRenderPass( task );
		}
		else
		{
			_OnRunTask();
			_BeginSubpass( task );
		}

		
		// set viewports
		const auto&	viewports = task.GetLogicalPass()->GetViewports();
		if ( not viewports.empty() ) {
			_dev.vkCmdSetViewport( _cmdBuffer, 0, uint(viewports.size()), viewports.data() );
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
	void VTaskProcessor::_ExtractDescriptorSets (const VPipelineResourceSet &resourceSet, OUT VkDescriptorSets_t &descriptorSets)
	{
		descriptorSets.resize( resourceSet.resources.size() );

		for (size_t i = 0; i < resourceSet.resources.size(); ++i)
		{
			const auto &				res		 = resourceSet.resources[i];
			VPipelineResources const*	ppln_res = _GetResource( res );
			PipelineResourceBarriers	visitor	 { *this, _currTask };

			for (auto& un : ppln_res->GetData()) {
				std::visit( visitor, un.res );
			}
			descriptorSets[i] = ppln_res->Handle();
		}
	}
	
/*
=================================================
	_BindPipelineResources
=================================================
*/
	void VTaskProcessor::_BindPipelineResources (const VPipelineLayout &layout, const VPipelineResourceSet &resourceSet, VkPipelineBindPoint bindPoint)
	{
		// update descriptor sets and add pipeline barriers
		VkDescriptorSets_t	descriptor_sets;
		_ExtractDescriptorSets( resourceSet, OUT descriptor_sets );

		if ( descriptor_sets.empty() )
			return;

		_dev.vkCmdBindDescriptorSets( _cmdBuffer,
									  bindPoint,
									  layout.Handle(),
									  0,
									  uint(descriptor_sets.size()),
									  descriptor_sets.data(),
									  uint(resourceSet.dynamicOffsets.size()),
									  resourceSet.dynamicOffsets.data() );
	}

/*
=================================================
	_PushConstants
=================================================
*/
	void VTaskProcessor::_PushConstants (const VPipelineLayout &layout, const _fg_hidden_::PushConstants_t &pushConstants) const
	{
		if ( pushConstants.empty() )
			return;

		auto const&		pc_map = layout.GetPushConstants();

		for (auto& pc : pushConstants)
		{
			auto	iter = pc_map.find( pc.id );
			ASSERT( iter != pc_map.end() );

			if ( iter != pc_map.end() )
			{
				ASSERT( pc.size == iter->second.size );

				_dev.vkCmdPushConstants( _cmdBuffer, layout.Handle(), VEnumCast( iter->second.stageFlags ), uint(iter->second.offset),
										 uint(iter->second.size), pc.data );
			}
		}
	}

/*
=================================================
	_BindPipeline
=================================================
*/
	inline void VTaskProcessor::_BindPipeline (const VComputePipeline* pipeline, const Optional<uint3> &localSize)
	{
		VkPipeline	ppln_id = _frameGraph.GetPipelineCache()->CreatePipelineInstance( *_frameGraph.GetResourceManager(), *pipeline, localSize, 0 );
		
		if ( _computePipeline.pipeline != ppln_id )
		{
			_computePipeline.pipeline = ppln_id;
			_dev.vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ppln_id );
		}
	}

/*
=================================================
	Visit (DispatchCompute)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<DispatchCompute> &task)
	{
		_OnRunTask();

		auto*	layout = _GetResource( task.pipeline->GetLayoutID() );

		_BindPipeline( task.pipeline, task.localGroupSize );
		_BindPipelineResources( *layout, task.GetResources(), VK_PIPELINE_BIND_POINT_COMPUTE );
		_PushConstants( *layout, task.pushConstants );

		_CommitBarriers();

		_dev.vkCmdDispatch( _cmdBuffer, task.groupCount.x, task.groupCount.y, task.groupCount.z );
	}
	
/*
=================================================
	Visit (DispatchComputeIndirect)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<DispatchComputeIndirect> &task)
	{
		_OnRunTask();
		
		auto*	layout = _GetResource( task.pipeline->GetLayoutID() );

		_BindPipeline( task.pipeline, task.localGroupSize );
		_BindPipelineResources( *layout, task.GetResources(), VK_PIPELINE_BIND_POINT_COMPUTE );
		_PushConstants( *layout, task.pushConstants );

		_AddBuffer( task.indirectBuffer, EResourceState::IndirectBuffer, task.indirectBufferOffset, VK_WHOLE_SIZE );	// TODO: buffer size
		_CommitBarriers();
		
		_dev.vkCmdDispatchIndirect( _cmdBuffer,
								    task.indirectBuffer->Handle(),
									task.indirectBufferOffset );
	}

/*
=================================================
	Visit (CopyBuffer)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<CopyBuffer> &task)
	{
		_OnRunTask();

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

			// TODO: check buffer intersection
			ASSERT( src.size + src.srcOffset <= src_buffer->Size() );
			ASSERT( src.size + src.dstOffset <= dst_buffer->Size() );

			//if ( task.SrcBuffer()->IsMutable() or task.DstBuffer()->IsMutable() )
			{
				_AddBuffer( src_buffer, EResourceState::TransferSrc, dst.srcOffset, dst.size );
				_AddBuffer( dst_buffer, EResourceState::TransferDst, dst.dstOffset, dst.size );
			}
		}
		
		_CommitBarriers();
		
		_dev.vkCmdCopyBuffer( _cmdBuffer,
							  src_buffer->Handle(),
							  dst_buffer->Handle(),
							  uint(regions.size()),
							  regions.data() );
	}
	
/*
=================================================
	Visit (CopyImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<CopyImage> &task)
	{
		_OnRunTask();

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
	}
	
/*
=================================================
	Visit (CopyBufferToImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<CopyBufferToImage> &task)
	{
		_OnRunTask();

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
	}
	
/*
=================================================
	Visit (CopyImageToBuffer)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<CopyImageToBuffer> &task)
	{
		_OnRunTask();

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
	}
	
/*
=================================================
	Visit (BlitImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<BlitImage> &task)
	{
		_OnRunTask();

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
	}
	
/*
=================================================
	Visit (ResolveImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<ResolveImage> &task)
	{
		_OnRunTask();
		
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
	}
	
/*
=================================================
	Visit (FillBuffer)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<FillBuffer> &task)
	{
		_OnRunTask();

		VLocalBuffer const *	dst_buffer = task.dstBuffer;

		_AddBuffer( dst_buffer, EResourceState::TransferDst, task.dstOffset, task.size );
		
		_CommitBarriers();
		
		_dev.vkCmdFillBuffer( _cmdBuffer,
							  dst_buffer->Handle(),
							  task.dstOffset,
							  task.size,
							  task.pattern );
	}
	
/*
=================================================
	Visit (ClearColorImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<ClearColorImage> &task)
	{
		_OnRunTask();

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
	}
	
/*
=================================================
	Visit (ClearDepthStencilImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<ClearDepthStencilImage> &task)
	{
		_OnRunTask();
		
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
	}

/*
=================================================
	Visit (UpdateBuffer)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<UpdateBuffer> &task)
	{
		_OnRunTask();
		
		VLocalBuffer const *	dst_buffer = task.dstBuffer;

		for (auto& reg : task.Regions()) {
			_AddBuffer( dst_buffer, EResourceState::TransferDst, reg.bufferOffset, reg.dataSize );
		}	
		_CommitBarriers();
		
		for (auto& reg : task.Regions()) {
			_dev.vkCmdUpdateBuffer( _cmdBuffer, dst_buffer->Handle(), reg.bufferOffset, reg.dataSize, reg.dataPtr );
		}
	}

/*
=================================================
	Visit (Present)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<Present> &task)
	{
		_OnRunTask();
		
		RawImageID	swapchain_image;
		CHECK( _frameGraph.GetSwapchain()->Acquire( ESwapchainImage::Primary, OUT swapchain_image ));

		VLocalImage const *		src_image	= task.srcImage;
		VLocalImage const *		dst_image	= _GetState( swapchain_image );
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
		_OnRunTask();

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
		_OnRunTask();

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

		VLocalBuffer const*		src_buffer = _GetState( staging_buffer );
		_AddBuffer( src_buffer, EResourceState::TransferSrc, region.srcOffset, region.size );
		_AddBuffer( task.dstBuffer, EResourceState::TransferDst, region.dstOffset, region.size );
		
		_CommitBarriers();
		
		_dev.vkCmdCopyBuffer( _cmdBuffer, src_buffer->Handle(), task.dstBuffer->Handle(), 1, &region );
	}

/*
=================================================
	Visit (BuildRayTracingGeometry)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<BuildRayTracingGeometry> &task)
	{
		_OnRunTask();
		
		_AddRTGeometry( task.RTGeometry(), { EResourceState::BuildRayTracingStructWrite, _currTask });
		_AddBuffer( task.ScratchBuffer(), EResourceState::RTASBuildingBufferReadWrite, 0, VkDeviceSize(task.ScratchBuffer()->Size()) );
		// TODO: vertex, index, transform, aabb buffer usage
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
	}
	
/*
=================================================
	Visit (BuildRayTracingScene)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<BuildRayTracingScene> &task)
	{
		_OnRunTask();
		
		task.RTScene()->SetGeometryInstances( task.GeometryIDs(), task.HitShadersPerGeometry(), task.MaxHitShaderCount() );

		_AddRTScene( task.RTScene(), { EResourceState::BuildRayTracingStructWrite, _currTask });
		_AddBuffer( task.ScratchBuffer(), EResourceState::RTASBuildingBufferReadWrite, 0, VkDeviceSize(task.ScratchBuffer()->Size()) );
		_AddBuffer( task.InstanceBuffer(), EResourceState::RTASBuildingBufferRead, 0, VkDeviceSize(task.InstanceBuffer()->Size()) );

		for (auto& blas : task.Geometries()) {
			_AddRTGeometry( blas, { EResourceState::BuildRayTracingStructRead, _currTask });
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
		}
	}
	
/*
=================================================
	Visit (TraceRays)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<TraceRays> &task)
	{
		_OnRunTask();
		
		auto*	layout = _GetResource( task.pipeline->GetLayoutID() );

		_BindPipeline( task.pipeline );
		_BindPipelineResources( *layout, task.GetResources(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV );
		_PushConstants( *layout, task.pushConstants );

		_AddBuffer( task.sbtBuffer, EResourceState::ShaderBindingBufferRead, Min( task.rayGenOffset, task.rayMissOffset, task.rayHitOffset ), VK_WHOLE_SIZE );

		_CommitBarriers();
		
		_dev.vkCmdTraceRaysNV( _cmdBuffer, 
								task.sbtBuffer->Handle(), task.rayGenOffset,
								task.sbtBuffer->Handle(), task.rayMissOffset, task.rayMissStride,
								task.sbtBuffer->Handle(), task.rayHitOffset,  task.rayHitStride,
								VK_NULL_HANDLE, 0, 0,
								task.groupCount.x, task.groupCount.y, task.groupCount.z );
	}

/*
=================================================
	_AddImageState
=================================================
*/
	inline void VTaskProcessor::_AddImageState (const VLocalImage *img, const ImageState &state)
	{
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
	void VTaskProcessor::_AddRTGeometry (const VLocalRTGeometry *geom, const RTGeometryState &state)
	{
		_pendingResourceBarriers.insert({ geom, &CommitResourceBarrier<VLocalRTGeometry> });

		geom->AddPendingState( state );

		if ( _frameGraph.GetDebugger() )
			_frameGraph.GetDebugger()->AddRTGeometryUsage( geom->ToGlobal(), state );
	}
	
/*
=================================================
	_AddRTScene
=================================================
*/
	void VTaskProcessor::_AddRTScene (const VLocalRTScene *scene, const RTSceneState &state)
	{
		_pendingResourceBarriers.insert({ scene, &CommitResourceBarrier<VLocalRTScene> });

		scene->AddPendingState( state );

		if ( _frameGraph.GetDebugger() )
			_frameGraph.GetDebugger()->AddRTSceneUsage( scene->ToGlobal(), state );
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
		}
	}
	

}	// FG
