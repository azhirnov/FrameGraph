// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/FG.h"

namespace FG
{

	enum class EShaderCompilationFlags : uint
	{
		AutoMapLocations			= 1 << 0,	// if enabled you can skip 'layout(binding=...)' and 'layout(location=...)' qualifiers	// deprecated

		Quiet						= 1 << 8,
		//KeepSrcShaderData			= 1 << 9,	// compiler will keep incoming GLSL source and adds SPIRV or VkShaderModule

		UseCurrentDeviceLimits		= 1 << 10,	// get current device properties and use it to setup spirv compiler

		// SPIRV compilation flags:
		Optimize					= 1 << 16,
		OptimizeSize				= 1 << 17,
		StrongOptimization			= 1 << 18,	// very slow, may be usable for offline compilation

		ParseAnnotations			= 1 << 20,

		_Last,
		Unknown						= 0,
	};
	FG_BIT_OPERATORS( EShaderCompilationFlags );

}	// FG
