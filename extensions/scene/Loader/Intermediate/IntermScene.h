// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/IntermMesh.h"
#include "scene/Loader/Intermediate/IntermMaterial.h"
#include "scene/Loader/Intermediate/IntermLight.h"
#include "scene/Utils/Math/Transform.h"
#include "scene/Renderer/Enums.h"

namespace FG
{

	//
	// Intermediate Scene
	//

	class IntermScene final : public std::enable_shared_from_this<IntermScene>
	{
	// types
	public:
		struct ModelData
		{
			StaticArray< Pair<IntermMeshPtr, IntermMaterialPtr>, uint(EDetailLevel::_Count) >	levels;
		};

		using NodeData_t	= Union< NullUnion, ModelData >;

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
		SceneNode					_root;
		Array< IntermMaterialPtr >	_materials;
		Array< IntermMeshPtr >		_meshes;
		Array< IntermLightPtr >		_lights;


	// methods
	public:
		IntermScene () {}

		IntermScene (Array<IntermMaterialPtr> &&materials,
					 Array<IntermMeshPtr> &&meshes,
					 Array<IntermLightPtr> &&lights,
					 SceneNode &&root);

		ND_ ArrayView<IntermMaterialPtr>	GetMaterials ()	const	{ return _materials; }
		ND_ ArrayView<IntermMeshPtr>		GetMeshes ()	const	{ return _meshes; }
		ND_ ArrayView<IntermLightPtr>		GetLights ()	const	{ return _lights; }
		ND_ SceneNode const&				GetRoot ()		const	{ return _root; }

		ND_ size_t  GetIndexOfMesh (const IntermMeshPtr &) const;
		ND_ size_t  GetIndexOfMaterial (const IntermMaterialPtr &) const;
	};
	
	using IntermScenePtr = SharedPtr< IntermScene >;

	
/*
=================================================
	constructor
=================================================
*/
	inline IntermScene::IntermScene (Array<IntermMaterialPtr> &&materials,
									 Array<IntermMeshPtr> &&meshes,
									 Array<IntermLightPtr> &&lights,
									 SceneNode &&root) :
		_root{ std::move(root) }, _materials{ std::move(materials) },
		_meshes{ std::move(meshes) }, _lights{ std::move(lights) }
	{}
	
/*
=================================================
	GetIndexOfMesh
=================================================
*/
	inline size_t  IntermScene::GetIndexOfMesh (const IntermMeshPtr &ptr) const
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
	inline size_t  IntermScene::GetIndexOfMaterial (const IntermMaterialPtr &ptr) const
	{
		// TODO: binary search
		for (size_t i = 0; i < _materials.size(); ++i) {
			if ( _materials[i] == ptr )
				return i;
		}
		return UMax;
	}


}	// FG
