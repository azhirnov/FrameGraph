// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/VFG.h"

namespace FG
{

	enum class EShaderCompilationFlags : uint
	{
		AutoMapLocations		= 1 << 0,	// if enabled you can skip 'layout(binding=...)' and 'layout(location=...)' qualifiers
		AlwaysWriteDiscard		= 1 << 1,	// (optimization) if enabled all layout qualifiers with 'writeonly' interpreted as 'EShaderAccess::WriteDiscard'

		// SPIRV compilation flags:
		GenerateDebugInfo		= 1 << 2,
		DisableOptimizer		= 1 << 3,
		OptimizeSize			= 1 << 4,

		Quiet					= 1 << 8,
		KeepSrcShaderData		= 1 << 9,	// compiled wiil keep GLSL and add SPIRV or VkShaderModule

		UseCurrentDeviceLimits	= 1 << 10,	// get current device properties and use it to setup spirv compiler

		_Last,
		Unknown					= 0,
	};
	FG_BIT_OPERATORS( EShaderCompilationFlags );

}	// FG
