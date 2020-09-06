// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDevice.h"
#include "ui/ImguiRenderer.h"
#include <chrono>

namespace FG
{

	//
	// UI Test Application
	//

	class UIApp final : public IWindowEventListener
	{
	// types
	private:
		using SecondsF		= std::chrono::duration< float >;
		using TimePoint_t	= std::chrono::high_resolution_clock::time_point;
		using KeyStates_t	= StaticArray< EKeyAction, 3 >;


	// variables
	private:
		VulkanDeviceInitializer	_vulkan;
		WindowPtr				_window;
		FrameGraph				_frameGraph;
		SwapchainID				_swapchainId;

		ImguiRenderer			_uiRenderer;

		TimePoint_t				_lastUpdateTime;
		RGBA32f					_clearColor;
		
		KeyStates_t				_mouseJustPressed;
		float2					_lastMousePos;


	// methods
	public:
		UIApp ();

		static void  Run ();


	// IWindowEventListener
	private:
		void  OnResize (const uint2 &size) override;
		void  OnRefresh () override {}
		void  OnDestroy () override {}
		void  OnUpdate () override {}
		void  OnKey (StringView, EKeyAction) override;
		void  OnMouseMove (const float2 &pos) override;
		

	private:
		bool  _Initialize (WindowPtr &&wnd);
		bool  _Update ();
		void  _Destroy ();

		bool  _UpdateInput ();
		bool  _UpdateUI ();
		bool  _Draw ();
	};


}	// FG
