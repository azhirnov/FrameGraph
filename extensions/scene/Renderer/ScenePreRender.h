// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/Enums.h"
#include "scene/Utils/Math/Camera.h"
#include "scene/Utils/Math/Frustum.h"
#include "scene/SceneManager/ISceneHierarchy.h"

namespace FG
{

	struct CameraInfo
	{
		Camera				camera;
		Frustum				frustum;
		vec2				visibilityRange;
		vec2				viewportSize;
		LayerBits_t			layers;
		DetailLevelRange	detailLevel;
		ECameraType			cameraType		= Default;
	};



	//
	// Scene Pre Pender
	//
	class ScenePreRender final
	{
		friend class IRenderTechnique;

	// types
	private:
		using ScenePtr = SharedPtr< const ISceneHierarchy >;


		struct CameraData : CameraInfo
		{
			static constexpr uint	MaxDependencies = 16;
			using Dependencies_t	= FixedArray< CameraData const*, MaxDependencies >;
			using Scenes_t			= HashSet< ScenePtr >;

			Dependencies_t		dependsOn;
			ViewportPtr			viewport;
			Scenes_t			scenes;
			// TODO: post effect settings
		};
		using CameraArray_t		= Deque< CameraData >;


		struct StackFrame
		{
			CameraData *		camera	= null;
			ScenePtr			scene	= null;
		};
		using CallStack_t		= Array< StackFrame >;
		
		using Listeners_t		= HashSet< ScenePtr >;


	// variables
	private:
		CameraArray_t	_cameras;
		Listeners_t		_listeners;
		CallStack_t		_stack;


	// methods
	public:
		ScenePreRender () {}

		void AddListener (const ScenePtr &scene);
		
		void AddScene (const ScenePtr &scene);

		void AddCamera (const Camera &data, const vec2 &range, DetailLevelRange detail,
						ECameraType type, LayerBits_t layers, const ViewportPtr &vp = null);
	};

	
	
/*
=================================================
	AddListener
----
	add scene hierarchy as listener to 'AddCamera' event
=================================================
*/
	inline void ScenePreRender::AddListener (const ScenePtr &scene)
	{
		ASSERT( scene );

		_listeners.insert( scene );
	}
	
/*
=================================================
	AddScene
=================================================
*/
	inline void ScenePreRender::AddScene (const ScenePtr &scene)
	{
		ASSERT( _stack.size() );
		ASSERT( scene );

		_stack.back().camera->scenes.insert( scene );
	}

/*
=================================================
	AddCamera
=================================================
*/
	inline void ScenePreRender::AddCamera (const Camera &data, const vec2 &range, DetailLevelRange detail,
										   ECameraType type, LayerBits_t layers, const ViewportPtr &vp)
	{
		ScenePtr	prev;
		auto&		curr = _cameras.emplace_back();

		curr.camera				= data;
		curr.visibilityRange	= range;
		curr.layers				= layers;
		curr.detailLevel		= detail;
		curr.cameraType			= type;
		curr.viewport			= vp;
		curr.frustum.Setup( curr.camera, curr.visibilityRange );

		if ( _stack.size() )
		{
			prev = _stack.back().scene;

			if ( _stack.back().camera )
				curr.dependsOn.push_back( _stack.back().camera );
		}

		auto&	frame	= _stack.emplace_back();
		frame.camera = &curr;

		for (auto& scene : _listeners)
		{
			if ( prev == scene )
				continue;

			frame.scene = scene;

			scene->PreDraw( curr, *this );
		}

		_stack.pop_back();
	}


}	// FG
