// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/IntermImage.h"
#include "scene/Renderer/Enums.h"
#include "scene/Math/GLM.h"

namespace FG
{

	//
	// Intermediate Material
	//

	class IntermMaterial final : public std::enable_shared_from_this<IntermMaterial>
	{
	// types
	public:
		enum class ETextureMapping : uint
		{
			UV,
			Sphere,
			Cylinder,
			Box,
			Plane,
			Unknown = ~0u,
		};
		
		struct MtrTexture
		{
			IntermImagePtr			image;
			String					name;
			mat3x3					uvTransform		= Default;	// TODO: set identity
			ETextureMapping			mapping			= Default;
			EAddressMode			addressModeU	= Default;
			EAddressMode			addressModeV	= Default;
			EAddressMode			addressModeW	= Default;
			EFilter					filter			= Default;
			uint					uvIndex			= UMax;
		};

		using Parameter = Union< NullUnion, float, RGBA32f, MtrTexture >;

		struct Settings
		{
			String		name;

			Parameter	albedo;				// texture or color		// UE4 'base color'
			Parameter	specular;			// texture or color		// UE4 'specular'
			Parameter	ambient;			// texture or color
			Parameter	emissive;			// texture or color
			Parameter	heightMap;			// texture
			Parameter	normalMap;			// texture
			Parameter	shininess;			// texture or float
			Parameter	opacity;			// textire or float
			Parameter	displacementMap;	// texture
			Parameter	lightMap;			// texture
			Parameter	reflectionMap;		// texture
			Parameter	roughtness;			// texture or float		// UE4 'roughtness'
			Parameter	metallic;			// texture or float		// UE4 'metallic'
			Parameter	subsurface;			// texture or color		// UE4 'subsurface color'
			Parameter	ambientOcclusion;	// texture				// UE4 'ambient occlusion'
			Parameter	refraction;			// texture or float		// UE4 'index of refraction'

			float		opticalDepth		= 0.0f;					//... exp( -distance * opticalDepth )
			float		shininessStrength	= 0.0f;
			float		alphaTestReference	= 0.0f;
			ECullMode	cullMode			= Default;
		};


	// variables
	private:
		Settings		_settings;
		LayerBits		_layers;


	// methods
	public:
		IntermMaterial () {}

		explicit IntermMaterial (Settings &&settings, LayerBits layers) :
			_settings{ std::move(settings) }, _layers{ layers }
		{}

		ND_ Settings const&		GetSettings ()		const	{ return _settings; }
		ND_ Settings &			EditSettings ()				{ return _settings; }
		ND_ LayerBits			GetRenderLayers ()	const	{ return _layers; }

		void SetRenderLayers (LayerBits value)				{ _layers = value; }
	};


}	// FG
