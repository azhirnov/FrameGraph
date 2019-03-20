// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"

#ifdef FG_ENABLE_GLFW
# include "glfw/include/GLFW/glfw3.h"

namespace FGC
{

	//
	// GLFW Window
	//

	class WindowGLFW final : public IWindow
	{
	// types
	private:
		struct VulkanSurface final : public IVulkanSurface
		{
		private:
			GLFWwindow *	_window;

		public:
			explicit VulkanSurface (GLFWwindow *wnd) : _window{wnd} {}

			ND_ ArrayView<const char*>	GetRequiredExtensions () const override;
			ND_ VkSurfaceKHR			Create (VkInstance inst) const override;
		};

		using Listeners_t	= HashSet< IWindowEventListener *>;


	// variables
	private:
		GLFWwindow *	_window;
		Listeners_t		_listeners;


	// methods
	public:
		WindowGLFW ();
		~WindowGLFW ();

		bool Create (uint2 size, StringView title) override;
		void AddListener (IWindowEventListener *listener) override;
		void RemoveListener (IWindowEventListener *listener) override;
		bool Update () override;
		void Quit () override;
		void Destroy () override;
		
		void SetTitle (StringView value) override;
		void SetSize (const uint2 &value) override;
		void SetPosition (const int2 &value) override;
		
		uint2 GetSize () const override;

		UniquePtr<IVulkanSurface>  GetVulkanSurface () const override;
		

	private:
		static void _GLFW_ErrorCallback (int code, const char* msg);
		static void _GLFW_RefreshCallback (GLFWwindow* wnd);
		static void _GLFW_ResizeCallback (GLFWwindow* wnd, int w, int h);
		static void _GLFW_KeyCallback (GLFWwindow* wnd, int key, int, int, int);
		static void _GLFW_MouseButtonCallback (GLFWwindow* wnd, int button, int action, int mods);
		static void _GLFW_CursorPos (GLFWwindow* wnd, double xpos, double ypos);

		ND_ static StringView  _MapKey (int key);
		ND_ static StringView  _MapMouseButton (int button);
	};


}	// FGC

#endif	// FG_ENABLE_GLFW
