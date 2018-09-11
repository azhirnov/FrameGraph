// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "framework/Window/IWindow.h"

#ifdef FG_ENABLE_SDL2
# ifdef _MSC_VER
#	pragma warning (push)
#	pragma warning (disable: 4005)	// macros redefinition
#	pragma warning (disable: 4668)	// '...' is not defined as a preprocessor macro
# endif

#	define SDL_MAIN_HANDLED
#	include "SDL2/include/SDL.h"

# ifdef _MSC_VER
#	pragma warning (pop)
# endif


namespace FG
{

	//
	// SDL2 Window
	//

	class WindowSDL2 final : public IWindow
	{
	// types
	private:
		struct VulkanSurface final : public IVulkanSurface
		{
		private:
			SDL_Window *	_window;

		public:
			explicit VulkanSurface (SDL_Window *wnd) : _window{wnd} {}

			ND_ Array<const char*>	GetRequiredExtensions () const override;
			ND_ VkSurfaceKHR		Create (VkInstance inst) const override;
		};


	// variables
	private:
		SDL_Window *			_window;
		uint					_wndID;
		IWindowEventListener *	_listener;


	// methods
	public:
		WindowSDL2 ();
		~WindowSDL2 ();


	private:
		bool Create (uint2 size, StringView caption, IWindowEventListener *listener) override;
		bool Update () override;
		void Quit () override;
		void Destroy () override;

		UniquePtr<IVulkanSurface>  GetVulkanSurface () const override;
	};


}	// FG

#endif	// FG_ENABLE_SDL2
