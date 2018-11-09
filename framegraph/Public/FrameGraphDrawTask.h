// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/PipelineResources.h"
#include "framegraph/Public/RenderState.h"
#include "framegraph/Public/VertexInputState.h"

namespace FG
{
	namespace _fg_hidden_
	{
		struct _BaseDrawTask {};


		//
		// Base Draw Task
		//
		template <typename BaseType>
		struct BaseDrawTask : _BaseDrawTask
		{
		// types
			using TaskName_t = StaticString<64>;

		// variables
			TaskName_t		taskName;
			RGBA8u			debugColor;
			
		// methods
			BaseDrawTask () {}
			BaseDrawTask (StringView name, RGBA8u color) : taskName{name}, debugColor{color} {}

			BaseType& SetName (StringView name)			{ taskName = name;  return static_cast<BaseType &>( *this ); }
			BaseType& SetDebugColor (RGBA8u color)		{ debugColor = color;  return static_cast<BaseType &>( *this ); }
		};

	}	// _fg_hidden_



	//
	// Draw
	//
	struct DrawTask final : _fg_hidden_::BaseDrawTask<DrawTask>
	{
	// types
		struct Buffer
		{
			RawBufferID		buffer;
			BytesU			offset;
		};

		struct Buffers_t final : public FixedMap< VertexBufferID, Buffer, FG_MaxVertexBuffers >
		{
			Buffers_t&  Add (const VertexBufferID &id, RawBufferID vb, BytesU offset)
			{
				this->insert_or_assign( id, Buffer{vb, offset} );
				return *this;
			}
		};

		using Scissors_t = FixedArray< RectI, FG_MaxViewports >;

		struct DrawCmd
		{
			uint		vertexCount		= 0;
			uint		instanceCount	= 1;
			uint		firstVertex		= 0;
			uint		firstInstance	= 0;
		};


	// variables
		RawGPipelineID			pipeline;
		PipelineResourceSet		resources;

		RenderState				renderState;
		EPipelineDynamicState	dynamicStates	= EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;

		VertexInputState		vertexInput;
		Buffers_t				vertexBuffers;
		
		DrawCmd					drawCmd;
		Scissors_t				scissors;


	// methods
		DrawTask () :
			BaseDrawTask<DrawTask>{ "Draw", HtmlColor::Bisque } {}

		DrawTask&  SetPipeline (const GPipelineID &ppln)							{ pipeline = ppln.Get();  return *this; }
		DrawTask&  SetVertices (uint first, uint count)								{ drawCmd.firstVertex = first;  drawCmd.vertexCount = count;  return *this; }
		DrawTask&  SetInstances (uint first, uint count)							{ drawCmd.firstInstance = first;  drawCmd.instanceCount = count;  return *this; }
		DrawTask&  SetRenderState (const RenderState &rs)							{ renderState = rs;  return *this; }
		DrawTask&  SetVertexInput (const VertexInputState &value)					{ vertexInput = value;  return *this; }
		DrawTask&  SetDynamicStates (EPipelineDynamicState value)					{ dynamicStates = value;  return *this; }

		DrawTask&  AddScissor (const RectI &rect)									{ ASSERT( rect.IsValid() );  scissors.push_back( rect );  return *this; }
		DrawTask&  AddScissor (const RectU &rect)									{ ASSERT( rect.IsValid() );  scissors.push_back( RectI{rect} );  return *this; }

		DrawTask&  AddResources (const DescriptorSetID &id, const PipelineResources *res)	{ resources.insert_or_assign( id, res );  return *this; }

		DrawTask&  AddBuffer (const VertexBufferID &id, const BufferID &vb, BytesU offset = 0_b){ vertexBuffers.Add( id, vb.Get(), offset );  return *this; }
	};



	//
	// Draw Indexed
	//
	struct DrawIndexedTask final : _fg_hidden_::BaseDrawTask<DrawIndexedTask>
	{
	// types
		using Buffers_t		= DrawTask::Buffers_t;
		using Scissors_t	= DrawTask::Scissors_t;

