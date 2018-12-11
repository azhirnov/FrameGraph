// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/IntermediateImage.h"
#include "scene/Utils/Math/GLM.h"

namespace FG
{

	//
	// Intermediate Material
	//

	class IntermediateMaterial final : public std::enable_shared_from_this<IntermediateMaterial>
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
			IntermediateImagePtr	image;
			String					name;
			mat3x3					uvTransform;
			ETextureMapping			mapping			= Default;
			EAddressMode			addressModeU	= Default;
			EAddressMode			addressModeV	= Default;
			EAddressMode			addressModeW	= Default;
			EFilter					filter			= Default;
			uint					uvIndex			= ~0u;
		};

		struct Settings
		{
			String		name;

			Optional<MtrTexture>	diffuseMap;
			Optional<MtrTexture>	specularMap;
			Optional<MtrTexture>	ambientMap;
			Optional<MtrTexture>	emissiveMap;
			Optional<MtrTexture>	heightMap;
			Optional<MtrTexture>	normalsMap;
			Optional<MtrTexture>	shininessMap;
			Optional<MtrTexture>	opacityMap;
			Optional<MtrTexture>	displacementMap;
			Optional<MtrTexture>	lightMap;
			Optional<MtrTexture>	reflectionMap;

			// TODO: parameters for PBR

			float4		diffuseColor;
			float4		specularColor;
			float4		ambientColor;
			float4		emissiveColor;
			float		shininess			= 0.0f;
			float		shininessStrength	= 0.0f;
			float		roughness			= 0.0f;
			float		opacity				= 0.0f;
			float		alphaTestReference	= 0.0f;
		};


	// variables
	private:
		Settings	_settings;


	// methods
	public:
		IntermediateMaterial () {}

		explicit IntermediateMaterial (Settings &&settings) : _settings{ std::move(settings) } {}

		ND_ Settings const&		GetSettings ()	const	{ return _settings; }
		ND_ Settings &			EditSettings ()			{ return _settings; }
	};
	
	using IntermediateMaterialPtr = SharedPtr< IntermediateMaterial >;


}	// FG
