// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/PipelineResources.h"
#include "framegraph/Public/RenderState.h"
#include "framegraph/Public/VertexInputState.h"

namespace FG
{
namespace _fg_hidden_
{

	//
	// Base Draw Task
	//
	template <typename TaskType>
	struct BaseDrawTask
	{
	// types
		using TaskName_t = StaticString<64>;

	// variables
		TaskName_t		taskName;
		RGBA8u			debugColor;
			
	// methods
		BaseDrawTask () {}
		BaseDrawTask (StringView name, RGBA8u color) : taskName{name}, debugColor{color} {}

		TaskType& SetName (StringView name)			{ taskName = name;  return static_cast<TaskType &>( *this ); }
		TaskType& SetDebugColor (RGBA8u color)		{ debugColor = color;  return static_cast<TaskType &>( *this ); }
	};
	
	
	struct VertexBuffer
	{
		RawBufferID		buffer;
		BytesU			offset;
	};


	struct PushConstantData
	{
		PushConstantID		id;
		uint8_t				data[ FG_MaxPushConstantsSize ];
	};
	using PushConstants_t	= FixedArray< PushConstantData, 4 >;
	

	struct StencilState
	{
		using Value_t = uint8_t;

		Vec<Value_t, 2>		reference, writeMask, compareMask;
	};
	
	using ColorBuffers_t	= FixedMap< RenderTargetID, RenderState::ColorBuffer, 4 >;
	using Scissors_t		= FixedArray< RectI, FG_MaxViewports >;


	//
	// Base Draw Call
	//
	template <typename TaskType>
	struct BaseDrawCall : BaseDrawTask<TaskType>
	{
	// types
		using StencilValue_t	= StencilState::Value_t;


	// variables
		PipelineResourceSet		resources;
		PushConstants_t			pushConstants;
		
		EPipelineDynamicState	dynamicStates	= EPipelineDynamicState::Viewport;
		Scissors_t				scissors;
		ColorBuffers_t			colorBuffers;
		StencilState			stencilState;
			

	// methods
		BaseDrawCall () : BaseDrawTask<TaskType>{} {}
		BaseDrawCall (StringView name, RGBA8u color) : BaseDrawTask<TaskType>{ name, color } {}

		TaskType&  AddResources (uint bindingIndex, const PipelineResources *res);

		TaskType&  AddScissor (const RectI &rect);
		TaskType&  AddScissor (const RectU &rect);
			
		TaskType&  AddColorBuffer (const RenderTargetID &id, EBlendFactor srcBlendFactor, EBlendFactor dstBlendFactor, EBlendOp blendOp, bool4 colorMask);
		TaskType&  AddColorBuffer (const RenderTargetID &id, EBlendFactor srcBlendFactorColor, EBlendFactor srcBlendFactorAlpha,
									EBlendFactor dstBlendFactorColor, EBlendFactor dstBlendFactorAlpha,
									EBlendOp blendOpColor, EBlendOp blendOpAlpha, bool4 colorMask);
		TaskType&  AddColorBuffer (const RenderTargetID &id, bool4 colorMask);

		TaskType&  SetStencilRef (uint front, uint back);
		TaskType&  SetStencilRef (uint value)						{ return SetStencilRef( value, value ); }

		TaskType&  SetStencilCompareMask (uint front, uint back);
		TaskType&  SetStencilCompareMask (uint value)				{ return SetStencilCompareMask( value, value ); }

		TaskType&  SetStencilWriteMask (uint front, uint back);
		TaskType&  SetStencilWriteMask (uint value)					{ return SetStencilWriteMask( value, value ); }

		template <typename ValueType>
		TaskType&  AddPushConstant (const PushConstantID &id, const ValueType &value)	{ return AddPushConstant( id, AddressOf(value), SizeOf<ValueType> ); }
		TaskType&  AddPushConstant (const PushConstantID &id, const void *ptr, BytesU size);
	};
	

	//
	// Base Draw Vertices
	//
	template <typename TaskType>
	struct BaseDrawVertices : BaseDrawCall<TaskType>
	{
	// types
		using Buffers_t = FixedMap< VertexBufferID, VertexBuffer, FG_MaxVertexBuffers >;


	// variables
		RawGPipelineID			pipeline;
		
		VertexInputState		vertexInput;
		Buffers_t				vertexBuffers;

