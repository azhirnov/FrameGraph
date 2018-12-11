// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SceneApp.h"
#include "scene/Loader/Assimp/AssimpLoader.h"
#include "scene/Loader/DevIL/DevILLoader.h"
#include "scene/Renderer/Prototype/RendererPrototype.h"
#include "scene/SceneManager/Simple/SimpleScene.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	SceneApp::SceneApp ()
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	SceneApp::~SceneApp ()
	{
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool SceneApp::Initialize (StringView sceneFile, ArrayView<StringView> textureDirs)
	{
		AssimpLoader			loader;
		AssimpLoader::Config	cfg;

		cfg.calculateTBN = true;

		IntermediateScenePtr	temp_scene = loader.Load( cfg, sceneFile );
		CHECK_ERR( temp_scene );

		DevILLoader		img_loader;
		CHECK_ERR( img_loader.Load( temp_scene, textureDirs ));


		renderer	= MakeShared<RendererPrototype>();
		scene		= MakeShared<SimpleScene>();

		//CHECK_ERR( scene->Build( renderer.operator->() ));

		return false;
	}
	
/*
=================================================
	Update
=================================================
*/
	bool SceneApp::Update ()
	{
		return false;
	}


}	// FG
