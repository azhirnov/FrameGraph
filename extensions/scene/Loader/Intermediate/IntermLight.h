// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Common.h"
#include "scene/Utils/Math/GLM.h"

namespace FG
{

	//
	// Intermediate Light
	//

	class IntermLight final : public std::enable_shared_from_this<IntermLight>
	{
	// types
	public:
		enum class ELightType
		{
			None,
			Point,
			Spot,
			Directional,
			Ambient,
			Area,
		};

		struct Settings
		{
			vec3			position;
			vec3			direction;
			vec3			upDirection		= {0.0f, 0.0f, 1.0f};

			vec3			attenuation;
			ELightType		type			= ELightType::None;
			bool			castShadow		= false;

			vec3			diffuseColor;
			vec3			specularColor;
			vec3			ambientColor;

			vec2			coneAngleInnerOuter;
		};


	// variables
	private:
		Settings		_settings;


	// methods
	public:
		IntermLight () {}

		explicit IntermLight (Settings &&settings) : _settings{std::move(settings)} {}
	};


}	// FG
