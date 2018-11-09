// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VTaskProcessor.h"
#include "VFrameGraphThread.h"
#include "VDrawTask.h"
//#include "VFramebuffer.h"
#include "VFrameGraphDebugger.h"
#include "Shared/EnumUtils.h"

namespace FG
{

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
		VTaskProcessor &		_tp;
		const Task				_currTask;


	// methods
	public:
		PipelineResourceBarriers (VTaskProcessor &tp, Task task) : _tp{tp}, _currTask{task} {}

		// ResourceGraph //
		void operator () (const PipelineResources::Buffer &buf);
		void operator () (const PipelineResources::Image &img);
		void operator () (const PipelineResources::Texture &tex);
		void operator () (const PipelineResources::SubpassInput &sp);
		void operator () (const PipelineResources::Sampler &) {}
		void operator () (const std::monostate &) {}
	};

	/*
	
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
		VTaskProcessor &	_tp;
		Task				_currTask;

		FragOutputs_t		_fragOutput;
		UniqFragOutputs_t	_uniqueOutputs;
		uint				_maxFragCount;

		bool				_earlyFragmentTests		: 1;
		bool				_lateFragmentTests		: 1;
		bool				_depthRead				: 1;
		bool				_depthWrite				: 1;
		bool				_stencilReadWrite		: 1;
		bool				_rasterizerDiscard		: 1;
		bool				_compatibleFragOutput	: 1;


	// methods
	public:
		explicit DrawTaskBarriers (VTaskProcessor &tp, Task task);

		inline void Visit (DrawTask &task);
		inline void Visit (DrawIndexedTask &task);

		inline void _ProcessPipeline (const PipelinePtr &ppln, const RenderState &rs);
		inline void _AddPipelineResourceBarriers (const PipelineResourceSet &);

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

		inline void Visit (const DrawTask &task);
		inline void Visit (const DrawIndexedTask &task);

		inline void _BindVertexBuffers (const DrawTask::Buffers_t &vertexBuffers, const VertexInputState &vertexInput) const;
		inline void _BindPipeline (const VLogicalRenderPass *logicalRP, const PipelinePtr &pipeline, const RenderState &renderState,
								   EPipelineDynamicState dynamicStates, const VertexInputState &vertexInput, VPipelineCache &pplnCache) const;
		inline void _BindPipelineResources (const PipelinePtr &pipeline, const PipelineResourceSet &resources) const;
		inline void _SetScissor (const DrawTask::Scissors_t &sc) const;
	};
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	operator (Buffer)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const PipelineResources::Buffer &buf)
	{
		LocalBufferID		buffer_id	= _tp._frameGraph.GetResourceManager()->Remap( buf.bufferId );
		VLocalBuffer const*	buffer		= _tp._frameGraph.GetResourceManager()->GetState( buffer_id );

		// validation
		DEBUG_ONLY(
			auto&	limits	= _tp._frameGraph.GetDevice().GetDeviceProperties().limits;

			if ( (buf.state & EResourceState::_StateMask) == EResourceState::UniformRead )
			{
				ASSERT( (buf.offset % limits.minUniformBufferOffsetAlignment) == 0 );
				ASSERT( buf.size <= limits.maxUniformBufferRange );
			}else{
				ASSERT( (buf.offset % limits.minStorageBufferOffsetAlignment) == 0 );
				ASSERT( buf.size <= limits.maxStorageBufferRange );
			}
		)

		_tp._AddBuffer( buffer, buf.state, VkDeviceSize(buf.offset), VkDeviceSize(buf.size) );
	}
		
/*
=================================================
	operator (Image)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const PipelineResources::Image &img)
	{
		LocalImageID		image_id	= _tp._frameGraph.GetResourceManager()->Remap( img.imageId );
		VLocalImage const*	image		= _tp._frameGraph.GetResourceManager()->GetState( image_id );

		_tp._AddImage( image, img.state, EResourceState_ToImageLayout( img.state ), *img.desc );
	}
		
/*
=================================================
	operator (Texture)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const PipelineResources::Texture &tex)
	{
		LocalImageID		image_id	= _tp._frameGraph.GetResourceManager()->Remap( tex.imageId );
		VLocalImage const*	image		= _tp._frameGraph.GetResourceManager()->GetState( image_id );

		// ignore 'tex.sampler'
		_tp._AddImage( image, tex.state, EResourceState_ToImageLayout( tex.state ), *tex.desc );
	}
	
/*
=================================================
	operator (Subpass)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const PipelineResources::SubpassInput &spi)
	{
		LocalImageID		image_id	= _tp._frameGraph.GetResourceManager()->Remap( spi.imageId );
		VLocalImage const*	image		= _tp._frameGraph.GetResourceManager()->GetState( image_id );
		
		_tp._AddImage( image, spi.state, EResourceState_ToImageLayout( spi.state ), *spi.desc );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*
	VTaskProcessor::DrawTaskBarriers::DrawTaskBarriers (VTaskProcessor &tp, Task task) :
		_tp{ tp },						_currTask{ task },
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
	_AddPipelineResourceBarriers
=================================================
*
	void VTaskProcessor::DrawTaskBarriers::_AddPipelineResourceBarriers (const PipelineResourceSet &resources)
	{
		for (const auto& res : resources)
		{
			VPipelineResourcesPtr		ppln_res = Cast<VPipelineResources>(res.second);

			PipelineResourceBarriers	visitor{ _tp, ppln_res, _currTask };

			for (auto& un : ppln_res->GetUniforms())
			{
				std::visit( visitor, un );
			}
		}
	}

/*
=================================================
	Visit (DrawTask)
=================================================
*
	void VTaskProcessor::DrawTaskBarriers::Visit (DrawTask &task)
	{
		// add pipeline resources
		_AddPipelineResourceBarriers( task.resources );

		// add vertex buffers
		for (auto& vb : task.vertexBuffers)
		{
			VkDeviceSize	offset	= VkDeviceSize(vb.second.offset);
			VkDeviceSize	size	= VK_WHOLE_SIZE;
			auto			iter	= task.vertexInput.BufferBindings().find( vb.first );

			if ( iter != task.vertexInput.BufferBindings().end() )
			{
				offset += VkDeviceSize(iter->second.stride) * task.drawCmd.firstVertex;
				size    = VkDeviceSize(iter->second.stride) * task.drawCmd.vertexCount;	// TODO: instance
			}
			
			vb.second.buffer = vb.second.buffer->GetReal( _currTask, EResourceState::VertexBuffer );

			_tp._AddBuffer( Cast<VBuffer>(vb.second.buffer), EResourceState::VertexBuffer, offset, size );
		}
		
		_ProcessPipeline( task.pipeline, task.renderState );
	}

/*
=================================================
	Visit (DrawIndexedTask)
=================================================
*
	void VTaskProcessor::DrawTaskBarriers::Visit (DrawIndexedTask &task)
	{
		// add pipeline resources
		_AddPipelineResourceBarriers( task.resources );
		
		// add vertex buffers
		for (auto& vb : task.vertexBuffers)
		{
			VkDeviceSize	offset	= VkDeviceSize(vb.second.offset);
			VkDeviceSize	size	= VK_WHOLE_SIZE;
			auto			iter	= task.vertexInput.BufferBindings().find( vb.first );
			
			if ( iter != task.vertexInput.BufferBindings().end() )
			{
				offset += VkDeviceSize(iter->second.stride) * task.drawCmd.vertexOffset;
				//size    = VkDeviceSize(iter->second.stride) * EIndex::MaxValue( task.indexBuffer.indexType );	// TODO: instance
			}
			
			vb.second.buffer = vb.second.buffer->GetReal( _currTask, EResourceState::VertexBuffer );

			_tp._AddBuffer( Cast<VBuffer>(vb.second.buffer), EResourceState::VertexBuffer, offset, size );
		}
		
		// add index buffer
		{
			const VkDeviceSize	index_size	= VkDeviceSize(EIndex_SizeOf( task.indexType ));
			VkDeviceSize		offset		= VkDeviceSize(task.indexBufferOffset);
			VkDeviceSize		size		= index_size * task.drawCmd.indexCount;

			task.indexBuffer = task.indexBuffer->GetReal( _currTask, EResourceState::IndexBuffer );

			_tp._AddBuffer( Cast<VBuffer>(task.indexBuffer), EResourceState::IndexBuffer, offset, size );
		}
		
		_ProcessPipeline( task.pipeline, task.renderState );
	}
	
/*
=================================================
	_ProcessPipeline
=================================================
*
	void VTaskProcessor::DrawTaskBarriers::_ProcessPipeline (const PipelinePtr &ppln, const RenderState &renderState)
	{
		VGraphicsPipelinePtr	pipeline = Cast<VGraphicsPipeline>(ppln);

		// merge fragment output
		auto	inst = pipeline->GetFragmentOutput();

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

		if ( renderState.depth.test )
			_depthRead = true;

		if ( renderState.depth.write )
			_depthWrite = true;

		if ( renderState.stencil.enabled )
			_stencilReadWrite = true;

		if ( renderState.rasterization.rasterizerDiscard )
			_rasterizerDiscard = true;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*
	VTaskProcessor::DrawTaskCommands::DrawTaskCommands (VTaskProcessor &tp, VFgTask<SubmitRenderPass> const* task, VkCommandBuffer cmd) :
		_tp{ tp },	_dev{ tp._dev },	_currTask{ task },	_cmdBuffer{ cmd }
	{
	}

/*
=================================================
	_BindVertexBuffers
=================================================
*
	void VTaskProcessor::DrawTaskCommands::_BindVertexBuffers (const DrawTask::Buffers_t &vertexBuffers, const VertexInputState &vertexInput) const
	{
		if ( vertexBuffers.empty() )
			return;

		static constexpr size_t	size	= DrawTask::Buffers_t::capacity();

		FixedArray< VkBuffer, size >		buffers;	buffers.resize( vertexBuffers.size() );
		FixedArray< VkDeviceSize, size >	offsets;	offsets.resize( vertexBuffers.size() );

		for (const auto& buf : vertexBuffers)
		{
			auto	iter = vertexInput.BufferBindings().find( buf.first );
			ASSERT( iter != vertexInput.BufferBindings().end() );

			if ( iter != vertexInput.BufferBindings().end() )
			{
				buffers[ iter->second.index ] = Cast<VBuffer>( buf.second.buffer )->GetBufferID();
				offsets[ iter->second.index ] = VkDeviceSize( buf.second.offset );
			}
		}

		_dev.vkCmdBindVertexBuffers( _cmdBuffer, 0, uint(buffers.size()), buffers.data(), offsets.data() );
	}
	
/*
=================================================
	_BindPipeline
=================================================
*
	void VTaskProcessor::DrawTaskCommands::_BindPipeline (const VLogicalRenderPass *logicalRP, const PipelinePtr &pipeline,
														  const RenderState &renderState, EPipelineDynamicState dynamicStates,
														  const VertexInputState &vertexInput, VPipelineCache &pplnCache) const
	{
		VkPipeline	ppln_id = pplnCache.CreatePipelineInstance( Cast<VGraphicsPipeline>(pipeline), logicalRP->GetRenderPass(), logicalRP->GetSubpassIndex(),
															    renderState, vertexInput, dynamicStates, 0, uint(logicalRP->GetViewports().size()) );
		
		_dev.vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln_id );
	}
	
/*
=================================================
	_BindPipelineResources
=================================================
*
	void VTaskProcessor::DrawTaskCommands::_BindPipelineResources (const PipelinePtr &pipeline, const PipelineResourceSet &resources) const
	{
		if ( resources.empty() )
			return;

		FixedArray< VkDescriptorSet, VPipelineResourceSet::capacity() >		descriptor_sets;
		descriptor_sets.resize( resources.size() );

		VGraphicsPipelinePtr	gpipeline = Cast<VGraphicsPipeline>(pipeline);

		for (const auto& res : resources)
		{
			uint	idx = gpipeline->GetBindingIndex( res.first );
			ASSERT( idx < descriptor_sets.size() );

			descriptor_sets[idx] = Cast<VPipelineResources>(res.second)->GetDescriptorSetID();
		}

		_dev.vkCmdBindDescriptorSets( _cmdBuffer,
									  VK_PIPELINE_BIND_POINT_GRAPHICS,
									  gpipeline->GetLayout()->GetLayoutID(),
									  0,
									  uint(descriptor_sets.size()),
									  descriptor_sets.data(),
									  0, null );
	}
	
/*
=================================================
	_SetScissor
=================================================
*
	void VTaskProcessor::DrawTaskCommands::_SetScissor (const DrawTask::Scissors_t &srcScissors) const
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
	Visit (DrawTask)
=================================================
*
	void VTaskProcessor::DrawTaskCommands::Visit (const DrawTask &task)
	{
		_tp._CmdDebugMarker( task.taskName );

		_BindPipeline( _currTask->GetLogicalPass(), task.pipeline, task.renderState,
					   task.dynamicStates, task.vertexInput, _tp._frameGraph.GetPipelineCache() );

		_BindPipelineResources( task.pipeline, task.resources );
		_BindVertexBuffers( task.vertexBuffers, task.vertexInput );
		
		_SetScissor( task.scissors );

		_dev.vkCmdDraw( _cmdBuffer,
						task.drawCmd.vertexCount,
						task.drawCmd.instanceCount,
						task.drawCmd.firstVertex,
						task.drawCmd.firstInstance );
	}
	
/*
=================================================
	Visit (DrawIndexedTask)
=================================================
*
	void VTaskProcessor::DrawTaskCommands::Visit (const DrawIndexedTask &task)
	{
		_tp._CmdDebugMarker( task.taskName );
		
		_BindPipeline( _currTask->GetLogicalPass(), task.pipeline, task.renderState,
					   task.dynamicStates, task.vertexInput, _tp._frameGraph.GetPipelineCache() );

		_BindPipelineResources( task.pipeline, task.resources );
		_BindVertexBuffers( task.vertexBuffers, task.vertexInput );
		
		_SetScissor( task.scissors );

		_dev.vkCmdBindIndexBuffer( _cmdBuffer,
									Cast<VBuffer>( task.indexBuffer )->GetBufferID(),
									VkDeviceSize( task.indexBufferOffset ),
									VEnumCast( task.indexType ) );

		_dev.vkCmdDrawIndexed( _cmdBuffer,
								task.drawCmd.indexCount,
								task.drawCmd.instanceCount,
								task.drawCmd.firstIndex,
								task.drawCmd.vertexOffset,
								task.drawCmd.firstInstance );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VTaskProcessor::VTaskProcessor (VFrameGraphThread &fg, VkCommandBuffer cmdbuf) :
		_frameGraph{ fg },		_dev{ fg.GetDevice() },
		_cmdBuffer{ cmdbuf },
		_pendingBufferBarriers{ fg.GetAllocator() },
		_pendingImageBarriers{ fg.GetAllocator() }
	{
		ASSERT( _cmdBuffer );

		VkCommandBufferBeginInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.pNext				= null;
		info.flags				= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		info.pInheritanceInfo	= null;

		VK_CALL( _dev.vkBeginCommandBuffer( _cmdBuffer, &info ));
	
		_isDebugMarkerSupported = _dev.HasExtension( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );		// TODO: cache
	}
	
/*
=================================================
	destructor
=================================================
*/
	VTaskProcessor::~VTaskProcessor ()
	{
		if ( _cmdBuffer )
		{
			VK_CALL( _dev.vkEndCommandBuffer( _cmdBuffer ));
		}
	}
	
