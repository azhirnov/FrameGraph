// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/PipelineResources.h"
#include "framegraph/Public/RenderState.h"
#include "framegraph/Public/VertexInputState.h"

namespace FG
{
namespace _fg_hidden_
{
	using TaskName_t = StaticString<64>;


	//
	// Base Draw Task
	//
	template <typename TaskType>
	struct BaseDrawTask
	{
	// variables
		TaskName_t		taskName;
		RGBA8u			debugColor;
			
	// methods
		BaseDrawTask () {}
		BaseDrawTask (StringView name, RGBA8u color) : taskName{name}, debugColor{color} {}

		TaskType& SetName (StringView name)			{ taskName = name;  return static_cast<TaskType &>( *this ); }
		TaskType& SetDebugColor (RGBA8u color)		{ debugColor = color;  return static_cast<TaskType &>( *this ); }
	};


	//
	// Graphics Shader Debug Mode
	//
	struct GraphicsShaderDebugMode
	{
	// types
		struct Fragment {
			uint	pixelCoordX;	// gl_FragCoord.x
			uint	pixelCoordY;	// gl_FragCoord.y
			uint	sample;			// gl_SampleID
			uint	primitive;		// gl_PrimitiveID
			// TODO: depth range
		};
		struct Vertex {
			uint	vertex;			// gl_VertexIndex
			uint	instance;		// gl_InstanceIndex
			uint	draw;			// gl_DrawID - for indirect drawing
		};
		struct TessControl {
			uint	invocatioon;	// gl_InvocationID
			uint	primitive;		// gl_PrimitiveID
		};
		struct TessEvaluation {
			uint	primitive;		// gl_PrimitiveID
		};
		struct Geometry {
			uint	invocatioon;	// gl_InvocationID
			uint	primitive;		// gl_PrimitiveIDIn
		};
		struct MeshTask {
			uint	invocation;		// gl_GlobalInvocationID.x
		};
		struct Mesh {
			uint	invocation;		// gl_GlobalInvocationID.x
		};

	// variables
		EShaderDebugMode	mode	= Default;
		EShader				shader	= Default;
		union {
			Vertex			vert;
			TessControl		tessCont;
			TessEvaluation	tessEval;
			Geometry		geom;
			MeshTask		task;
			Mesh			mesh;
			Fragment		frag;
		};
	};
	
	
	//
	// Render States
	//
	struct VertexBuffer
	{
		RawBufferID			buffer;
		BytesU				offset;
	};


	struct PushConstantData
	{
		PushConstantID		id;
		Bytes<uint16_t>		size;
		uint8_t				data[ FG_MaxPushConstantsSize ];
	};
	using PushConstants_t	= FixedArray< PushConstantData, 4 >;
	

	struct DynamicStates
	{
		EStencilOp		stencilFailOp;					// stencil test failed
		EStencilOp		stencilDepthFailOp;				// depth and stencil tests are passed
		EStencilOp		stencilPassOp;					// stencil test passed and depth test failed
		uint8_t			stencilReference;				// front & back
		uint8_t			stencilWriteMask;				// front & back
		uint8_t			stencilCompareMask;				// front & back

		ECullMode		cullMode;

		ECompareOp		depthCompareOp;
		bool			depthTest				: 1;
		bool			depthWrite				: 1;
		bool			stencilTest				: 1;	// enable stencil test

		bool			rasterizerDiscard		: 1;
		bool			frontFaceCCW			: 1;

		bool			hasStencilTest			: 1;
		bool			hasStencilFailOp		: 1;
		bool			hasStencilDepthFailOp	: 1;
		bool			hasStencilPassOp		: 1;
		bool			hasStencilReference		: 1;
		bool			hasStencilWriteMask		: 1;
		bool			hasStencilCompareMask	: 1;
		bool			hasDepthCompareOp		: 1;
		bool			hasDepthTest			: 1;
		bool			hasDepthWrite			: 1;
		bool			hasCullMode				: 1;
		bool			hasRasterizedDiscard	: 1;
		bool			hasFrontFaceCCW			: 1;

		DynamicStates ()
		{
			memset( this, 0, sizeof(*this) );
		}
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
		using StencilValue_t	= decltype(DynamicStates::stencilReference);
		using DebugMode			= GraphicsShaderDebugMode;


	// variables
		PipelineResourceSet		resources;
		PushConstants_t			pushConstants;
		Scissors_t				scissors;
		ColorBuffers_t			colorBuffers;
		DynamicStates			dynamicStates;
		DebugMode				debugMode;
			

