// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VTaskProcessor.h"
#include "VCommandBuffer.h"
#include "VPipelineCache.h"
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
		return _fgThread.ToLocal( id );
	}
	
	template <typename ID>
	forceinline auto const*  VTaskProcessor::_GetResource (ID id) const
	{
		return _fgThread.AcquireTemporary( id );
	}
	
	template <typename ResType>
	static void CommitResourceBarrier (const void *res, VBarrierManager &barrierMngr, Ptr<VLocalDebugger> debugger)
	{
		return static_cast<ResType const*>(res)->CommitBarrier( barrierMngr, debugger );
	}

	inline VTaskProcessor::Statistic_t&  VTaskProcessor::Stat () const
	{
		return _fgThread.EditStatistic().renderer;
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
		void operator () (const UniformID &, const PipelineResources::Buffer &buf);
		void operator () (const UniformID &, const PipelineResources::Image &img);
		void operator () (const UniformID &, const PipelineResources::Texture &tex);
		//void operator () (const UniformID &, const PipelineResources::SubpassInput &sp);
		void operator () (const UniformID &, const PipelineResources::Sampler &) {}
		void operator () (const UniformID &, const PipelineResources::RayTracingScene &);
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


	// variables
	private:
		VTaskProcessor &			_tp;
		VLogicalRenderPass const&	_logicalRP;

		FragOutputs_t				_fragOutput;
		uint						_maxFragCount;

		bool						_earlyFragmentTests		: 1;
		bool						_lateFragmentTests		: 1;
		bool						_depthWrite				: 1;
		bool						_stencilWrite			: 1;
		bool						_rasterizerDiscard		: 1;
		bool						_compatibleFragOutput	: 1;


	// methods
	public:
		DrawTaskBarriers (VTaskProcessor &tp, const VLogicalRenderPass &);

		void Visit (const VFgDrawTask<FG::DrawVertices> &task);
		void Visit (const VFgDrawTask<FG::DrawIndexed> &task);
		void Visit (const VFgDrawTask<FG::DrawMeshes> &task);
		void Visit (const VFgDrawTask<FG::DrawVerticesIndirect> &task);
		void Visit (const VFgDrawTask<FG::DrawIndexedIndirect> &task);
		void Visit (const VFgDrawTask<FG::DrawMeshesIndirect> &task);
		void Visit (const VFgDrawTask<FG::CustomDraw> &task);

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
		VFgTask<SubmitRenderPass> const*	_currTask;
		VkCommandBuffer						_cmdBuffer;


	// methods
	public:
		DrawTaskCommands (VTaskProcessor &tp, VFgTask<SubmitRenderPass> const* task, VkCommandBuffer cmd);

		void Visit (const VFgDrawTask<FG::DrawVertices> &task);
		void Visit (const VFgDrawTask<FG::DrawIndexed> &task);
		void Visit (const VFgDrawTask<FG::DrawMeshes> &task);
		void Visit (const VFgDrawTask<FG::DrawVerticesIndirect> &task);
		void Visit (const VFgDrawTask<FG::DrawIndexedIndirect> &task);
		void Visit (const VFgDrawTask<FG::DrawMeshesIndirect> &task);
		void Visit (const VFgDrawTask<FG::CustomDraw> &task);

	private:
		void _BindVertexBuffers (ArrayView<VLocalBuffer const*> vertexBuffers, ArrayView<VkDeviceSize> vertexOffsets) const;

		template <typename DrawTask>
		void _BindPipelineResources (const VPipelineLayout &layout, const DrawTask &task) const;
	};
	


	//
	// Draw Context
	//
	class VTaskProcessor::DrawContext final : public IDrawContext
	{
	// constants
	private:
		enum {
			GRAPHICS_BIT	= 1,
			MESH_BIT		= 2,
			ALL_BITS		= GRAPHICS_BIT | MESH_BIT,
		};


	// variables
	private:
		VTaskProcessor &			_tp;
		VLogicalRenderPass const&	_logicalRP;
		
		VGraphicsPipeline const*	_gpipeline		= null;
		VMeshPipeline const*		_mpipeline		= null;
		VPipelineLayout const*		_pplnLayout		= null;

			
		RenderState					_renderState;
		EPipelineDynamicState		_dynamicStates	= Default;

		VertexInputState			_vertexInput;
		uint						_changed : 2;


	// methods
	public:
		DrawContext (VTaskProcessor &tp, const VLogicalRenderPass &rp);

		Context_t  GetData () override;
		void Reset () override;
		void BindPipeline (RawGPipelineID id, EPipelineDynamicState dynamicState) override;
		void BindPipeline (RawMPipelineID id, EPipelineDynamicState dynamicState) override;
		void BindResources (const DescriptorSetID &id, const PipelineResources &res) override;
		void PushConstants (const PushConstantID &id, const void *data, BytesU dataSize) override;
		void BindShadingRateImage (RawImageID value, ImageLayer layer, MipmapLevel level) override;
		void BindVertexAttribs (const VertexInputState &) override;
		void BindVertexBuffer (const VertexBufferID &id, RawBufferID vbuf, BytesU offset) override;
		void BindIndexBuffer (RawBufferID ibuf, BytesU offset, EIndex type) override;
		void SetColorBuffer (RenderTargetID id, const RenderState::ColorBuffer &value) override;
		void SetLogicOp (ELogicOp value) override;
		void SetBlendColor (const RGBA32f &value) override;
		void SetStencilBuffer (const RenderState::StencilBufferState &value) override;
		void SetDepthBuffer (const RenderState::DepthBufferState &value) override;
		void SetInputAssembly (const RenderState::InputAssemblyState &value) override;
		void SetRasterization (const RenderState::RasterizationState &value) override;
		void SetMultisample (const RenderState::MultisampleState &value) override;
		void SetStencilCompareMask (uint value) override;
		void SetStencilWriteMask (uint value) override;
		void SetStencilReference (uint value) override;
		void SetShadingRatePalette (uint viewportIndex, ArrayView<EShadingRatePalette> value) override;
		void DrawVertices (uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance) override;
		void DrawIndexed (uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset, uint firstInstance) override;
		void DrawVerticesIndirect (RawBufferID indirectBuffer, BytesU indirectBufferOffset, uint drawCount, BytesU stride) override;
		void DrawIndexedIndirect (RawBufferID indirectBuffer, BytesU indirectBufferOffset, uint drawCount, BytesU stride) override;
		void DrawMeshes (uint taskCount, uint firstTask) override;
		void DrawMeshesIndirect (RawBufferID indirectBuffer, BytesU indirectBufferOffset, uint drawCount, BytesU stride) override;
		
	private:
		void _BindPipeline (uint mask);
	};
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	operator (Buffer)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const UniformID &, const PipelineResources::Buffer &buf)
	{
		for (uint i = 0; i < buf.elementCount; ++i)
		{
			auto&				elem	= buf.elements[i];
			VLocalBuffer const*	buffer	= _tp._ToLocal( elem.bufferId );
			if ( not buffer )
				continue;

			const VkDeviceSize	offset	= VkDeviceSize(elem.offset) + (buf.dynamicOffsetIndex < _dynamicOffsets.size() ? _dynamicOffsets[buf.dynamicOffsetIndex] : 0);		
			const VkDeviceSize	size	= VkDeviceSize(elem.size == ~0_b ? buffer->Size() - offset : elem.size);

			// validation
			{
				ASSERT( (size >= buf.staticSize) and (buf.arrayStride == 0 or (size - buf.staticSize) % buf.arrayStride == 0) );
				ASSERT( offset < buffer->Size() );
				ASSERT( offset + size <= buffer->Size() );

				auto&	limits	= _tp._fgThread.GetDevice().GetDeviceProperties().limits;

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
	}
		
/*
=================================================
	operator (Image / Texture / SubpassInput)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const UniformID &, const PipelineResources::Image &img)
	{
		for (uint i = 0; i < img.elementCount; ++i)
		{
			auto&				elem	= img.elements[i];
			VLocalImage const*  image	= _tp._ToLocal( elem.imageId );

			if ( image )
				_tp._AddImage( image, img.state, EResourceState_ToImageLayout( img.state, image->AspectMask() ), elem.desc );
		}
	}
	
	void VTaskProcessor::PipelineResourceBarriers::operator () (const UniformID &, const PipelineResources::Texture &tex)
	{
		for (uint i = 0; i < tex.elementCount; ++i)
		{
			auto&				elem	= tex.elements[i];
			VLocalImage const*  image	= _tp._ToLocal( elem.imageId );

			if ( image )
				_tp._AddImage( image, tex.state, EResourceState_ToImageLayout( tex.state, image->AspectMask() ), elem.desc );
		}
	}

/*
=================================================
	operator (RayTracingScene)
=================================================
*/
	void VTaskProcessor::PipelineResourceBarriers::operator () (const UniformID &, const PipelineResources::RayTracingScene &rts)
	{
		for (uint i = 0; i < rts.elementCount; ++i)
		{
			VLocalRTScene const*  scene = _tp._ToLocal( rts.elements[i].sceneId );
			if ( not scene )
				return;

			_tp._AddRTScene( scene, EResourceState::RayTracingShaderRead );

			auto&	data = scene->ToGlobal()->CurrentData();
			SHAREDLOCK( data.guard );
			ASSERT( data.geometryInstances.size() );

			for (auto& inst : data.geometryInstances)
			{
				if ( auto* geom = _tp._ToLocal( inst.geometry.Get() ))
				{
					_tp._AddRTGeometry( geom, EResourceState::RayTracingShaderRead );
				}
			}
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
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<FG::DrawVertices> &task)
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
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<FG::DrawIndexed> &task)
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
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<FG::DrawVerticesIndirect> &task)
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
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<FG::DrawIndexedIndirect> &task)
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
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<FG::DrawMeshes> &task)
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
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<FG::DrawMeshesIndirect> &task)
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
	Visit (CustomDraw)
