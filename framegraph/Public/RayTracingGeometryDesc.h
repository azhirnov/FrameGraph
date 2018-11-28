// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/VertexEnums.h"
#include "framegraph/Public/IDs.h"

namespace FG
{

	enum class ERayTracingGeometryFlags
	{
		Opaque						= 1 << 0,
		NoDuplicateAnyHitInvocation	= 1 << 1,
		_Last,
		Unknown						= 0,
	};
	FG_BIT_OPERATORS( ERayTracingGeometryFlags );



	//
	// Ray Tracing Geometry Description (experimental)
	//

	struct RayTracingGeometryDesc
	{
	// types
		using EFlags = ERayTracingGeometryFlags;

		struct Geometry
		{
			RawBufferID		vertexBuffer;
			BytesU			vertexOffset;
			Bytes<uint>		vertexStride;
			uint			vertexCount			= 0;
			EVertexType		vertexFormat		= Default;

			// optional:
			RawBufferID		indexBuffer;
			BytesU			indexOffset;
			uint			indexCount			= 0;
			EIndex			indexType			= Default;

			// optional:
			RawBufferID		transformBuffer;
			BytesU			transformOffset;

			EFlags			flags				= Default;
		};

		struct AABBData
		{
			float3		min;
			float3		max;
		};

		struct AABB
		{
			RawBufferID		aabbBuffer;			// array of 'AABBData'
			BytesU			aabbOffset;
			Bytes<uint>		aabbStride;
			uint			aabbCount			= 0;
			EFlags			flags				= Default;
		};


	// variables
		ArrayView< Geometry >	geometry;
		ArrayView< AABB >		aabb;


	// methods
		RayTracingGeometryDesc () {}
		explicit RayTracingGeometryDesc (ArrayView<AABB> aabb) : aabb{aabb} {}
		explicit RayTracingGeometryDesc (ArrayView<Geometry> geometry) : geometry{geometry} {}
		RayTracingGeometryDesc (ArrayView<Geometry> geometry, ArrayView<AABB> aabb) : geometry{geometry}, aabb{aabb} {}
	};


}	// FG
