// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/Loader/Intermediate/IntermScene.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	IntermScene::IntermScene (MaterialMap_t &&materials, MeshMap_t &&meshes, LightMap_t &&lights, SceneNode &&root) :
		_root{ std::move(root) }, _materials{ std::move(materials) },
		_meshes{ std::move(meshes) }, _lights{ std::move(lights) }
	{}
	
/*
=================================================
	constructor
=================================================
*/
	IntermScene::IntermScene (ArrayView<IntermMaterialPtr> materials,
							  ArrayView<IntermMeshPtr> meshes,
							  ArrayView<IntermLightPtr> lights,
							  SceneNode &&root) :
		_root{ std::move(root) }
	{
		for (auto& mtr : materials) {
			_materials.insert({ mtr, uint(_materials.size()) });
		}
		for (auto& mesh : meshes) {
			_meshes.insert({ mesh, uint(_meshes.size()) });
		}
		for (auto& light : lights) {
			_lights.insert({ light, uint(_lights.size()) });
		}
	}

/*
=================================================
	GetIndexOfMesh
=================================================
*/
	uint  IntermScene::GetIndexOfMesh (const IntermMeshPtr &ptr) const
	{
		auto	iter = _meshes.find( ptr );
		return iter != _meshes.end() ? iter->second : UMax;
	}
	
/*
=================================================
	GetIndexOfMaterial
=================================================
*/
	uint  IntermScene::GetIndexOfMaterial (const IntermMaterialPtr &ptr) const
	{
		auto	iter = _materials.find( ptr );
		return iter != _materials.end() ? iter->second : UMax;
	}
	
/*
=================================================
	RecursiveFindNode
=================================================
*
	ND_ static IntermScene::SceneNode const*  RecursiveFindNode (const IntermScene::SceneNode &)
	{
	}

/*
=================================================
	FindNode
=================================================
*
	IntermScene::SceneNode const*  IntermScene::FindNode (StringView name) const
	{

	}

/*
=================================================
	Append
=================================================
*/
	bool IntermScene::Append (const IntermScene &other, const Transform &transform)
	{
		for (auto& mtr : other._materials) {
			_materials.insert({ mtr.first, uint(_materials.size()) });
		}
		for (auto& mesh : other._meshes) {
			_meshes.insert({ mesh.first, uint(_meshes.size()) });
		}
		for (auto& light : other._lights) {
			_lights.insert({ light.first, uint(_lights.size()) });
		}

		_root.nodes.push_back( other.GetRoot() );
		_root.nodes.back().localTransform += transform;

		return true;
	}


}	// FG
