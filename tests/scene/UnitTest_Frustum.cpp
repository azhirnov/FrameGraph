// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/Utils/Math/Frustum.h"
#include "UnitTest_Common.h"


static void Frustum_Test1 ()
{
	Camera		camera;
	Frustum		frustum;
	AABB		bbox;
	
	camera.SetPerspective( 60.0_deg, 1.5f, vec2(0.1f, 100.0f) );
	frustum.Setup( camera );
	
	TEST( frustum.IsVisible( vec3{0.0f, 0.0f, 10.0f} ));
	TEST( frustum.IsVisible( vec3{5.0f, 0.0f, 10.0f} ));
	TEST( frustum.IsVisible( vec3{0.0f, 30.0f, 90.0f} ));
	TEST( frustum.IsVisible( vec3{0.0f, 0.0f, 0.1f} ));
	TEST( not frustum.IsVisible( vec3{0.0f, 0.0f, 110.0f} ));
	TEST( not frustum.IsVisible( vec3{0.0f, 0.0f, -10.0f} ));
	TEST( not frustum.IsVisible( vec3{0.0f, 10.0f, 10.0f} ));
	TEST( not frustum.IsVisible( vec3{0.0f, -10.0f, 10.0f} ));
	TEST( not frustum.IsVisible( vec3{ 10.0f, 0.0f, 10.0f} ));
	TEST( not frustum.IsVisible( vec3{-10.0f, 0.0f, 10.0f} ));

	bbox.SetExtent(vec3{ 2.0f }).SetCenter(vec3{ 0.0f, 0.0f, 10.0f });
	TEST( frustum.IsVisible( bbox ));

	bbox.SetCenter(vec3{ 0.0f, 0.0f, -10.0f });
	TEST( not frustum.IsVisible( bbox ));

	vec3	rays[4];
	TEST( frustum.GetRays( OUT rays[0], OUT rays[1], OUT rays[2], OUT rays[3] ));

	TEST( All(Equals( rays[0], vec3{ 0.6f, -0.4f, -0.69f}, 0.05f )) );
	TEST( All(Equals( rays[1], vec3{ 0.6f,  0.4f, -0.69f}, 0.05f )) );
	TEST( All(Equals( rays[2], vec3{-0.6f, -0.4f, -0.69f}, 0.05f )) );
	TEST( All(Equals( rays[3], vec3{-0.6f,  0.4f, -0.69f}, 0.05f )) );
}


static void Frustum_Test2 ()
{
	Camera		camera;
	Frustum		frustum;
	AABB		bbox;
	
	camera.SetPerspective( 60.0_deg, 1.5f, vec2(0.1f, 100.0f) )
		  .Move(vec3{ 100.0f, 0.0f, 50.0f }).Rotate( 180.0_deg, vec3{0.0f, 1.0f, 0.0f});

	frustum.Setup( camera );

	TEST( frustum.IsVisible( camera.transform.position + vec3{0.0f, 0.0f, -10.0f} ));
	TEST( not frustum.IsVisible( camera.transform.position + vec3{0.0f, 0.0f, 10.0f} ));
}


static void Frustum_Test3 ()
{
	Camera		camera;
	Frustum		frustum1;
	Frustum		frustum2;
	
	camera.SetPerspective( 60.0_deg, 1.5f, vec2(0.1f, 100.0f) );
	frustum1.Setup( camera );

	camera.Move(vec3{ 50.0f, 0.0f, 50.0f }).Rotate( -90.0_deg, vec3{0.0f, 1.0f, 0.0f});
	frustum2.Setup( camera );

	TEST( frustum1.IsVisible( frustum2 ));

	camera = Camera{};
	camera.SetPerspective( 60.0_deg, 1.5f, vec2(0.1f, 100.0f) ).Move(vec3{0.0f, 0.0f, -2.0f}).Rotate( 180_deg, vec3{0.0f, 1.0f, 0.0f});
	frustum2.Setup( camera );
	
	// TODO
	//TEST( not frustum1.IsVisible( frustum2 ));
}


extern void UnitTest_Frustum ()
{
	Frustum_Test1();
	Frustum_Test2();
	Frustum_Test3();

	FG_LOGI( "UnitTest_Frustum - passed" );
}
