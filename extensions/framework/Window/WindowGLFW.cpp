// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "WindowGLFW.h"
#include "stl/Containers/Singleton.h"
#include "stl/Algorithms/StringUtils.h"

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
		_window{ null }
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
	bool WindowGLFW::Create (uint2 size, StringView title)
	{
		CHECK_ERR( not _window );

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
                                    title.data(),
                                    null,
                                    null );
		CHECK_ERR( _window );

		glfwSetWindowUserPointer( _window, this );
		glfwSetWindowRefreshCallback( _window, &_GLFW_RefreshCallback );
		glfwSetFramebufferSizeCallback( _window, &_GLFW_ResizeCallback );
		glfwSetKeyCallback( _window, &_GLFW_KeyCallback );

		return true;
	}
	
/*
=================================================
	AddListener
=================================================
*/
	void WindowGLFW::AddListener (IWindowEventListener *listener)
	{
		ASSERT( listener );
		_listeners.insert( listener );
	}
	
/*
=================================================
	RemoveListener
=================================================
*/
	void WindowGLFW::RemoveListener (IWindowEventListener *listener)
	{
		ASSERT( listener );
		_listeners.erase( listener );
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
		
		for (auto& listener : self->_listeners) {
			listener->OnRefrash();
		}
	}
	
/*
=================================================
	_GLFW_ResizeCallback
=================================================
*/
	void WindowGLFW::_GLFW_ResizeCallback (GLFWwindow* wnd, int w, int h)
	{
		auto*	self = static_cast<WindowGLFW *>(glfwGetWindowUserPointer( wnd ));
		
		for (auto& listener : self->_listeners) {
			listener->OnResize( uint2{int2{ w, h }} );
		}
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
		for (auto& listener : _listeners) {
			listener->OnDestroy();
		}
		_listeners.clear();

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
	SetTitle
=================================================
*/
	void WindowGLFW::SetTitle (StringView value)
	{
		CHECK_ERR( _window, void() );

		glfwSetWindowTitle( _window, value.data() );
	}
	
/*
=================================================
	GetSize
=================================================
*/
	uint2 WindowGLFW::GetSize () const
	{
		CHECK_ERR( _window );

		int2	size;
		glfwGetWindowSize( _window, OUT &size.x, OUT &size.y );

		return uint2(size);
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
