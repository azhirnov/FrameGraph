// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	enum class EFilter : uint
	{
		Nearest,
		Linear,

		Unknown	= ~0u,
	};


	enum class EMipmapFilter : uint
	{
		Nearest,
		Linear,
		
		Unknown	= ~0u,
	};


	enum class EAddressMode : uint
	{
		Repeat,
		MirrorRepeat,
		ClampToEdge,
		ClampToBorder,
		MirrorClampToEdge,
		
		Unknown	= ~0u,
	};


	enum class EBorderColor : uint
	{
		FloatTransparentBlack,
		FloatOpaqueBlack,
		FloatOpaqueWhite,

		IntTransparentBlack,
		IntOpaqueBlack,
		IntOpaqueWhite,
		
		Unknown	= ~0u,
	};

}	// FG
