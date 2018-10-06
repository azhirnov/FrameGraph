// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "WindowSFML.h"
#include "framework/Vulkan/VulkanSurface.h"

#ifdef FG_ENABLE_SFML

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	WindowSFML::WindowSFML ()
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	WindowSFML::~WindowSFML ()
	{
		Destroy();
	}
	
/*
=================================================
	Create
=================================================
*/
	bool WindowSFML::Create (uint2 size, StringView title)
	{
		CHECK_ERR( not _window.isOpen() );

		_window.create( sf::VideoMode(size.x, size.y), String(title), sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close );
		CHECK_ERR( _window.isOpen() );
	
		return true;
	}
	
/*
=================================================
	AddListener
=================================================
*/
	void WindowSFML::AddListener (IWindowEventListener *listener)
	{
		ASSERT( listener );
		_listeners.insert( listener );
	}
	
/*
=================================================
	RemoveListener
=================================================
*/
	void WindowSFML::RemoveListener (IWindowEventListener *listener)
	{
		ASSERT( listener );
		_listeners.erase( listener );
	}

/*
=================================================
	Update
=================================================
*/
	bool WindowSFML::Update ()
	{
		if ( not _window.isOpen() )
			return false;

		sf::Event	event;
		while ( _window.pollEvent( OUT event ) )
		{
			if ( event.type == sf::Event::Closed ) {
				Quit();
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
	void WindowSFML::Quit ()
	{
		Destroy();
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void WindowSFML::Destroy ()
	{
		Listeners_t		listeners;
		std::swap( listeners, _listeners );

		for (auto& listener : listeners) {
			listener->OnDestroy();
		}

		_window.close();
	}
		
/*
=================================================
	SetTitle
=================================================
*/
	void WindowSFML::SetTitle (StringView value)
	{
		CHECK_ERR( _window.isOpen(), void() );

		_window.setTitle( String(value) );
	}
	
/*
=================================================
	SetSize
=================================================
*/
	void WindowSFML::SetSize (const uint2 &value)
	{
		CHECK_ERR( _window.isOpen(), void() );

		_window.setSize({ int(value.x), int(value.y) });
	}
	
/*
=================================================
	SetPosition
=================================================
*/
	void WindowSFML::SetPosition (const int2 &value)
	{
		CHECK_ERR( _window.isOpen(), void() );

		_window.setPosition({ value.x, value.y });
	}

/*
=================================================
	GetSize
=================================================
*/
	uint2 WindowSFML::GetSize () const
	{
		CHECK_ERR( _window.isOpen() );

		auto	size = _window.getSize();
		return uint2{ size.x, size.y };
	}

/*
=================================================
	GetVulkanSurface
=================================================
*/
	UniquePtr<IVulkanSurface>  WindowSFML::GetVulkanSurface () const
	{
		return UniquePtr<IVulkanSurface>{new VulkanSurface( &_window )};
	}

}	// FG
//-----------------------------------------------------------------------------
	

# if defined(PLATFORM_WINDOWS) or defined(VK_USE_PLATFORM_WIN32_KHR)
#	define NOMINMAX
#	define NOMCX
#	define NOIME
#	define NOSERVICE
#	define WIN32_LEAN_AND_MEAN
#	include <Windows.h>
# endif

namespace FG
{
/*
=================================================
	GetRequiredExtensions
=================================================
*/
	Array<const char*>  WindowSFML::VulkanSurface::GetRequiredExtensions () const
	{
		return FG::VulkanSurface::GetRequiredExtensions();
	}
	
/*
=================================================
	Create
=================================================
*/
	VkSurfaceKHR  WindowSFML::VulkanSurface::Create (VkInstance instance) const
	{
#		if defined(PLATFORM_WINDOWS) or defined(VK_USE_PLATFORM_WIN32_KHR)
		return FG::VulkanSurface::CreateWin32Surface( instance, ::GetModuleHandle(LPCSTR(null)), _window->getSystemHandle() );
#		endif
	}


}	// FG

#endif	// FG_ENABLE_SFML