=================================================
*/
	inline void VTaskProcessor::DrawTaskBarriers::Visit (const VFgDrawTask<FG::CustomDraw> &task)
	{
		EResourceState	stages = _tp._fgThread.GetDevice().GetGraphicsShaderStages();

		for (auto& item : task.GetImages())
		{
			ImageViewDesc	desc{ item.first->Description() };
			_tp._AddImage( item.first, (item.second | stages), EResourceState_ToImageLayout( item.second, item.first->AspectMask() ), desc );
		}

		for (auto& item : task.GetBuffers())
		{
			_tp._AddBuffer( item.first, item.second, 0, VK_WHOLE_SIZE );
		}
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
		_tp{ tp },	_currTask{ task },	_cmdBuffer{ cmd }
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

		_tp.vkCmdBindVertexBuffers( _cmdBuffer, 0, uint(buffers.size()), buffers.data(), vertexOffsets.data() );
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
			_tp.vkCmdBindDescriptorSets( _cmdBuffer,
										  VK_PIPELINE_BIND_POINT_GRAPHICS,
										  layout.Handle(),
										  layout.GetFirstDescriptorSet(),
										  uint(task.descriptorSets.size()),
										  task.descriptorSets.data(),
										  uint(task.GetResources().dynamicOffsets.size()),
										  task.GetResources().dynamicOffsets.data() );
			_tp.Stat().descriptorBinds++;
		}
		
		if ( task.GetDebugModeIndex() != Default )
		{
			VkDescriptorSet		desc_set;
			uint				offset, binding;
			_tp._fgThread.GetBatch().GetDescriptotSet( task.GetDebugModeIndex(), OUT binding, OUT desc_set, OUT offset );
			
			_tp.vkCmdBindDescriptorSets( _cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout.Handle(), binding, 1, &desc_set, 1, &offset );
			_tp.Stat().descriptorBinds++;
		}
	}