		struct DrawCmd
		{
			uint		indexCount		= 0;
			uint		instanceCount	= 1;
			uint		firstIndex		= 0;
			int			vertexOffset	= 0;
			uint		firstInstance	= 0;
		};


	// variables
		RawGPipelineID			pipeline;
		PipelineResourceSet		resources;
		
		RenderState				renderState;
		EPipelineDynamicState	dynamicStates	= EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;

		VertexInputState		vertexInput;
		Buffers_t				vertexBuffers;
		
		RawBufferID				indexBuffer;
		BytesU					indexBufferOffset;
		EIndex					indexType		= Default;
		
		DrawCmd					drawCmd;
		Scissors_t				scissors;


	// methods
		DrawIndexedTask () :
			BaseDrawTask<DrawIndexedTask>{ "DrawIndexed", HtmlColor::Bisque } {}

		DrawIndexedTask&  SetPipeline (const GPipelineID &ppln)								{ pipeline = ppln.Get();  return *this; }
		DrawIndexedTask&  SetIndices (uint first, uint count)								{ drawCmd.firstIndex = first;  drawCmd.indexCount = count;  return *this; }
		DrawIndexedTask&  SetInstances (uint first, uint count)								{ drawCmd.firstInstance = first;  drawCmd.instanceCount = count;  return *this; }
		DrawIndexedTask&  SetIndexBuffer (const BufferID &ib, BytesU off, EIndex type)		{ indexBuffer = ib.Get();  indexBufferOffset = off;  indexType = type;  return *this; }
		DrawIndexedTask&  SetRenderState (const RenderState &rs)							{ renderState = rs;  return *this; }
		
		DrawIndexedTask&  AddScissor (const RectI &rect)									{ ASSERT( rect.IsValid() );  scissors.push_back( rect );  return *this; }
		DrawIndexedTask&  AddScissor (const RectU &rect)									{ ASSERT( rect.IsValid() );  scissors.push_back( RectI{rect} );  return *this; }

		DrawIndexedTask&  AddBuffer (const VertexBufferID &id, const BufferID &vb, BytesU offset = 0_b)	{ vertexBuffers.Add( id, vb.Get(), offset );  return *this; }
	};



	//
	// Clear Attachments
	//


	//
	// Draw External Command Buffer
	//


	//
	// Draw Mesh Task
	//
	struct DrawMeshTask final : _fg_hidden_::BaseDrawTask<DrawMeshTask>
	{
	// types
		using Scissors_t	= DrawTask::Scissors_t;
		
		struct DrawCmd
		{
			uint		count	= 0;
			uint		first	= 0;
		};


	// variables
		RawMPipelineID			pipeline;
		PipelineResourceSet		resources;
		
		RenderState				renderState;
		EPipelineDynamicState	dynamicStates	= EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;
		
		DrawCmd					drawCmd;
		Scissors_t				scissors;


	// methods
		DrawMeshTask () :
			BaseDrawTask<DrawMeshTask>{ "DrawMeshTask", HtmlColor::Bisque } {}

		DrawMeshTask&  SetPipeline (const MPipelineID &ppln)			{ pipeline = ppln.Get();  return *this; }
		DrawMeshTask&  SetCommand (uint first, uint count)				{ drawCmd.first = first;  drawCmd.count = count;  return *this; }
		DrawMeshTask&  SetRenderState (const RenderState &rs)			{ renderState = rs;  return *this; }
		
		DrawMeshTask&  AddScissor (const RectI &rect)					{ ASSERT( rect.IsValid() );  scissors.push_back( rect );  return *this; }
		DrawMeshTask&  AddScissor (const RectU &rect)					{ ASSERT( rect.IsValid() );  scissors.push_back( RectI{rect} );  return *this; }
	};
	

