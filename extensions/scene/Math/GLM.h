// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Common.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_CXX14
//#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_CTOR_INIT

#ifdef COMPILER_MSVC
#	pragma warning (push)
#	pragma warning (disable: 4201)
#	pragma warning (disable: 4127)
#endif

#include "glm.hpp"

#include "ext/matrix_double2x2.hpp"
#include "ext/matrix_double2x2_precision.hpp"
#include "ext/matrix_double2x3.hpp"
#include "ext/matrix_double2x3_precision.hpp"
#include "ext/matrix_double2x4.hpp"
#include "ext/matrix_double2x4_precision.hpp"
#include "ext/matrix_double3x2.hpp"
#include "ext/matrix_double3x2_precision.hpp"
#include "ext/matrix_double3x3.hpp"
#include "ext/matrix_double3x3_precision.hpp"
#include "ext/matrix_double3x4.hpp"
#include "ext/matrix_double3x4_precision.hpp"
#include "ext/matrix_double4x2.hpp"
#include "ext/matrix_double4x2_precision.hpp"
#include "ext/matrix_double4x3.hpp"
#include "ext/matrix_double4x3_precision.hpp"
#include "ext/matrix_double4x4.hpp"
#include "ext/matrix_double4x4_precision.hpp"

#include "ext/matrix_float2x2.hpp"
#include "ext/matrix_float2x2_precision.hpp"
#include "ext/matrix_float2x3.hpp"
#include "ext/matrix_float2x3_precision.hpp"
#include "ext/matrix_float2x4.hpp"
#include "ext/matrix_float2x4_precision.hpp"
#include "ext/matrix_float3x2.hpp"
#include "ext/matrix_float3x2_precision.hpp"
#include "ext/matrix_float3x3.hpp"
#include "ext/matrix_float3x3_precision.hpp"
#include "ext/matrix_float3x4.hpp"
#include "ext/matrix_float3x4_precision.hpp"
#include "ext/matrix_float4x2.hpp"
#include "ext/matrix_float4x2_precision.hpp"
#include "ext/matrix_float4x3.hpp"
#include "ext/matrix_float4x3_precision.hpp"
#include "ext/matrix_float4x4.hpp"
#include "ext/matrix_float4x4_precision.hpp"

#include "ext/matrix_relational.hpp"

#include "ext/quaternion_double.hpp"
#include "ext/quaternion_double_precision.hpp"
#include "ext/quaternion_float.hpp"
#include "ext/quaternion_float_precision.hpp"
#include "ext/quaternion_geometric.hpp"
#include "ext/quaternion_relational.hpp"

#include "ext/scalar_constants.hpp"
#include "ext/scalar_int_sized.hpp"
#include "ext/scalar_relational.hpp"

#include "ext/vector_bool1.hpp"
#include "ext/vector_bool1_precision.hpp"
#include "ext/vector_bool2.hpp"
#include "ext/vector_bool2_precision.hpp"
#include "ext/vector_bool3.hpp"
#include "ext/vector_bool3_precision.hpp"
#include "ext/vector_bool4.hpp"
#include "ext/vector_bool4_precision.hpp"

#include "ext/vector_double1.hpp"
#include "ext/vector_double1_precision.hpp"
#include "ext/vector_double2.hpp"
#include "ext/vector_double2_precision.hpp"
#include "ext/vector_double3.hpp"
#include "ext/vector_double3_precision.hpp"
#include "ext/vector_double4.hpp"
#include "ext/vector_double4_precision.hpp"

#include "ext/vector_float1.hpp"
#include "ext/vector_float1_precision.hpp"
#include "ext/vector_float2.hpp"
#include "ext/vector_float2_precision.hpp"
#include "ext/vector_float3.hpp"
#include "ext/vector_float3_precision.hpp"
#include "ext/vector_float4.hpp"
#include "ext/vector_float4_precision.hpp"

#include "ext/vector_int1.hpp"
#include "ext/vector_int1_precision.hpp"
#include "ext/vector_int2.hpp"
#include "ext/vector_int2_precision.hpp"
#include "ext/vector_int3.hpp"
#include "ext/vector_int3_precision.hpp"
#include "ext/vector_int4.hpp"
#include "ext/vector_int4_precision.hpp"

