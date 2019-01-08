// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/IntermediateMesh.h"
#include "scene/Loader/Intermediate/IntermediateMaterial.h"
#include "scene/Loader/Intermediate/IntermediateLight.h"
#include "scene/Utils/Math/Transform.h"
#include "scene/Renderer/Enums.h"

namespace FG
{

	//
	// Intermediate Scene
	//

	class IntermediateScene final : public std::enable_shared_from_this<IntermediateScene>
	{
	// types
	public:
		struct MeshNode
		{
			IntermediateMeshPtr			mesh;
			IntermediateMaterialPtr		material;
			EDetailLevel				detail		= EDetailLevel::High;

			MeshNode () {}
		};

		using NodeData_t	= Union< std::monostate, MeshNode >;

		struct SceneNode
		{
			Transform			localTransform;
			String				name;
			Array<NodeData_t>	data;
			Array<SceneNode>	nodes;

			SceneNode () {}
		};


	// variables
	private:
		SceneNode							_root;
		Array< IntermediateMaterialPtr >	_materials;
		Array< IntermediateMeshPtr >		_meshes;
		Array< IntermediateLightPtr >		_lights;


	// methods
	public:
		IntermediateScene () {}

		IntermediateScene (Array<IntermediateMaterialPtr> &&materials,
						   Array<IntermediateMeshPtr> &&meshes,
						   Array<IntermediateLightPtr> &&lights,
						   SceneNode &&root);

		ND_ ArrayView<IntermediateMaterialPtr>	GetMaterials ()	const	{ return _materials; }
		ND_ ArrayView<IntermediateMeshPtr>		GetMeshes ()	const	{ return _meshes; }
		ND_ ArrayView<IntermediateLightPtr>		GetLights ()	const	{ return _lights; }
		ND_ SceneNode const&					GetRoot ()		const	{ return _root; }

		ND_ size_t  GetIndexOfMesh (const IntermediateMeshPtr &) const;
		ND_ size_t  GetIndexOfMaterial (const IntermediateMaterialPtr &) const;
	};
	
	using IntermediateScenePtr = SharedPtr< IntermediateScene >;

	
/*
=================================================
	constructor
=================================================
*/
	inline IntermediateScene::IntermediateScene (Array<IntermediateMaterialPtr> &&materials,
												 Array<IntermediateMeshPtr> &&meshes,
												 Array<IntermediateLightPtr> &&lights,
												 SceneNode &&root) :
		_root{ std::move(root) }, _materials{ std::move(materials) },
		_meshes{ std::move(meshes) }, _lights{ std::move(lights) }
	{}
	
/*
=================================================
	GetIndexOfMesh
=================================================
*/
	inline size_t  IntermediateScene::GetIndexOfMesh (const IntermediateMeshPtr &ptr) const
	{
		// TODO: binary search
		for (size_t i = 0; i < _meshes.size(); ++i) {
			if ( _meshes[i] == ptr )
				return i;
		}
		return UMax;
	}
	
/*
=================================================
	GetIndexOfMaterial
=================================================
*/
	inline size_t  IntermediateScene::GetIndexOfMaterial (const IntermediateMaterialPtr &ptr) const
	{
		// TODO: binary search
		for (size_t i = 0; i < _materials.size(); ++i) {
			if ( _materials[i] == ptr )
				return i;
		}
		return UMax;
	}


}	// FG
