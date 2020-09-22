// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{
	
	enum class EBlendFactor : uint8_t
	{
		// src - from shader
		// dst - from render target
		// result = srcColor * srcBlend [blendOp] dstColor * dstBlend;
		Zero,						// 0
		One,						// 1
		SrcColor,					// src
		OneMinusSrcColor,			// 1 - src
		DstColor,					// dst
		OneMinusDstColor,			// 1 - dst
		SrcAlpha,					// src.a
		OneMinusSrcAlpha,			// 1 - src.a
		DstAlpha,					// dst.a
		OneMinusDstAlpha,			// 1 - dst.a
		ConstColor,					// cc
		OneMinusConstColor,			// 1 - cc
		ConstAlpha,					// cc.a
		OneMinusConstAlpha,			// 1 - cc.a
		SrcAlphaSaturate,			// rgb * min( src.a, dst.a ), a * 1

		Src1Color,					// 
		OneMinusSrc1Color,			// 
		Src1Alpha,					// 
		OneMinusSrc1Alpha,			// 

		Unknown						= uint8_t(~0u),
	};


	enum class EBlendOp : uint8_t
	{
		// src - from shader
		// dst - from render target
		// result = srcColor * srcBlend [blendOp] dstColor * dstBlend;
		Add,			// S+D
		Sub,			// S-D
		RevSub,			// D-S
		Min,			// min(S,D)
		Max,			// max(S,D)
		Unknown			= uint8_t(~0u),
	};


	enum class ELogicOp : uint8_t
	{
		None,				// disabled
		Clear,				// 0
		Set,				// 1
		Copy,				// S
		CopyInverted,		// ~S
		NoOp,				// D
		Invert,				// ~D
		And,				// S & D
		NotAnd,				// ~ ( S & D )
		Or,					// S | D
		NotOr,				// ~ ( S | D )
		Xor,				// S ^ D
		Equiv,				// ~ ( S ^ D )
		AndReverse,			// S & ~D
		AndInverted,		// ~S & D
		OrReverse,			// S | ~D
		OrInverted,			// ~S | D
		Unknown				= uint8_t(~0u),
	};


	enum class ECompareOp : uint8_t
	{
		Never,			// false
		Less,			// <
		Equal,			// ==
		LEqual,			// <=
		Greater,		// >
		NotEqual,		// !=
		GEqual,			// >=
		Always,			// true
		Unknown			= uint8_t(~0u),
	};


	enum class EStencilOp : uint8_t
	{
		Keep,			// s				- Keeps the current value.
		Zero,			// 0				- Sets the stencil buffer value to 0.
		Replace,		// ref				- Sets the stencil buffer value to 'StencilFaceState::reference'.
		Incr,			// min( ++s, 0 )	- Increments the current stencil buffer value. Clamps to the maximum representable unsigned value.
		IncrWrap,		// ++s & maxvalue	- Increments the current stencil buffer value. Wraps stencil buffer value to zero when incrementing the maximum representable unsigned value.
		Decr,			// max( --s, 0 )	- Decrements the current stencil buffer value. Clamps to 0.
		DecrWrap,		// --s & maxvalue	- Decrements the current stencil buffer value. Wraps stencil buffer value to the maximum representable unsigned value when decrementing a stencil buffer value of zero.
		Invert,			// ~s				- Bitwise inverts the current stencil buffer value.
		Unknown			= uint8_t(~0u),
	};


	enum class EPolygonMode : uint8_t
	{
		Point,
		Line,
		Fill,
		Unknown		= uint8_t(~0u),
	};

	
	enum class EPrimitive : uint8_t
	{
		Point,
		
		LineList,
		LineStrip,
		LineListAdjacency,
		LineStripAdjacency,

		TriangleList,
		TriangleStrip,
		TriangleFan,
		TriangleListAdjacency,
		TriangleStripAdjacency,

		Patch,

		_Count,
		Unknown		= uint8_t(~0u),
	};
	

	enum class ECullMode : uint8_t
	{
		None		= 0,
		Front		= 1	<< 0,
		Back		= 1 << 1,
		FontAndBack	= Front | Back,
		Unknown		= None,
	};
	FG_BIT_OPERATORS( ECullMode );
	

	enum class EPipelineDynamicState : uint8_t
	{
		Unknown					= 0,
		Viewport				= 1 << 0,
		Scissor					= 1 << 1,
		StencilCompareMask		= 1 << 2,
		StencilWriteMask		= 1 << 3,
		StencilReference		= 1 << 4,
		ShadingRatePalette		= 1 << 5,
		_Last,

		All						= ((_Last-1) << 1) - 1,
		RasterizerMask			= All,
		Default					= Viewport | Scissor,
	};
	FG_BIT_OPERATORS( EPipelineDynamicState );


}	// FG