/*
=================================================
	Visit (DrawVertices)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<FG::DrawVertices> &task)
	{
		//_tp._CmdDebugMarker( task.GetName() );

		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_tp._SetScissor( *_currTask->GetLogicalPass(), task.GetScissors() );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_tp.vkCmdDraw( _cmdBuffer, cmd.vertexCount, cmd.instanceCount, cmd.firstVertex, cmd.firstInstance );
		}
		_tp.Stat().drawCalls += uint(task.commands.size());
	}
	
/*
=================================================
	Visit (DrawIndexed)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<FG::DrawIndexed> &task)
	{
		//_tp._CmdDebugMarker( task.GetName() );
		
		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_tp._SetScissor( *_currTask->GetLogicalPass(), task.GetScissors() );
		_tp._BindIndexBuffer( task.indexBuffer->Handle(), VkDeviceSize(task.indexBufferOffset), VEnumCast(task.indexType) );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_tp.vkCmdDrawIndexed( _cmdBuffer, cmd.indexCount, cmd.instanceCount,
								   cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance );
		}
		_tp.Stat().drawCalls += uint(task.commands.size());
	}
	
/*
=================================================
	Visit (DrawVerticesIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<FG::DrawVerticesIndirect> &task)
	{
		//_tp._CmdDebugMarker( task.GetName() );
		
		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_tp._SetScissor( *_currTask->GetLogicalPass(), task.GetScissors() );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_tp.vkCmdDrawIndirect( _cmdBuffer,
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
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<FG::DrawIndexedIndirect> &task)
	{
		//_tp._CmdDebugMarker( task.GetName() );
		
		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );

		_BindVertexBuffers( task.GetVertexBuffers(), task.GetVBOffsets() );
		_tp._SetScissor( *_currTask->GetLogicalPass(), task.GetScissors() );
		_tp._BindIndexBuffer( task.indexBuffer->Handle(), VkDeviceSize(task.indexBufferOffset), VEnumCast(task.indexType) );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_tp.vkCmdDrawIndexedIndirect( _cmdBuffer,
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
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<FG::DrawMeshes> &task)
	{
		//_tp._CmdDebugMarker( task.GetName() );
		
		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );
		
		_tp._SetScissor( *_currTask->GetLogicalPass(), task.GetScissors() );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_tp.vkCmdDrawMeshTasksNV( _cmdBuffer, cmd.count, cmd.first );
		}
		_tp.Stat().drawCalls += uint(task.commands.size());
	}
	
/*
=================================================
	Visit (DrawMeshesIndirect)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<FG::DrawMeshesIndirect> &task)
	{
		//_tp._CmdDebugMarker( task.GetName() );
		
		VPipelineLayout const*	layout = null;

		_tp._BindPipeline( *_currTask->GetLogicalPass(), task, OUT layout );
		_BindPipelineResources( *layout, task );
		_tp._PushConstants( *layout, task.pushConstants );
		
		_tp._SetScissor( *_currTask->GetLogicalPass(), task.GetScissors() );
		_tp._SetDynamicStates( task.dynamicStates );

		for (auto& cmd : task.commands)
		{
			_tp.vkCmdDrawMeshTasksIndirectNV( _cmdBuffer,
											   task.indirectBuffer->Handle(),
											   VkDeviceSize(cmd.indirectBufferOffset),
											   cmd.drawCount,
											   uint(cmd.stride) );
		}
		_tp.Stat().drawCalls += uint(task.commands.size());
	}
	
/*
=================================================
	Visit (CustomDraw)
=================================================
*/
	inline void VTaskProcessor::DrawTaskCommands::Visit (const VFgDrawTask<FG::CustomDraw> &task)
	{
		DrawContext	ctx{ _tp, *_currTask->GetLogicalPass() };

		task.callback( ctx );
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	constructor
=================================================
*/
	VTaskProcessor::DrawContext::DrawContext (VTaskProcessor &tp, const VLogicalRenderPass &rp) :
		_tp{ tp }, _logicalRP{ rp }, _changed{ ALL_BITS }
	{
		_renderState.color			= _logicalRP.GetColorState();
		_renderState.depth			= _logicalRP.GetDepthState();
		_renderState.stencil		= _logicalRP.GetStencilState();
		_renderState.rasterization	= _logicalRP.GetRasterizationState();
		_renderState.multisample	= _logicalRP.GetMultisampleState();
	}
	
/*
=================================================
	GetData
=================================================
*/
	IDrawContext::Context_t  VTaskProcessor::DrawContext::GetData ()
	{
		VRenderPass const*		render_pass	= _tp._GetResource( _logicalRP.GetRenderPassID() );

		VulkanDrawContext	result;
		result.commandBuffer	= BitCast<CommandBufferVk_t>( _tp._cmdBuffer );
		result.renderPass		= BitCast<RenderPassVk_t>( render_pass->Handle() );
		result.subpassIndex		= _logicalRP.GetSubpassIndex();

		return result;
	}
	
/*
=================================================
	Reset
=================================================
*/
	void VTaskProcessor::DrawContext::Reset ()
	{
		_gpipeline		= null;
		_pplnLayout		= null;
		_mpipeline		= null;
		_vertexInput	= Default;
		_changed		= ALL_BITS;
		_dynamicStates	= Default;
		
		_renderState.color			= _logicalRP.GetColorState();
		_renderState.depth			= _logicalRP.GetDepthState();
		_renderState.stencil		= _logicalRP.GetStencilState();
		_renderState.rasterization	= _logicalRP.GetRasterizationState();
		_renderState.multisample	= _logicalRP.GetMultisampleState();
		_renderState.inputAssembly	= Default;

		_tp._indexBuffer			= VK_NULL_HANDLE;
		_tp._graphicsPipeline		= Default;
		_tp._isDefaultScissor		= false;
		_tp._perPassStatesUpdated	= false;
	}

/*
=================================================
	BindPipeline
=================================================
*/
	void VTaskProcessor::DrawContext::BindPipeline (RawGPipelineID id, EPipelineDynamicState dynamicState)
	{
		_dynamicStates	= dynamicState;
		_gpipeline		= _tp._GetResource( id );
		_mpipeline		= null;
		_changed		|= ALL_BITS;
	}
	
/*
=================================================
	_BindPipeline
=================================================
*/
	void VTaskProcessor::DrawContext::_BindPipeline (uint mask)
	{
		mask = _changed & mask;

		if ( _gpipeline and (mask & GRAPHICS_BIT) )
		{
			_changed ^= GRAPHICS_BIT;

			VkPipeline	ppln_id;
			_tp._fgThread.GetPipelineCache().CreatePipelineInstance(
											_tp._fgThread,
											_logicalRP,
											*_gpipeline,
											_vertexInput,
											_renderState,
											_dynamicStates,
											Default,
											OUT ppln_id, OUT _pplnLayout );

			_tp._BindPipeline2( _logicalRP, ppln_id );
			_tp._SetScissor( _logicalRP, Default );
		}

		if ( _mpipeline and (mask & MESH_BIT) )
		{
			_changed ^= MESH_BIT;
			
			VkPipeline	ppln_id;
			_tp._fgThread.GetPipelineCache().CreatePipelineInstance(
											_tp._fgThread,
											_logicalRP,
											*_mpipeline,
											_renderState,
											_dynamicStates,
											Default,
											OUT ppln_id, OUT _pplnLayout );
		
			_tp._BindPipeline2( _logicalRP, ppln_id );
			_tp._SetScissor( _logicalRP, Default );
		}
	}

/*
=================================================
	BindPipeline
=================================================
*/
	void VTaskProcessor::DrawContext::BindPipeline (RawMPipelineID id, EPipelineDynamicState dynamicState)
	{
		_dynamicStates	= dynamicState;
		_gpipeline		= null;
		_mpipeline		= _tp._GetResource( id );
		_changed		|= ALL_BITS;
	}
	
/*
=================================================
	BindResources
=================================================
*/
	void VTaskProcessor::DrawContext::BindResources (const DescriptorSetID &id, const PipelineResources &res)
	{
		_BindPipeline( ALL_BITS );
		CHECK_ERR( _pplnLayout, void());

		VPipelineResources const*	ppln_res = _tp._fgThread.GetResourceManager().CreateDescriptorSet( res );
		VkDescriptorSet				ds		 = ppln_res->Handle();
		ArrayView<uint>				dyn_offs = res.GetDynamicOffsets();

		RawDescriptorSetLayoutID	ds_layout;
		uint						binding;
		_pplnLayout->GetDescriptorSetLayout( id, OUT ds_layout, OUT binding );

		_tp.vkCmdBindDescriptorSets( _tp._cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pplnLayout->Handle(), binding, 1, &ds, uint(dyn_offs.size()), dyn_offs.data() );
		_tp.Stat().descriptorBinds++;
	}
	
/*
=================================================
	PushConstants
=================================================
*/
	void VTaskProcessor::DrawContext::PushConstants (const PushConstantID &id, const void *data, BytesU dataSize)
	{
		_BindPipeline( ALL_BITS );
		CHECK_ERR( _pplnLayout, void());

		auto	iter = _pplnLayout->GetPushConstants().find( id );
		if ( iter != _pplnLayout->GetPushConstants().end() )
		{
			ASSERT( uint(dataSize) == uint(iter->second.size) );

			_tp.vkCmdPushConstants( _tp._cmdBuffer, _pplnLayout->Handle(), VEnumCast( iter->second.stageFlags ), uint(iter->second.offset), uint(dataSize), data );
			_tp.Stat().pushConstants++;
		}
	}

/*
=================================================
	BindVertexAttribs
=================================================
*/
	void VTaskProcessor::DrawContext::BindVertexAttribs (const VertexInputState &value)
	{
		_vertexInput = value;
		_changed	 |= GRAPHICS_BIT;
	}
	
/*
=================================================
	BindVertexBuffer
=================================================
*/
	void VTaskProcessor::DrawContext::BindVertexBuffer (const VertexBufferID &id, RawBufferID vbuf, BytesU offset)
	{
		auto	iter = _vertexInput.BufferBindings().find( id );
		auto*	buf  = _tp._GetResource( vbuf );

		if ( iter != _vertexInput.BufferBindings().end() and buf )
		{
			VkBuffer		vk_buf	= buf->Handle();
			VkDeviceSize	off		{ offset };

			_tp.vkCmdBindVertexBuffers( _tp._cmdBuffer, iter->second.index, 1, &vk_buf, &off );
			_tp.Stat().vertexBufferBindings++;
		}
	}
	
/*
=================================================
	BindIndexBuffer
=================================================
*/
	void VTaskProcessor::DrawContext::BindIndexBuffer (RawBufferID ibuf, BytesU offset, EIndex type)
	{
		auto*	buf = _tp._GetResource( ibuf );
		if ( buf ) {
			_tp._BindIndexBuffer( buf->Handle(), VkDeviceSize(offset), VEnumCast(type) );
		}
	}
	
/*
=================================================
	SetColorBuffer
=================================================
*/
	void VTaskProcessor::DrawContext::SetColorBuffer (RenderTargetID id, const RenderState::ColorBuffer &value)
	{
		CHECK_ERR( size_t(id) < _renderState.color.buffers.size(), void());

		_renderState.color.buffers[ uint(id) ] = value;
		_changed |= ALL_BITS;
	}
	
/*
=================================================
	SetLogicOp
=================================================
*/
	void VTaskProcessor::DrawContext::SetLogicOp (ELogicOp value)
	{
		_renderState.color.logicOp = value;
		_changed |= ALL_BITS;
	}
	
/*
=================================================
	SetBlendColor
=================================================
*/
	void VTaskProcessor::DrawContext::SetBlendColor (const RGBA32f &value)
	{
		_renderState.color.blendColor = value;
		_changed |= ALL_BITS;
	}
	
/*
=================================================
	SetStencilBuffer
=================================================
*/
	void VTaskProcessor::DrawContext::SetStencilBuffer (const RenderState::StencilBufferState &value)
	{
		_renderState.stencil = value;
		_changed |= ALL_BITS;
	}
	
/*
=================================================
	SetDepthBuffer
=================================================
*/
	void VTaskProcessor::DrawContext::SetDepthBuffer (const RenderState::DepthBufferState &value)
	{
		_renderState.depth = value;
		_changed |= ALL_BITS;
	}
	
/*
=================================================
	SetInputAssembly
=================================================
*/
	void VTaskProcessor::DrawContext::SetInputAssembly (const RenderState::InputAssemblyState &value)
	{
		_renderState.inputAssembly = value;
		_changed |= GRAPHICS_BIT;
	}
	
/*
=================================================
	SetRasterization
=================================================
*/
	void VTaskProcessor::DrawContext::SetRasterization (const RenderState::RasterizationState &value)
	{
		_renderState.rasterization = value;
		_changed |= ALL_BITS;
	}
	
/*
=================================================
	SetMultisample
=================================================
*/
	void VTaskProcessor::DrawContext::SetMultisample (const RenderState::MultisampleState &value)
	{
		_renderState.multisample = value;
		_changed |= ALL_BITS;
	}
	
/*
=================================================
	SetStencilCompareMask
=================================================
*/
	void VTaskProcessor::DrawContext::SetStencilCompareMask (uint value)
	{
		_BindPipeline( ALL_BITS );
		_tp.vkCmdSetStencilCompareMask( _tp._cmdBuffer, VK_STENCIL_FRONT_AND_BACK, value );
	}
	
/*
=================================================
	SetStencilWriteMask
=================================================
*/
	void VTaskProcessor::DrawContext::SetStencilWriteMask (uint value)
	{
		_BindPipeline( ALL_BITS );
		_tp.vkCmdSetStencilWriteMask( _tp._cmdBuffer, VK_STENCIL_FRONT_AND_BACK, value );
	}
	
/*
=================================================
	SetStencilReference
=================================================
*/
	void VTaskProcessor::DrawContext::SetStencilReference (uint value)
	{
		_BindPipeline( ALL_BITS );
		_tp.vkCmdSetStencilReference( _tp._cmdBuffer, VK_STENCIL_FRONT_AND_BACK, value );
	}
	
/*
=================================================
	SetShadingRatePalette
=================================================
*/
	void VTaskProcessor::DrawContext::SetShadingRatePalette (uint viewportIndex, ArrayView<EShadingRatePalette> value)
	{
		_BindPipeline( ALL_BITS );
		
		StaticArray< VkShadingRatePaletteEntryNV, 32 >	entries;
		entries[0] = VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV;

		VkShadingRatePaletteNV			palette = {};
		palette.shadingRatePaletteEntryCount = Max( 1u, uint(value.size()) );
		palette.pShadingRatePaletteEntries   = entries.data();

		for (uint i = 0; i < value.size(); ++i) {
			entries[i] = VEnumCast( value[i] );
		}

		_tp.vkCmdSetViewportShadingRatePaletteNV( _tp._cmdBuffer, viewportIndex, 1, &palette );
	}
	
/*
=================================================
	BindShadingRateImage
=================================================
*/
	void VTaskProcessor::DrawContext::BindShadingRateImage (RawImageID value, ImageLayer layer, MipmapLevel level)
	{
		auto*	image = _tp._GetResource( value );
		CHECK_ERR( image, void());
		
		ImageViewDesc	desc;
		desc.viewType	= EImage::Tex2D;
		desc.format		= EPixelFormat::R8U;
		desc.baseLevel	= level;
		desc.baseLayer	= layer;
		desc.aspectMask	= EImageAspect::Color;
		
		VkImageView	view = image->GetView( _tp._fgThread.GetDevice(), false, desc );

		_tp._BindShadingRateImage( view );
	}

/*
=================================================
	DrawVertices
=================================================
*/
	void VTaskProcessor::DrawContext::DrawVertices (uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance)
	{
		CHECK( _gpipeline );
		_BindPipeline( GRAPHICS_BIT );

		_tp.vkCmdDraw( _tp._cmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance );
	}
	
/*
=================================================
	DrawIndexed
=================================================
*/
	void VTaskProcessor::DrawContext::DrawIndexed (uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset, uint firstInstance)
	{
		CHECK( _gpipeline );
		_BindPipeline( GRAPHICS_BIT );

		_tp.vkCmdDrawIndexed( _tp._cmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
	}
	
/*
=================================================
	DrawVerticesIndirect
=================================================
*/
	void VTaskProcessor::DrawContext::DrawVerticesIndirect (RawBufferID indirectBuffer, BytesU indirectBufferOffset, uint drawCount, BytesU stride)
	{
		auto*	buf = _tp._GetResource( indirectBuffer );
		
		CHECK_ERR( buf, void());
		CHECK( _gpipeline );

		_BindPipeline( GRAPHICS_BIT );
		_tp.vkCmdDrawIndirect( _tp._cmdBuffer, buf->Handle(), VkDeviceSize(indirectBufferOffset), drawCount, uint(stride) );
	}
	
/*
=================================================
	DrawIndexedIndirect
=================================================
*/
	void VTaskProcessor::DrawContext::DrawIndexedIndirect (RawBufferID indirectBuffer, BytesU indirectBufferOffset, uint drawCount, BytesU stride)
	{
		auto*	buf = _tp._GetResource( indirectBuffer );
		CHECK_ERR( buf, void());
		CHECK( _gpipeline );
		
		_BindPipeline( GRAPHICS_BIT );
		_tp.vkCmdDrawIndexedIndirect( _tp._cmdBuffer, buf->Handle(), VkDeviceSize(indirectBufferOffset), drawCount, uint(stride) );
	}
	
/*
=================================================
	DrawMeshes
=================================================
*/
	void VTaskProcessor::DrawContext::DrawMeshes (uint taskCount, uint firstTask)
	{
		CHECK( _mpipeline );
		_BindPipeline( MESH_BIT );

		_tp.vkCmdDrawMeshTasksNV( _tp._cmdBuffer, taskCount, firstTask );
	}
	
/*
=================================================
	DrawMeshesIndirect
=================================================
*/
	void VTaskProcessor::DrawContext::DrawMeshesIndirect (RawBufferID indirectBuffer, BytesU indirectBufferOffset, uint drawCount, BytesU stride)
	{
		auto*	buf = _tp._GetResource( indirectBuffer );
		CHECK_ERR( buf, void());
		CHECK( _mpipeline );
		
		_BindPipeline( MESH_BIT );
		_tp.vkCmdDrawMeshTasksIndirectNV( _tp._cmdBuffer, buf->Handle(), VkDeviceSize(indirectBufferOffset), drawCount, uint(stride) );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VTaskProcessor::VTaskProcessor (VCommandBuffer &fgThread, VkCommandBuffer cmd) :
		_fgThread{ fgThread },
		_cmdBuffer{ cmd },						_enableDebugUtils{ _fgThread.GetDevice().IsDebugUtilsEnabled() },
		_isDefaultScissor{ false },				_perPassStatesUpdated{ false },
		_pendingResourceBarriers{ fgThread.GetAllocator() }
	{
		ASSERT( _cmdBuffer );
		
		VulkanDeviceFn_Init( _fgThread.GetDevice() );

		//_CmdPushDebugGroup( "SubBatch: "s << batchId.GetName() << ", index: " << ToString(indexInBatch) );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VTaskProcessor::~VTaskProcessor ()
	{
		//_CmdPopDebugGroup();
	}
	
/*
=================================================
	Visit*_DrawVertices
=================================================
*/
	void VTaskProcessor::Visit1_DrawVertices (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawVertices>*>( taskData ));
	}
	
	void VTaskProcessor::Visit2_DrawVertices (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawVertices>*>( taskData ));
	}
	
/*
=================================================
	Visit*_DrawIndexed
=================================================
*/
	void VTaskProcessor::Visit1_DrawIndexed (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawIndexed>*>( taskData ));
	}

	void VTaskProcessor::Visit2_DrawIndexed (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawIndexed>*>( taskData ));
	}
	
