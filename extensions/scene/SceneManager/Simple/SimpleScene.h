// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/SceneManager/ISceneHierarchy.h"
#include "scene/Loader/Intermediate/IntermediateScene.h"

namespace FG
{

	//
	// Simple Scene
	//

	class SimpleScene final : public ISceneHierarchy
	{
	// types
	private:
		struct Instance
		{
			Transform		transform;
			AABB			boundingBox;
			uint			index			= UMax;		// in '_perInstanceUB' and in '_modelLODs'
		};

		struct Model
		{
			GPipelineID			pipeline;
			PipelineResources	resources;
			uint				materialID		= UMax;		// in '_materials'
			uint				meshID			= UMax;		// in '_meshes'
			ERenderLayer		layer			= Default;

			Model () {}
			Model (const Model &other) : materialID{other.materialID}, meshID{other.meshID} { ASSERT( not other.pipeline ); }
		};

		struct Mesh
		{
			AABB			boundingBox;
			EPrimitive		topology		= Default;
			ECullMode		cullMode		= Default;
			uint			attribsIndex	= UMax;		// in '_vertexInput'
			uint			patchSize		= 0;		// for tessellation
			uint			vertexOffset	= 0;
			uint			indexCount		= 0;
			uint			firstIndex		= 0;
		};

		struct Material
		{
			RawImageID		albedoTex;
			RawImageID		specularTex;
			RawImageID		roughtnessTex;
			RawImageID		metallicTex;
			ETextureType	textureBits		= Default;
			uint			dataID			= UMax;		// in '_materialsUB'
		};
		
		using VertexAttribs_t	= Array< VertexInputState >;
		using ModelsRange_t		= uint2;	// first, count in '_models'
		using RenderLayers_t	= StaticArray< ModelsRange_t, uint(ERenderLayer::_Count) >;
		using DetailLevels_t	= StaticArray< RenderLayers_t, uint(EDetailLevel::_Count) >;


	// variables
	private:
		DetailLevelRange		_detailLevel;
		AABB					_boundingBox;
		
		Array< Instance >		_instances;
		Array<DetailLevels_t>	_modelLODs;
		Array< Model >			_models;
		Array< Mesh >			_meshes;
		Array< Material >		_materials;
		
		VertexAttribs_t			_vertexAttribs;
		BufferID				_vertexBuffer;
		BufferID				_indexBuffer;
		BufferID				_perInstanceUB;
		BufferID				_materialsUB;
		EIndex					_indexType		= Default;


	// methods
	public:
		SimpleScene ();

		bool Create (const FGThreadPtr &, const IntermediateScenePtr &, const ImageCachePtr &);
		void Destroy (const FGThreadPtr &);
		
		bool Build (const FGThreadPtr &, const RenderTechniquePtr &) override;
		void PreDraw (const CameraInfo &, ScenePreRender &) const override;
		void Draw (RenderQueue &) const override;

		AABB  CalculateBoundingVolume () const override		{ return _boundingBox; }


	private:
		bool _ConvertMeshes (const FGThreadPtr &, const IntermediateScenePtr &);
		bool _ConvertMaterials (const FGThreadPtr &, const IntermediateScenePtr &, const ImageCachePtr &);
		bool _ConvertHierarchy (const IntermediateScenePtr &);
		bool _UpdatePerObjectUniforms (const FGThreadPtr &);
		bool _BuildModels (const FGThreadPtr &, const RenderTechniquePtr &);
		bool _CreateMesh (const Transform &, const IntermediateScenePtr &, const IntermediateScene::MeshNode &);
	};


}	// FG
