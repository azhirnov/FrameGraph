// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/SceneManager/DefaultSceneManager.h"
#include "scene/SceneManager/DefaultImageCache.h"
#include "scene/SceneManager/ISceneHierarchy.h"
#include "scene/SceneManager/IViewport.h"
#include "scene/Renderer/IRenderTechnique.h"
#include "scene/Renderer/ScenePreRender.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	DefaultSceneManager::DefaultSceneManager ()
	{
	}
	
/*
=================================================
	Create
=================================================
*/
	bool DefaultSceneManager::Create (const CommandBuffer &cmdbuf)
	{
		CHECK_ERR( cmdbuf );

		Destroy( cmdbuf->GetFrameGraph() );

		auto	img_cache = MakeShared<DefaultImageCache>();
		CHECK_ERR( img_cache->Create( cmdbuf ));

		_imageCache = img_cache;
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void DefaultSceneManager::Destroy (const FrameGraph &fg)
	{
		if ( not fg )
			return;

		for (auto& scene : _hierarchies)
		{
			scene->Destroy( fg );
		}

		if ( _imageCache )
			_imageCache->Destroy( fg );

		_hierarchies.clear();
		_imageCache = null;
		_renderTech	= null;
	}

/*
=================================================
	Build
=================================================
*/
	bool DefaultSceneManager::Build (const RenderTechniquePtr &renderTech)
	{
		CHECK_ERR( renderTech );

		auto	fg  = renderTech->GetFrameGraph();
		auto	cmd = fg->Begin( CommandBufferDesc{ EQueueType::Graphics });

		for (auto& scene : _hierarchies)
		{
			CHECK_ERR( scene->Build( cmd, renderTech ));
		}

		CHECK_ERR( fg->Execute( cmd ));

		_renderTech = renderTech;
		return true;
	}
	
/*
=================================================
	Draw
=================================================
*/
	bool DefaultSceneManager::Draw (ArrayView<ViewportPtr> viewports)
	{
		CHECK_ERR( viewports.size() > 0 );
		CHECK_ERR( _renderTech );	// call 'Build' at first

		// prepare
		ScenePreRender	pre_render;
		
		for (auto& scene : _hierarchies) {
			pre_render.AddListener( scene );
		}

		for (auto& vp : viewports) {
			vp->Prepare( pre_render );
		}

		// draw
		CHECK_ERR( _renderTech->Render( pre_render ));
		return true;
	}
	
/*
=================================================
	Add
=================================================
*/
	bool DefaultSceneManager::Add (const SceneHierarchyPtr &scene)
	{
		CHECK_ERR( scene );

		auto[iter, inserted] = _hierarchies.insert( scene );

		if ( inserted and _renderTech )
		{
			auto	fg  = _renderTech->GetFrameGraph();
			auto	cmd = fg->Begin( CommandBufferDesc{ EQueueType::Graphics });

			if ( not scene->Build( cmd, _renderTech ))
			{
				_hierarchies.erase( iter );
				RETURN_ERR( "building scene failed" );
			}

			CHECK_ERR( fg->Execute( cmd ));
		}

		return inserted;
	}
	
/*
=================================================
	Remove
=================================================
*/
	bool DefaultSceneManager::Remove (const SceneHierarchyPtr &scene)
	{
		return _hierarchies.erase( scene ) == 1;
	}


}	// FG
