// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/SceneManager/ISceneHierarchy.h"
#include "scene/Loader/Intermediate/IntermScene.h"

namespace FG
{

	//
	// Simple Ray Tracing Scene
	//

	class SimpleRayTracingScene final : public ISceneHierarchy
	{
	// types
	private:
		struct VertexAttrib
		{
			vec3	normal;
			float	_padding1;
			vec2	texcoord0;
			vec2	texcoord1;
		};

		struct Primitive
		{
			uvec3	face;
			uint	materialID;
		};

		struct Material
		{
			vec3	albedoColor		{1.0f};
			uint	albedoMap		= 0;
			vec3	specularColor	{0.0f};
			uint	specularMap		= 0;
			uint	normalMap		= 0;
			float	roughtness		= 1.0f;
			float	metallic		= 0.0f;
			float	_padding1;
		};

		struct MaterialsData
		{
			HashMap< IntermImagePtr, uint >		albedoMaps;
			HashMap< IntermImagePtr, uint >		normalMaps;
		};

		struct MeshData
		{
			Array<vec3>				vertices;
			Array<uint>				indices;
			Array<Primitive>		primitives;
			Array<VertexAttrib>		attribs;
		};


	// variables
	private:
		RTSceneID			_rtScene;
		RTShaderTableID		_shaderTable;
		BufferID			_primitivesBuffer;
		BufferID			_attribsBuffer;
		BufferID			_materialsBuffer;
		Array<RawImageID>	_albedoMaps;
		Array<RawImageID>	_normalMaps;
		Array<RawImageID>	_specularMaps;
		PipelineResources	_resources;

		AABB				_boundingBox;


	// methods
	public:
		SimpleRayTracingScene ();
		
		bool Create (const FGThreadPtr &, const IntermScenePtr &, const ImageCachePtr &, const Transform & = Default);
		void Destroy (const FGThreadPtr &) override;
		
		bool Build (const FGThreadPtr &, const RenderTechniquePtr &) override;
		void PreDraw (const CameraInfo &, ScenePreRender &) const override;
		void Draw (RenderQueue &) const override;

		AABB  CalculateBoundingVolume () const override		{ return _boundingBox; }

	private:
		bool _CreateMaterials (const FGThreadPtr &, const IntermScenePtr &, const ImageCachePtr &);
		bool _CreateGeometry (const FGThreadPtr &, const IntermScenePtr &, const Transform &);
		bool _ConvertHierarchy (const IntermScenePtr &, const Transform &, OUT MeshData &) const;
		bool _CreateMesh (const IntermScenePtr &, const IntermScene::ModelData &, const Transform &, INOUT MeshData &) const;

		bool _UpdateShaderTable (const FGThreadPtr &, const RenderTechniquePtr &);
	};


}	// FG
