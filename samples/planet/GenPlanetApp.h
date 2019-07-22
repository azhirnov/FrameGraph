// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "SphericalCube.h"
#include "scene/BaseSceneApp.h"

namespace FG
{

	//
	// Generated Planet Application
	//

	class GenPlanetApp final : public BaseSceneApp
	{
	// types
	private:
		struct PlanetData
		{
			mat4x4		viewProj;
			vec4		position;
			vec2		clipPlanes;
			float		_padding[2];

			vec3		lightDirection;
			float		tessLevel;
		};

		struct PlanetEditorPC
		{
			float2		center;
			float		radius;
		};


	// variables
	private:
		ImageID					_depthBuffer;

		struct {
			SphericalCube			cube;
			ImageID					heightMap;
			ImageID					normalMap;
			ImageID					dbgColorMap;
			BufferID				ubuffer;
			GPipelineID				pipeline;
			PipelineResources		resources;
		}						_planet;

		struct {
			CPipelineID				pipeline;
			PipelineResources		resources;
		}						_generator;

		RawSamplerID			_linearSampler;

		
	// methods
	public:
		GenPlanetApp () {}
		~GenPlanetApp ();

		bool Initialize ();


	// BaseSceneApp
	private:
		bool DrawScene () override;
		

	// IWindowEventListener
	private:
		void OnResize (const uint2 &size) override;
		void OnKey (StringView, EKeyAction) override;


	private:
		bool _CreatePlanet (const CommandBuffer &);
		bool _GenerateHeightMap (const CommandBuffer &);
		
		ND_ static String  _LoadShader (StringView filename);
	};

}	// FG
