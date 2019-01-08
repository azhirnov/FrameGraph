// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/VFG.h"

namespace FG
{

	using RenderTechniquePtr	= SharedPtr< class IRenderTechnique >;
	using SceneHierarchyPtr		= SharedPtr< class ISceneHierarchy >;
	using SceneManagerPtr		= SharedPtr< class ISceneManager >;
	using ViewportPtr			= SharedPtr< class IViewport >;
	using ImageCachePtr			= SharedPtr< class IImageCache >;
	
	class ScenePreRender;
	class RenderQueue;
	struct CameraInfo;


}	// FG
