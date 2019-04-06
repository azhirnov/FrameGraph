// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/FG.h"

namespace FG
{

	enum class EShaderCompilationFlags : uint
	{
		AutoMapLocations			= 1 << 0,	// if enabled you can skip 'layout(binding=...)' and 'layout(location=...)' qualifiers
		//AlwaysWriteDiscard			= 1 << 1,	// (optimization) if enabled all layout qualifiers with 'writeonly' interpreted as 'EShaderAccess::WriteDiscard'
		//AlwaysBufferDynamicOffset	= 1 << 2,	// for uniform and storage buffer always use dynamic offsets

		Quiet						= 1 << 8,
		//KeepSrcShaderData			= 1 << 9,	// compiled wiil keep GLSL and add SPIRV or VkShaderModule

		UseCurrentDeviceLimits		= 1 << 10,	// get current device properties and use it to setup spirv compiler

		// SPIRV compilation flags:
		Optimize					= 1 << 16,
		OptimizeSize				= 1 << 17,
		StrongOptimization			= 1 << 18,	// very slow, may be usable for offline compilation

		ParseAnnoations				= 1 << 20,

		_Last,
		Unknown						= 0,
	};
	FG_BIT_OPERATORS( EShaderCompilationFlags );

}	// FG
