// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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
	public:
		class Model;
		class Light;


	// variables
	private:
		Array<SharedPtr<Model>>		_staticModels;
		Array<SharedPtr<Light>>		_lights;


	// methods
	public:
		SimpleScene ();

		bool Create (const IntermediateScenePtr &);
		
		bool Build (const RenderTechniquePtr &) override;
		void PreDraw (const CameraInfo &, ScenePreRender &) const override;
		void Draw (RenderQueue &) const override;

		AABB  CalculateBoundingVolume () const override		{ return AABB{}; }	// TODO
	};


}	// FG
