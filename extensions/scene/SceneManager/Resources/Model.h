// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/IntermMesh.h"
#include "scene/SceneManager/Resources/Material.h"
#include "scene/Math/AABB.h"

namespace FG
{
/*
	//
	// Model
	//

	class Model final : public IDrawable3D
	{
	// types
	private:
		struct Mesh
		{
			BytesU			vbOffset;
			EPrimitive		topology		= Default;
			uint			attribsIndex	= UMax;
			uint			patchSize		= 0;		// for tessellation

			BytesU			ibOffset;
			EIndex			indexType		= Default;
			uint			indexCount		= 0;

			Mesh () {}
		};

		struct SubModel
		{
			MaterialPtr		mtr;
			uint			meshIndex		= UMax;
			ERenderLayer	layer			= Default;
			uint			detailLevel		= 0;

			SubModel () {}
		};

		using VertexAttribs_t	= Array< VertexInputState >;
		using Meshes_t			= Array< Mesh >;
		using Models_t			= Array< SubModel >;
		using ModelsRange_t		= ArrayView< SubModel >;
		using RenderLayers_t	= StaticArray< ModelsRange_t, uint(ERenderLayer::_Count) >;
		using DetailLevels_t	= StaticArray< RenderLayers_t, uint(EDetailLevel::_Count) >;


	// variables
	private:
		VertexInputState	_vertexInput;
		BufferID			_vertexBuffer;
		BufferID			_indexBuffer;

		Models_t			_subModels;
		DetailLevels_t		_levels;		// LODs[] { layers[] { models[] } }
		DetailLevelRange	_detailLevel;

		AABB				_boundingBox;


	// methods
	public:
		Model ();



		bool Build (const IRenderTechnique *) override;
		void Draw (const RenderQueue &, const Transform &) const override;
	};

	using ModelPtr = SharedPtr< Model >;
	*/

}	// FG