/*
=================================================
	Visit*_DrawMeshes
=================================================
*/
	void VTaskProcessor::Visit1_DrawMeshes (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawMeshes>*>( taskData ));
	}

	void VTaskProcessor::Visit2_DrawMeshes (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawMeshes>*>( taskData ));
	}
	
/*
=================================================
	Visit*_DrawVerticesIndirect
=================================================
*/
	void VTaskProcessor::Visit1_DrawVerticesIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawVerticesIndirect>*>( taskData ));
	}

	void VTaskProcessor::Visit2_DrawVerticesIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawVerticesIndirect>*>( taskData ));
	}
	
/*
=================================================
	Visit*_DrawIndexedIndirect
=================================================
*/
	void VTaskProcessor::Visit1_DrawIndexedIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawIndexedIndirect>*>( taskData ));
	}

	void VTaskProcessor::Visit2_DrawIndexedIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawIndexedIndirect>*>( taskData ));
	}
	
/*
=================================================
	Visit*_DrawMeshesIndirect
=================================================
*/
	void VTaskProcessor::Visit1_DrawMeshesIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawMeshesIndirect>*>( taskData ));
	}

	void VTaskProcessor::Visit2_DrawMeshesIndirect (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::DrawMeshesIndirect>*>( taskData ));
	}
	