#include "ext/vector_relational.hpp"

#include "ext/vector_uint1.hpp"
#include "ext/vector_uint1_precision.hpp"
#include "ext/vector_uint2.hpp"
#include "ext/vector_uint2_precision.hpp"
#include "ext/vector_uint3.hpp"
#include "ext/vector_uint3_precision.hpp"
#include "ext/vector_uint4.hpp"
#include "ext/vector_uint4_precision.hpp"

#include "gtc/bitfield.hpp"
#include "gtc/color_space.hpp"
#include "gtc/constants.hpp"
#include "gtc/epsilon.hpp"
#include "gtc/integer.hpp"
#include "gtc/matrix_access.hpp"
#include "gtc/matrix_integer.hpp"
#include "gtc/matrix_inverse.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/noise.hpp"
#include "gtc/packing.hpp"
#include "gtc/quaternion.hpp"
#include "gtc/random.hpp"
#include "gtc/reciprocal.hpp"
#include "gtc/round.hpp"
#include "gtc/type_precision.hpp"
#include "gtc/type_ptr.hpp"
#include "gtc/ulp.hpp"
#include "gtc/vec1.hpp"

#include "gtx/matrix_decompose.hpp"
#include "gtx/norm.hpp"

#ifdef COMPILER_MSVC
#	pragma warning (pop)
#endif


namespace FG
{	
/*
=================================================
	VertexDesc
=================================================
*/
	template <glm::length_t L, typename T, glm::qualifier Q>
	struct VertexDesc< glm::vec<L, T, Q> > : VertexDesc< Vec<T,L> >
	{};

}	// FG


namespace FGC
{
	using namespace ::glm;

/*
=================================================
	Equals
=================================================
*/
	template <typename T>
	inline bool2  Equals (const glm::tvec2<T> &lhs, const glm::tvec2<T> &rhs, const T &err = std::numeric_limits<T>::epsilon())
	{
		return {Equals( lhs.x, rhs.x, err ),
				Equals( lhs.y, rhs.y, err )};
	}

	template <typename T>
	inline bool3  Equals (const glm::tvec3<T> &lhs, const glm::tvec3<T> &rhs, const T &err = std::numeric_limits<T>::epsilon())
	{
		return {Equals( lhs.x, rhs.x, err ),
				Equals( lhs.y, rhs.y, err ),
				Equals( lhs.z, rhs.z, err )};
	}
	
/*
=================================================
	ToString
=================================================
*/
	template <typename T, int length>
	inline String  ToString (const glm::vec<length, T> &vec)
	{
		String	result = "( ";
		for (int i = 0; i < length; ++i)
		{
			if ( i ) result += ", ";
			result += std::to_string( vec[i] );
		}
		result += " )";
		return result;
	}

/*
=================================================
	Identity
=================================================
*/
namespace _fg_hidden_
{
	template <typename T>
	struct TMatIdentity;

	template <typename T>
	struct TQuatIdentity;
	
	template <length_t C, length_t R, typename T, qualifier Q>
	struct TMatIdentity< mat<C,R,T,Q> >
	{
		static constexpr mat<C,R,T,Q>  Get ()
		{
			mat<C,R,T,Q> result {T(0)};
			for (length_t i = 0, len = Min( C, R ); i < len; ++i) {
				result[i][i] = T(1);
			}
			return result;
		}
	};

	template <typename T>
	struct TQuatIdentity< tquat<T> >
	{
		static constexpr tquat<T>  Get ()
		{
			return tquat<T>{ T(1), T(0), T(0), T(0) };
		}
	};

	struct GLMIdentity final
	{
		constexpr GLMIdentity ()
		{}
		
		template <typename T>
		ND_ constexpr operator T () const
		{
			if constexpr( IsSameTypes< T, tquat<typename T::value_type> > )
				return TQuatIdentity<T>::Get();
			else
				return TMatIdentity<T>::Get();
		}

	};
}	// _fg_hidden_


    static constexpr _fg_hidden_::GLMIdentity	Identity {};

	static constexpr glm::quat					Quat_Identity	= Identity;
	static const     glm::mat4x4				Mat4x4_Identity = Identity;
	static constexpr glm::mat4x4				Mat4x4_One		{ 1.0f };


}	// FGC
