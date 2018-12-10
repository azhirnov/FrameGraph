// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	enum class ERayTracingGeometryFlags : uint
	{
		Opaque						= 1 << 0,
		NoDuplicateAnyHitInvocation	= 1 << 1,
		_Last,
		Unknown						= 0,
	};
	FG_BIT_OPERATORS( ERayTracingGeometryFlags );
	

	enum class ERayTracingInstanceFlags : uint
	{
		TriangleCullDisable		= 1 << 0,
		TriangleFrontCCW		= 1 << 1,
		ForceOpaque				= 1 << 2,
		ForceNonOpaque			= 1 << 3,
		_Last,
		Unknown					= 0,
	};
	FG_BIT_OPERATORS( ERayTracingInstanceFlags );


	// TODO: rename
	enum class ERayTracingFlags : uint
	{
		AllowUpdate				= 1 << 0,
		AllowCompaction			= 1 << 1,
		PreferFastTrace			= 1 << 2,
		PreferFastBuild			= 1 << 3,
		LowMemory				= 1 << 4,
		_Last,
		Unknown					= 0,
	};
	FG_BIT_OPERATORS( ERayTracingFlags );


}	// FG
