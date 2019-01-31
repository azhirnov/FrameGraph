// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/SceneManager/Simple/SimpleRayTracingScene.h"
#include "scene/SceneManager/IImageCache.h"
#include "scene/Renderer/ScenePreRender.h"
#include "scene/Renderer/RenderQueue.h"
#include "scene/Renderer/IRenderTechnique.h"
#include "scene/Loader/Intermediate/IntermMesh.h"
#include "framegraph/Shared/EnumUtils.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	SimpleRayTracingScene::SimpleRayTracingScene ()
	{
	}
	
/*
=================================================
	Create
=================================================
*/
	bool SimpleRayTracingScene::Create (const FGThreadPtr &, const IntermScenePtr &, const ImageCachePtr &)
	{
		return false;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void SimpleRayTracingScene::Destroy (const FGThreadPtr &)
	{
	}
	
/*
=================================================
	Build
=================================================
*/
	bool SimpleRayTracingScene::Build (const FGThreadPtr &, const RenderTechniquePtr &)
	{
		return false;
	}
	
/*
=================================================
	PreDraw
=================================================
*/
	void SimpleRayTracingScene::PreDraw (const CameraInfo &, ScenePreRender &) const
	{
	}
	
/*
=================================================
	Draw
=================================================
*/
	void SimpleRayTracingScene::Draw (RenderQueue &) const
	{
	}


}	// FG
