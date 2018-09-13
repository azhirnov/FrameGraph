// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "framework/Window/WindowGLFW.h"
#include "stl/include/Singleton.h"
#include "stl/include/StringUtils.h"

#ifdef FG_ENABLE_GLFW

namespace FG
{
namespace {

	struct GLFWInstance
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
	WindowGLFW::WindowGLFW () :
		_window{ null },
		_listener{ null }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	WindowGLFW::~WindowGLFW ()
	{
		Destroy();
	}
	
/*
=================================================
	Create
=================================================
*/
	bool WindowGLFW::Create (uint2 size, StringView caption, IWindowEventListener *listener)
	{
		auto&	inst = *Singleton<GLFWInstance>();

		if ( not inst.initialized )
		{
			glfwSetErrorCallback( &_GLFW_ErrorCallback );

			CHECK_ERR( glfwInit() == GLFW_TRUE );
			CHECK_ERR( glfwVulkanSupported() == GLFW_TRUE );

			inst.initialized = true;
		}

		++inst.refCounter;


		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

        _window = glfwCreateWindow( int(size.x),
                                    int(size.y),
                                    caption.data(),
                                    null,
                                    null );
		CHECK_ERR( _window );

		glfwSetWindowUserPointer( _window, this );
		glfwSetWindowRefreshCallback( _window, &_GLFW_RefreshCallback );
		glfwSetFramebufferSizeCallback( _window, &_GLFW_ResizeCallback );
		glfwSetKeyCallback( _window, &_GLFW_KeyCallback );

		_listener = listener;
		return true;
	}
	
/*
=================================================
	_GLFW_ErrorCallback
=================================================
*/
	void WindowGLFW::_GLFW_ErrorCallback (int code, const char* msg)
	{
		FG_LOGE( "GLFW error: " + ToString( code ) + ", msg: \"" + msg + "\"" );
	}
	
/*
=================================================
	_GLFW_RefreshCallback
=================================================
*/
	void WindowGLFW::_GLFW_RefreshCallback (GLFWwindow* wnd)
	{
		auto*	self = static_cast<WindowGLFW *>(glfwGetWindowUserPointer( wnd ));
		
		if ( self->_listener )
			self->_listener->OnRefrash();
	}
	
/*
=================================================
	_GLFW_ResizeCallback
=================================================
*/
	void WindowGLFW::_GLFW_ResizeCallback (GLFWwindow* wnd, int w, int h)
	{
		auto*	self = static_cast<WindowGLFW *>(glfwGetWindowUserPointer( wnd ));
		
		if ( self->_listener )
			self->_listener->OnResize( uint2{int2{ w, h }} );
	}

/*
=================================================
	_GLFW_KeyCallback
=================================================
*/
	void WindowGLFW::_GLFW_KeyCallback (GLFWwindow* wnd, int key, int, int, int)
	{
		// TODO
	}

/*
=================================================
	Update
=================================================
*/
	bool WindowGLFW::Update ()
	{
		if ( not _window )
			return false;

		if ( glfwWindowShouldClose( _window ) )
			return false;

		glfwPollEvents();

		if ( _listener )
			_listener->OnUpdate();

		return true;
	}
	
/*
=================================================
	Quit
=================================================
*/
	void WindowGLFW::Quit ()
	{
		if ( _window )
		{
			glfwSetWindowShouldClose( _window, GLFW_TRUE );
		}
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void WindowGLFW::Destroy ()
	{
		if ( _listener )
		{
			_listener->OnDestroy();
			_listener = null;
		}

		if ( _window )
		{
			glfwDestroyWindow( _window );
			_window = null;
		

			// delete GLFW instance
			auto&	inst = *Singleton<GLFWInstance>();

			if ( (--inst.refCounter) == 0 and inst.initialized )
			{
				glfwTerminate();

				inst.initialized = false;
			}
		}
	}
	
/*
=================================================
	GetVulkanSurface
=================================================
*/
	UniquePtr<IVulkanSurface>  WindowGLFW::GetVulkanSurface () const
	{
		return UniquePtr<IVulkanSurface>{new VulkanSurface( _window )};
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	GetRequiredExtensions
=================================================
*/
	Array<const char*>  WindowGLFW::VulkanSurface::GetRequiredExtensions () const
	{
		uint32_t			required_extension_count = 0;
		const char **		required_extensions		 = glfwGetRequiredInstanceExtensions( OUT &required_extension_count );
		Array<const char*>	extensions;
		
		extensions.assign( required_extensions, required_extensions + required_extension_count );
		return extensions;
	}
	
/*
=================================================
	Create
=================================================
*/
	VkSurfaceKHR  WindowGLFW::VulkanSurface::Create (VkInstance inst) const
	{
		VkSurfaceKHR	surf = VK_NULL_HANDLE;
		glfwCreateWindowSurface( inst, _window, null, OUT &surf );
		return surf;
	}


}	// FG

#endif	// FG_ENABLE_GLFW
