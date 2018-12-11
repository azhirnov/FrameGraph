// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/SceneManager/ICamera.h"

namespace FG
{

	//
	// FPS Camera
	//

	class FpsCamera final : public ICamera
	{
	// variables
	private:


	// methods
	public:
		FpsCamera ();
		
		bool  IsVisible (const AABB &) const override;
		bool  IsVisible (const Sphere &) const override;

		float  MaxVisibilityRange () const override;
		uint2  MinMaxDetailLevel () const override;
	};


}	// FG
