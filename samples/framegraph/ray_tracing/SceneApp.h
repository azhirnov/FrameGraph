// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/IRenderTechnique.h"
#include "scene/SceneManager/ISceneManager.h"
#include "scene/SceneManager/IViewport.h"
#include "scene/Utils/Ext/FPSCamera.h"
#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDeviceExt.h"
#include <chrono>

namespace FG
{

	//
	// Scene Application
	//

	class SceneApp final : public IViewport, public IWindowEventListener
	{
	// types
	private:
		using TimePoint_t	= std::chrono::high_resolution_clock::time_point;
		using SecondsF		= std::chrono::duration< float >;


	// variables
	private:
		VulkanDeviceExt			_vulkan;
		WindowPtr				_window;
		
		FGInstancePtr			_fgInstance;
		FGThreadPtr				_frameGraph;

		RenderTechniquePtr		_renderTech;
		SceneManagerPtr			_scene;

		FPSCamera				_camera;
		vec3					_positionDelta;
		vec2					_mouseDelta;
		vec2					_lastMousePos;
		TimePoint_t				_lastUpdateTime;
		bool					_mousePressed	= false;

		static constexpr float	_velocity		= 1.0f;
		
		struct {
			Nanoseconds				gpuTimeSum				{0};
			Nanoseconds				cpuTimeSum				{0};
			TimePoint_t				lastUpdateTime;
			uint					frameCounter			= 0;
			const uint				UpdateIntervalMillis	= 500;
		}						_frameStat;

		
	// methods
	public:
		SceneApp ();
		~SceneApp ();

		bool Initialize ();
		bool Update ();
		

	// IWindowEventListener
	private:
		void OnRefresh () override {}
		void OnDestroy () override {}
		void OnUpdate () override {}
		void OnResize (const uint2 &size) override;
		void OnKey (StringView, EKeyAction) override;
		void OnMouseMove (const float2 &) override;


	// IViewport //
	private:
		void Prepare (ScenePreRender &) override;
		void Draw (RenderQueue &) const override;
		void AfterRender (const FGThreadPtr &, const Present &) override;


	private:
		bool _CreateFrameGraph ();

		void _UpdateFrameStat ();

		ND_ SceneHierarchyPtr  _LoadScene2 () const;
	};

}	// FG
