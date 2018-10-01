// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "WindowSDL2.h"
#include "framework/Vulkan/VulkanSurface.h"
#include "stl/Containers/Singleton.h"

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
		_wndID{ 0 }
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
	bool WindowSDL2::Create (uint2 surfaceSize, StringView title)
	{
		CHECK_ERR( not _window );

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

		CHECK_ERR( (_window = SDL_CreateWindow( title.data(),
											    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
											    surfaceSize.x, surfaceSize.y,
											    flags )) != null );

		_wndID = SDL_GetWindowID( _window );

		SDL_SetWindowData( _window, "mgf", this );
		
		return true;
	}
	
/*
=================================================
	AddListener
=================================================
*/
	void WindowSDL2::AddListener (IWindowEventListener *listener)
	{
		ASSERT( listener );
		_listeners.insert( listener );
	}
	
/*
=================================================
	RemoveListener
=================================================
*/
	void WindowSDL2::RemoveListener (IWindowEventListener *listener)
	{
		ASSERT( listener );
		_listeners.erase( listener );
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
							
							for (auto& listener : _listeners) {
								listener->OnResize( uint2(size) );
							}
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
		
		for (auto& listener : _listeners) {
			listener->OnUpdate();
		}
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
		for (auto& listener : _listeners) {
			listener->OnDestroy();
		}
		_listeners.clear();

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
	SetTitle
=================================================
*/
	void WindowSDL2::SetTitle (StringView value)
	{
		CHECK_ERR( _window, void() );

		SDL_SetWindowTitle( _window, value.data() );
	}
	
/*
=================================================
	GetSize
=================================================
*/
	uint2 WindowSDL2::GetSize () const
	{
		CHECK_ERR( _window );

		int2	size;
		SDL_GetWindowSize( _window, OUT &size.x, OUT &size.y );

		return uint2(size);
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
//-----------------------------------------------------------------------------



/*
=================================================
	GetRequiredExtensions
=================================================
*/
	Array<const char*>  WindowSDL2::VulkanSurface::GetRequiredExtensions () const
	{
		return FG::VulkanSurface::GetRequiredExtensions();
	}
	
/*
=================================================
	Create
=================================================
*/
	VkSurfaceKHR  WindowSDL2::VulkanSurface::Create (VkInstance instance) const
	{
		SDL_SysWMinfo	info = {};
			
		SDL_VERSION( OUT &info.version );
		CHECK_ERR( SDL_GetWindowWMInfo( _window, OUT &info ) == SDL_TRUE );
		
		switch ( info.subsystem )
		{
			case SDL_SYSWM_X11 :
				return 0;	// TODO

			case SDL_SYSWM_WAYLAND :
				return 0;	// TODO

			case SDL_SYSWM_MIR :
				return 0;	// TODO

			case SDL_SYSWM_ANDROID :
				return 0;	// TODO
				
# if		defined(PLATFORM_WINDOWS) or defined(VK_USE_PLATFORM_WIN32_KHR)
			case SDL_SYSWM_WINDOWS :
				return FG::VulkanSurface::CreateWin32Surface( instance, info.info.win.hinstance, info.info.win.window );
#			endif
		}

		RETURN_ERR( "current subsystem type is not supported!" );
	}

}	// FG

#endif	// FG_ENABLE_SDL2
