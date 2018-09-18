// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"

#ifdef FG_ENABLE_SDL2
# ifdef COMPILER_MSVC
#	pragma warning (push)
#	pragma warning (disable: 4005)	// macros redefinition
#	pragma warning (disable: 4668)	// '...' is not defined as a preprocessor macro
# endif

#	define SDL_MAIN_HANDLED
#	include "SDL2/include/SDL.h"

# ifdef COMPILER_MSVC
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

		bool Create (uint2 size, StringView title, IWindowEventListener *listener) override;
		bool Update () override;
		void Quit () override;
		void Destroy () override;
		
		void SetTitle (StringView value) override;
		
		uint2 GetSize () const override;

		UniquePtr<IVulkanSurface>  GetVulkanSurface () const override;
	};


}	// FG

#endif	// FG_ENABLE_SDL2
