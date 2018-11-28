// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/IDs.h"
#include "stl/Math/Matrix.h"

namespace FG
{

	enum class ERayTracingInstanceFlags : uint
	{
		TriangleCullDisable		= 1 << 0,
		TriangleFrontCCW		= 1 << 1,
		ForceOpaque				= 1 << 2,
		ForceNonOpaque			= 1 << 3,
		_Last,
		Unknown					= 0,
	};



	//
	// Ray Tracing Scene Description (experimental)
	//

	struct RayTracingSceneDesc
	{
	// types
		using Matrix4x3		= Matrix< float, 4, 3, EMatrixOrder::RowMajor >;
		using EFlags		= ERayTracingInstanceFlags;

		struct Instance
		{
			RawRTGeometryID		geometryId;
			Matrix4x3			transform;
			uint				instanceID		= 0;
			uint				instanceOffset	= 0;
			EFlags				flags			= Default;
			uint8_t				mask			= 0xFF;
		};


	// variables
		ArrayView< Instance >		instances;


	// methods
		RayTracingSceneDesc () {}
		explicit RayTracingSceneDesc (ArrayView<Instance> instances) : instances{instances} {}
	};


}	// FG
