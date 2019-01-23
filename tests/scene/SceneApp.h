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
	// Scene Tests Application
	//

	class SceneApp final : public IViewport, public IWindowEventListener
	{
	// variables
	private:
		VulkanDeviceExt			vulkan;
		WindowPtr				window;
		
		FGInstancePtr			fgInstance;
		FGThreadPtr				frameGraph;
		IPipelineCompilerPtr	pplnCompiler;

		RenderTechniquePtr		renderTech;
		SceneManagerPtr			scene;

		
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
	};

}	// FG