		EPrimitive				topology			= Default;
		bool					primitiveRestart	= false;	// if 'true' then index with -1 value will restarting the assembly of primitives


	// methods
		BaseDrawVertices () : BaseDrawCall<TaskType>{} {}
		BaseDrawVertices (StringView name, RGBA8u color) : BaseDrawCall<TaskType>{ name, color } {}

		TaskType&  SetTopology (EPrimitive value)					{ topology = value;  return static_cast<TaskType &>( *this ); }
		TaskType&  SetPipeline (const GPipelineID &ppln)			{ ASSERT( ppln );  pipeline = ppln.Get();  return static_cast<TaskType &>( *this ); }

		TaskType&  SetVertexInput (const VertexInputState &value)	{ vertexInput = value;  return static_cast<TaskType &>( *this ); }
		TaskType&  SetPrimitiveRestartEnabled (bool value)			{ primitiveRestart = value;  return static_cast<TaskType &>( *this ); }

		TaskType&  AddBuffer (const VertexBufferID &id, const BufferID &vb, BytesU offset = 0_b);
	};

}	// _fg_hidden_



	//
	// Draw Vertices
	//
	struct DrawVertices final : _fg_hidden_::BaseDrawVertices<DrawVertices>
	{
	// types
		struct DrawCmd
		{
			uint		vertexCount		= 0;
			uint		instanceCount	= 1;
			uint		firstVertex		= 0;
			uint		firstInstance	= 0;
		};
		using DrawCommands_t	= FixedArray< DrawCmd, 16 >;


	// variables
		DrawCommands_t			commands;


	// methods
		DrawVertices () :
			BaseDrawVertices<DrawVertices>{ "DrawVertices", HtmlColor::Bisque } {}

		DrawVertices&  AddDrawCmd (uint vertexCount, uint instanceCount	= 1, uint firstVertex = 0, uint firstInstance = 0)
		{
			ASSERT( vertexCount > 0 );
			commands.emplace_back( vertexCount, instanceCount, firstVertex, firstInstance );
			return *this;
		}
	};



	//
	// Draw Indexed Vertices
	//
	struct DrawIndexed final : _fg_hidden_::BaseDrawVertices<DrawIndexed>
	{
	// types
		struct DrawCmd
		{
			uint		indexCount		= 0;
			uint		instanceCount	= 1;
			uint		firstIndex		= 0;
			int			vertexOffset	= 0;
			uint		firstInstance	= 0;
		};
		using DrawCommands_t	= FixedArray< DrawCmd, 16 >;


	// variables
		RawBufferID				indexBuffer;
		BytesU					indexBufferOffset;
		EIndex					indexType		= Default;
		
		DrawCommands_t			commands;


	// methods
		DrawIndexed () :
			BaseDrawVertices<DrawIndexed>{ "DrawIndexed", HtmlColor::Bisque } {}

		DrawIndexed&  SetIndexBuffer (const BufferID &ib, BytesU off, EIndex type)
		{
			ASSERT( ib );
			indexBuffer			= ib.Get();
			indexBufferOffset	= off;
			indexType			= type;
			return *this;
		}

		DrawIndexed&  AddDrawCmd (uint indexCount, uint instanceCount = 1, uint firstIndex = 0, int vertexOffset = 0, uint firstInstance = 0)
		{
			ASSERT( indexCount > 0 );
			commands.emplace_back( indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
			return *this;
		}
	};



	//
	// Draw Vertices indirect
	//
	struct DrawVerticesIndirect final : _fg_hidden_::BaseDrawVertices<DrawVerticesIndirect>
	{
	// types
		struct DrawCmd
		{
			BytesU			indirectBufferOffset;
			uint			drawCount;
			Bytes<uint>		stride;
		};
		using DrawCommands_t	= FixedArray< DrawCmd, 16 >;
		
		struct DrawIndirectCommand
		{
			uint	vertexCount;
			uint	instanceCount;
			uint	firstVertex;
			uint	firstInstance;
		};


	// variables
		DrawCommands_t			commands;
		RawBufferID				indirectBuffer;		// contains array of 'DrawIndirectCommand'


	// methods
		DrawVerticesIndirect () :
			BaseDrawVertices<DrawVerticesIndirect>{ "DrawVerticesIndirect", HtmlColor::Bisque } {}

