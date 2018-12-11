// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/App/BaseApp.h"
#include "scene/Renderer/IRenderTechnique.h"
#include "scene/SceneManager/ISceneHierarchy.h"

namespace FG
{

	//
	// Scene Tests Application
	//

	class SceneApp final : public BaseApp
	{
	private:
		RenderTechniquePtr		renderer;
		SceneHierarchyPtr		scene;


	public:
		SceneApp ();
		~SceneApp ();

		bool Initialize (StringView sceneFile, ArrayView<StringView> textureDirs);
		bool Update ();
	};

}	// FG