/*
=================================================
	Visit*_CustomDraw
=================================================
*/
	void VTaskProcessor::Visit1_CustomDraw (void *visitor, void *taskData)
	{
		static_cast<DrawTaskBarriers *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::CustomDraw>*>( taskData ));
	}

	void VTaskProcessor::Visit2_CustomDraw (void *visitor, void *taskData)
	{
		static_cast<DrawTaskCommands *>(visitor)->Visit( *static_cast<VFgDrawTask<FG::CustomDraw>*>( taskData ));
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
			
			vkCmdInsertDebugUtilsLabelEXT( _cmdBuffer, &info );
		}
	}

/*
=================================================
	_CmdPushDebugGroup
=================================================
*/
	void VTaskProcessor::_CmdPushDebugGroup (StringView text) const
	{
		/*if ( text.size() and _enableDebugUtils )
		{
			VkDebugUtilsLabelEXT	info = {};
			info.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			info.pLabelName	= text.data();
			MemCopy( info.color, _dbgColor );

			vkCmdBeginDebugUtilsLabelEXT( _cmdBuffer, &info );
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
			vkCmdEndDebugUtilsLabelEXT( _cmdBuffer );
		}*/
	}
	
/*
=================================================
	_SetScissor
=================================================
*/
	void VTaskProcessor::_SetScissor (const VLogicalRenderPass &logicalPsss, ArrayView<RectI> srcScissors)
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

			vkCmdSetScissor( _cmdBuffer, 0, uint(vk_scissors.size()), vk_scissors.data() );
			_isDefaultScissor = false;
		}
		else
		if ( not _isDefaultScissor )
		{
			const auto&	vk_scissors = logicalPsss.GetScissors();

			vkCmdSetScissor( _cmdBuffer, 0, uint(vk_scissors.size()), vk_scissors.data() );
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
				vkCmdSetStencilCompareMask( _cmdBuffer, VK_STENCIL_FRONT_AND_BACK, state.stencilCompareMask );

			if ( state.hasStencilReference )
				vkCmdSetStencilReference( _cmdBuffer, VK_STENCIL_FRONT_AND_BACK, state.stencilReference );

			if ( state.hasStencilWriteMask )
				vkCmdSetStencilWriteMask( _cmdBuffer, VK_STENCIL_FRONT_AND_BACK, state.stencilWriteMask );

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
			EResourceState	state	= rt.state;
			VkImageLayout&	layout	= rt._layout;
			layout = EResourceState_ToImageLayout( state, rt.imagePtr->AspectMask() );

			_AddImage( rt.imagePtr, state, layout, rt.desc );
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

		view = image->GetView( _fgThread.GetDevice(), false, desc );

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
			vkCmdBindShadingRateImageNV( _cmdBuffer, _shadingRateImage, VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV );
		}
	}
	
