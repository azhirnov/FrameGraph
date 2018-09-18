// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"

#ifdef FG_ENABLE_GLFW
# include "glfw/include/GLFW/glfw3.h"

namespace FG
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

			ND_ Array<const char*>	GetRequiredExtensions () const override;
			ND_ VkSurfaceKHR		Create (VkInstance inst) const override;
		};


	// variables
	private:
	    GLFWwindow *			_window;
		IWindowEventListener *	_listener;


	// methods
	public:
		WindowGLFW ();
		~WindowGLFW ();

		bool Create (uint2 size, StringView title, IWindowEventListener *listener) override;
		bool Update () override;
		void Quit () override;
		void Destroy () override;
		
		void SetTitle (StringView value) override;
		
		uint2 GetSize () const override;

		UniquePtr<IVulkanSurface>  GetVulkanSurface () const override;
		

	private:
		static void _GLFW_ErrorCallback (int code, const char* msg);
		static void _GLFW_RefreshCallback (GLFWwindow* wnd);
		static void _GLFW_ResizeCallback (GLFWwindow* wnd, int w, int h);
		static void _GLFW_KeyCallback (GLFWwindow* wnd, int key, int, int, int);
	};


}	// FG

#endif	// FG_ENABLE_GLFW