		DrawVerticesIndirect&  SetIndirectBuffer (const BufferID &buffer)
		{
			ASSERT( buffer );
			indirectBuffer = buffer.Get();
			return *this;
		}

		DrawVerticesIndirect&  AddDrawCmd (uint drawCount, BytesU indirectBufferOffset = 0_b, BytesU stride = SizeOf<DrawIndirectCommand>)
		{
			ASSERT( drawCount > 0 );
			commands.emplace_back( indirectBufferOffset, drawCount, Bytes<uint>{stride} );
			return *this;
		}
	};



	//
	// Draw Indexed Vertices Indirect
	//
	struct DrawIndexedIndirect final : _fg_hidden_::BaseDrawVertices<DrawIndexedIndirect>
	{
	// types
		using DrawCommands_t = DrawVerticesIndirect::DrawCommands_t;
		
		struct DrawIndexedIndirectCommand
		{
			uint	indexCount;
			uint	instanceCount;
			uint	firstIndex;
			int		vertexOffset;
			uint	firstInstance;
		};


	// variables
		RawBufferID				indexBuffer;
		BytesU					indexBufferOffset;
		EIndex					indexType		= Default;

		DrawCommands_t			commands;

		RawBufferID				indirectBuffer;		// contains array of 'DrawIndexedIndirectCommand'


	// methods
		DrawIndexedIndirect () :
			BaseDrawVertices<DrawIndexedIndirect>{ "DrawIndexedIndirect", HtmlColor::Bisque } {}
		
		DrawIndexedIndirect&  SetIndexBuffer (const BufferID &ib, BytesU off, EIndex type)
		{
			ASSERT( ib );
			indexBuffer			= ib.Get();
			indexBufferOffset	= off;
			indexType			= type;
			return *this;
		}

		DrawIndexedIndirect&  SetIndirectBuffer (const BufferID &buffer)
		{
			ASSERT( buffer );
			indirectBuffer = buffer.Get();
			return *this;
		}

		DrawIndexedIndirect&  AddDrawCmd (uint drawCount, BytesU indirectBufferOffset = 0_b, BytesU stride = SizeOf<DrawIndexedIndirectCommand>)
		{
			ASSERT( drawCount > 0 );
			commands.emplace_back( indirectBufferOffset, drawCount, Bytes<uint>{stride} );
			return *this;
		}
	};



	//
	// Draw Meshes
	//
	struct DrawMeshes final : _fg_hidden_::BaseDrawCall<DrawMeshes>
	{
	// types
		struct DrawCmd
		{
			uint		count	= 0;
			uint		first	= 0;
		};
		using DrawCommands_t	= FixedArray< DrawCmd, 16 >;


	// variables
		RawMPipelineID			pipeline;
		DrawCommands_t			commands;


	// methods
		DrawMeshes () :
			BaseDrawCall<DrawMeshes>{ "DrawMeshes", HtmlColor::Bisque } {}

		DrawMeshes&  SetPipeline (const MPipelineID &ppln)			{ ASSERT( ppln );  pipeline = ppln.Get();  return *this; }

		DrawMeshes&  AddDrawCmd (uint count, uint first = 0)		{ ASSERT( count > 0 );  commands.push_back({ count, first });  return *this; }
	};



	//
	// Draw Meshes Indirect
	//
	struct DrawMeshesIndirect final : _fg_hidden_::BaseDrawCall<DrawMeshesIndirect>
	{
	// types
		using DrawCommands_t = DrawVerticesIndirect::DrawCommands_t;
		
		struct DrawMeshTasksIndirectCommand
		{
			uint		taskCount;
			uint		firstTask;
		};


	// variables
		RawMPipelineID			pipeline;
		DrawCommands_t			commands;
		RawBufferID				indirectBuffer;		// contains array of 'DrawMeshTasksIndirectCommand'


	// methods
		DrawMeshesIndirect () :
			BaseDrawCall<DrawMeshesIndirect>{ "DrawMeshesIndirect", HtmlColor::Bisque } {}

		DrawMeshesIndirect&  SetPipeline (const MPipelineID &ppln)
		{
			ASSERT( ppln );
			pipeline = ppln.Get();
			return *this;
		}
		
		DrawMeshesIndirect&  SetIndirectBuffer (const BufferID &buffer)
		{
			ASSERT( buffer );
			indirectBuffer = buffer.Get();
			return *this;
		}

