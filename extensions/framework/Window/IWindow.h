// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"
#include "stl/Containers/FixedArray.h"
#include "stl/CompileTime/DefaultType.h"
#include "stl/Algorithms/MemUtils.h"
#include "vulkan_loader/VulkanLoader.h"
#include "vulkan_loader/VulkanCheckError.h"

namespace FG
{

	//
	// Window Event Listener interface
	//

	class IWindowEventListener
	{
	// types
	public:
		enum class EKeyAction {
			Down,		// single event when key down
			Up,			// signle event when key up
			Pressed,	// continiously event until key is pressed
		};
		
	// interface
	public:
		virtual void OnResize (const uint2 &size) = 0;
		virtual void OnRefrash () = 0;
		virtual void OnDestroy () = 0;
		virtual void OnUpdate () = 0;
		virtual void OnKey (StringView key, EKeyAction action) = 0;
	};



	//
	// Vulkan Surface interface
	//

	class IVulkanSurface
	{
	public:
		virtual ~IVulkanSurface () {}
		ND_ virtual Array<const char*>	GetRequiredExtensions () const = 0;
		ND_ virtual VkSurfaceKHR		Create (VkInstance inst) const = 0;
	};



	//
	// Window interface
	//

	class IWindow
	{
	// types
	public:
		using EKeyAction = IWindowEventListener::EKeyAction;
		
	// interface
	public:
		virtual ~IWindow () {}
		virtual bool Create (uint2 size, StringView title) = 0;
		virtual void AddListener (IWindowEventListener *listener) = 0;
		virtual void RemoveListener (IWindowEventListener *listener) = 0;
		virtual bool Update () = 0;
		virtual void Quit () = 0;
		virtual void Destroy () = 0;
		virtual void SetTitle (StringView value) = 0;
		virtual void SetSize (const uint2 &value) = 0;
		virtual void SetPosition (const int2 &value) = 0;

		ND_ virtual uint2 GetSize () const = 0;

		ND_ virtual UniquePtr<IVulkanSurface>  GetVulkanSurface () const = 0;
	};


	using WindowPtr	= UniquePtr< IWindow >;

}	// FG