	// methods
		BaseDrawCall () : BaseDrawTask<TaskType>{} {}
		BaseDrawCall (StringView name, RGBA8u color) : BaseDrawTask<TaskType>{ name, color } {}
		
		TaskType&  AddResources (const DescriptorSetID &id, const PipelineResources *res);

		TaskType&  AddScissor (const RectI &rect);
		TaskType&  AddScissor (const RectU &rect);
			
		TaskType&  AddColorBuffer (const RenderTargetID &id, EBlendFactor srcBlendFactor, EBlendFactor dstBlendFactor, EBlendOp blendOp, bool4 colorMask);
		TaskType&  AddColorBuffer (const RenderTargetID &id, EBlendFactor srcBlendFactorColor, EBlendFactor srcBlendFactorAlpha,
									EBlendFactor dstBlendFactorColor, EBlendFactor dstBlendFactorAlpha,
									EBlendOp blendOpColor, EBlendOp blendOpAlpha, bool4 colorMask);
		TaskType&  AddColorBuffer (const RenderTargetID &id, bool4 colorMask);

		TaskType&  SetStencilTestEnabled (bool value);
		TaskType&  SetStencilReference (uint value);
		TaskType&  SetStencilCompareMask (uint value);
		TaskType&  SetStencilWriteMask (uint value);
		TaskType&  SetStencilFailOp (EStencilOp value);
		TaskType&  SetStencilDepthFailOp (EStencilOp value);
		TaskType&  SetStencilPassOp (EStencilOp value);
		//TaskType&  SetStencilCompareOp (ECompareOp value);

		TaskType&  SetDepthCompareOp (ECompareOp value);
		TaskType&  SetDepthTestEnabled (bool value);
		TaskType&  SetDepthWriteEnabled (bool value);
		
		TaskType&  SetCullMode (ECullMode value);
		TaskType&  SetRasterizerDiscard (bool value);
		TaskType&  SetFrontFaceCCW (bool value);

		template <typename ValueType>
		TaskType&  AddPushConstant (const PushConstantID &id, const ValueType &value)	{ return AddPushConstant( id, AddressOf(value), SizeOf<ValueType> ); }
		TaskType&  AddPushConstant (const PushConstantID &id, const void *ptr, BytesU size);
		
		TaskType&  EnableVertexDebugTrace (uint vertex, uint instance = UMax, uint draw = UMax);
		TaskType&  EnableTessControlDebugTrace (uint invocation, uint primitive);
		TaskType&  EnableTessEvaluationDebugTrace (uint primitive);
		TaskType&  EnableGeometryDebugTrace (uint invocation, uint primitive);
		TaskType&  EnableMeshTaskDebugTrace (uint invocation);
		TaskType&  EnableMeshDebugTrace (uint invocation);
		TaskType&  EnableFragmentDebugTrace (uint x, uint y, uint sample = UMax, uint primitive = UMax);
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

		DrawVertices&  Draw (uint vertexCount, uint instanceCount	= 1, uint firstVertex = 0, uint firstInstance = 0)
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

		DrawIndexed&  Draw (uint indexCount, uint instanceCount = 1, uint firstIndex = 0, int vertexOffset = 0, uint firstInstance = 0)
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

		DrawVerticesIndirect&  Draw (uint drawCount, BytesU indirectBufferOffset = 0_b, BytesU stride = SizeOf<DrawIndirectCommand>)
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
		using DrawCmd			= DrawVerticesIndirect::DrawCmd;
		using DrawCommands_t	= DrawVerticesIndirect::DrawCommands_t;
		
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

		DrawIndexedIndirect&  Draw (uint drawCount, BytesU indirectBufferOffset = 0_b, BytesU stride = SizeOf<DrawIndexedIndirectCommand>)
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

		DrawMeshes&  Draw (uint count, uint first = 0)		{ ASSERT( count > 0 );  commands.push_back({ count, first });  return *this; }
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

