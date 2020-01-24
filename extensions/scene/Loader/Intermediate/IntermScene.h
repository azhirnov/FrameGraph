// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/IntermMesh.h"
#include "scene/Loader/Intermediate/IntermMaterial.h"
#include "scene/Loader/Intermediate/IntermLight.h"
#include "scene/Math/Transform.h"
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

		using MaterialMap_t	= HashMap< IntermMaterialPtr, uint >;
		using MeshMap_t		= HashMap< IntermMeshPtr, uint >;
		using LightMap_t	= HashMap< IntermLightPtr, uint >;


	// variables
	private:
		SceneNode			_root;
		MaterialMap_t		_materials;
		MeshMap_t			_meshes;
		LightMap_t			_lights;


	// methods
	public:
		IntermScene () {}
		IntermScene (MaterialMap_t &&materials, MeshMap_t &&meshes, LightMap_t &&lights, SceneNode &&root);
		IntermScene (ArrayView<IntermMaterialPtr> materials,
					 ArrayView<IntermMeshPtr> meshes,
					 ArrayView<IntermLightPtr> lights,
					 SceneNode &&root);

		ND_ MaterialMap_t const&	GetMaterials ()	const	{ return _materials; }
		ND_ MeshMap_t const&		GetMeshes ()	const	{ return _meshes; }
		ND_ LightMap_t const&		GetLights ()	const	{ return _lights; }
		ND_ SceneNode const&		GetRoot ()		const	{ return _root; }

		ND_ uint  GetIndexOfMesh (const IntermMeshPtr &) const;
		ND_ uint  GetIndexOfMaterial (const IntermMaterialPtr &) const;

		ND_ SceneNode const*  FindNode (StringView name) const;

		bool Append (const IntermScene &, const Transform &transform = Default);
	};


}	// FG
