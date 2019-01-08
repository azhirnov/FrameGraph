// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
	DefaultSceneManager::DefaultSceneManager () :
		_imageCache{ MakeShared<DefaultImageCache>() }
	{
	}

/*
=================================================
	Build
=================================================
*/
	bool DefaultSceneManager::Build (const FGThreadPtr &fg, const RenderTechniquePtr &renderTech)
	{
		CHECK_ERR( renderTech );

		for (auto& scene : _hierarchies)
		{
			CHECK_ERR( scene->Build( fg, renderTech ));
		}

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
			if ( not scene->Build( null, _renderTech ) )	// TODO
			{
				_hierarchies.erase( iter );
				RETURN_ERR( "building scene failed" );
			}
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
