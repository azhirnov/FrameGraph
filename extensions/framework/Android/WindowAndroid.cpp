// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "WindowAndroid.h"

#ifdef PLATFORM_ANDROID

# include "framework/Vulkan/VulkanSurface.h"
# include "android_native_app_glue.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	WindowAndroid::WindowAndroid (android_app* app) :
		_application{app}, _window{null}, _sensorManager{null}, _pause{false}
	{
	}
	
/*
=================================================
	constructor
=================================================
*/
	WindowAndroid::~WindowAndroid ()
	{
		Destroy();
	}
	
/*
=================================================
	Create
=================================================
*/
	bool WindowAndroid::Create (uint2 size, StringView)
	{
		CHECK_ERR( _application );
		CHECK_ERR( not _window );
		
		_application->userData		= this;
		_application->onAppCmd		= _HandleCmd;
		_application->onInputEvent	= _HandleInput;

		_windowSize = size;
		return true;
	}
	
/*
=================================================
	AddListener
=================================================
*/
	void WindowAndroid::AddListener (IWindowEventListener *listener)
	{
		ASSERT( listener );
		_listeners.insert( listener );
	}
	
/*
=================================================
	RemoveListener
=================================================
*/
	void WindowAndroid::RemoveListener (IWindowEventListener *listener)
	{
		ASSERT( listener );
		_listeners.erase( listener );
	}
	
/*
=================================================
	Update
=================================================
*/
	bool WindowAndroid::Update ()
	{
		if ( not (_window and _application) )
			return false;
		
		int ident;
		int events;
		android_poll_source* source;

		while ((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0)
		{
			if (source != NULL) {
				source->process( _application, source );
			}

			if ( _application->destroyRequested ) {
				Destroy();
				return false;
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
	void WindowAndroid::Quit ()
	{
		_window = null;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void WindowAndroid::Destroy ()
	{
		_application	= null;
		_window			= null;
		_windowSize		= Default;
	}
	
/*
=================================================
	GetSize
=================================================
*/
	uint2 WindowAndroid::GetSize () const
	{
		return _windowSize;
	}
	
/*
=================================================
	GetVulkanSurface
=================================================
*/
	UniquePtr<IVulkanSurface>  WindowAndroid::GetVulkanSurface () const
	{
		return UniquePtr<IVulkanSurface>{new VulkanSurface( _window )};
	}

/*
=================================================
	_HandleInput
=================================================
*/
	int32_t  WindowAndroid::_HandleInput (android_app* app, AInputEvent* event)
	{
		WindowAndroid*	self = static_cast<WindowAndroid *>(app->userData);

		//if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		//	engine->state.x = AMotionEvent_getX(event, 0);
		//	engine->state.y = AMotionEvent_getY(event, 0);
		//	return 1;
		//}
		return 0;
	}

/*
=================================================
	_HandleCmd
=================================================
*/
	void WindowAndroid::_HandleCmd (android_app* app, int32_t cmd)
	{
		WindowAndroid*	self = static_cast<WindowAndroid *>(app->userData);

		switch (cmd)
		{
			case APP_CMD_SAVE_STATE :
				break;

			case APP_CMD_INIT_WINDOW :
				//if (engine->app->window != NULL) {
				//	engine_init_display(engine);
				//	engine_draw_frame(engine);
				//}
				break;

			case APP_CMD_TERM_WINDOW :
				self->Destroy();
				break;

			case APP_CMD_GAINED_FOCUS :
				//if (engine->accelerometerSensor != NULL) {
				//	ASensorEventQueue_enableSensor(engine->sensorEventQueue, engine->accelerometerSensor);
				//	ASensorEventQueue_setEventRate(engine->sensorEventQueue, engine->accelerometerSensor, (1000L / 60) * 1000);
				//}
				self->_pause = false;
				break;

			case APP_CMD_LOST_FOCUS :
				//if (engine->accelerometerSensor != NULL) {
				//	ASensorEventQueue_disableSensor(engine->sensorEventQueue, engine->accelerometerSensor);
				//}
				self->_pause = true;

				for (auto& listener : self->_listeners) {
					listener->OnRefresh();
				}
				break;
		}
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	GetRequiredExtensions
=================================================
*/
	Array<const char*>  WindowAndroid::VulkanSurface::GetRequiredExtensions () const
	{
		return FG::VulkanSurface::GetRequiredExtensions();
	}
	
/*
=================================================
	Create
=================================================
*/
	VkSurfaceKHR  WindowAndroid::VulkanSurface::Create (VkInstance instance) const
	{
		return FG::VulkanSurface::CreateAndroidSurface( instance, _window );
	}


}	// FG

#endif	// PLATFORM_ANDROID