/*
=================================================
	_CreateRenderPass
=================================================
*/
	bool VTaskProcessor::_CreateRenderPass (ArrayView<VLogicalRenderPass*> logicalPasses)
	{
		using Attachment_t = FixedArray< Pair<RawImageID, ImageViewDesc>, FG_MaxColorBuffers+1 >;

		RawRenderPassID		rp_id = _fgThread.GetResourceManager().CreateRenderPass( logicalPasses, "" );
		CHECK_ERR( rp_id );

		VRenderPass const*	render_pass		= _fgThread.GetResourceManager().GetResource( rp_id );
		Attachment_t		render_targets;	  render_targets.resize( render_pass->GetCreateInfo().attachmentCount );
		Optional<RectI>		total_area;
		const uint			depth_index		= render_pass->GetCreateInfo().attachmentCount-1;

		for (const auto& lrp : logicalPasses)
		{
			// merge rendering areas
			if ( total_area.has_value() )
				total_area->Merge( lrp->GetArea() );
			else
				total_area = lrp->GetArea();

			if ( lrp->GetDepthStencilTarget().IsDefined() )
			{
				auto&	src = lrp->GetDepthStencilTarget();
				auto&	dst = render_targets[ depth_index ];

				// compare attachments
				CHECK_ERR( not dst.first or (dst.first == src.imageId and dst.second == src.desc) );

				dst.first	= src.imageId;
				dst.second	= src.desc;
			}

			for (const auto& ct : lrp->GetColorTargets())
			{	
				auto&	dst = render_targets[ ct.index ];
				
				// compare attachments
				CHECK_ERR( not dst.first or (dst.first == ct.imageId and dst.second == ct.desc) );

				dst.first	= ct.imageId;
				dst.second	= ct.desc;
			}
		}
		
		RawFramebufferID	fb_id = _fgThread.GetResourceManager().CreateFramebuffer( render_targets, rp_id, uint2(total_area->Size()), 1, "" );
		CHECK_ERR( fb_id );
		
		uint	subpass = 0;
		for (auto& pass : logicalPasses) {
			pass->_SetRenderPass( rp_id, subpass++, fb_id, depth_index );
		}
		return true;
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
		CHECK( _CreateRenderPass( logical_passes ));
		

		// begin render pass
		VFramebuffer const*	framebuffer = _GetResource( task.GetLogicalPass()->GetFramebufferID() );
		VRenderPass const*	render_pass = _GetResource( task.GetLogicalPass()->GetRenderPassID() );
		RectI const&		area		= task.GetLogicalPass()->GetArea();

		VkRenderPassBeginInfo	pass_info = {};
		pass_info.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		pass_info.renderPass				= render_pass->Handle();
		pass_info.renderArea.offset.x		= area.left;
		pass_info.renderArea.offset.y		= area.top;
		pass_info.renderArea.extent.width	= uint(area.Width());
		pass_info.renderArea.extent.height	= uint(area.Height());
		pass_info.clearValueCount			= render_pass->GetCreateInfo().attachmentCount;
		pass_info.pClearValues				= task.GetLogicalPass()->GetClearValues().data();
		pass_info.framebuffer				= framebuffer->Handle();
		
		vkCmdBeginRenderPass( _cmdBuffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE );

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

		vkCmdNextSubpass( _cmdBuffer, VK_SUBPASS_CONTENTS_INLINE );
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
		if ( task.GetLogicalPass()->GetDrawTasks().empty() )
			return;

		// invalidate some states
		_isDefaultScissor		= false;
		_perPassStatesUpdated	= false;

		if ( not task.IsSubpass() )
		{
			_CmdPushDebugGroup( task.Name() );
			_BeginRenderPass( task );
		}
		else
		{
			_CmdPopDebugGroup();
			_CmdPushDebugGroup( task.Name() );
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
			vkCmdEndRenderPass( _cmdBuffer );
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
			res.pplnRes->ForEachUniform( visitor );

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
												 VkPipelineBindPoint bindPoint, ShaderDbgIndex debugModeIndex)
	{
		// update descriptor sets and add pipeline barriers
		VkDescriptorSets_t	descriptor_sets;
		_ExtractDescriptorSets( layout, resourceSet, OUT descriptor_sets );

		if ( descriptor_sets.size() )
		{
			vkCmdBindDescriptorSets( _cmdBuffer,
									  bindPoint,
									  layout.Handle(),
									  layout.GetFirstDescriptorSet(),
									  uint(descriptor_sets.size()),
									  descriptor_sets.data(),
									  uint(resourceSet.dynamicOffsets.size()),
									  resourceSet.dynamicOffsets.data() );
			Stat().descriptorBinds++;
		}

		if ( debugModeIndex != Default )
		{
			VkDescriptorSet		desc_set;
			uint				offset, binding;
			_fgThread.GetBatch().GetDescriptotSet( debugModeIndex, OUT binding, OUT desc_set, OUT offset );

			vkCmdBindDescriptorSets( _cmdBuffer, bindPoint, layout.Handle(), binding, 1, &desc_set, 1, &offset );
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

				vkCmdPushConstants( _cmdBuffer, layout.Handle(), VEnumCast( iter->second.stageFlags ), uint(iter->second.offset),
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
			vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineId );
			Stat().graphicsPipelineBindings++;
		}
		
		// all pipelines in current render pass have same viewport count and same dynamic states, so this values should not be invalidated.
		if ( _perPassStatesUpdated )
			return;

		_perPassStatesUpdated = true;
		
		if ( auto viewports = logicalRP.GetViewports(); viewports.size() )
			vkCmdSetViewport( _cmdBuffer, 0, uint(viewports.size()), viewports.data() );

		if ( auto palette = logicalRP.GetShadingRatePalette(); palette.size() )
			vkCmdSetViewportShadingRatePaletteNV( _cmdBuffer, 0, uint(palette.size()), palette.data() );
	}
	
/*
=================================================
	OverrideColorStates
=================================================
*/
	static void OverrideColorStates (INOUT RenderState::ColorBuffersState &currColorStates, const _fg_hidden_::ColorBuffers_t &newStates)
	{
		for (auto& cb : newStates)
		{
			currColorStates.buffers[ uint(cb.first) ] = cb.second;
		}
	}
	
/*
=================================================
	OverrideDepthStencilStates
=================================================
*/
	static void OverrideDepthStencilStates (INOUT RenderState::DepthBufferState &currDepthState, INOUT RenderState::StencilBufferState &currStencilState,
											INOUT RenderState::RasterizationState &currRasterState, INOUT EPipelineDynamicState &dynamicState,
											const _fg_hidden_::DynamicStates &newStates)
	{
		currDepthState.test					= newStates.hasDepthTest ? newStates.depthTest : currDepthState.test;
		currDepthState.write				= newStates.hasDepthWrite ? newStates.depthWrite : currDepthState.write;
		currStencilState.enabled			= newStates.hasStencilTest ? newStates.stencilTest : currStencilState.enabled;
		currRasterState.cullMode			= newStates.hasCullMode ? newStates.cullMode : currRasterState.cullMode;
		currRasterState.rasterizerDiscard	= newStates.hasRasterizedDiscard ? newStates.rasterizerDiscard : currRasterState.rasterizerDiscard;
		currRasterState.frontFaceCCW		= newStates.hasFrontFaceCCW ? newStates.frontFaceCCW : currRasterState.frontFaceCCW;

		if ( currDepthState.test )
		{
			currDepthState.compareOp = newStates.hasDepthCompareOp ? newStates.depthCompareOp : currDepthState.compareOp;
		}

		// override stencil states
		if ( currStencilState.enabled )
		{
			if ( newStates.hasStencilFailOp )
				currStencilState.front.failOp = currStencilState.back.failOp = newStates.stencilFailOp;

			if ( newStates.hasStencilDepthFailOp )
				currStencilState.front.depthFailOp = currStencilState.back.depthFailOp = newStates.stencilDepthFailOp;

			if ( newStates.hasStencilPassOp )
				currStencilState.front.passOp = currStencilState.back.passOp = newStates.stencilPassOp;

			dynamicState |= newStates.hasStencilCompareMask ? EPipelineDynamicState::StencilCompareMask : Default;
			dynamicState |= newStates.hasStencilReference   ? EPipelineDynamicState::StencilReference   : Default;
			dynamicState |= newStates.hasStencilWriteMask   ? EPipelineDynamicState::StencilWriteMask   : Default;
		}
	}
	
/*
=================================================
	SetupExtensions
=================================================
*/
	static void SetupExtensions (const VLogicalRenderPass &logicalRP, INOUT EPipelineDynamicState &dynamicState)
	{
		if ( logicalRP.HasShadingRateImage() )
			dynamicState |= EPipelineDynamicState::ShadingRatePalette;
	}

/*
=================================================
	_BindPipeline
=================================================
*/
	inline void VTaskProcessor::_BindPipeline (const VLogicalRenderPass &logicalRP, const VBaseDrawVerticesTask &task, VPipelineLayout const* &pplnLayout)
	{
		RenderState				render_state;
		EPipelineDynamicState	dynamic_states = EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;
		
		render_state.color			= logicalRP.GetColorState();
		render_state.depth			= logicalRP.GetDepthState();
		render_state.stencil		= logicalRP.GetStencilState();
		render_state.rasterization	= logicalRP.GetRasterizationState();
		render_state.multisample	= logicalRP.GetMultisampleState();

		render_state.inputAssembly.topology			= task.topology;
		render_state.inputAssembly.primitiveRestart = task.primitiveRestart;

		OverrideColorStates( INOUT render_state.color, task.colorBuffers );
		OverrideDepthStencilStates( INOUT render_state.depth, INOUT render_state.stencil,
								    INOUT render_state.rasterization, INOUT dynamic_states, task.dynamicStates );
		SetupExtensions( logicalRP, INOUT dynamic_states );

		VkPipeline	ppln_id;
		_fgThread.GetPipelineCache().CreatePipelineInstance(
										_fgThread,
										logicalRP,
										*task.pipeline,
										task.vertexInput,
										render_state,
										dynamic_states,
										task.GetDebugModeIndex(),
										OUT ppln_id, OUT pplnLayout );

		_BindPipeline2( logicalRP, ppln_id );
	}
	
/*
=================================================
	_BindPipeline
=================================================
*/
	inline void VTaskProcessor::_BindPipeline (const VLogicalRenderPass &logicalRP, const VBaseDrawMeshes &task, VPipelineLayout const* &pplnLayout)
	{
		RenderState				render_state;
		EPipelineDynamicState	dynamic_states = EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;
		
		render_state.color			= logicalRP.GetColorState();
		render_state.depth			= logicalRP.GetDepthState();
		render_state.stencil		= logicalRP.GetStencilState();
		render_state.rasterization	= logicalRP.GetRasterizationState();
		render_state.multisample	= logicalRP.GetMultisampleState();

		OverrideColorStates( INOUT render_state.color, task.colorBuffers );
		OverrideDepthStencilStates( INOUT render_state.depth, INOUT render_state.stencil,
								    INOUT render_state.rasterization, INOUT dynamic_states, task.dynamicStates );
		SetupExtensions( logicalRP, INOUT dynamic_states );

		VkPipeline	ppln_id;
		_fgThread.GetPipelineCache().CreatePipelineInstance(
										_fgThread,
										logicalRP,
										*task.pipeline,
										render_state,
										dynamic_states,
										task.GetDebugModeIndex(),
										OUT ppln_id, OUT pplnLayout );
		
		_BindPipeline2( logicalRP, ppln_id );
	}

/*
=================================================
	_BindPipeline
=================================================
*/
	inline void VTaskProcessor::_BindPipeline (const VComputePipeline* pipeline, const Optional<uint3> &localSize,
											   ShaderDbgIndex debugModeIndex, VkPipelineCreateFlags flags, OUT VPipelineLayout const* &pplnLayout)
	{
		VkPipeline	ppln_id;
		_fgThread.GetPipelineCache().CreatePipelineInstance(
										_fgThread,
										*pipeline, localSize,
										flags,
										debugModeIndex,
										OUT ppln_id, OUT pplnLayout );
		
		if ( _computePipeline.pipeline != ppln_id )
		{
			_computePipeline.pipeline = ppln_id;
			vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ppln_id );
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
		_CmdDebugMarker( task.Name() );

		VPipelineLayout const*	layout = null;

		_BindPipeline( task.pipeline, task.localGroupSize, task.GetDebugModeIndex(), VK_PIPELINE_CREATE_DISPATCH_BASE, OUT layout );
		_BindPipelineResources( *layout, task.GetResources(), VK_PIPELINE_BIND_POINT_COMPUTE, task.GetDebugModeIndex() );
		_PushConstants( *layout, task.pushConstants );

		_CommitBarriers();

		for (auto& cmd : task.commands)
		{
			vkCmdDispatchBase( _cmdBuffer, cmd.baseGroup.x, cmd.baseGroup.y, cmd.baseGroup.z,
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
		_CmdDebugMarker( task.Name() );
		
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
			vkCmdDispatchIndirect( _cmdBuffer,
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
		_CmdDebugMarker( task.Name() );

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
		
		vkCmdCopyBuffer( _cmdBuffer,
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
		_CmdDebugMarker( task.Name() );

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
		
		vkCmdCopyImage( _cmdBuffer,
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
		_CmdDebugMarker( task.Name() );

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
		
		vkCmdCopyBufferToImage( _cmdBuffer,
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
		_CmdDebugMarker( task.Name() );

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
		
		vkCmdCopyImageToBuffer( _cmdBuffer,
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
		_CmdDebugMarker( task.Name() );

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
		
		vkCmdBlitImage( _cmdBuffer,
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
		_CmdDebugMarker( task.Name() );
		
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
		
		vkCmdResolveImage(	_cmdBuffer,
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
	Visit (GenerateMipmaps)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<GenerateMipmaps> &task)
	{
		_CmdDebugMarker( task.Name() );

		VLocalImage const*		image		= task.image;
		const VkImage			vk_image	= image->Handle();
		const uint				level_count	= Min( task.levelCount, image->MipmapLevels() - Min( image->MipmapLevels()-1, task.baseLevel ));
		const uint				arr_layers	= image->ArrayLayers();
		const uint3				dimension	= image->Dimension() >> task.baseLevel;
		VkImageSubresourceRange	subres;

		subres.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		subres.baseArrayLayer	= 0;
		subres.layerCount		= arr_layers;
		subres.baseMipLevel		= task.baseLevel;
		subres.levelCount		= level_count;

		_AddImage( image, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subres );
		_CommitBarriers();
	
		VkImageMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.image				= vk_image;

		for (uint i = 1; i < level_count; ++i)
		{
			const int3	src_size	= Max( 1u, dimension >> (i-1) );
			const int3	dst_size	= Max( 1u, dimension >> i );
			
			// undefined -> transfer_dst_optimal
			barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask		= 0;
			barrier.dstAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, (task.baseLevel+i), 1, 0, arr_layers };

			vkCmdPipelineBarrier( _cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
									0, null, 0, null, 1, &barrier );

			VkImageBlit		region	= {};
			region.srcOffsets[0]	= { 0, 0, 0 };
			region.srcOffsets[1]	= { src_size.x, src_size.y, src_size.z };
			region.srcSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, (task.baseLevel+i-1), 0, arr_layers };
			region.dstOffsets[0]	= { 0, 0, 0 };
			region.dstOffsets[1]	= { dst_size.x, dst_size.y, dst_size.z };
			region.dstSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, (task.baseLevel+i), 0, arr_layers };

			vkCmdBlitImage( _cmdBuffer, vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							 vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR );
			
			Stat().transferOps++;

			// read after write
			barrier.oldLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask		= VK_ACCESS_TRANSFER_READ_BIT;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, (task.baseLevel+i), 1, 0, arr_layers };

			vkCmdPipelineBarrier( _cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
									0, null, 0, null, 1, &barrier );
		}
	}

/*
=================================================
	Visit (FillBuffer)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<FillBuffer> &task)
	{
		_CmdDebugMarker( task.Name() );

		VLocalBuffer const *	dst_buffer = task.dstBuffer;

		_AddBuffer( dst_buffer, EResourceState::TransferDst, task.dstOffset, task.size );
		
		_CommitBarriers();
		
		vkCmdFillBuffer( _cmdBuffer,
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
		_CmdDebugMarker( task.Name() );

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
		
		vkCmdClearColorImage( _cmdBuffer,
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
		_CmdDebugMarker( task.Name() );
		
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
		
		vkCmdClearDepthStencilImage( _cmdBuffer,
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
		_CmdDebugMarker( task.Name() );
		
		VLocalBuffer const *	dst_buffer = task.dstBuffer;

		for (auto& reg : task.Regions()) {
			_AddBuffer( dst_buffer, EResourceState::TransferDst, reg.bufferOffset, reg.dataSize );
		}	
		_CommitBarriers();
		
		for (auto& reg : task.Regions()) {
			vkCmdUpdateBuffer( _cmdBuffer, dst_buffer->Handle(), reg.bufferOffset, reg.dataSize, reg.dataPtr );
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
		_CmdDebugMarker( task.Name() );
		
		RawImageID	swapchain_image;
		CHECK( task.swapchain->Acquire( _fgThread, ESwapchainImage::Primary, OUT swapchain_image ));

		VLocalImage const *		src_image	= task.srcImage;
		VLocalImage const *		dst_image	= _ToLocal( swapchain_image );
		const uint3				src_dim		= src_image->Dimension();
		const uint3				dst_dim		= dst_image->Dimension();
		const VkFilter			filter		= src_dim.x == dst_dim.x and src_dim.y == dst_dim.y ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
		VkImageBlit				region;

		CHECK( src_image != dst_image );
		
		region.srcSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, task.mipmap.Get(), task.layer.Get(), 1 };
		region.srcOffsets[0]	= VkOffset3D{ 0, 0, 0 };
		region.srcOffsets[1]	= VkOffset3D{ int(src_dim.x), int(src_dim.y), 1 };
			
		region.dstSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.dstOffsets[0]	= VkOffset3D{ 0, 0, 0 };
		region.dstOffsets[1]	= VkOffset3D{ int(dst_dim.x), int(dst_dim.y), 1 };

		_AddImage( src_image, EResourceState::TransferSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, region.srcSubresource );
		_AddImage( dst_image, EResourceState::TransferDst | EResourceState::InvalidateBefore, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region.dstSubresource );
		_CommitBarriers();
		
		vkCmdBlitImage( _cmdBuffer,
						 src_image->Handle(),
						 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						 dst_image->Handle(),
						 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						 1, &region,
						 filter );
	}

/*
=================================================
	Visit (PresentVR)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<PresentVR> &task)
	{
		/*
		_CmdDebugMarker( task.Name() );

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
		_CmdDebugMarker( task.Name() );

		VPipelineCache::BufferCopyRegions_t	copy_regions;

		CHECK_ERR( _fgThread.GetPipelineCache().InitShaderTable(
													_fgThread,
													task.pipeline,
													*task.rtScene->ToGlobal(),
													task.rayGenShader,
													task.GetShaderGroups(),
													task.maxRecursionDepth,
													INOUT *task.shaderTable,
													OUT copy_regions ), void());

		for (auto& copy : copy_regions)
		{
			_AddBuffer( copy.srcBuffer, EResourceState::TransferSrc, copy.region.srcOffset, copy.region.size );
			_AddBuffer( copy.dstBuffer, EResourceState::TransferDst, copy.region.dstOffset, copy.region.size );
		}
		
		_CommitBarriers();
		
		for (auto& copy : copy_regions) {
			vkCmdCopyBuffer( _cmdBuffer, copy.srcBuffer->Handle(), copy.dstBuffer->Handle(), 1, &copy.region );
		}

		Stat().transferOps += uint(copy_regions.size());
	}

/*
=================================================
	Visit (BuildRayTracingGeometry)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<BuildRayTracingGeometry> &task)
	{
		_CmdDebugMarker( task.Name() );
		
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

		vkCmdBuildAccelerationStructureNV( _cmdBuffer, &info,
											VK_NULL_HANDLE, 0,
											VK_FALSE,
											task.RTGeometry()->Handle(), VK_NULL_HANDLE,
											task.ScratchBuffer()->Handle(),
											task.ScratchBufferOffset() );
		Stat().buildASCalls++;
	}
	
/*
=================================================
	Visit (BuildRayTracingScene)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<BuildRayTracingScene> &task)
	{
		_CmdDebugMarker( task.Name() );
		
		task.RTScene()->ToGlobal()->SetGeometryInstances( _fgThread.GetResourceManager(), task.Instances(), task.InstanceCount(),
														  task.HitShadersPerInstance(), task.MaxHitShaderCount() );

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

		vkCmdBuildAccelerationStructureNV( _cmdBuffer, &info,
											task.InstanceBuffer()->Handle(), task.InstanceBufferOffset(),
											VK_FALSE,
											task.RTScene()->Handle(), VK_NULL_HANDLE,
											task.ScratchBuffer()->Handle(),
											task.ScratchBufferOffset() );
		Stat().buildASCalls++;
	}
	
/*
=================================================
	Visit (TraceRays)
=================================================
*/
	void VTaskProcessor::Visit (const VFgTask<TraceRays> &task)
	{
		_CmdDebugMarker( task.Name() );

		const bool			is_debuggable	= (task.GetDebugModeIndex() != Default);
		EShaderDebugMode	dbg_mode		= Default;
		EShaderStages		dbg_stages		= Default;
		RawPipelineLayoutID	layout_id;
		VkPipeline			pipeline		= VK_NULL_HANDLE;
		VkDeviceSize		raygen_offset	= 0;
		VkDeviceSize		raymiss_offset	= 0;
		VkDeviceSize		raymiss_stride	= 0;
		VkDeviceSize		rayhit_offset	= 0;
		VkDeviceSize		rayhit_stride	= 0;
		VkDeviceSize		callable_offset	= 0;
		VkDeviceSize		callable_stride	= 0;
		VkDeviceSize		block_size		= 0;

		if ( is_debuggable )
		{
			auto&	debugger = _fgThread.GetBatch();
			auto*	ppln	 = _GetResource( task.shaderTable->GetPipeline() );

			CHECK_ERR( ppln, void());
			CHECK( debugger.GetDebugModeInfo( task.GetDebugModeIndex(), OUT dbg_mode, OUT dbg_stages ));

			for (auto& shader : ppln->GetShaderModules())
			{
				if ( shader.debugMode == dbg_mode )
					debugger.SetShaderModule( task.GetDebugModeIndex(), shader.module );
			}
		}

		CHECK_ERR( task.shaderTable->GetBindings( dbg_mode, OUT layout_id, OUT pipeline, OUT block_size, OUT raygen_offset,
												  OUT raymiss_offset, OUT raymiss_stride, OUT rayhit_offset, OUT rayhit_stride,
												  OUT callable_offset, OUT callable_stride ), void());

		VPipelineLayout const*	layout		= _GetResource( layout_id );
		VLocalBuffer const*		sbt_buffer	= _ToLocal( task.shaderTable->GetBuffer() );
		
		if ( _rayTracingPipeline.pipeline != pipeline )
		{
			_rayTracingPipeline.pipeline = pipeline;
			vkCmdBindPipeline( _cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline );
			Stat().rayTracingPipelineBindings++;
		}

		_BindPipelineResources( *layout, task.GetResources(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, task.GetDebugModeIndex() );
		_PushConstants( *layout, task.pushConstants );

		_AddBuffer( sbt_buffer, EResourceState::UniformRead | EResourceState::_RayTracingShader, raygen_offset, block_size );
		_CommitBarriers();
		
		vkCmdTraceRaysNV( _cmdBuffer, 
							sbt_buffer->Handle(), raygen_offset,
							sbt_buffer->Handle(), raymiss_offset, raymiss_stride,
							sbt_buffer->Handle(), rayhit_offset,  rayhit_stride,
							sbt_buffer->Handle(), callable_offset, callable_stride,
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

		if ( _fgThread.GetDebugger() )
			_fgThread.GetDebugger()->AddImageUsage( img->ToGlobal(), state );
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
		
		if ( _fgThread.GetDebugger() )
			_fgThread.GetDebugger()->AddBufferUsage( buf->ToGlobal(), state );
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

		if ( _fgThread.GetDebugger() )
			_fgThread.GetDebugger()->AddRTGeometryUsage( geom->ToGlobal(), RTGeometryState{ state, _currTask });
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

		if ( _fgThread.GetDebugger() )
			_fgThread.GetDebugger()->AddRTSceneUsage( scene->ToGlobal(), RTSceneState{ state, _currTask });
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
			res.second( res.first, _fgThread.GetBarrierManager(), _fgThread.GetDebugger() );
		}

		_pendingResourceBarriers.clear();

		_fgThread.GetBarrierManager().Commit( _fgThread.GetDevice(), _cmdBuffer );
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

			vkCmdBindIndexBuffer( _cmdBuffer, _indexBuffer, _indexBufferOffset, _indexType );
			Stat().indexBufferBindings++;
		}
	}
	

}	// FG
