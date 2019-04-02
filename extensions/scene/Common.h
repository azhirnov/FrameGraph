// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/FG.h"

namespace FG
{

	using RenderTechniquePtr	= SharedPtr< class IRenderTechnique >;
	using SceneHierarchyPtr		= SharedPtr< class ISceneHierarchy >;
	using SceneManagerPtr		= SharedPtr< class ISceneManager >;
	using ViewportPtr			= SharedPtr< class IViewport >;
	using ImageCachePtr			= SharedPtr< class IImageCache >;
	
	using IntermImagePtr		= SharedPtr< class IntermImage >;
	using IntermLightPtr		= SharedPtr< class IntermLight >;
	using IntermMaterialPtr		= SharedPtr< class IntermMaterial >;
	using IntermMeshPtr			= SharedPtr< class IntermMesh >;
	using IntermScenePtr		= SharedPtr< class IntermScene >;
	using VertexAttributesPtr	= SharedPtr< class VertexAttributes >;

	class ScenePreRender;
	class RenderQueue;
	struct CameraInfo;


}	// FG