		DrawMeshesIndirect&  AddDrawCmd (uint drawCount, BytesU indirectBufferOffset = 0_b, BytesU stride = SizeOf<DrawMeshTasksIndirectCommand>)
		{
			ASSERT( drawCount > 0 );
			commands.emplace_back( indirectBufferOffset, drawCount, Bytes<uint>{stride} );
			return *this;
		}
	};
	


	//
	// Clear Attachments
	//
	struct ClearAttachments final : _fg_hidden_::BaseDrawTask<ClearAttachments>
	{
	// methods
		ClearAttachments () :
			BaseDrawTask<ClearAttachments>{ "ClearAttachments", HtmlColor::Bisque } {}
	};


	
namespace _fg_hidden_
{
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::AddResources (uint bindingIndex, const PipelineResources *res)
	{
		ASSERT( bindingIndex < resources.size() );
		resources[bindingIndex] = res;
		return static_cast<TaskType &>( *this );
	}

	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::AddScissor (const RectI &rect)
	{
		ASSERT( rect.IsValid() );
		scissors.push_back( rect );
		dynamicStates |= EPipelineDynamicState::Scissor;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::AddScissor (const RectU &rect)
	{
		ASSERT( rect.IsValid() );
		scissors.push_back( RectI{rect} );
		dynamicStates |= EPipelineDynamicState::Scissor;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::AddColorBuffer (const RenderTargetID &id, EBlendFactor srcBlendFactor, EBlendFactor dstBlendFactor, EBlendOp blendOp, bool4 colorMask)
	{
		return AddColorBuffer( id, srcBlendFactor, srcBlendFactor, dstBlendFactor, dstBlendFactor, blendOp, blendOp, colorMask );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::AddColorBuffer (const RenderTargetID &id, EBlendFactor srcBlendFactorColor, EBlendFactor srcBlendFactorAlpha,
																EBlendFactor dstBlendFactorColor, EBlendFactor dstBlendFactorAlpha,
																EBlendOp blendOpColor, EBlendOp blendOpAlpha, bool4 colorMask)
	{
		ASSERT( id );

		RenderState::ColorBuffer	cb;
		cb.blend			= true;
		cb.srcBlendFactor	= { srcBlendFactorColor, srcBlendFactorAlpha };
		cb.dstBlendFactor	= { dstBlendFactorColor, dstBlendFactorAlpha };
		cb.blendOp			= { blendOpColor, blendOpAlpha };
		cb.colorMask		= colorMask;

		colorBuffers.insert({ id, std::move(cb) });
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::AddColorBuffer (const RenderTargetID &id, bool4 colorMask)
	{
		ASSERT( id );

		RenderState::ColorBuffer	cb;
		cb.colorMask = colorMask;
		
		colorBuffers.insert({ id, std::move(cb) });
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetStencilRef (uint front, uint back)
	{
		stencilState.reference.x = StencilValue_t(front);
		stencilState.reference.y = StencilValue_t(back);
		dynamicStates |= EPipelineDynamicState::StencilReference;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetStencilCompareMask (uint front, uint back)
	{
		stencilState.compareMask.x = StencilValue_t(front);
		stencilState.compareMask.y = StencilValue_t(back);
		dynamicStates |= EPipelineDynamicState::StencilCompareMask;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetStencilWriteMask (uint front, uint back)
	{
		stencilState.writeMask.x = StencilValue_t(front);
		stencilState.writeMask.y = StencilValue_t(back);
		dynamicStates |= EPipelineDynamicState::StencilWriteMask;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::AddPushConstant (const PushConstantID &id, const void *ptr, BytesU size)
	{
		ASSERT( id.IsDefined() );
		pushConstants.emplace_back( id );
		MemCopy( pushConstants.back().data, BytesU::SizeOf(pushConstants.back().data), ptr, size );
		return static_cast<TaskType &>( *this );
	}
//-----------------------------------------------------------------------------

	
	template <typename TaskType>
	inline TaskType&  BaseDrawVertices<TaskType>::AddBuffer (const VertexBufferID &id, const BufferID &vb, BytesU offset)
	{
		//ASSERT( id.IsDefined() );	// one buffer may be unnamed
		ASSERT( vb );
		vertexBuffers.insert_or_assign( id, _fg_hidden_::VertexBuffer{vb.Get(), offset} );
		return static_cast<TaskType &>( *this );
	}

}	// _fg_hidden_
}	// FG
