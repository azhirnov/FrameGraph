// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SceneApp.h"
#include "scene/Loader/Assimp/AssimpLoader.h"
#include "scene/Loader/DevIL/DevILLoader.h"
#include "scene/Renderer/Prototype/RendererPrototype.h"
#include "scene/SceneManager/Simple/SimpleRayTracingScene.h"
#include "scene/SceneManager/DefaultSceneManager.h"

namespace FG
{
	
/*
=================================================
	destructor
=================================================
*/
	SceneApp::~SceneApp ()
	{
		if ( _scene )
		{
			_scene->Destroy( _frameGraph );
			_scene = null;
		}

		if ( _renderTech )
		{
			_renderTech->Destroy();
			_renderTech = null;
		}
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool SceneApp::Initialize ()
	{
		// will crash if it is not created as shared pointer
		ASSERT( shared_from_this() );

		CHECK_ERR( _CreateFrameGraph( uint2(1024, 768), "Ray tracing", {}, FG_DATA_PATH "_debug_output" ));


		// upload resource data
		auto	cmdbuf = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });
		{
			auto	renderer	= MakeShared<RendererPrototype>();
			auto	scene		= MakeShared<DefaultSceneManager>();
				
			CHECK_ERR( scene->Create( cmdbuf ));
			_scene = scene;

			CHECK_ERR( renderer->Create( _frameGraph ));
			_renderTech = renderer;

			CHECK_ERR( _scene->Add( _LoadScene2( cmdbuf )) );
		}
		CHECK_ERR( _frameGraph->Execute( cmdbuf ));
		CHECK_ERR( _frameGraph->Flush() );
		

		// create pipelines
		CHECK_ERR( _scene->Build( _renderTech ));
		CHECK_ERR( _frameGraph->Flush() );

		GetFPSCamera().SetPosition({ -0.06f, 0.29f, 0.93f });
		return true;
	}
	
/*
=================================================
	_LoadScene2
=================================================
*/
	SceneHierarchyPtr  SceneApp::_LoadScene2 (const CommandBuffer &cmdbuf) const
	{
		AssimpLoader			loader;
		AssimpLoader::Config	cfg;

		IntermScenePtr	sponza = loader.Load( cfg, FG_DATA_PATH "../_data/sponza/sponza.obj" );
		CHECK_ERR( sponza );
		
		DevILLoader		img_loader;
		CHECK_ERR( img_loader.Load( sponza, {FG_DATA_PATH "../_data/sponza"}, _scene->GetImageCache() ));
		
		IntermScenePtr	bunny = loader.Load( cfg, FG_DATA_PATH "../_data/bunny/bunny.obj" );
		CHECK_ERR( bunny );
		
		// setup material
		{
			auto&	mtr = bunny->GetMaterials().begin()->first->EditSettings();
			mtr.albedo			= RGBA32f{ 0.8f, 0.8f, 1.0f, 1.0f };
			mtr.opticalDepth	= 2.6f;
			mtr.refraction		= 1.31f;	// ice
		}

		Transform	transform;
		transform.scale = 20.0f;
		transform.position.y += 4.5f;
		sponza->Append( *bunny, transform );

		transform.scale	= 0.01f;
		transform.position = vec3(0.0f);

		auto	hierarchy = MakeShared<SimpleRayTracingScene>();
		CHECK_ERR( hierarchy->Create( cmdbuf, sponza, _scene->GetImageCache(), transform ));

		return hierarchy;
	}

/*
=================================================
	DrawScene
=================================================
*/
	bool SceneApp::DrawScene ()
	{
		_scene->Draw({ shared_from_this() });
		return true;
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void SceneApp::OnKey (StringView key, EKeyAction action)
	{
		BaseSceneApp::OnKey( key, action );
	}

/*
=================================================
	Prepare
=================================================
*/
	void SceneApp::Prepare (ScenePreRender &preRender)
	{
		_UpdateCamera();

		LayerBits	layers;
		layers[uint(ERenderLayer::RayTracing)] = true;

		preRender.AddCamera( GetCamera(), GetSurfaceSizeF(), GetViewRange(), DetailLevelRange{}, ECameraType::Main, layers, shared_from_this() );
	}
	
/*
=================================================
	Draw
=================================================
*/
	void SceneApp::Draw (RenderQueue &) const
	{
		// TODO: draw UI
	}

}	// FG

/*
=================================================
	main
=================================================
*/
int main ()
{
	using namespace FG;
	
	auto	scene = MakeShared<SceneApp>();

	CHECK_ERR( scene->Initialize(), -1 );

	for (; scene->Update();) {}

	return 0;
}
