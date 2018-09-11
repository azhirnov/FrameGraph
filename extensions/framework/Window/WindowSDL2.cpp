// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "framework/Window/WindowSDL2.h"
#include "stl/include/Singleton.h"

#ifdef FG_ENABLE_SDL2
#	include "SDL2/include/SDL_syswm.h"

namespace FG
{
namespace {

	struct SDL2Instance
	{
		uint	refCounter	= 0;
		bool	initialized	= false;
	};
}

/*
=================================================
	constructor
=================================================
*/
	WindowSDL2::WindowSDL2 () :
		_window{ null },
		_wndID{ 0 },
		_listener{ null }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	WindowSDL2::~WindowSDL2 ()
	{
		Destroy();
	}
	
/*
=================================================
	Create
=================================================
*/
	bool WindowSDL2::Create (uint2 surfaceSize, StringView caption, IWindowEventListener *listener)
	{
		auto&	inst = *Singleton<SDL2Instance>();

		if ( not inst.initialized )
		{
			CHECK( SDL_Init( SDL_INIT_EVERYTHING ) == 0 );
			inst.initialized = true;
		}

		++inst.refCounter;

		//const int	count	= SDL_GetNumVideoDisplays();
		//const int	disp_idx= 0;

		//SDL_Rect	area	= {};
		//CHECK( SDL_GetDisplayUsableBounds( disp_idx, OUT &area ) );

		//const int2	pos		= int2(area.x + area.w/2 - surfaceSize.x/2, area.y + area.h/2 - surfaceSize.y/2);

		const uint	flags	= int(SDL_WINDOW_ALLOW_HIGHDPI) | int(SDL_WINDOW_RESIZABLE);

		CHECK_ERR( (_window = SDL_CreateWindow( caption.data(),
											    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
											    surfaceSize.x, surfaceSize.y,
											    flags )) != null );

		_wndID = SDL_GetWindowID( _window );

		SDL_SetWindowData( _window, "mgf", this );
		
		_listener = listener;
		return true;
	}

/*
=================================================
	Update
=================================================
*/
	bool WindowSDL2::Update ()
	{
		if ( not _window )
			return false;

		SDL_Event	ev;
		while ( SDL_PollEvent( OUT &ev ) )
		{
			switch ( ev.type )
			{
				case SDL_QUIT :
				case SDL_APP_TERMINATING :
					Quit();
					return false;

				case SDL_APP_WILLENTERBACKGROUND :
					break;

				case SDL_APP_DIDENTERBACKGROUND :
					break;

				case SDL_APP_WILLENTERFOREGROUND :
					break;

				case SDL_APP_DIDENTERFOREGROUND :
					break;

				case SDL_WINDOWEVENT :
				{
					switch ( ev.window.event )
					{
						case SDL_WINDOWEVENT_SHOWN :
						{
							break;
						}

						case SDL_WINDOWEVENT_HIDDEN :
						{
							break;
						}

						case SDL_WINDOWEVENT_RESIZED :
						case SDL_WINDOWEVENT_MOVED :
						case SDL_WINDOWEVENT_SIZE_CHANGED :
						{
							int2	size;
							SDL_GetWindowSize( _window, OUT &size.x, OUT &size.y );

							if ( _listener )
								_listener->OnResize( uint2(size) );
							break;
						}

						case SDL_WINDOWEVENT_CLOSE :
						{
							Quit();
							return false;
						}
					}
					break;
				}
			}
		}

		if ( _listener )
			_listener->OnUpdate();

		return true;
	}
	
/*
=================================================
	Quit
=================================================
*/
	void WindowSDL2::Quit ()
	{
		Destroy();
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void WindowSDL2::Destroy ()
	{
		if ( _listener )
		{
			_listener->OnDestroy();
			_listener = null;
		}

		if ( _window )
		{
			SDL_DestroyWindow( _window );
			_window = null;
			_wndID = 0;
		
			auto&	inst = *Singleton<SDL2Instance>();

			if ( --inst.refCounter == 0 and inst.initialized )
			{
				SDL_Quit();
				inst.initialized = false;
			}
		}
	}
	
/*
=================================================
	GetVulkanSurface
=================================================
*/
	UniquePtr<IVulkanSurface>  WindowSDL2::GetVulkanSurface () const
	{
		return UniquePtr<IVulkanSurface>{new VulkanSurface( _window )};
	}

}	// FG
//-----------------------------------------------------------------------------


	
	
// Android
#if	(defined( ANDROID ) or defined( __ANDROID__ ))

#	include "vulkan/vulkan_android.h"
//-----------------------------------------------------------------------------


// Windows
#elif (defined( _WIN32 ) || defined( _WIN64 ) || defined( WIN32 ) || defined( WIN64 ) || \
	   defined( __CYGWIN__ ) || defined( __MINGW32__ ) || defined( __MINGW32__ ))

#	include <Windows.h>
#	include "vulkan/vulkan_win32.h"

namespace FG
{
/*
=================================================
	GetRequiredExtensions
=================================================
*/
	Array<const char*>  WindowSDL2::VulkanSurface::GetRequiredExtensions () const
	{
		Array<const char*>	ext = { VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
		return ext;
	}
	
/*
=================================================
	Create
=================================================
*/
	VkSurfaceKHR  WindowSDL2::VulkanSurface::Create (VkInstance instance) const
	{
		VkSurfaceKHR					surface;
		VkWin32SurfaceCreateInfoKHR		surface_info = {};
		SDL_SysWMinfo					info		 = {};
			
		SDL_VERSION( OUT &info.version );
		CHECK_ERR( SDL_GetWindowWMInfo( _window, OUT &info ) == SDL_TRUE );
		CHECK_ERR( info.subsystem == SDL_SYSWM_WINDOWS );

		surface_info.sType		= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surface_info.hinstance	= info.info.win.hinstance;
		surface_info.hwnd		= info.info.win.window;
		
		PFN_vkCreateWin32SurfaceKHR create_surface = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr( instance, "vkCreateWin32SurfaceKHR" );
		CHECK_ERR( create_surface );

		VK_CHECK( create_surface( instance, &surface_info, null, OUT &surface ) );
		return surface;
	}
}	// FG
//-----------------------------------------------------------------------------


// Linux
#elif (defined( __linux__ ) || defined( __gnu_linux__ ))

#	include "vulkan/vulkan_xlib.h"
#	include "vulkan/vulkan_xcb.h"
//-----------------------------------------------------------------------------


// MacOS, iOS
#elif (defined( __APPLE__ ) || defined( MACOSX ))

#	include "vulkan/vulkan_ios.h"
#	include "vulkan/vulkan_macos.h"
//-----------------------------------------------------------------------------


#endif
#endif	// FG_ENABLE_SDL2
