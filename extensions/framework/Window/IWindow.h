// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/include/Vec.h"
#include "stl/include/FixedArray.h"
#include "stl/include/DefaultType.h"
#include "vulkan_loader/VulkanLoader.h"
#include "vulkan_loader/VulkanCheckError.h"

namespace FG
{

	//
	// Window Event Listener interface
	//

	class IWindowEventListener
	{
	public:
		virtual void OnResize (const uint2 &size) = 0;
		virtual void OnRefrash () = 0;
		virtual void OnDestroy () = 0;
		virtual void OnUpdate () = 0;
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
	public:
		virtual ~IWindow () {}
		virtual bool Create (uint2 size, StringView title, IWindowEventListener *listener) = 0;
		virtual bool Update () = 0;
		virtual void Quit () = 0;
		virtual void Destroy () = 0;
		virtual void SetTitle (StringView value) = 0;

		ND_ virtual uint2 GetSize () const = 0;

		ND_ virtual UniquePtr<IVulkanSurface>  GetVulkanSurface () const = 0;
	};


	using WindowPtr	= UniquePtr< IWindow >;

}	// FG
