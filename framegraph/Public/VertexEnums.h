// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{
	
	enum class EVertexType : uint
	{
		// vector size
		_VecOffset		= 0,
		_Vec1			= 1 << _VecOffset,
		_Vec2			= 2 << _VecOffset,
		_Vec3			= 3 << _VecOffset,
		_Vec4			= 4 << _VecOffset,
		_VecMask		= 0xF << _VecOffset,

		// type
		_TypeOffset		= 8,
		_Byte			=  1 << _TypeOffset,
		_UByte			=  2 << _TypeOffset,
		_Short			=  3 << _TypeOffset,
		_UShort			=  4 << _TypeOffset,
		_Int			=  5 << _TypeOffset,
		_UInt			=  6 << _TypeOffset,
		_Long			=  7 << _TypeOffset,
		_ULong			=  8 << _TypeOffset,
		_Half			=  9 << _TypeOffset,
		_Float			= 10 << _TypeOffset,
		_Double			= 11 << _TypeOffset,
		_TypeMask		= 0xF << _TypeOffset,
		
		// flags
		_FlagsOffset	= 16,
		NormalizedFlag	= 1 << _FlagsOffset,


		// default types
		Byte			= _Byte | _Vec1,
		Byte2			= _Byte | _Vec2,
		Byte3			= _Byte | _Vec3,
		Byte4			= _Byte | _Vec4,
			
		Byte_Norm		= Byte  | NormalizedFlag,
		Byte2_Norm		= Byte2 | NormalizedFlag,
		Byte3_Norm		= Byte3 | NormalizedFlag,
		Byte4_Norm		= Byte4 | NormalizedFlag,

		UByte			= _UByte | _Vec1,
		UByte2			= _UByte | _Vec2,
		UByte3			= _UByte | _Vec3,
		UByte4			= _UByte | _Vec4,
			
		UByte_Norm		= UByte  | NormalizedFlag,
		UByte2_Norm		= UByte2 | NormalizedFlag,
		UByte3_Norm		= UByte3 | NormalizedFlag,
		UByte4_Norm		= UByte4 | NormalizedFlag,
			
		Short			= _Short | _Vec1,
		Short2			= _Short | _Vec2,
		Short3			= _Short | _Vec3,
		Short4			= _Short | _Vec4,
			
		Short_Norm		= Short  | NormalizedFlag,
		Short2_Norm		= Short2 | NormalizedFlag,
		Short3_Norm		= Short3 | NormalizedFlag,
		Short4_Norm		= Short4 | NormalizedFlag,
			
		UShort			= _UShort | _Vec1,
		UShort2			= _UShort | _Vec2,
		UShort3			= _UShort | _Vec3,
		UShort4			= _UShort | _Vec4,
			
		UShort_Norm		= UShort  | NormalizedFlag,
		UShort2_Norm	= UShort2 | NormalizedFlag,
		UShort3_Norm	= UShort3 | NormalizedFlag,
		UShort4_Norm	= UShort4 | NormalizedFlag,
			
		Int				= _Int | _Vec1,
		Int2			= _Int | _Vec2,
		Int3			= _Int | _Vec3,
		Int4			= _Int | _Vec4,
			
		UInt			= _UInt | _Vec1,
		UInt2			= _UInt | _Vec2,
		UInt3			= _UInt | _Vec3,
		UInt4			= _UInt | _Vec4,
			
		Long			= _Long | _Vec1,
		Long2			= _Long | _Vec2,
		Long3			= _Long | _Vec3,
		Long4			= _Long | _Vec4,
			
		ULong			= _ULong | _Vec1,
		ULong2			= _ULong | _Vec2,
		ULong3			= _ULong | _Vec3,
		ULong4			= _ULong | _Vec4,
			
		Half			= _Half | _Vec1,
		Half2			= _Half | _Vec2,
		Half3			= _Half | _Vec3,
		Half4			= _Half | _Vec4,

		Float			= _Float | _Vec1,
		Float2			= _Float | _Vec2,
		Float3			= _Float | _Vec3,
		Float4			= _Float | _Vec4,
			
		Double			= _Double | _Vec1,
		Double2			= _Double | _Vec2,
		Double3			= _Double | _Vec3,
		Double4			= _Double | _Vec4,

		Unknown			= 0,
	};
	FG_BIT_OPERATORS( EVertexType );
	

	enum class EVertexInputRate : uint
	{
		Vertex,
		Instance,
		Unknown	= ~0u,
	};
	
	
	enum class EIndex : uint
	{
		UShort,
		UInt,
		Unknown = ~0u,
	};


}	// FG
