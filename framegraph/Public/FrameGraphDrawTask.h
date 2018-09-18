// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/PipelineResources.h"
#include "framegraph/Public/LowLevel/RenderState.h"
#include "framegraph/Public/LowLevel/VertexInputState.h"

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
			BufferPtr	buffer;
			BytesU		offset;
		};

		struct Buffers_t final : public FixedMap< VertexBufferID, Buffer, FG_MaxVertexBuffers >
		{
			Buffers_t&  Add (const VertexBufferID &id, const BufferPtr &vb, BytesU offset)
			{
				this->insert( Pair<VertexBufferID, Buffer>{ id, Buffer{vb, offset} });
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
		PipelinePtr				pipeline;
		PipelineResourceSet		resources;

		RenderState				renderState;
		EPipelineDynamicState	dynamicStates	= EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;

		VertexInputState		vertexInput;
		Buffers_t				vertexBuffers;
		
		DrawCmd					drawCmd;
		Scissors_t				scissors;


	// methods
		DrawTask () :
			BaseDrawTask<DrawTask>{ "Draw", HtmlColor::Bisque() } {}

		DrawTask&  SetPipeline (const PipelinePtr &ppln)							{ pipeline = ppln;  return *this; }
		DrawTask&  SetVertices (uint first, uint count)								{ drawCmd.firstVertex = first;  drawCmd.vertexCount = count;  return *this; }
		DrawTask&  SetInstances (uint first, uint count)							{ drawCmd.firstInstance = first;  drawCmd.instanceCount = count;  return *this; }
		DrawTask&  SetRenderState (const RenderState &rs)							{ renderState = rs;  return *this; }
		DrawTask&  SetVertexInput (const VertexInputState &value)					{ vertexInput = value;  return *this; }
		DrawTask&  SetDynamicStates (EPipelineDynamicState value)					{ dynamicStates = value;  return *this; }

		DrawTask&  AddScissor (const RectI &rect)									{ ASSERT( rect.IsValid() );  scissors.push_back( rect );  return *this; }
		DrawTask&  AddScissor (const RectU &rect)									{ ASSERT( rect.IsValid() );  scissors.push_back( RectI{rect} );  return *this; }

		DrawTask&  AddResources (const DescriptorSetID &id, const PipelineResourcesPtr &res)		{ resources.insert({ id, res });  return *this; }

		DrawTask&  AddBuffer (const VertexBufferID &id, const BufferPtr &vb, BytesU offset = 0_b)	{ vertexBuffers.Add( id, vb, offset );  return *this; }
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
		PipelinePtr				pipeline;
		PipelineResourceSet		resources;
		
		RenderState				renderState;
		EPipelineDynamicState	dynamicStates	= EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;

		VertexInputState		vertexInput;
		Buffers_t				vertexBuffers;
		
		BufferPtr				indexBuffer;
		BytesU					indexBufferOffset;
		EIndex					indexType		= Default;
		
		DrawCmd					drawCmd;
		Scissors_t				scissors;


	// methods
		DrawIndexedTask () :
			BaseDrawTask<DrawIndexedTask>{ "DrawIndexed", HtmlColor::Bisque() } {}

		DrawIndexedTask&  SetPipeline (const PipelinePtr &ppln)								{ pipeline = ppln;  return *this; }
		DrawIndexedTask&  SetIndices (uint first, uint count)								{ drawCmd.firstIndex = first;  drawCmd.indexCount = count;  return *this; }
		DrawIndexedTask&  SetInstances (uint first, uint count)								{ drawCmd.firstInstance = first;  drawCmd.instanceCount = count;  return *this; }
		DrawIndexedTask&  SetIndexBuffer (const BufferPtr &ib, BytesU off, EIndex type)		{ indexBuffer = ib;  indexBufferOffset = off;  indexType = type;  return *this; }
		DrawIndexedTask&  SetRenderState (const RenderState &rs)							{ renderState = rs;  return *this; }
		
		DrawIndexedTask&  AddScissor (const RectI &rect)									{ ASSERT( rect.IsValid() );  scissors.push_back( rect );  return *this; }
		DrawIndexedTask&  AddScissor (const RectU &rect)									{ ASSERT( rect.IsValid() );  scissors.push_back( RectI{rect} );  return *this; }

		DrawIndexedTask&  AddBuffer (const VertexBufferID &id, const BufferPtr &vb, BytesU offset = 0_b)	{ vertexBuffers.Add( id, vb, offset );  return *this; }
	};



	//
	// Clear Attachments
	//


	//
	// Draw External Command Buffer
	//
	

	//
	// Render Pass Description
	//
	struct RenderPassDesc
	{
	// types
		using ClearValue_t	= Union< RGBA32f, RGBA32u, RGBA32i, DepthStencil >;
		
		struct RT
		{
			ImagePtr			image;		// may be image module in initial state (created by CreateRenderTarget or other)
			ImageViewDesc		desc;		// may be used to specialize level, layer, different format, layout, ...
			ClearValue_t		clearValue;	// default is black color
			EAttachmentLoadOp	loadOp		= EAttachmentLoadOp::Load;
			EAttachmentStoreOp	storeOp		= EAttachmentStoreOp::Store;

			RT () {}
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


		RenderPassDesc&  AddTarget (const RenderTargetID &id, const ImagePtr &image)
		{
			return AddTarget( id, image, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store );
		}

		RenderPassDesc&  AddTarget (const RenderTargetID &id, const ImagePtr &image,
									EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp)
		{
			return AddTarget( id, image, ImageViewDesc(image->Description()), loadOp, storeOp );
		}
		
		template <typename ClearVal>
		RenderPassDesc&  AddTarget (const RenderTargetID &id, const ImagePtr &image,
									const ClearVal &clearValue, EAttachmentStoreOp storeOp)
		{
			return AddTarget( id, image, ImageViewDesc(image->Description()), clearValue, storeOp );
		}
		
		RenderPassDesc&  AddTarget (const RenderTargetID &id, const ImagePtr &image, const ImageViewDesc &desc,
									EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp)
		{
			ASSERT( image );
			ASSERT( loadOp != EAttachmentLoadOp::Clear );	// clear value is not defined

            RT  rt;
            rt.image        = image;
            rt.desc		    = desc;
            rt.loadOp       = loadOp;
            rt.storeOp      = storeOp;

            renderTargets.insert({ id, std::move(rt) });
			return *this;
		}

		template <typename ClearVal>
		RenderPassDesc&  AddTarget (const RenderTargetID &id, const ImagePtr &image, const ImageViewDesc &desc,
									const ClearVal &clearValue, EAttachmentStoreOp storeOp)
		{
			ASSERT( image );

            RT  rt;
            rt.image        = image;
            rt.desc		    = desc;
            rt.clearValue   = clearValue;
            rt.loadOp       = EAttachmentLoadOp::Clear;
            rt.storeOp      = storeOp;

            renderTargets.insert({ id, std::move(rt) });
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
