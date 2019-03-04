// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	enum class ERayTracingGeometryFlags : uint
	{
		Opaque						= 1 << 0,	// indicates that this geometry does not invoke the any-hit shaders even if present in a hit group
		NoDuplicateAnyHitInvocation	= 1 << 1,
		_Last,
		Unknown						= 0,
	};
	FG_BIT_OPERATORS( ERayTracingGeometryFlags );
	

	enum class ERayTracingInstanceFlags : uint
	{
		TriangleCullDisable		= 1 << 0,
		TriangleFrontCCW		= 1 << 1,
		ForceOpaque				= 1 << 2,	// enable ERayTracingGeometryFlags::Opaque flag
		ForceNonOpaque			= 1 << 3,	// disable ERayTracingGeometryFlags::Opaque flag
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
