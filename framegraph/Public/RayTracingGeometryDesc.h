// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/VertexEnums.h"
#include "framegraph/Public/RayTracingEnums.h"
#include "framegraph/Public/IDs.h"
#include "framegraph/Public/VertexDesc.h"
#include "stl/Math/Matrix.h"

namespace FG
{

	//
	// Ray Tracing Geometry Description
	//

	struct RayTracingGeometryDesc
	{
	// types
		using EFlags = ERayTracingGeometryFlags;

		struct Triangles
		{
		// variables
			GeometryID		geometryId;
			EFlags			flags			= Default;

			uint			vertexCount		= 0;
			EVertexType		vertexFormat	= Default;

			// optional:
			uint			indexCount		= 0;
			EIndex			indexType		= Default;

		// methods
			Triangles () {}
			explicit Triangles (const GeometryID &id) : geometryId{id} {}

			template <typename T, typename Idx>	Triangles&  SetVertices (Idx count);
			template <typename Idx>				Triangles&  SetVertices (Idx count, EVertexType fmt);
			template <typename Idx>				Triangles&  SetIndices (Idx count, EIndex type);
			template <typename T, typename Idx>	Triangles&  SetIndices (Idx count);
												Triangles&  AddFlags (EFlags value)			{ flags |= value;  return *this; }
												Triangles&  SetID (const GeometryID &id)	{ geometryId = id;  return *this; }
		};

		struct AABB
		{
		// variables
			GeometryID		geometryId;
			EFlags			flags			= Default;
			uint			aabbCount		= 0;

		// methods
			AABB () {}
			explicit AABB (const GeometryID &id) : geometryId{id} {}

			template <typename Idx>	AABB&  SetCount (Idx count)			{ STATIC_ASSERT(IsInteger<Idx>);  aabbCount = CheckCast<uint>(count);  return *this; }
									AABB&  AddFlags (EFlags value)		{ flags |= value;  return *this; }
									AABB&  SetID (const GeometryID &id)	{ geometryId = id;  return *this; }
		};


	// variables
		ArrayView< Triangles >	triangles;
		ArrayView< AABB >		aabbs;
		ERayTracingFlags		flags		= Default;


	// methods
		RayTracingGeometryDesc () {}
		explicit RayTracingGeometryDesc (ArrayView<AABB> aabbs) : aabbs{aabbs} {}
		explicit RayTracingGeometryDesc (ArrayView<Triangles> triangles) : triangles{triangles} {}
		RayTracingGeometryDesc (ArrayView<Triangles> triangles, ArrayView<AABB> aabbs) : triangles{triangles}, aabbs{aabbs} {}
	};

	

	template <typename T, typename Idx>
	inline RayTracingGeometryDesc::Triangles&
		RayTracingGeometryDesc::Triangles::SetVertices (Idx count)
	{
		STATIC_ASSERT( IsInteger<Idx> );
		vertexCount  = CheckCast<uint>(count);
		vertexFormat = VertexDesc<T>::attrib;
		return *this;
	}
	
	template <typename Idx>
	inline RayTracingGeometryDesc::Triangles&
		RayTracingGeometryDesc::Triangles::SetVertices (Idx count, EVertexType fmt)
	{
		STATIC_ASSERT( IsInteger<Idx> );
		vertexCount  = CheckCast<uint>(count);
		vertexFormat = fmt;
		return *this;
	}
	
	template <typename Idx>
	inline RayTracingGeometryDesc::Triangles&
		RayTracingGeometryDesc::Triangles::SetIndices (Idx count, EIndex type)
	{
		STATIC_ASSERT( IsInteger<Idx> );
		indexCount = CheckCast<uint>(count);
		indexType  = type;
		return *this;
	}
	
	template <typename T, typename Idx>
	inline RayTracingGeometryDesc::Triangles&
		RayTracingGeometryDesc::Triangles::SetIndices (Idx count)
	{
		using type = std::remove_cv_t< std::remove_reference_t<T> >;

		STATIC_ASSERT( IsInteger<Idx> );
		STATIC_ASSERT( (IsSameTypes<type, uint32_t>) or (IsSameTypes<type, uint16_t>) );

		indexCount = CheckCast<uint>(count); 
		indexType  = IsSameTypes<type, uint> ? EIndex::UInt : EIndex::UShort;
		return *this;
	}


}	// FG
