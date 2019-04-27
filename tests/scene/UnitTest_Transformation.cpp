// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/Math/Transform.h"
#include "UnitTest_Common.h"


static quat QRotateX (float angle)
{
	return rotate(quat(1.0f, 0.0f, 0.0f, 0.0f), angle, vec3{1.0f, 0.0f, 0.0f});
}

static quat QRotateY (float angle)
{
	return rotate(quat(1.0f, 0.0f, 0.0f, 0.0f), angle, vec3{0.0f, 1.0f, 0.0f});
}

static quat QRotateZ (float angle)
{
	return rotate(quat(1.0f, 0.0f, 0.0f, 0.0f), angle, vec3{0.0f, 0.0f, 1.0f});
}

static quat QRotate (float angleX, float angleY, float angleZ)
{
	return QRotateX(angleX) * QRotateY(angleY) * QRotateZ(angleZ);
}


static void Transformation_Test1 ()
{
	quat		q = QRotateX( radians(45.0f) );
	vec3		p = vec3{10.0f, -3.0f, 2.5f};
	mat4x4		m = translate( Mat4x4_One, p ) * mat4_cast(q);

	Transform	t{ m };

	TEST( Equals( q.x, t.orientation.x ));
	TEST( Equals( q.y, t.orientation.y ));
	TEST( Equals( q.z, t.orientation.z ));
	TEST( Equals( q.w, t.orientation.w ));
	
	TEST( Equals( p.x, t.position.x ));
	TEST( Equals( p.y, t.position.y ));
	TEST( Equals( p.z, t.position.z ));
	
	TEST( Equals( 1.0f, t.scale ));
}


static void Transformation_Test2 ()
{
	const Transform	tr{ vec3{1.0f, 2.0f, 3.0f},
						QRotate(45.0f, 0.0f, 10.0f),
						2.0f };

	const mat4x4	mat = tr.ToMatrix();

	const vec3		point0{ -1.0f, -1.0f, -1.0f };
	const vec3		point1{ 1.0f, 1.0f, 1.0f };

	const vec3		mat_point0	= vec3(mat * vec4( point0, 1.0f ));
	const vec3		mat_point1	= vec3(mat * vec4( point1, 1.0f ));

	const vec3		tr_point0	= tr.ToGlobalPosition( point0 );
	const vec3		tr_point1	= tr.ToGlobalPosition( point1 );

	TEST( Equals( mat_point0.x, tr_point0.x ));
	TEST( Equals( mat_point0.y, tr_point0.y ));
	TEST( Equals( mat_point0.z, tr_point0.z ));

	TEST( Equals( mat_point1.x, tr_point1.x ));
	TEST( Equals( mat_point1.y, tr_point1.y ));
	TEST( Equals( mat_point1.z, tr_point1.z ));
}


extern void UnitTest_Transformation ()
{
	Transformation_Test1();
	Transformation_Test2();

	FG_LOGI( "UnitTest_Transformation - passed" );
}