/*
=================================================
	Visit*_DrawTask
=================================================
*
	void VTaskProcessor::Visit1_DrawTask (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( static_cast<FGDrawTask<DrawTask>*>( taskData )->Data() );
	}
	
	void VTaskProcessor::Visit2_DrawTask (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( static_cast<FGDrawTask<DrawTask>*>( taskData )->Data() );
	}
	
/*
=================================================
	Visit*_DrawIndexedTask
=================================================
*
	void VTaskProcessor::Visit1_DrawIndexedTask (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( static_cast<FGDrawTask<DrawIndexedTask>*>( taskData )->Data() );
	}

	void VTaskProcessor::Visit2_DrawIndexedTask (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( static_cast<FGDrawTask<DrawIndexedTask>*>( taskData )->Data() );
	}

/*
=================================================
	_CmdDebugMarker
=================================================
*/
	void VTaskProcessor::_CmdDebugMarker (StringView text) const
	{
		if ( not _isDebugMarkerSupported )
			return;

		VkDebugMarkerMarkerInfoEXT	info = {};
		info.sType			= VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		info.pMarkerName	= text.data();

		_dev.vkCmdDebugMarkerInsertEXT( _cmdBuffer, &info );
	}
	
/*
=================================================
	_OnRunTask
=================================================
*/
	forceinline void  VTaskProcessor::_OnRunTask (const IFrameGraphTask *task) const
	{
		_CmdDebugMarker( task->Name() );
		
		if ( _frameGraph.GetDebugger() )
			_frameGraph.GetDebugger()->RunTask( task );
	}
	