		DrawMeshesIndirect&  Draw (uint drawCount, BytesU indirectBufferOffset = 0_b, BytesU stride = SizeOf<DrawMeshTasksIndirectCommand>)
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
	inline TaskType&  BaseDrawCall<TaskType>::AddResources (const DescriptorSetID &id, const PipelineResources *res)
	{
		ASSERT( id.IsDefined() and res );
		resources.insert({ id, res });
		return static_cast<TaskType &>( *this );
	}

	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::AddScissor (const RectI &rect)
	{
		ASSERT( rect.IsValid() );
		scissors.push_back( rect );
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::AddScissor (const RectU &rect)
	{
		ASSERT( rect.IsValid() );
		scissors.push_back( RectI{rect} );
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
	inline TaskType&  BaseDrawCall<TaskType>::SetDepthCompareOp (ECompareOp value)
	{
		dynamicStates.depthCompareOp = value;
		dynamicStates.hasDepthCompareOp = true;
		return static_cast<TaskType &>( *this );
	}

	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetDepthTestEnabled (bool value)
	{
		dynamicStates.depthTest = value;
		dynamicStates.hasDepthTest = true;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetDepthWriteEnabled (bool value)
	{
		dynamicStates.depthWrite = value;
		dynamicStates.hasDepthWrite = true;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetStencilTestEnabled (bool value)
	{
		dynamicStates.stencilTest = value;
		dynamicStates.hasStencilTest = true;
		return static_cast<TaskType &>( *this );
	}

	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetStencilReference (uint value)
	{
		dynamicStates.stencilReference = StencilValue_t(value);
		dynamicStates.hasStencilReference = true;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetStencilCompareMask (uint value)
	{
		dynamicStates.stencilCompareMask = StencilValue_t(value);
		dynamicStates.hasStencilCompareMask = true;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetStencilWriteMask (uint value)
	{
		dynamicStates.stencilWriteMask = StencilValue_t(value);
		dynamicStates.hasStencilWriteMask = true;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetStencilFailOp (EStencilOp value)
	{
		dynamicStates.stencilFailOp = value;
		dynamicStates.hasStencilFailOp = true;
		return static_cast<TaskType &>( *this );
	}

	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetStencilDepthFailOp (EStencilOp value)
	{
		dynamicStates.stencilDepthFailOp = value;
		dynamicStates.hasStencilDepthFailOp = true;
		return static_cast<TaskType &>( *this );
	}

	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetStencilPassOp (EStencilOp value)
	{
		dynamicStates.stencilPassOp = value;
		dynamicStates.hasStencilPassOp = true;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetCullMode (ECullMode value)
	{
		dynamicStates.cullMode = value;
		dynamicStates.hasCullMode = true;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetRasterizerDiscard (bool value)
	{
		dynamicStates.rasterizerDiscard = value;
		dynamicStates.hasRasterizedDiscard = true;
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::SetFrontFaceCCW (bool value)
	{
		dynamicStates.frontFaceCCW = value;
		dynamicStates.hasFrontFaceCCW = true;
		return static_cast<TaskType &>( *this );
	}

	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::AddPushConstant (const PushConstantID &id, const void *ptr, BytesU size)
	{
		ASSERT( id.IsDefined() );
		pushConstants.emplace_back( id, Bytes<uint16_t>(size) );
		MemCopy( pushConstants.back().data, BytesU::SizeOf(pushConstants.back().data), ptr, size );
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::EnableVertexDebugTrace (uint vertex, uint instance, uint draw)
	{
		debugMode.mode	 = EShaderDebugMode::Trace;
		debugMode.shader = EShader::Vertex;
		debugMode.vert	 = { vertex, instance, draw };
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::EnableTessControlDebugTrace (uint invocation, uint primitive)
	{
		debugMode.mode		= EShaderDebugMode::Trace;
		debugMode.shader	= EShader::TessControl;
		debugMode.tessCont	= { invocation, primitive };
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::EnableTessEvaluationDebugTrace (uint primitive)
	{
		debugMode.mode		= EShaderDebugMode::Trace;
		debugMode.shader	= EShader::TessEvaluation;
		debugMode.tessEval	= { primitive };
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::EnableGeometryDebugTrace (uint invocation, uint primitive)
	{
		debugMode.mode	 = EShaderDebugMode::Trace;
		debugMode.shader = EShader::Geometry;
		debugMode.geom	 = { invocation, primitive };
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::EnableMeshTaskDebugTrace (uint invocation)
	{
		debugMode.mode	 = EShaderDebugMode::Trace;
		debugMode.shader = EShader::MeshTask;
		debugMode.task	 = { invocation };
		return static_cast<TaskType &>( *this );
	}
	
	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::EnableMeshDebugTrace (uint invocation)
	{
		debugMode.mode	 = EShaderDebugMode::Trace;
		debugMode.shader = EShader::Mesh;
		debugMode.mesh	 = { invocation };
		return static_cast<TaskType &>( *this );
	}

	template <typename TaskType>
	inline TaskType&  BaseDrawCall<TaskType>::EnableFragmentDebugTrace (uint x, uint y, uint sample, uint primitive)
	{
		debugMode.mode	 = EShaderDebugMode::Trace;
		debugMode.shader = EShader::Fragment;
		debugMode.frag	 = { x, y, sample, primitive };
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
