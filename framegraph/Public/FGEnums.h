// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	enum class ESwapchainImage : uint
	{
		Primary,

		// for VR:
		LeftEye,
		RightEye,
	};


}	// FG
