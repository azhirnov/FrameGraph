// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "framegraph/Public/LowLevel/Types.h"

namespace FG
{

	enum class EFilter : uint
	{
		Nearest,
		Linear,
	};


	enum class EMipmapFilter : uint
	{
		Nearest,
		Linear,
	};


	enum class EAddressMode : uint
	{
		Repeat,
		MirrorRepeat,
		ClampToEdge,
		ClampToBorder,
		MirrorClampToEdge,
	};


	enum class EBorderColor : uint
	{
		FloatTransparentBlack,
		FloatOpaqueBlack,
		FloatOpaqueWhite,

		IntTransparentBlack,
		IntOpaqueBlack,
		IntOpaqueWhite,
	};

}	// FG
