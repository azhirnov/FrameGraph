// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"

#ifdef FG_ENABLE_SFML
# include <SFML/Window.hpp>

namespace FG
{

	//
	// Window SFML
	//

	class WindowSFML final : public IWindow
	{
	// types
	private:
		struct VulkanSurface final : public IVulkanSurface
		{
		private:
			const sf::Window *	_window;

		public:
			explicit VulkanSurface (const sf::Window *wnd) : _window{wnd} {}

			ND_ Array<const char*>	GetRequiredExtensions () const override;
			ND_ VkSurfaceKHR		Create (VkInstance inst) const override;
		};

		using Listeners_t	= HashSet< IWindowEventListener *>;


	// variables
	private:
		sf::Window		_window;
		Listeners_t		_listeners;


	// methods
	public:
		WindowSFML ();
		~WindowSFML ();

		bool Create (uint2 size, StringView title) override;
		void AddListener (IWindowEventListener *listener) override;
		void RemoveListener (IWindowEventListener *listener) override;
		bool Update () override;
		void Quit () override;
		void Destroy () override;
		
		void SetTitle (StringView value) override;
		
		uint2 GetSize () const override;

		UniquePtr<IVulkanSurface>  GetVulkanSurface () const override;
	};


}	// FG

#endif	// FG_ENABLE_SFML
