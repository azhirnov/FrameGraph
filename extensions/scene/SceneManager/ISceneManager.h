// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	
*/

#pragma once

#include "scene/Common.h"

namespace FG
{

	//
	// Scene Manager interface
	//

	class ISceneManager : public std::enable_shared_from_this<ISceneManager>
	{
	// interface
	public:
		virtual void Destroy (const FGThreadPtr &) = 0;

		virtual bool Build (const RenderTechniquePtr &) = 0;
		virtual bool Draw (ArrayView<ViewportPtr>) = 0;

		virtual bool Add (const SceneHierarchyPtr &) = 0;
		virtual bool Remove (const SceneHierarchyPtr &) = 0;

		//virtual bool Add (const EntityPtr &) = 0;
		//virtual bool Remove (const EntityPtr &) = 0;

		ND_ virtual ImageCachePtr  GetImageCache () = 0;
	};


}	// FG
