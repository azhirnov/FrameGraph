// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
			uint	materialID	: 16;
			uint	objectID	: 16;
		};

		struct Material
		{
			vec3	albedoColor			{1.0f};
			uint	albedoMap			= 0;
			vec3	specularColor		{0.0f};
			uint	specularMap			= 0;
			uint	normalMap			= 0;
			float	roughtness			= 1.0f;
			float	metallic			= 0.0f;
			float	indexOfRefraction	= 1.0f;
			float	opticalDepth		= -1.0f;
			float	_padding1[3];
		};

		struct MeshData
		{
			Array<vec3>				vertices;
			Array<uint>				indices;
			Array<Primitive>		primitives;
			Array<VertexAttrib>		attribs;
		};
		using MeshMap_t = HashMap< GeometryID, MeshData* >;

		using EnabledInstances_t = BitSet< 2 >;


	// variables
	private:
		RTSceneID			_rtScene;
		RTShaderTableID		_shaderTable;
		Array<BufferID>		_primitivesBuffers;
		Array<BufferID>		_attribsBuffers;
		BufferID			_materialsBuffer;
		Array<RawImageID>	_albedoMaps;
		Array<RawImageID>	_normalMaps;
		Array<RawImageID>	_specularMaps;
		PipelineResources	_resources;
		EnabledInstances_t	_enabledInstances;
		AABB				_boundingBox;


	// methods
	public:
		SimpleRayTracingScene ();
		~SimpleRayTracingScene ();
		
		bool Create (const CommandBuffer &, const IntermScenePtr &, const ImageCachePtr &, const Transform & = Default);
		void Destroy (const FrameGraph &) override;
		
		bool Build (const CommandBuffer &, const RenderTechniquePtr &) override;
		void PreDraw (const CameraInfo &, ScenePreRender &) const override;
		void Draw (RenderQueue &) const override;

		AABB  CalculateBoundingVolume () const override		{ return _boundingBox; }

	private:
		bool _CreateMaterials (const CommandBuffer &, const IntermScenePtr &, const ImageCachePtr &);
		bool _CreateGeometries (const CommandBuffer &, const IntermScenePtr &, const Transform &);
		bool _ConvertHierarchy (const IntermScenePtr &, const Transform &, OUT MeshMap_t &) const;
		bool _CreateMesh (const IntermScenePtr &, const IntermScene::ModelData &, const Transform &, uint, INOUT MeshMap_t &) const;

		ND_ Task		_CreateGeometry (const CommandBuffer &, uint index, const MeshData &meshData, OUT RTGeometryID &geom);
		ND_ MeshData*	_ChooseMaterialType (const IntermMaterialPtr &, MeshMap_t &) const;

		bool _UpdateShaderTable (const CommandBuffer &, const RenderTechniquePtr &);
	};


}	// FG
