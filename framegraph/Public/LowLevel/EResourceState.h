// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/Types.h"

namespace FG
{
	
	//
	// Resource State
	//
	enum class EResourceState : uint
	{
		Unknown = 0,

		// pipeline access
		_Access_ShaderStorage,				// uniform buffer, storage buffer, image storage
		_Access_Uniform,					// uniform buffer only
		_Access_ShaderSample,				// texture only
		_Access_InputAttachment,			// same as ShaderRead but without barrier
		_Access_TransientAttachment,		// color render target in cache memory
		_Access_Transfer,					// copy buffer/image
		_Access_ColorAttachment,			// color render target
		_Access_DepthStencilAttachment,		// depth/stencil write/test
		_Access_Host,						// resource mapping
		_Access_Present,					// image only
		_Access_IndirectBuffer,
		_Access_IndexBuffer,
		_Access_VertexBuffer,
		_Access_ConditionalRendering,
		_Access_CommandProcess,
		_Access_ShadingRateImage,
		_Access_AccelerationStructure,		// raytracing
		_AccessLast,
		_AccessMask				= (1 << 8) - 1,

		// shader stages
		_VertexShader			= 1 << 8,
		_TessControlShader		= 1 << 9,
		_TessEvaluationShader	= 1 << 10,
		_GeometryShader			= 1 << 11,
		_FragmentShader			= 1 << 12,
		_ComputeShader			= 1 << 13,
		_TaskShader				= 1 << 14,
		_MeshShader				= 1 << 15,
		_RayTracingShader		= 1 << 16,		// AnyHitShader, ClosestHitShader, MissShader
		_ShaderMask				= _VertexShader | _TessControlShader | _TessEvaluationShader |
								  _GeometryShader | _FragmentShader | _ComputeShader |
								  _TaskShader | _MeshShader | _RayTracingShader,
			
		// for ColorAttachment, DepthStencilAttachment
		ClearBefore				= 1 << 25,
		InvalidateBefore		= 1 << 26,
		InvalidateAfter			= 1 << 27,

		// for DepthStencilAttachment
		EarlyFragmentTests		= 1 << 28,
		LateFragmentTests		= 1 << 29,

		// read/write access
		_Read					= 1u << 30,
		_Write					= 1u << 31,
		_ReadWrite				= _Read | _Write,

			
		// default states
		_StateMask						= _AccessMask | _ReadWrite,

		ShaderRead						= _Access_ShaderStorage | _Read,
		ShaderWrite						= _Access_ShaderStorage | _Write,
		ShaderReadWrite					= _Access_ShaderStorage | _ReadWrite,

		UniformRead						= _Access_Uniform | _Read,
		ShaderSample					= _Access_ShaderSample | _Read,
		InputAttachment					= _Access_InputAttachment | _Read,
		TransientAttachment				= _Access_TransientAttachment,

		TransferSrc						= _Access_Transfer | _Read,
		TransferDst						= _Access_Transfer | _Write,	// InvalidateBefore
			
		ColorAttachmentRead				= _Access_ColorAttachment | _Read,
		ColorAttachmentWrite			= _Access_ColorAttachment | _Write,
		ColorAttachmentReadWrite		= _Access_ColorAttachment | _ReadWrite,
			
		DepthStencilAttachmentRead		= _Access_DepthStencilAttachment | _Read,
		DepthStencilAttachmentWrite		= _Access_DepthStencilAttachment | _Write,
		DepthStencilAttachmentReadWrite	= _Access_DepthStencilAttachment | _ReadWrite,

		HostRead						= _Access_Host | _Read,
		HostWrite						= _Access_Host | _Write,		// InvalidateBefore
		HostReadWrite					= _Access_Host | _ReadWrite,
			
		PresentImage					= _Access_Present | _Read,

		IndirectBuffer					= _Access_IndirectBuffer | _Read,
		IndexBuffer						= _Access_IndexBuffer | _Read,
		VertexBuffer					= _Access_VertexBuffer | _Read,
	};

	FG_BIT_OPERATORS( EResourceState );
	
	STATIC_ASSERT( EResourceState::_AccessLast < EResourceState::_AccessMask );

}	// FG
