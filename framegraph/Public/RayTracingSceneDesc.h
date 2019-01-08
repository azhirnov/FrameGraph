// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/IDs.h"
#include "framegraph/Public/RayTracingEnums.h"
#include "stl/Math/Matrix.h"

namespace FG
{

	//
	// Ray Tracing Scene Description (experimental)
	//

	struct RayTracingSceneDesc
	{
	// variables
		uint				maxInstanceCount	= 0;
		ERayTracingFlags	flags				= Default;


	// methods
		RayTracingSceneDesc () {}
		explicit RayTracingSceneDesc (uint instanceCount, ERayTracingFlags flags = Default) : maxInstanceCount{instanceCount}, flags{flags} {}
	};


}	// FG