	//
	// Render Pass Description
	//
	struct RenderPassDesc
	{
	// types
		using ClearValue_t	= Union< std::monostate, RGBA32f, RGBA32u, RGBA32i, DepthStencil >;
		
		struct RT
		{
			RawImageID					image;		// may be image module in initial state (created by CreateRenderTarget or other)
			Optional< ImageViewDesc >	desc;		// may be used to specialize level, layer, different format, layout, ...
			ClearValue_t				clearValue;	// default is black color
			EAttachmentLoadOp			loadOp		= EAttachmentLoadOp::Load;
			EAttachmentStoreOp			storeOp		= EAttachmentStoreOp::Store;
		};
		using Targets_t	= FixedMap< RenderTargetID, RT, FG_MaxColorBuffers+1 >;

		struct Viewport
		{
			RectF	rect;
			float	minDepth	= 0.0f;
			float	maxDepth	= 1.0f;
		};
		using Viewports_t = FixedArray< Viewport, FG_MaxViewports >;


	// variables
		Targets_t				renderTargets;
		Viewports_t				viewports;
		RectI					area;
		bool					parallelExecution	= true;		// (optimization) if 'false' all draw and compute tasks will be executed in initial order
		bool					canBeMerged			= true;		// (optimization) g-buffer render passes can be merged, but don't merge conditional passes
		// TODO: push constants, specialization constants


	// methods
		explicit RenderPassDesc (const RectI &area) : area{area}
		{
			ASSERT( area.IsValid() );
		}

		explicit RenderPassDesc (const int2 &size) : RenderPassDesc{ RectI{int2(), size} }
		{}


		RenderPassDesc&  AddTarget (const RenderTargetID &id, const ImageID &image)
		{
			return AddTarget( id, image, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store );
		}

		RenderPassDesc&  AddTarget (const RenderTargetID &id, const ImageID &image, EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp)
		{
			ASSERT( loadOp != EAttachmentLoadOp::Clear );	// clear value is not defined
			renderTargets.insert_or_assign( id, RT{image.Get(), {}, ClearValue_t{}, loadOp, storeOp} );
			return *this;
		}
		
		template <typename ClearVal>
		RenderPassDesc&  AddTarget (const RenderTargetID &id, const ImageID &image, const ClearVal &clearValue, EAttachmentStoreOp storeOp)
		{
			renderTargets.insert_or_assign( id, RT{image.Get(), {}, clearValue, EAttachmentLoadOp::Clear, storeOp} );
			return *this;
		}
		
		RenderPassDesc&  AddTarget (const RenderTargetID &id, const ImageID &image, const ImageViewDesc &desc, EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp)
		{
			ASSERT( loadOp != EAttachmentLoadOp::Clear );	// clear value is not defined
			renderTargets.insert_or_assign( id, RT{image.Get(), desc, ClearValue_t{}, loadOp, storeOp} );
			return *this;
		}

		template <typename ClearVal>
		RenderPassDesc&  AddTarget (const RenderTargetID &id, const ImageID &image, const ImageViewDesc &desc, const ClearVal &clearValue, EAttachmentStoreOp storeOp)
		{
			renderTargets.insert_or_assign( id, RT{image.Get(), desc, clearValue, EAttachmentLoadOp::Clear, storeOp} );
			return *this;
		}


		template <typename T>
		RenderPassDesc&  AddViewport (const Rectangle<T> &rect, float minDepth = 0.0f, float maxDepth = 1.0f)
		{
			ASSERT( rect.IsValid() );
			viewports.push_back({ RectF{rect}, minDepth, maxDepth });
			return *this;
		}

		template <typename T>
		RenderPassDesc&  AddViewport (const Vec<T,2> &size, float minDepth = 0.0f, float maxDepth = 1.0f)
		{
			viewports.push_back({ RectF{float2(), float2(size)}, minDepth, maxDepth });
			return *this;
		}
	};

}	// FG
