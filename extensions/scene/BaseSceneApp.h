// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Math/FPSCamera.h"
#include "scene/Math/VRCamera.h"
#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDevice.h"
#include "scene/SceneManager/IViewport.h"
#include "framework/VR/IVRDevice.h"

namespace FG
{

	//
	// Base Scene Application
	//

	class BaseSceneApp : public IViewport, public IWindowEventListener, public IVRDeviceEventListener
	{
	// types
	protected:
		using TimePoint_t	= std::chrono::high_resolution_clock::time_point;
		using SecondsF		= std::chrono::duration< float >;
		using FGStat_t		= IFrameGraph::Statistics;

		struct AppConfig
		{
		// types
			enum class EVRMode
			{
				Disabled,
				Emulator,
				OpenVR,
			};

		// variables
			String			windowTitle;
			uint2			surfaceSize;
			EVRMode			vrMode		        = EVRMode::Disabled;
			Array<String>	shaderDirectories;
			String			dbgOutputPath;
            String          deviceName;
            bool            enableDebugLayers   = true;
			bool			vsync				= true;
		};


	// variables
	protected:
		FrameGraph				_frameGraph;

	private:
		VulkanDeviceInitializer	_vulkan;
		WindowPtr				_window;
		VRDevicePtr				_vrDevice;
		
		SwapchainID				_swapchainId;

		// for double buffering
		CommandBuffer			_submittedBuffers[2];
		uint					_frameId : 1;

		// camera
		FPSCamera				_camera;
		Rad						_cameraFov			= 60_deg;
		// vr camera
		VRCamera				_vrCamera;

		vec3					_positionDelta;
		vec2					_mouseDelta;
		vec2					_lastMousePos;		// pixels
		TimePoint_t				_lastUpdateTime;
		SecondsF				_timeDelta			{0.0f};
		bool					_mousePressed		= false;
		bool					_enableCamera		= true;
		float					_cameraVelocity		= 1.0f;
		vec2					_mouseSens			{ 0.01f, 0.01f };
		vec2					_viewRange			{ 0.05f, 100.0f };
		
		String					_debugOutputPath;
		String					_title;
		
		struct {
			FGStat_t				fgStat;
			Nanoseconds				gpuTimeSum				{0};
			Nanoseconds				cpuTimeSum				{0};
			TimePoint_t				lastUpdateTime;
			uint					frameCounter			= 0;
			const uint				UpdateIntervalMillis	= 500;
		}						_frameStat;

		
	// methods
	protected:
		BaseSceneApp ();
		~BaseSceneApp ();

		bool  _CreateFrameGraph (const AppConfig &cfg);
		void  _DestroyFrameGraph ();
		void  _SetLastCommandBuffer (const CommandBuffer &);

		void  _UpdateCamera ();
		
		void  _SetupCamera (Rad fovY, const vec2 &viewRange);
		void  _SetCameraVelocity (float value);
		void  _SetMouseSens (vec2 value);
		void  _EnableCameraMovement (bool enable);

		void  _VRPresent (const VulkanDevice::VQueue &queue, RawImageID leftEyeImage, RawImageID righteyeImage);

	private:
		void  _UpdateFrameStat ();
		void  _OnShaderTraceReady (StringView name, ArrayView<String> output) const;


	public:
		ND_ vec2					GetMousePos ()		const	{ return _lastMousePos; }
		ND_ bool					IsMousePressed ()	const	{ return _mousePressed; }

		ND_ VulkanDevice const&		GetVulkan ()		const	{ return _vulkan; }
		ND_ Ptr<IWindow>			GetWindow ()				{ return _window.get(); }
		ND_ Ptr<IVRDevice>			GetVRDevice ()				{ return _vrDevice.get(); }
		ND_ uint2					GetSurfaceSize ()	const	{ return _window ? _window->GetSize() : uint2(); }

		ND_ RawSwapchainID			GetSwapchain ()		const	{ return _swapchainId; }

		// vr camera
		ND_ VRCamera &				GetVRCamera ()				{ return _vrCamera; }
		ND_ VRCamera const&			GetVRCamera ()		const	{ return _vrCamera; }
		ND_ bool					IsActiveVR ()		const;

		// viewport camera
		ND_ Camera const&			GetCamera ()		const	{ return _camera.GetCamera(); }
		ND_ Frustum const&			GetFrustum ()		const	{ return _camera.GetFrustum(); }
		ND_ FPSCamera &				GetFPSCamera ()				{ return _camera; }
		ND_ Rad						GetCameraFov ()		const	{ return _cameraFov; }
		ND_ vec2 const&				GetViewRange ()		const	{ return _viewRange; }

		ND_ Nanoseconds				CurrentTime ()		const	{ return _lastUpdateTime.time_since_epoch(); }
		ND_ SecondsF				FrameTime ()		const	{ return _timeDelta; }
		ND_ FGStat_t const&			GetStatistic ()		const	{ return _frameStat.fgStat; }


	// interface
	public:
		ND_ virtual bool  Update ();
	protected:
		ND_ virtual bool  DrawScene () = 0;
			virtual void  OnUpdateFrameStat (OUT String &) const {}


	// IWindowEventListener
	protected:
		void  OnRefresh () override {}
		void  OnDestroy () override {}
		void  OnUpdate () override {}
		void  OnResize (const uint2 &size) override;
		void  OnKey (StringView, EKeyAction) override;
		void  OnMouseMove (const float2 &) override;
		

	// IVRDeviceEventListener
	protected:
		void  HmdStatusChanged (EHmdStatus) override;
		void  OnAxisStateChanged (ControllerID id, StringView name, const float2 &value, const float2 &delta, float dt) override;
		void  OnButton (ControllerID id, StringView btn, EButtonAction action) override;


	// IViewport //
	private:
		void  Prepare (ScenePreRender &) override {}
		void  Draw (RenderQueue &) const override {}
		void  AfterRender (const CommandBuffer &, Present &&) override;
	};

}	// FG
