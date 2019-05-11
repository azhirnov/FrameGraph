// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/IRenderTechnique.h"
#include "scene/SceneManager/ISceneManager.h"
#include "scene/BaseSceneApp.h"

namespace FG
{

	//
	// Scene Tests Application
	//

	class SceneApp final : public BaseSceneApp
	{
	// variables
	private:
		RenderTechniquePtr		_renderTech;
		SceneManagerPtr			_scene;

		
	// methods
	public:
		SceneApp ();
		~SceneApp ();

		bool Initialize ();


	// BaseSceneApp
	private:
		bool DrawScene () override;

	// IViewport //
	private:
		void Prepare (ScenePreRender &) override;
		void Draw (RenderQueue &) const override;
	};

}	// FG
