// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/SceneManager/ISceneHierarchy.h"
#include "scene/Loader/Intermediate/IntermediateScene.h"

namespace FG
{

	//
	// Simple Ray Tracing Scene
	//

	class SimpleRayTracingScene final : public ISceneHierarchy
	{
	// variables
	private:
		AABB					_boundingBox;


	// methods
	public:
		SimpleRayTracingScene ();

		bool Create (const FGThreadPtr &, const IntermediateScenePtr &, const ImageCachePtr &);
		void Destroy (const FGThreadPtr &);
		
		bool Build (const FGThreadPtr &, const RenderTechniquePtr &) override;
		void PreDraw (const CameraInfo &, ScenePreRender &) const override;
		void Draw (RenderQueue &) const override;

		AABB  CalculateBoundingVolume () const override		{ return _boundingBox; }

	};


}	// FG
