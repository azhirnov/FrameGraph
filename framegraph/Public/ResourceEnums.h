// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	enum class EQueueType : uint
	{
		Graphics,			// also supports compute and transfer commands
		AsyncCompute,		// separate compute queue
		AsyncTransfer,		// separate transfer queue
		//Present,			// queue must support present, may be a separate queue
		_Count,
		Unknown			= ~0u,
	};
	

	enum class EQueueUsage : uint
	{
		Unknown			= 0,
		Graphics		= 1 << uint(EQueueType::Graphics),
		AsyncCompute	= 1 << uint(EQueueType::AsyncCompute),
		AsyncTransfer	= 1 << uint(EQueueType::AsyncTransfer),
		_Last,
		All				= ((_Last-1) << 1) - 1,
	};
	FG_BIT_OPERATORS( EQueueUsage );
	

	forceinline constexpr EQueueUsage&  operator |= (EQueueUsage &lhs, EQueueType rhs)
	{
		ASSERT( uint(rhs) < 32 );
		return lhs = BitCast<EQueueUsage>( uint(lhs) | (1u << (uint(rhs) & 31)) );
	}

	forceinline constexpr EQueueUsage   operator |  (EQueueUsage lhs, EQueueType rhs)
	{
		ASSERT( uint(rhs) < 32 );
		return BitCast<EQueueUsage>( uint(lhs) | (1u << (uint(rhs) & 31)) );
	}


	enum class EMemoryType : uint
	{
		Default			= 0,			// local in GPU
		HostRead		= 1 << 0,
		HostWrite		= 1 << 1,
		Dedicated		= 1 << 2,		// force to use dedicated allocation
		//AllowAliasing	= 1 << 3,		// 
		//Sparse		= 1 << 4,
		_Last,
	};
	FG_BIT_OPERATORS( EMemoryType );


	enum class EBufferUsage : uint
	{
		TransferSrc		= 1 << 0,
		TransferDst		= 1 << 1,
		UniformTexel	= 1 << 2,
		StorageTexel	= 1 << 3,
		Uniform			= 1 << 4,
		Storage			= 1 << 5,
		Index			= 1 << 6,
		Vertex			= 1 << 7,
		Indirect		= 1 << 8,
		RayTracing		= 1 << 9,
		_Last,
		
		All				= ((_Last-1) << 1) - 1,
		Transfer		= TransferDst | TransferSrc,
		Unknown			= 0,
	};
	FG_BIT_OPERATORS( EBufferUsage );


	enum class EImage : uint
	{
		Tex1D		= 0,
		Tex1DArray,
		Tex2D,
		Tex2DArray,
		Tex2DMS,
		Tex2DMSArray,
		TexCube,
		TexCubeArray,
		Tex3D,
		Buffer,
		Unknown		= ~0u,
	};


	enum class EImageUsage : uint
	{
		TransferSrc				= 1 << 0,		// for all copy operations
		TransferDst				= 1 << 1,		// for all copy operations
		Sampled					= 1 << 2,		// access in shader as texture
		Storage					= 1 << 3,		// access in shader as image
		ColorAttachment			= 1 << 4,		// color or resolve attachment
		DepthStencilAttachment	= 1 << 5,		// depth/stencil attachment
		TransientAttachment		= 1 << 6,		// color, resolve, depth/stencil, input attachment
		InputAttachment			= 1 << 7,		// input attachment in shader
		ShadingRate				= 1 << 8,
		_Last,

		All						= ((_Last-1) << 1) - 1,
		Transfer				= TransferSrc | TransferDst,
		Unknown					= 0,
	};
	FG_BIT_OPERATORS( EImageUsage );


	enum class EImageAspect : uint
	{
		Color			= 1 << 0,
		Depth			= 1 << 1,
		Stencil			= 1 << 2,
		Metadata		= 1 << 3,
		_Last,

		DepthStencil	= Depth | Stencil,
		Auto			= ~0u,
		Unknown			= 0,
	};
	FG_BIT_OPERATORS( EImageAspect );
	
	
	enum class EAttachmentLoadOp : uint
	{
		Invalidate,
		Load,
		Clear,
		Unknown		= ~0u,
	};


	enum class EAttachmentStoreOp : uint
	{
		Invalidate,
		Store,
		Unknown		= ~0u,
	};


	enum class EShadingRatePalette : uint8_t
	{
		NoInvocations	= 0,
		Block_1x1_16	= 1,	// 16 invocations per 1x1 pixel block
		Block_1x1_8		= 2,	//  8 invocations per 1x1 pixel block
		Block_1x1_4		= 3,	//  4 invocations per 1x1 pixel block
		Block_1x1_2		= 4,	//  2 invocations per 1x1 pixel block
		Block_1x1_1		= 5,	//  1 invocation  per 1x1 pixel block
		Block_2x1_1		= 6,	//  1 invocation  per 2x1 pixel block
		Block_1x2_1		= 7,	//  1 invocation  per 1x2 pixel block
		Block_2x2_1		= 8,	//  1 invocation  per 2x2 pixel block
		Block_4x2_1		= 9,	//  1 invocation  per 4x2 pixel block
		Block_2x4_1		= 10,	//  1 invocation  per 2x4 pixel block
		Block_4x4_1		= 11,	//  1 invocation  per 4x4 pixel block
		_Count,
	};
	

	enum class EPixelFormat : uint
	{
		// signed normalized
		RGBA16_SNorm,
		RGBA8_SNorm,
		RGB16_SNorm,
		RGB8_SNorm,
		RG16_SNorm,
		RG8_SNorm,
		R16_SNorm,
		R8_SNorm,
			
		// unsigned normalized
		RGBA16_UNorm,
		RGBA8_UNorm,
		RGB16_UNorm,
		RGB8_UNorm,
		RG16_UNorm,
		RG8_UNorm,
		R16_UNorm,
		R8_UNorm,
		RGB10_A2_UNorm,
		RGBA4_UNorm,
		RGB5_A1_UNorm,
		RGB_5_6_5_UNorm,

		// BGRA
		BGR8_UNorm,
		BGRA8_UNorm,
			
		// sRGB
		sRGB8,
		sRGB8_A8,
		sBGR8,
		sBGR8_A8,

		// signed integer
		R8I,
		RG8I,
		RGB8I,
		RGBA8I,
		R16I,
		RG16I,
		RGB16I,
		RGBA16I,
		R32I,
		RG32I,
		RGB32I,
		RGBA32I,
			
		// unsigned integer
		R8U,
		RG8U,
		RGB8U,
		RGBA8U,
		R16U,
		RG16U,
		RGB16U,
		RGBA16U,
		R32U,
		RG32U,
		RGB32U,
		RGBA32U,
		RGB10_A2U,
			
		// float
		R16F,
		RG16F,
		RGB16F,
		RGBA16F,
		R32F,
		RG32F,
		RGB32F,
		RGBA32F,
		RGB_11_11_10F,

		// depth stencil
		Depth16,
		Depth24,
		Depth32F,
		Depth16_Stencil8,
		Depth24_Stencil8,
		Depth32F_Stencil8,
			
		// compressed
		BC1_RGB8_UNorm,
		BC1_sRGB8,
		BC1_RGB8_A1_UNorm,
		BC1_sRGB8_A1,
		BC2_RGBA8_UNorm,
		BC2_sRGB8_A8,
		BC3_RGBA8_UNorm,
		BC3_sRGB8,
		BC4_R8_SNorm,
		BC4_R8_UNorm,
		BC5_RG8_SNorm,
		BC5_RG8_UNorm,
		BC7_RGBA8_UNorm,
		BC7_sRGB8_A8,
		BC6H_RGB16F,
		BC6H_RGB16UF,
		ETC2_RGB8_UNorm,
		ECT2_sRGB8,
		ETC2_RGB8_A1_UNorm,
		ETC2_sRGB8_A1,
		ETC2_RGBA8_UNorm,
		ETC2_sRGB8_A8,
		EAC_R11_SNorm,
		EAC_R11_UNorm,
		EAC_RG11_SNorm,
		EAC_RG11_UNorm,
		ASTC_RGBA_4x4,
		ASTC_RGBA_5x4,
		ASTC_RGBA_5x5,
		ASTC_RGBA_6x5,
		ASTC_RGBA_6x6,
		ASTC_RGBA_8x5,
		ASTC_RGBA_8x6,
		ASTC_RGBA_8x8,
		ASTC_RGBA_10x5,
		ASTC_RGBA_10x6,
		ASTC_RGBA_10x8,
		ASTC_RGBA_10x10,
		ASTC_RGBA_12x10,
		ASTC_RGBA_12x12,
		ASTC_sRGB8_A8_4x4,
		ASTC_sRGB8_A8_5x4,
		ASTC_sRGB8_A8_5x5,
		ASTC_sRGB8_A8_6x5,
		ASTC_sRGB8_A8_6x6,
		ASTC_sRGB8_A8_8x5,
		ASTC_sRGB8_A8_8x6,
		ASTC_sRGB8_A8_8x8,
		ASTC_sRGB8_A8_10x5,
		ASTC_sRGB8_A8_10x6,
		ASTC_sRGB8_A8_10x8,
		ASTC_sRGB8_A8_10x10,
		ASTC_sRGB8_A8_12x10,
		ASTC_sRGB8_A8_12x12,

		_Count,
		Unknown					= ~0u,
	};
	

	enum class EFragOutput : uint
	{
		Unknown		= 0,
		Int4		= uint( EPixelFormat::RGBA32I ),
		UInt4		= uint( EPixelFormat::RGBA32U ),
		Float4		= uint( EPixelFormat::RGBA32F ),
	};


}	// FG