/*
=================================================
	_GetState
=================================================
*/
	template <typename ID>
	forceinline auto const*  VTaskProcessor::_GetState (ID id) const
	{
		return _frameGraph.GetResourceManager()->GetState( id );
	}

/*
=================================================
	_AddRenderTargetBarriers
=================================================
*
	void VTaskProcessor::_AddRenderTargetBarriers (const VLogicalRenderPass *logicalRP, const DrawTaskBarriers &info)
	{
		if ( logicalRP->GetDepthStencilTarget().IsDefined() )
		{
			const auto &	rt		= logicalRP->GetDepthStencilTarget();
			EResourceState	state	= rt.state;
			VImagePtr		image	= Cast<VImage>( rt.image->GetReal( _currTask, state ));
			VkImageLayout	layout	= EResourceState_ToImageLayout( rt.state );

			if ( info.IsEarlyFragmentTests() )
				state |= EResourceState::EarlyFragmentTests;

			if ( info.IsLateFragmentTests() )
				state |= EResourceState::LateFragmentTests;
			
			_AddImage( image, state, layout, rt.desc );
		}

		for (const auto& rt : logicalRP->GetColorTargets())
		{
			VImagePtr		image	= Cast<VImage>( rt.second.image->GetReal( _currTask, rt.second.state ));
			VkImageLayout	layout	= EResourceState_ToImageLayout( rt.second.state );

			_AddImage( image, rt.second.state, layout, rt.second.desc );
		}
	}
	
/*
=================================================
	_ExtractClearValues
=================================================
*
	void VTaskProcessor::_ExtractClearValues (const VLogicalRenderPass *logicalRP, OUT VkClearValues_t &clearValues) const
	{
		clearValues.resize( logicalRP->GetColorTargets().size() + uint(logicalRP->GetDepthStencilTarget().IsDefined()) );

		if ( logicalRP->GetDepthStencilTarget().IsDefined() )
		{
			clearValues[0] = logicalRP->GetDepthStencilTarget().clearValue;
		}

		for (const auto& ct : logicalRP->GetColorTargets())
		{
			uint	index;
			CHECK( logicalRP->GetRenderPass()->GetColorAttachmentIndex( ct.first, OUT index ));

			clearValues[index] = ct.second.clearValue;
		}
	}

/*
=================================================
	_BeginRenderPass
=================================================
*
	void VTaskProcessor::_BeginRenderPass (const VFgTask<SubmitRenderPass> &task)
	{
		ASSERT( not task.IsSubpass() );

		FixedArray< VLogicalRenderPass*, 32 >		logical_passes;

		for (auto* iter = &task; iter != null; iter = iter->GetNextSubpass())
		{
			logical_passes.push_back( iter->GetLogicalPass() );
		}

		
		// add barriers
		DrawTaskBarriers	barrier_visitor{ *this, _currTask };

		for (auto& pass : logical_passes)
		{
			for (auto& draw : pass->GetDrawTasks())
			{
				draw->Process1( &barrier_visitor );
			}
		}


		// create render pass and framebuffer
		CHECK( _frameGraph.GetRenderPassCache().CreateRenderPasses( logical_passes, barrier_visitor.GetFragOutputs() ));
		
		VFramebufferPtr	framebuffer	= task.GetLogicalPass()->GetFramebuffer();

		_AddRenderTargetBarriers( task.GetLogicalPass(), barrier_visitor );
		_CommitBarriers();


		// begin render pass
		VRenderPassPtr	render_pass = task.GetLogicalPass()->GetRenderPass();
		RectI const&	area		= task.GetLogicalPass()->GetArea();

		VkClearValues_t		clear_values;
		_ExtractClearValues( task.GetLogicalPass(), OUT clear_values );

		VkRenderPassBeginInfo	pass_info = {};
		pass_info.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		pass_info.renderPass				= render_pass->GetRenderPassID();
		pass_info.renderArea.offset.x		= area.left;
		pass_info.renderArea.offset.y		= area.top;
        pass_info.renderArea.extent.width	= uint(area.Width());
        pass_info.renderArea.extent.height	= uint(area.Height());
		pass_info.clearValueCount			= uint(clear_values.size());
		pass_info.pClearValues				= clear_values.data();
		pass_info.framebuffer				= framebuffer->GetFramebufferID();
		
		_dev.vkCmdBeginRenderPass( _cmdBuffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE );
	}
	
/*
=================================================
	_BeginSubpass
=================================================
*
	void VTaskProcessor::_BeginSubpass (const VFgTask<SubmitRenderPass> &task)
	{
		ASSERT( task.IsSubpass() );

		// TODO: barriers for attachments

		_dev.vkCmdNextSubpass( _cmdBuffer, VK_SUBPASS_CONTENTS_INLINE );
		
		// TODO
		/*vkCmdClearAttachments( _cmdId,
							   uint( attachments.Count() ),
							   attachments.ptr(),
							   uint( clear_rects.Count() ),
							   clear_rects.ptr() );* /
	}

/*
=================================================
	Visit (SubmitRenderPass)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<SubmitRenderPass> &task)
	{
		_OnRunTask( &task );
		/*
		if ( task.IsSubpass() )
			_BeginSubpass( task );
		else
			_BeginRenderPass( task );


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
		if ( task.IsLastSubpass() ) {
			_dev.vkCmdEndRenderPass( _cmdBuffer );
		}*/
	}
	
