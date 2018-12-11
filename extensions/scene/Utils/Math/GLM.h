// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Common.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_CXX14
#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_SWIZZLE

#ifdef COMPILER_MSVC
#	pragma warning (push)
#	pragma warning (disable: 4201)
#	pragma warning (disable: 4127)
#endif

#include "glm.hpp"
#include "ext.hpp"
#include "gtx/matrix_decompose.hpp"

#ifdef COMPILER_MSVC
#	pragma warning (pop)
#endif


namespace FG
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

}	// FG
