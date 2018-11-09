// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/VertexEnums.h"

namespace FG
{

	//
	// Vertex Description
	//
	template <typename T>
	struct VertexDesc
	{};


	// Float types
#	ifdef FG_HALF_TYPE
	template <>
	struct VertexDesc <half>
	{
		typedef half					type;
		static const EVertexType		attrib	= EVertexType::Half;
	};
#	endif

	template <>
	struct VertexDesc <float>
	{
		typedef float					type;
		static const EVertexType		attrib	= EVertexType::Float;
	};

	template <>
	struct VertexDesc <double>
	{
		typedef double					type;
		static const EVertexType		attrib	= EVertexType::Double;
	};


	// Integer types
	template <>
	struct VertexDesc <int8_t>
	{
		typedef int8_t					type;
		static const EVertexType		attrib	= EVertexType::Byte;
	};

	template <>
	struct VertexDesc <uint8_t>
	{
		typedef uint8_t					type;
		static const EVertexType		attrib	= EVertexType::UByte;
	};


	template <>
	struct VertexDesc <int16_t>
	{
		typedef int16_t					type;
		static const EVertexType		attrib	= EVertexType::Short;
	};

	template <>
	struct VertexDesc <uint16_t>
	{
		typedef uint16_t				type;
		static const EVertexType		attrib	= EVertexType::UShort;
	};


	template <>
	struct VertexDesc <int32_t>
	{
		typedef int32_t					type;
		static const EVertexType		attrib	= EVertexType::Int;
	};

	template <>
	struct VertexDesc <uint32_t>
	{
		typedef uint32_t				type;
		static const EVertexType		attrib	= EVertexType::UInt;
	};


	template <>
	struct VertexDesc <int64_t>
	{
		typedef int64_t					type;
		static const EVertexType		attrib	= EVertexType::Long;
	};

	template <>
	struct VertexDesc <uint64_t>
	{
		typedef uint64_t				type;
		static const EVertexType		attrib	= EVertexType::ULong;
	};


	// Vector types
	template <typename T, uint I>
	struct VertexDesc < Vec<T,I> >
	{
		typedef Vec<T,I>				type;
		static const EVertexType		attrib	= EVertexType( 
														(VertexDesc< T >::attrib & EVertexType::_TypeMask) |
														EVertexType(I << uint(EVertexType::_VecOffset)) );
	};


	template <>
	struct VertexDesc <RGBA32f>
	{
		typedef RGBA32f					type;
		static const EVertexType		attrib	= EVertexType::Float4;
	};

	template <>
	struct VertexDesc <RGBA32i>
	{
		typedef RGBA32i					type;
		static const EVertexType		attrib	= EVertexType::Int4;
	};

	template <>
	struct VertexDesc <RGBA32u>
	{
		typedef RGBA32u					type;
		static const EVertexType		attrib	= EVertexType::UInt4;
	};

	template <>
	struct VertexDesc <RGBA8u>
	{
		typedef RGBA8u					type;
		static const EVertexType		attrib	= EVertexType::UByte4_Norm;
	};
	

}	// FG
