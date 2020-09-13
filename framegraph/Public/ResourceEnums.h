// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"
#include "stl/CompileTime/Math.h"

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
		TransferSrc			= 1 << 0,
		TransferDst			= 1 << 1,
		UniformTexel		= 1 << 2,		// glsl: 'uniform samplerBuffer'
		StorageTexel		= 1 << 3,		// glsl: 'uniform imageBuffer'
		Uniform				= 1 << 4,		// uniform buffer
		Storage				= 1 << 5,		// shader storage buffer
		Index				= 1 << 6,		// index buffer
		Vertex				= 1 << 7,		// vertex buffer
		Indirect			= 1 << 8,		// indirect buffer for draw and dispatch
		RayTracing			= 1 << 9,		// for scratch buffer, instance data, shader binding table
		//ShaderAddress		= 1 << 10,		// shader device address	// not supported yet
		
		// special flags for IsSupported() method
		VertexPplnStore		= 1 << 11,		// same as 'Storage', storage buffer store and atomic operations in vertex, geometry, tessellation shaders
		FragmentPplnStore	= 1 << 12,		// same as 'Storage', storage buffer store and atomic operations in fragment shader
		StorageTexelAtomic	= 1 << 13,		// same as 'StorageTexel', atomic ops on imageBuffer
		_Last,
		
		All					= ((_Last-1) << 1) - 1,
		Transfer			= TransferDst | TransferSrc,
		Unknown				= 0,
	};
	FG_BIT_OPERATORS( EBufferUsage );

	
	enum class EImageDim : uint8_t
	{
		_1D,
		_2D,
		_3D,
		OneDim		= _1D,
		TwoDim		= _2D,
		ThreeDim	= _3D,
		Unknown		= uint8_t(~0u),
	};
	
	static constexpr auto	EImageDim_1D	= EImageDim::_1D;
	static constexpr auto	EImageDim_2D	= EImageDim::_2D;
	static constexpr auto	EImageDim_3D	= EImageDim::_3D;


	enum class EImage : uint8_t
	{
		_1D,
		_2D,
		_3D,
		_1DArray,
		_2DArray,
		Cube,
		CubeArray,
		OneDim			= _1D,
		TwoDim			= _2D,
		ThreeDim		= _3D,
		OneDimArray		= _1DArray,
		TwoDimArray		= _2DArray,
		Unknown			= uint8_t(~0u),
	};

	static constexpr auto	EImage_1D			= EImage::_1D;
	static constexpr auto	EImage_2D			= EImage::_2D;
	static constexpr auto	EImage_3D			= EImage::_3D;
	static constexpr auto	EImage_1DArray		= EImage::_1DArray;
	static constexpr auto	EImage_2DArray		= EImage::_2DArray;
	static constexpr auto	EImage_Cube			= EImage::Cube;
	static constexpr auto	EImage_CubeArray	= EImage::CubeArray;


	enum class EImageFlags : uint8_t
	{
		CubeCompatible				= 1 << 0,	// allows to create CubeMap and CubeMapArray from 2D Array
		MutableFormat				= 1 << 1,	// allows to change image format
		Array2DCompatible			= 1 << 2,	// allows to create 2D Array view from 3D image
		BlockTexelViewCompatible	= 1 << 3,	// allows to create view with uncompressed format for compressed image
		_Last,
		
		Unknown						= 0,
	};
	FG_BIT_OPERATORS( EImageFlags );


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
		//FragmentDensityMap	= 1 << 9,		// not supported yet

		// special flags for IsSupported() method
		StorageAtomic			= 1 << 10,		// same as 'Storage', atomic operations on image
		ColorAttachmentBlend	= 1 << 11,		// same as 'ColorAttachment', blend operations on render target
		SampledMinMax			= 1 << 13,		// same as 'Sampled'
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
	

	enum class EImageSampler : uint
	{
		// dimension
		_DimOffset		= CT_IntLog2< uint(EPixelFormat::_Count) > + 1,
		_DimMask		= 0xF << _DimOffset,
		_1D				= 1 << _DimOffset,
		_1DArray		= 2 << _DimOffset,
		_2D				= 3 << _DimOffset,
		_2DArray		= 4 << _DimOffset,
		_2DMS			= 5 << _DimOffset,
		_2DMSArray		= 6 << _DimOffset,
		_Cube			= 7 << _DimOffset,
		_CubeArray		= 8 << _DimOffset,
		_3D				= 9 << _DimOffset,

		// type
		_TypeOffset		= _DimOffset + 4,
		_TypeMask		= 0xF << _TypeOffset,
		_Float			= 1 << _TypeOffset,
		_Int			= 2 << _TypeOffset,
		_Uint			= 3 << _TypeOffset,

		// flags
		_FlagsOffset	= _TypeOffset + 4,
		_FlagsMask		= 0xF << _FlagsOffset,
		_Shadow			= 1 << _FlagsOffset,

		// format
		_FormatMask		= (1u << _DimOffset) - 1,

		// default
		Float1D			= _Float | _1D,
		Float1DArray	= _Float | _1DArray,
		Float2D			= _Float | _2D,
		Float2DArray	= _Float | _2DArray,
		Float2DMS		= _Float | _2DMS,
		Float2DMSArray	= _Float | _2DMSArray,
		FloatCube		= _Float | _Cube,
		FloatCubeArray	= _Float | _CubeArray,
		Float3D			= _Float | _3D,
		
		Int1D			= _Int | _1D,
		Int1DArray		= _Int | _1DArray,
		Int2D			= _Int | _2D,
		Int2DArray		= _Int | _2DArray,
		Int2DMS			= _Int | _2DMS,
		Int2DMSArray	= _Int | _2DMSArray,
		IntCube			= _Int | _Cube,
		IntCubeArray	= _Int | _CubeArray,
		Int3D			= _Int | _3D,
		
		Uint1D			= _Uint | _1D,
		Uint1DArray		= _Uint | _1DArray,
		Uint2D			= _Uint | _2D,
		Uint2DArray		= _Uint | _2DArray,
		Uint2DMS		= _Uint | _2DMS,
		Uint2DMSArray	= _Uint | _2DMSArray,
		UintCube		= _Uint | _Cube,
		UintCubeArray	= _Uint | _CubeArray,
		Uint3D			= _Uint | _3D,

		Unknown			= ~0u,
	};
	FG_BIT_OPERATORS( EImageSampler );
	

	enum class EFragOutput : uint
	{
		Unknown		= 0,
		Int4		= uint( EPixelFormat::RGBA32I ),
		UInt4		= uint( EPixelFormat::RGBA32U ),
		Float4		= uint( EPixelFormat::RGBA32F ),
	};


}	// FG
