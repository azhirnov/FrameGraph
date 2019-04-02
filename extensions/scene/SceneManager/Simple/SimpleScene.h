// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/SceneManager/ISceneHierarchy.h"
#include "scene/Loader/Intermediate/IntermScene.h"

namespace FG
{

	//
	// Simple Scene Hierarchy
	//

	class SimpleScene final : public ISceneHierarchy
	{
	// types
	private:
		struct Instance
		{
			Transform		transform;
			AABB			boundingBox;
			uint			index			= UMax;		// in '_modelLODs'
			uint			lastIndex		= 0;
		};

		struct Model
		{
			RawGPipelineID		pipeline;
			PipelineResources	resources;
			uint				materialID		= UMax;		// in '_materials'
			uint				meshID			= UMax;		// in '_meshes'
			ERenderLayer		layer			= Default;

			Model () {}
		};

		struct Mesh
		{
			AABB			boundingBox;
			EPrimitive		topology		= Default;
			ECullMode		cullMode		= Default;
			uint			attribsIndex	= UMax;		// in '_vertexAttribs'
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
		
		using ModelLevels_t	= StaticArray< uint, uint(EDetailLevel::_Count) >;

		struct ModelLevel
		{
			ModelLevels_t	levels;				// in '_models'
			ERenderLayer	layer	= Default;

			//ND_ bool operator > (const ModelLevel &) const;
		};

		using VertexAttribs_t	= Array< VertexAttributesPtr >;
		using DetailLevels_t	= Array< ModelLevel >;


	// variables
	private:
		DetailLevelRange		_detailLevel;
		AABB					_boundingBox;
		
		Array< Instance >		_instances;
		DetailLevels_t			_modelLODs;
		Array< Model >			_models;
		Array< Mesh >			_meshes;
		Array< Material >		_materials;
		
		VertexAttribs_t			_vertexAttribs;
		BufferID				_vertexBuffer;
		BufferID				_indexBuffer;
		BufferID				_perInstanceUB;
		BufferID				_materialsUB;
		BytesU					_vertexStride;
		EIndex					_indexType		= Default;


	// methods
	public:
		SimpleScene ();

		bool Create (const FrameGraph &, const CommandBuffer &, const IntermScenePtr &, const ImageCachePtr &, const Transform & = Default);
		void Destroy (const FrameGraph &) override;
		
		bool Build (const CommandBuffer &, const RenderTechniquePtr &) override;
		void PreDraw (const CameraInfo &, ScenePreRender &) const override;
		void Draw (RenderQueue &) const override;

		AABB  CalculateBoundingVolume () const override		{ return _boundingBox; }


	private:
		bool _ConvertMeshes (const FrameGraph &, const CommandBuffer &, const IntermScenePtr &);
		bool _ConvertMaterials (const FrameGraph &, const CommandBuffer &, const IntermScenePtr &, const ImageCachePtr &);
		bool _ConvertHierarchy (const IntermScenePtr &, const Transform &);
		bool _UpdatePerObjectUniforms (const FrameGraph &);
		bool _BuildModels (const FrameGraph &, const RenderTechniquePtr &);
		bool _CreateMesh (const Transform &, const IntermScenePtr &, const IntermScene::ModelData &);
	};


}	// FG
