// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"

#ifdef FG_ENABLE_SFML
# include <SFML/Window.hpp>

namespace FGC
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
			Array<const char*>	_extensions;

		public:
			explicit VulkanSurface (const sf::Window *wnd);

			ND_ ArrayView<const char*>	GetRequiredExtensions () const override	{ return _extensions; }
			ND_ VkSurfaceKHR			Create (VkInstance inst) const override;
		};

		using Listeners_t	= HashSet< IWindowEventListener *>;


	// variables
	private:
		sf::Window		_window;
		Listeners_t		_listeners;


	// methods
	public:
		WindowSFML ();
		~WindowSFML () override;

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
	};


}	// FGC

#endif	// FG_ENABLE_SFML
