// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/SceneManager/ISceneManager.h"

namespace FG
{

	//
	// Default Scene Manager
	//

	class DefaultSceneManager final : public ISceneManager
	{
	// variables
	private:
		RenderTechniquePtr			_renderTech;
		HashSet<SceneHierarchyPtr>	_hierarchies;
		ImageCachePtr				_imageCache;


	// methods
	public:
		DefaultSceneManager ();
		
		bool Create (const FrameGraph &);
		void Destroy (const FrameGraph &) override;

		bool Build (const RenderTechniquePtr &) override;
		bool Draw (ArrayView<ViewportPtr>) override;

		bool Add (const SceneHierarchyPtr &) override;
		bool Remove (const SceneHierarchyPtr &) override;
		
		ImageCachePtr  GetImageCache () override	{ return _imageCache; }
	};


}	// FG
