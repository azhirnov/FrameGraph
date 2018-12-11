// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/SceneManager/Simple/SimpleScene.h"
#include "scene/SceneManager/Simple/SimpleScene.Model.h"
#include "scene/Renderer/ScenePreRender.h"
#include "scene/Renderer/RenderQueue.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	SimpleScene::SimpleScene ()
	{
	}
	
/*
=================================================
	Create
=================================================
*/
	bool SimpleScene::Create (const IntermediateScenePtr &scene)
	{


		return false;
	}

/*
=================================================
	Build
=================================================
*/
	bool SimpleScene::Build (const RenderTechniquePtr &)
	{
		return false;
	}
	
/*
=================================================
	PreDraw
=================================================
*/
	void SimpleScene::PreDraw (const CameraInfo &, ScenePreRender &preRender) const
	{
		preRender.AddScene( shared_from_this() );
	}

/*
=================================================
	Draw
=================================================
*/
	void SimpleScene::Draw (RenderQueue &queue) const
	{

	}


}	// FG