/*
=================================================
	_BindPipelineResources
=================================================
*/
	void VTaskProcessor::_BindPipelineResources (RawCPipelineID pipelineId, const VPipelineResourceSet &resources, ArrayView<uint> dynamicOffsets)
	{
		FixedArray< VkDescriptorSet, VPipelineResourceSet::capacity() >		descriptor_sets;
		descriptor_sets.resize( resources.size() );

		VComputePipeline const*		cppln	= _frameGraph.GetResourceManager()->GetResource( pipelineId );
		VPipelineLayout const*		layout	= _frameGraph.GetResourceManager()->GetResource( cppln->GetLayoutID() );
		
		// update descriptor sets and add pipeline barriers
		for (const auto& res : resources)
		{
			VPipelineResources const*	ppln_res = _frameGraph.GetResourceManager()->GetResource( res.second );

			PipelineResourceBarriers	visitor{ *this, _currTask };

			for (auto& un : ppln_res->GetData())
			{
				std::visit( visitor, un.res );
			}

			uint						idx;
			RawDescriptorSetLayoutID	ds_layout;
			CHECK( layout->GetDescriptorSetLayout( res.first, OUT ds_layout, OUT idx ));

			descriptor_sets[idx] = _frameGraph.GetResourceManager()->GetResource( res.second )->Handle();
		}

		_dev.vkCmdBindDescriptorSets( _cmdBuffer,
									  VK_PIPELINE_BIND_POINT_COMPUTE,
									  layout->Handle(),
									  0,
									  uint(descriptor_sets.size()),
									  descriptor_sets.data(),
									  uint(dynamicOffsets.size()),
									  dynamicOffsets.data() );
	}
	
