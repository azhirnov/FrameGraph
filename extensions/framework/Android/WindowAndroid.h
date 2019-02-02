// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"

#ifdef PLATFORM_ANDROID

struct ANativeWindow;
struct AInputEvent;
struct ASensorManager;
struct android_app;

namespace FG
{

	//
	// Android Window
	//

	class WindowAndroid final : public IWindow
	{
	// types
	private:
		struct VulkanSurface final : public IVulkanSurface
		{
		private:
			ANativeWindow *	_window;

		public:
			explicit VulkanSurface (ANativeWindow *wnd) : _window{wnd} {}

			ND_ Array<const char*>	GetRequiredExtensions () const override;
			ND_ VkSurfaceKHR		Create (VkInstance inst) const override;
		};

		using Listeners_t	= HashSet< IWindowEventListener *>;


	// variables
	private:
		android_app *		_application;
		ANativeWindow *		_window;
		ASensorManager*		_sensorManager;
		uint2				_windowSize;
		Listeners_t			_listeners;
		bool				_pause;


	// methods
	public:
		explicit WindowAndroid (android_app* app);
		~WindowAndroid ();

		bool Create (uint2 size, StringView title) override;
		void AddListener (IWindowEventListener *listener) override;
		void RemoveListener (IWindowEventListener *listener) override;
		bool Update () override;
		void Quit () override;
		void Destroy () override;
		
		void SetTitle (StringView) override {}
		void SetSize (const uint2 &) override {}
		void SetPosition (const int2 &) override {}
		
		uint2 GetSize () const override;

		UniquePtr<IVulkanSurface>  GetVulkanSurface () const override;
		

	private:
		static void		_HandleCmd (android_app* app, int32_t cmd);
		static int32_t	_HandleInput (android_app* app, AInputEvent* event);

		void _InitDisplay (ANativeWindow* window);

		ND_ static StringView  _MapKey (int key);
		ND_ static StringView  _MapMouseButton (int button);
	};


}	// FG

#endif	// PLATFORM_ANDROID
