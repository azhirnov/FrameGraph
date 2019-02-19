// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Common.h"

namespace FG
{

	enum class ECameraType : uint8_t
	{
		Main,
		Reflection,
		Shadow,
		Unknown	= 0xFF,
	};


	enum class ETextureType : uint
	{
		Unknown			= 0,
		Albedo			= 1 << 0,
		Normal			= 1 << 1,
		Height			= 1 << 2,
		Reflection		= 1 << 3,	// TODO: static & dynamic ?
		Specular		= 1 << 4,
		Roughtness		= 1 << 5,
		Metallic		= 1 << 6,
	};
	FG_BIT_OPERATORS( ETextureType );


	enum class ERenderLayer : uint8_t
	{
		Layer_1		= 0,
		Layer_32	= 31,
		_Count,

		// default names:
		Shadow		= Layer_1,	// shadow map
		DepthOnly,				// depth prepass
		Background,
		Opaque_1,				// large objects
		Opaque_2,
		Opaque_3,
		Foreground,
		Emission,
		Translucent,
		RayTracing,
		HUD,

		Unknown		= 0xFF,
	};
	using LayerBits = BitSet< uint(ERenderLayer::_Count) >;


	enum class EDetailLevel : uint8_t
	{
		High		= 0,
		Low			= 7,
		_Count,
		Unknown		= 0xFF,
	};

	ND_ forceinline EDetailLevel  operator + (EDetailLevel lhs, uint rhs)  { return EDetailLevel( uint(lhs) + rhs ); }


	struct DetailLevelRange
	{
		EDetailLevel	min		= EDetailLevel::High;
		EDetailLevel	max		= EDetailLevel::Low;

		DetailLevelRange () {}
		DetailLevelRange (EDetailLevel min, EDetailLevel max) : min{min}, max{max} {}

		ND_ EDetailLevel  Clamp (EDetailLevel value) const
		{
			return EDetailLevel(FG::Clamp( uint(value), uint(min), uint(max) ));
		}
	};


}	// FG
