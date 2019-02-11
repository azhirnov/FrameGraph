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

		struct MeshData
		{
			Array<vec3>			vertices;
			Array<uint>			indices;
			Array<Primitive>	primitives;
			Array<VertexAttrib>	attribs;
		};


	// variables
	private:
		RTSceneID			_rtScene;
		RTShaderTableID		_shaderTable;
		BufferID			_primitivesBuffer;
		BufferID			_attribsBuffer;
		PipelineResources	_resources;

		AABB				_boundingBox;


	// methods
	public:
		SimpleRayTracingScene ();
		
		bool Create (const FGThreadPtr &, const IntermScenePtr &, const ImageCachePtr &, const Transform & = Default);
		void Destroy (const FGThreadPtr &);
		
		bool Build (const FGThreadPtr &, const RenderTechniquePtr &) override;
		void PreDraw (const CameraInfo &, ScenePreRender &) const override;
		void Draw (RenderQueue &) const override;

		AABB  CalculateBoundingVolume () const override		{ return _boundingBox; }

	private:
		bool _CreateGeometry (const FGThreadPtr &, const IntermScenePtr &, const Transform &);
		bool _ConvertHierarchy (const IntermScenePtr &, const Transform &, OUT MeshData &) const;
		bool _CreateMesh (const IntermScenePtr &, const IntermScene::ModelData &, const Transform &, INOUT MeshData &) const;

		bool _UpdateShaderTable (const FGThreadPtr &, const RenderTechniquePtr &);
	};


}	// FG
