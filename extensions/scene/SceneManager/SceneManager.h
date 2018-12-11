// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/SceneManager/ISceneManager.h"

namespace FG
{

	//
	// Scene Manager
	//

	class SceneManager final : public ISceneManager
	{
	// variables
	private:
		RenderTechniquePtr			_renderTech;
		HashSet<SceneHierarchyPtr>	_hierarchies;


	// methods
	public:
		SceneManager ();

		bool Build (const RenderTechniquePtr &) override;
		bool Draw (ArrayView<ViewportPtr>) override;

		bool Add (const SceneHierarchyPtr &) override;
		bool Remove (const SceneHierarchyPtr &) override;
	};


}	// FG
