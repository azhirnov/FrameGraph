// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	https://en.wikipedia.org/wiki/List_of_refractive_indices
	https://en.wikipedia.org/wiki/Optical_depth
*/

#pragma once

#include "scene/Renderer/IRenderTechnique.h"
#include "scene/SceneManager/ISceneManager.h"
#include "scene/BaseSceneApp.h"

namespace FG
{

	//
	// Ray Tracing Application
	//

	class SceneApp final : public BaseSceneApp
	{
	// variables
	private:
		RenderTechniquePtr		_renderTech;
		SceneManagerPtr			_scene;

		
	// methods
	public:
		SceneApp () {}
		~SceneApp ();

		bool Initialize ();


	// BaseSceneApp
	private:
		bool DrawScene () override;
		

	// IWindowEventListener
	private:
		void OnKey (StringView, EKeyAction) override;


	// IViewport //
	private:
		void Prepare (ScenePreRender &) override;
		void Draw (RenderQueue &) const override;


	private:
		ND_ SceneHierarchyPtr  _LoadScene2 (const CommandBuffer &) const;
	};


}	// FG