/*
=================================================
	_BindPipeline
=================================================
*/
	void VTaskProcessor::_BindPipeline (RawCPipelineID pipelineId, const Optional<uint3> &localSize) const
	{
		VkPipeline	ppln_id = _frameGraph.GetPipelineCache()->CreatePipelineInstance( *_frameGraph.GetResourceManager(), pipelineId, localSize, 0 );
		
		_dev.vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ppln_id );
	}

/*
=================================================
	Visit (DispatchCompute)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<DispatchCompute> &task)
	{
		_OnRunTask( &task );

		_isCompute = true;

		_BindPipelineResources( task.GetPipeline(), task.GetResources(), task.GetDynamicOffsets() );
		_BindPipeline( task.GetPipeline(), task.LocalSize() );
		_CommitBarriers();

		const uint3		group_count	= task.GroupCount();

		_dev.vkCmdDispatch( _cmdBuffer, group_count.x, group_count.y, group_count.z );
	}
	
/*
=================================================
	Visit (DispatchIndirectCompute)
=================================================
*
	void VTaskProcessor::Visit (const VFgTask<DispatchIndirectCompute> &task)
	{
		_OnRunTask( _currQueue, &task );

		_isCompute = true;
		
		// update descriptor sets and add pipeline barriers
		for (const auto& res : task.GetResources())
		{
			PipelineResourceBarriers	visitor{ *this, res.second, _currTask };

			for (auto& un : res.second->GetUniforms())
			{
				std::visit( visitor, un );
			}
		}
		
		VBufferPtr	indirect_buffer = Cast<VBuffer>( task.IndirectBuffer()->GetReal( _currTask, EResourceState::IndirectBuffer ));

		_AddBuffer( indirect_buffer, EResourceState::IndirectBuffer, task.IndirectBufferOffset(), VK_WHOLE_SIZE );		
		_CommitBarriers();

		
		// prepare & dispatch
		_BindPipeline( task.GetPipeline(), task.LocalSize() );
		_BindPipelineResources( task.GetPipeline(), task.GetResources() );
		
		_dev.vkCmdDispatchIndirect( _cmdBuffer,
									indirect_buffer->GetBufferID(),
									task.IndirectBufferOffset() );
	}

/*
=================================================
	Visit (CopyBuffer)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<CopyBuffer> &task)
	{
		_OnRunTask( &task );

		VLocalBuffer const *	src_buffer	= _GetState( task.SrcBuffer() );
		VLocalBuffer const *	dst_buffer	= _GetState( task.DstBuffer() );
		BufferCopyRegions_t		regions;	regions.resize( task.Regions().size() );

		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&	src = task.Regions()[i];
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
		_OnRunTask( &task );

		VLocalImage const *		src_image	= _GetState( task.SrcImage() );
		VLocalImage const *		dst_image	= _GetState( task.DstImage() );
		ImageCopyRegions_t		regions;	regions.resize( task.Regions().size() );

		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&		src					= task.Regions()[i];
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

			_AddImage( src_image, EResourceState::TransferSrc, task.SrcLayout(), dst.srcSubresource );
			_AddImage( dst_image, EResourceState::TransferDst, task.DstLayout(), dst.dstSubresource );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdCopyImage( _cmdBuffer,
							 src_image->Handle(),
							 task.SrcLayout(),
							 dst_image->Handle(),
							 task.DstLayout(),
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
		_OnRunTask( &task );

		VLocalBuffer const *		src_buffer	= _GetState( task.SrcBuffer() );
		VLocalImage const *			dst_image	= _GetState( task.DstImage() );
		BufferImageCopyRegions_t	regions;	regions.resize( task.Regions().size() );
		
		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&			src			= task.Regions()[i];
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
			_AddImage(  dst_image,  EResourceState::TransferDst, task.DstLayout(), dst.imageSubresource );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdCopyBufferToImage( _cmdBuffer,
									 src_buffer->Handle(),
									 dst_image->Handle(),
									 task.DstLayout(),
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
		_OnRunTask( &task );

		VLocalImage const *			src_image	= _GetState( task.SrcImage() );
		VLocalBuffer const *		dst_buffer	= _GetState( task.DstBuffer() );
		BufferImageCopyRegions_t	regions;	regions.resize( task.Regions().size() );
		
		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&			src			= task.Regions()[i];
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

			_AddImage(  src_image,  EResourceState::TransferSrc, task.SrcLayout(), dst.imageSubresource );
			_AddBuffer( dst_buffer, EResourceState::TransferDst, dst, src_image );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdCopyImageToBuffer( _cmdBuffer,
									 src_image->Handle(),
									 task.SrcLayout(),
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
		_OnRunTask( &task );

		VLocalImage const *		src_image	= _GetState( task.SrcImage() );
		VLocalImage const *		dst_image	= _GetState( task.DstImage() );
		BlitRegions_t			regions;	regions.resize( task.Regions().size() );
		
		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&		src	= task.Regions()[i];
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

			_AddImage( src_image, EResourceState::TransferSrc, task.SrcLayout(), dst.srcSubresource );
			_AddImage( dst_image, EResourceState::TransferDst, task.DstLayout(), dst.dstSubresource );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdBlitImage( _cmdBuffer,
							 src_image->Handle(),
							 task.SrcLayout(),
							 dst_image->Handle(),
							 task.DstLayout(),
							 uint(regions.size()),
							 regions.data(),
							 task.Filter() );
	}
	
/*
=================================================
	Visit (ResolveImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<ResolveImage> &task)
	{
		_OnRunTask( &task );
		
		VLocalImage	const *		src_image	= _GetState( task.SrcImage() );
		VLocalImage const *		dst_image	= _GetState( task.DstImage() );
		ResolveRegions_t		regions;	regions.resize( task.Regions().size() );
		
		for (size_t i = 0, count = regions.size(); i < count; ++i)
		{
			const auto&			src				= task.Regions()[i];
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

			_AddImage( src_image, EResourceState::TransferSrc, task.SrcLayout(), dst.srcSubresource );
			_AddImage( dst_image, EResourceState::TransferDst, task.DstLayout(), dst.dstSubresource );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdResolveImage(	_cmdBuffer,
								src_image->Handle(),
								task.SrcLayout(),
								dst_image->Handle(),
								task.DstLayout(),
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
		_OnRunTask( &task );

		VLocalBuffer const *	dst_buffer = _GetState( task.DstBuffer() );

		_AddBuffer( dst_buffer, EResourceState::TransferDst, task.DstOffset(), task.Size() );
		
		_CommitBarriers();
		
		_dev.vkCmdFillBuffer( _cmdBuffer,
							  dst_buffer->Handle(),
							  task.DstOffset(),
							  task.Size(),
							  task.Pattern() );
	}
	
/*
=================================================
	Visit (ClearColorImage)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<ClearColorImage> &task)
	{
		_OnRunTask( &task );

		VLocalImage const *		dst_image	= _GetState( task.DstImage() );
		ImageClearRanges_t		ranges;		ranges.resize( task.Ranges().size() );
		
		for (size_t i = 0, count = ranges.size(); i < count; ++i)
		{
			const auto&		src	= task.Ranges()[i];
			auto&			dst	= ranges[i];
			
			dst.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			dst.baseMipLevel	= src.baseMipLevel.Get();
			dst.levelCount		= src.levelCount;
			dst.baseArrayLayer	= src.baseLayer.Get();
			dst.layerCount		= src.layerCount;

			_AddImage( dst_image, EResourceState::TransferDst, task.DstLayout(), dst );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdClearColorImage( _cmdBuffer,
								   dst_image->Handle(),
								   task.DstLayout(),
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
		_OnRunTask( &task );
		
		VLocalImage const *		dst_image	= _GetState( task.DstImage() );
		ImageClearRanges_t		ranges;		ranges.resize( task.Ranges().size() );
		
		for (size_t i = 0, count = ranges.size(); i < count; ++i)
		{
			const auto&		src	= task.Ranges()[i];
			auto&			dst	= ranges[i];
			
			dst.aspectMask		= VEnumCast( src.aspectMask, dst_image->PixelFormat() );
			dst.baseMipLevel	= src.baseMipLevel.Get();
			dst.levelCount		= src.levelCount;
			dst.baseArrayLayer	= src.baseLayer.Get();
			dst.layerCount		= src.layerCount;

			_AddImage( dst_image, EResourceState::TransferDst, task.DstLayout(), dst );
		}
		
		_CommitBarriers();
		
		_dev.vkCmdClearDepthStencilImage( _cmdBuffer,
										  dst_image->Handle(),
										  task.DstLayout(),
										  &task.ClearValue(),
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
		_OnRunTask( &task );
		
		VLocalBuffer const *	dst_buffer = _GetState( task.DstBuffer() );

		_AddBuffer( dst_buffer, EResourceState::TransferDst, task.DstBufferOffset(), task.DataSize() );
		
		_CommitBarriers();
		
		_dev.vkCmdUpdateBuffer( _cmdBuffer,
								dst_buffer->Handle(),
								task.DstBufferOffset(),
								task.DataSize(),
								task.DataPtr() );
	}

/*
=================================================
	Visit (Present)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<Present> &task)
	{
		_OnRunTask( &task );

		/*_AddImage( task.GetImage(), EResourceState::TransferSrc | EResourceState::_InvalidateAfter, EImageLayout::TransferSrcOptimal,
				   RangeU(0, 1) + task.GetLayer().Get(), RangeU(0, 1), EImageAspect::Color );

		GpuMsg::ThreadBeginFrame	begin_frame;
		_gpuThread->Send( begin_frame );

		_barrierMngr.AddBarrier( GpuMsg::CmdPipelineBarrier{}.AddImage({}) );

		_cmdBuilder->Send( GpuMsg::CmdBlitImage{ task.GetImage(), EImageLayout::TransferSrcOptimal,
												 begin_frame.result->image, EImageLayout::TransferDstOptimal, true }
								.AddRegion(	{}, uint3(), uint3(),
											{}, uint3(), uint3() ));
											
		_CommitBarriers();
		
		_gpuThread->Send( GpuMsg::ThreadEndFrame{} );

		// TODO*/
	}

