// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Utils/Math/GLM.h"

namespace FG
{

	//
	// Transformation
	//

	template <typename T>
	struct Transformation
	{
	// types
	public:
		using Value_t	= T;
		using Vec3_t	= glm::tvec3< T >;
		using Quat_t	= glm::tquat< T >;
		using Mat4_t	= glm::tmat4x4< T >;
		using Self		= Transformation< T >;


	// variables
	public:
		Quat_t		orientation		{ T(1), T(0), T(0), T(0) };
		Vec3_t		position		{ T(0) };
		Value_t		scale			{ T(1) };


	// methods
	public:
		Transformation () {}
		Transformation (const Self &tr) = default;

		Transformation (const Vec3_t &pos, const Quat_t &orient, const T &scale = T(1)) :
			orientation{orient}, position{pos}, scale{scale} {}

		explicit Transformation (const Mat4_t &mat);
		
			Self &	operator += (const Self &rhs);
		ND_ Self	operator +  (const Self &rhs)	const	{ return Self{*this} += rhs; }
		
			Self &	operator -= (const Self &rhs)			{ return Self{*this} += rhs.Inversed(); }
		ND_ Self	operator -  (const Self &rhs)	const	{ return Self{*this} -= rhs; }

		ND_ bool	operator == (const Self &rhs)	const;
		ND_ bool	operator != (const Self &rhs)	const	{ return not (*this == rhs); }

			Self &	Move (const Vec3_t &delta);
			Self &	Rotate (const Quat_t &delta);
			Self &	Scale (const Vec3_t &scale);

			Self &	Inverse ();
		ND_ Self	Inversed ()	const						{ return Self{*this}.Inverse(); }

		ND_ Mat4_t	ToMatrix () const;


		// local space to global
		ND_ Vec3_t	ToGlobalVector (const Vec3_t &local)	const;
		ND_ Vec3_t	ToGlobalPosition (const Vec3_t &local)	const	{ return ToGlobalVector( local ) + position; }

		// global space to local
		ND_ Vec3_t	ToLocalVector (const Vec3_t &global)	const;
		ND_ Vec3_t	ToLocalPosition (const Vec3_t &global)	const	{ return ToLocalVector( global - position ); }
	};

	
	using Transform = Transformation<float>;

	
/*
=================================================
	constructor (mat4x4)
=================================================
*/
	template <typename T>
	inline Transformation<T>::Transformation (const Mat4_t &mat)
	{
		Vec3_t		scale3;
		Vec3_t		skew;
		tvec4<T>	perspective;
		glm::decompose( mat, OUT scale3, OUT orientation, OUT position, OUT skew, OUT perspective );

		ASSERT( Equals( scale3.x, scale3.y ) and Equals( scale3.x, scale3.z ) );
		scale = scale3.x;
	}
	
/*
=================================================
	operator +=
=================================================
*/
	template <typename T>
	inline Transformation<T>&  Transformation<T>::operator += (const Self &rhs)
	{
		position	+= orientation * (rhs.position * scale);
		orientation	*= rhs.orientation;
		scale		*= rhs.scale;
		return *this;
	}
	
/*
=================================================
	operator ==
=================================================
*/
	template <typename T>
	inline bool Transformation<T>::operator == (const Self &rhs) const
	{
		return	orientation			== rhs.orientation	and
				glm::all( position	== rhs.position )	and
				glm::all( scale		== rhs.scale );
	}
	
/*
=================================================
	Move
=================================================
*/
	template <typename T>
	inline Transformation<T>&  Transformation<T>::Move (const Vec3_t &delta)
	{
		position += orientation * (delta * scale);
		return *this;
	}
	
/*
=================================================
	Rotate
=================================================
*/
	template <typename T>
	inline Transformation<T>&  Transformation<T>::Rotate (const Quat_t &delta)
	{
		orientation *= delta;
		return *this;
	}
	
/*
=================================================
	Scale
=================================================
*/
	template <typename T>
	inline Transformation<T>&  Transformation<T>::Scale (const Vec3_t &scale)
	{
		this->scale *= scale;
		return *this;
	}
	
/*
=================================================
	Inverse
=================================================
*/
	template <typename T>
	inline Transformation<T>&  Transformation<T>::Inverse ()
	{
		orientation.Inverse();
		scale		= T(1) / scale;
		position	= orientation * (-position * scale);
		return *this;
	}
	
/*
=================================================
	ToGlobalVector
=================================================
*/
	template <typename T>
	inline typename Transformation<T>::Vec3_t
		Transformation<T>::ToGlobalVector (const Vec3_t &local) const
	{
		return orientation * (local * scale);
	}
	
/*
=================================================
	ToLocalVector
=================================================
*/
	template <typename T>
	inline typename Transformation<T>::Vec3_t
		Transformation<T>::ToLocalVector (const Vec3_t &global) const
	{
		return (orientation.Inversed() * global) / scale;
	}
	
/*
=================================================
	ToMatrix
=================================================
*/
	template <typename T>
	inline typename Transformation<T>::Mat4_t  Transformation<T>::ToMatrix () const
	{
		Mat4_t	result = glm::mat4_cast( orientation );
		
		result[3] = tvec4<T>{ position, T(1) };

		return result * glm::scale(Vec3_t{ scale, scale, scale });
	}


}	// FG
