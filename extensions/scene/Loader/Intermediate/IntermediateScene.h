// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/IntermediateMesh.h"
#include "scene/Loader/Intermediate/IntermediateMaterial.h"
#include "scene/Loader/Intermediate/IntermediateLight.h"
#include "scene/Utils/Math/Transform.h"

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
		SceneNode						_root;
		Array<IntermediateMaterialPtr>	_materials;
		Array<IntermediateMeshPtr>		_meshes;
		Array<IntermediateLightPtr>		_lights;


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

}	// FG