/*
=================================================
	Visit (PresentVR)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<PresentVR> &task)
	{
		_OnRunTask( &task );

		/*_AddImage( task.GetLeftEyeImage(), EResourceState::PresentImage | EResourceState::_InvalidateAfter, EImageLayout::PresentSrc,
				   ImageRange{ task.GetLeftEyeLayer(), 1, 0_mipmap, 1 }, EImageAspect::Color );
		
		_AddImage( task.GetRightEyeImage(), EResourceState::PresentImage | EResourceState::_InvalidateAfter, EImageLayout::PresentSrc,
				   ImageRange{ task.GetRightEyeLayer(), 1, 0_mipmap, 1 }, EImageAspect::Color );

		_CommitBarriers();*/
		
		// TODO
	}
	
/*
=================================================
	_AddImageState
=================================================
*/
	inline void VTaskProcessor::_AddImageState (const VLocalImage *img, const ImageState &state)
	{
		ASSERT( not state.range.IsEmpty() );

		_pendingImageBarriers.insert( img );

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
		_pendingBufferBarriers.insert( buf );

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

		const uint			bpp			= EPixelFormat_BitPerPixel( img->PixelFormat(), VEnumRevert(reg.imageSubresource.aspectMask) );
		const VkDeviceSize	row_size	= (reg.imageExtent.width * bpp) / 8;
		const VkDeviceSize	row_pitch	= (reg.bufferRowLength * bpp) / 8;
		const VkDeviceSize	slice_pitch	= reg.bufferImageHeight * row_pitch;
		const uint			dim_z		= Max( reg.imageSubresource.layerCount, reg.imageExtent.depth );
		BufferState			buf_state	{ state, 0, row_size, _currTask };

#if 1
		// one big barrier
		buf_state.range = BufferRange{ 0, slice_pitch * dim_z } + reg.bufferOffset;
		
		_AddBufferState( buf, buf_state );
#else
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
	_CommitBarriers
=================================================
*/
	inline void VTaskProcessor::_CommitBarriers ()
	{
		for (auto& buf : _pendingBufferBarriers)
		{
			buf->CommitBarrier( _barrierMngr, _frameGraph.GetDebugger() );
		}

		for (auto& img : _pendingImageBarriers)
		{
			img->CommitBarrier( _barrierMngr, _frameGraph.GetDebugger() );
		}

		_pendingBufferBarriers.clear();
		_pendingImageBarriers.clear();

		_barrierMngr.Commit( _dev, _cmdBuffer );
	}

	
}	// FG
