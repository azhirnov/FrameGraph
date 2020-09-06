// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "BaseSceneApp.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/VR/OpenVRDevice.h"
#include "framework/VR/VRDeviceEmulator.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"
#include "stl/Platforms/WindowsHeader.h"


namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	BaseSceneApp::BaseSceneApp () :
		_frameId{0}
	{
		_lastUpdateTime = _frameStat.lastUpdateTime = TimePoint_t::clock::now();
	}
	
/*
=================================================
	destructor
=================================================
*/
	BaseSceneApp::~BaseSceneApp ()
	{
		_DestroyFrameGraph();
	}

/*
=================================================
	_CreateFrameGraph
=================================================
*/
	bool  BaseSceneApp::_CreateFrameGraph (const AppConfig &cfg)
	{
		FG_LOGI( "Create "s << IFrameGraph::GetVersion() );

		// initialize window
		{
			#if defined( FG_ENABLE_GLFW )
				_window.reset( new WindowGLFW() );

			#elif defined( FG_ENABLE_SDL2 )
				_window.reset( new WindowSDL2() );

			#else
			#	error Unknown window library!
			#endif

			_title = cfg.windowTitle;

			CHECK_ERR( _window->Create( cfg.surfaceSize, _title ));
			_window->AddListener( this );
		}

		// initialize VR
		{
			#if defined(FG_ENABLE_OPENVR)
			if ( cfg.vrMode == AppConfig::EVRMode::OpenVR ) {
				_vrDevice.reset( new OpenVRDevice{});
			}
			#endif

			if ( cfg.vrMode == AppConfig::EVRMode::Emulator ) {
				WindowPtr	wnd{ new WindowGLFW{}};
				CHECK_ERR( wnd->Create( { 1024, 512 }, "emulator" ));
				_vrDevice.reset( new VRDeviceEmulator{ std::move(wnd) });
			}

			if ( _vrDevice and not _vrDevice->Create() ) {
				_vrDevice.reset();
			}
		}

		// initialize vulkan device
		{
			ArrayView<const char*>	layers		= _vrDevice ? Default : (cfg.enableDebugLayers ? VulkanDeviceInitializer::GetRecomendedInstanceLayers() : Default);
			Array<const char*>		inst_ext;
			Array<const char*>		dev_ext;
			Array<String>			vr_inst_ext	= _vrDevice ? _vrDevice->GetRequiredInstanceExtensions() : Default;
			Array<String>			vr_dev_ext	= _vrDevice ? _vrDevice->GetRequiredDeviceExtensions() : Default;

			for (auto& ext : vr_inst_ext) {
				inst_ext.push_back( ext.c_str() );
			}
			for (auto& ext : vr_dev_ext) {
				dev_ext.push_back( ext.c_str() );
			}
			
			CHECK_ERR( _vulkan.CreateInstance( _window->GetVulkanSurface(), _title, IFrameGraph::GetVersion(), layers, inst_ext, {1,2} ));

			if ( not _vulkan.ChooseDevice( cfg.deviceName ))
				CHECK_ERR( _vulkan.ChooseHighPerformanceDevice() );

			CHECK_ERR( _vulkan.CreateLogicalDevice( {{VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_SPARSE_BINDING_BIT}, {VK_QUEUE_COMPUTE_BIT}, {VK_QUEUE_TRANSFER_BIT}}, dev_ext ));
			
			_vulkan.CreateDebugCallback( DefaultDebugMessageSeverity );
		}

		// setup device info
		VulkanDeviceInfo			vulkan_info;
		VulkanSwapchainCreateInfo	swapchain_info;
		{
			vulkan_info.instance		= BitCast<InstanceVk_t>( _vulkan.GetVkInstance() );
			vulkan_info.physicalDevice	= BitCast<PhysicalDeviceVk_t>( _vulkan.GetVkPhysicalDevice() );
			vulkan_info.device			= BitCast<DeviceVk_t>( _vulkan.GetVkDevice() );
			
			swapchain_info.surface		= BitCast<SurfaceVk_t>( _vulkan.GetVkSurface() );
			swapchain_info.surfaceSize	= _window->GetSize();

			if ( _vrDevice or not cfg.vsync )
				swapchain_info.presentModes.push_back( BitCast<PresentModeVk_t>(VK_PRESENT_MODE_MAILBOX_KHR) );
			else
				swapchain_info.presentModes.push_back( BitCast<PresentModeVk_t>(VK_PRESENT_MODE_FIFO_KHR) );	// enable vsync

			for (auto& q : _vulkan.GetVkQueues())
			{
				VulkanDeviceInfo::QueueInfo	qi;
				qi.handle		= BitCast<QueueVk_t>( q.handle );
				qi.familyFlags	= BitCast<QueueFlagsVk_t>( q.familyFlags );
				qi.familyIndex	= q.familyIndex;
				qi.priority		= q.priority;
				qi.debugName	= q.debugName;

				vulkan_info.queues.push_back( qi );
			}
		}

		if ( _vrDevice )
		{
			CHECK_ERR( _vrDevice->SetVKDevice( BitCast<IVRDevice::InstanceVk_t>(_vulkan.GetVkInstance()),
											   BitCast<IVRDevice::PhysicalDeviceVk_t>(_vulkan.GetVkPhysicalDevice()),
											   BitCast<IVRDevice::DeviceVk_t>(_vulkan.GetVkDevice()) ));
			_vrDevice->AddListener( this );
		}

		// initialize framegraph
		{
			_frameGraph = IFrameGraph::CreateFrameGraph( vulkan_info );
			CHECK_ERR( _frameGraph );

			_swapchainId = _frameGraph->CreateSwapchain( swapchain_info );
			CHECK_ERR( _swapchainId );

			_frameGraph->SetShaderDebugCallback([this] (auto name, auto, auto, auto output) { _OnShaderTraceReady( name, output ); });
		}

		// add glsl pipeline compiler
		// TODO: ShaderCache also adds shader compiler, so remove one of them
		{
			auto	compiler = MakeShared<VPipelineCompiler>( vulkan_info.instance, vulkan_info.physicalDevice, vulkan_info.device );
			compiler->SetCompilationFlags( EShaderCompilationFlags::Quiet				|
										   EShaderCompilationFlags::ParseAnnotations	|
										   EShaderCompilationFlags::Optimize			|
										   EShaderCompilationFlags::UseCurrentDeviceLimits );

			for (auto& dir : cfg.shaderDirectories) {
				compiler->AddDirectory( dir );
			}

			_frameGraph->AddPipelineCompiler( compiler );
		}
		
		// setup debug output
	#ifdef FS_HAS_FILESYSTEM
		if ( cfg.dbgOutputPath.size() )
		{
			FS::path	path{ cfg.dbgOutputPath };
		
			if ( FS::exists( path ))
				FS::remove_all( path );
		
			CHECK( FS::create_directory( path ));

			_debugOutputPath = path.string();
		}
	#endif	// FS_HAS_FILESYSTEM

		_SetupCamera( _cameraFov, _viewRange );
		return true;
	}
	
/*
=================================================
	_DestroyFrameGraph
=================================================
*/
	void  BaseSceneApp::_DestroyFrameGraph ()
	{
		for (auto& cmd : _submittedBuffers) {
			cmd = null;
		}

		if ( _frameGraph )
		{
			_frameGraph->ReleaseResource( _swapchainId );
			_frameGraph->Deinitialize();
			_frameGraph = null;
		}

		if ( _vrDevice )
		{
			_vrDevice->Destroy();
			_vrDevice.reset();
		}
		
		_vulkan.DestroyLogicalDevice();
		_vulkan.DestroyInstance();

		if ( _window )
		{
			_window->Destroy();
			_window.reset();
		}
	}
	
/*
=================================================
	_SetupCamera
=================================================
*/
	void  BaseSceneApp::_SetupCamera (Rad fovY, const vec2 &viewRange)
	{
		_cameraFov	= fovY;
		_viewRange	= viewRange;
		
		const uint2	wnd_size	= _window->GetSize();
		const vec2	view_size	{ wnd_size.x, wnd_size.y };

		_camera.SetPerspective( _cameraFov, view_size.x / view_size.y, _viewRange.x, _viewRange.y );
		
		if ( _vrDevice ) {
			_vrDevice->SetupCamera( VecCast(_viewRange) );
		}
	}
	
/*
=================================================
	_SetCameraVelocity
=================================================
*/
	void  BaseSceneApp::_SetCameraVelocity (float value)
	{
		_cameraVelocity = value;
	}
	
/*
=================================================
	_SetMouseSens
=================================================
*/
	void  BaseSceneApp::_SetMouseSens (vec2 value)
	{
		_mouseSens = value;
	}
	
/*
=================================================
	_EnableCameraMovement
=================================================
*/
	void  BaseSceneApp::_EnableCameraMovement (bool enable)
	{
		_enableCamera = enable;
	}
	
/*
=================================================
	_VRPresent
=================================================
*/
	void  BaseSceneApp::_VRPresent (const VulkanDevice::VQueue &queue, RawImageID leftEyeImage, RawImageID righteyeImage)
	{
		const auto&			desc_l 		= _frameGraph->GetApiSpecificDescription( leftEyeImage );
		const auto&			desc_r		= _frameGraph->GetApiSpecificDescription( righteyeImage );
		const auto&			vk_desc_l	= std::get<VulkanImageDesc>( desc_l );
		const auto&			vk_desc_r	= std::get<VulkanImageDesc>( desc_r );
		IVRDevice::VRImage	vr_img;

		vr_img.currQueue		= BitCast<IVRDevice::QueueVk_t>(queue.handle);
		vr_img.queueFamilyIndex	= queue.familyIndex;
		vr_img.dimension		= vk_desc_l.dimension.xy();
		vr_img.bounds			= RectF{ 0.0f, 0.0f, 1.0f, 1.0f };
		vr_img.format			= BitCast<IVRDevice::FormatVk_t>(vk_desc_l.format);
		vr_img.sampleCount		= vk_desc_l.samples;

		vr_img.handle = BitCast<IVRDevice::ImageVk_t>(vk_desc_l.image);
		GetVRDevice()->Submit( vr_img, IVRDevice::Eye::Left );
					
		vr_img.handle = BitCast<IVRDevice::ImageVk_t>(vk_desc_r.image);
		GetVRDevice()->Submit( vr_img, IVRDevice::Eye::Right );
	}

/*
=================================================
	IsActiveVR
=================================================
*/
	bool  BaseSceneApp::IsActiveVR () const
	{
		return	_vrDevice and
				(_vrDevice->GetHmdStatus() == IVRDevice::EHmdStatus::Active or
				 _vrDevice->GetHmdStatus() == IVRDevice::EHmdStatus::Mounted);
	}
	
/*
=================================================
	_UpdateCamera
=================================================
*/
	void  BaseSceneApp::_UpdateCamera ()
	{
		if ( IsActiveVR() )
		{
			auto&	cam = _vrDevice->GetCamera();
			
			if ( length2( _positionDelta ) > 0.001f )
			{
				_positionDelta = normalize(_positionDelta) * _cameraVelocity * _timeDelta.count();
				_vrCamera.Move2( _positionDelta );
				_positionDelta = vec3{0.0f};
			}

			_vrCamera.SetViewProjection( MatCast(cam.left.proj), MatCast(cam.left.view),
										 MatCast(cam.right.proj), MatCast(cam.right.view),
										 MatCast(cam.pose), VecCast(cam.position) );
			_camera.SetPosition( _vrCamera.Position() );
			_camera.SetRotation( _vrCamera.Orientation() );
		}
		else
		{
			const uint2	wnd_size	= _window->GetSize();
			const vec2	view_size	{ wnd_size.x, wnd_size.y };

			if ( All( wnd_size > uint2(0) ))
			{
				_camera.SetPerspective( _cameraFov, view_size.x / view_size.y, _viewRange.x, _viewRange.y );
			}

			_camera.Rotate( -_mouseDelta.x, _mouseDelta.y );

			if ( length2( _positionDelta ) > 0.001f )
			{
				_positionDelta = normalize(_positionDelta) * _cameraVelocity * _timeDelta.count();
				_camera.Move2( _positionDelta );
				_positionDelta = vec3{0.0f};
			}
			_vrCamera.SetPosition( _camera.GetCamera().transform.position );
		}

		_mouseDelta = vec2{0.0f};
	}

/*
=================================================
	OnResize
=================================================
*/
	void  BaseSceneApp::OnResize (const uint2 &size)
	{
		if ( Any( size == uint2(0) ))
			return;

		CHECK( _frameGraph->WaitIdle() );

		VulkanSwapchainCreateInfo	swapchain_info;
		swapchain_info.surface		= BitCast<SurfaceVk_t>( _vulkan.GetVkSurface() );
		swapchain_info.surfaceSize  = size;

		_swapchainId = _frameGraph->CreateSwapchain( swapchain_info, _swapchainId.Release() );
		CHECK_FATAL( _swapchainId );
		
		const vec2	view_size { size.x, size.y };
		_camera.SetPerspective( _cameraFov, view_size.x / view_size.y, _viewRange.x, _viewRange.y );
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void  BaseSceneApp::OnKey (StringView key, EKeyAction action)
	{
		if ( action != EKeyAction::Up )
		{
			// forward/backward
			if ( key == "W" )			_positionDelta.x += 1.0f;	else
			if ( key == "S" )			_positionDelta.x -= 1.0f;

			// left/right
			if ( key == "D" )			_positionDelta.y -= 1.0f;	else
			if ( key == "A" )			_positionDelta.y += 1.0f;

			// up/down
			if ( key == "V" )			_positionDelta.z += 1.0f;	else
			if ( key == "C" )			_positionDelta.z -= 1.0f;

			// rotate up/down
			if ( key == "arrow up" )	_mouseDelta.y -= _mouseSens.y;	else
			if ( key == "arrow down" )	_mouseDelta.y += _mouseSens.y;

			// rotate left/right
			if ( key == "arrow right" )	_mouseDelta.x += _mouseSens.x;	else
			if ( key == "arrow left" )	_mouseDelta.x -= _mouseSens.x;
		}

		if ( action == EKeyAction::Down )
		{
			if ( key == "escape" and _window )	_window->Quit();
		}

		if ( key == "left mb" )			_mousePressed = (action != EKeyAction::Up);
	}
	
/*
=================================================
	OnMouseMove
=================================================
*/
	void  BaseSceneApp::OnMouseMove (const float2 &pos)
	{
		if ( _mousePressed and _enableCamera )
		{
			vec2	delta = vec2{pos.x, pos.y} - _lastMousePos;
			_mouseDelta   += delta * _mouseSens;
		}
		_lastMousePos = vec2{pos.x, pos.y};
	}
	
/*
=================================================
	HmdStatusChanged
=================================================
*/
	void  BaseSceneApp::HmdStatusChanged (EHmdStatus)
	{
	}
	
/*
=================================================
	OnAxisStateChanged
=================================================
*/
	void  BaseSceneApp::OnAxisStateChanged (ControllerID id, StringView name, const float2 &value, const float2 &delta, float dt)
	{
		if ( name == "dpad" )
		{
			_positionDelta += vec3{ value.y * dt, -value.x * dt, 0.0f } * 1.0f;
		}

		//FG_LOGI( "Idx: "s << ToString(index) << ", delta: " << ToString(delta) );
		Unused( id, delta );
	}
	
/*
=================================================
	OnButton
=================================================
*/
	void  BaseSceneApp::OnButton (ControllerID id, StringView btn, EButtonAction action)
	{
		Unused( id, btn, action );
		/*if ( id == ControllerID::RightHand and action != EButtonAction::Up )
		{
			if ( btn == "dpad up" )			_positionDelta.x += 1.0f;	else
			if ( btn == "dpad down" )		_positionDelta.x -= 1.0f;
			if ( btn == "dpad right" )		_positionDelta.y -= 1.0f;	else
			if ( btn == "dpad left" )		_positionDelta.y += 1.0f;
		}*/
	}

/*
=================================================
	_UpdateFrameStat
=================================================
*/
	void  BaseSceneApp::_UpdateFrameStat ()
	{
		using namespace std::chrono;

		++_frameStat.frameCounter;

		TimePoint_t		now			= TimePoint_t::clock::now();
		int64_t			duration	= duration_cast<milliseconds>(now - _frameStat.lastUpdateTime).count();

		auto&	stat = _frameStat.fgStat;
		_frameGraph->GetStatistics( OUT stat );

		_frameStat.gpuTimeSum += stat.renderer.gpuTime;
		_frameStat.cpuTimeSum += stat.renderer.cpuTime + stat.renderer.submitingTime + stat.renderer.waitingTime;

		if ( duration > _frameStat.UpdateIntervalMillis )
		{
			uint		fps_value	= uint(float(_frameStat.frameCounter) / float(duration) * 1000.0f + 0.5f);
			Nanoseconds	gpu_time	= _frameStat.gpuTimeSum / _frameStat.frameCounter;
			Nanoseconds	cpu_time	= _frameStat.cpuTimeSum / _frameStat.frameCounter;

			_frameStat.lastUpdateTime	= now;
			_frameStat.gpuTimeSum		= Default;
			_frameStat.cpuTimeSum		= Default;
			_frameStat.frameCounter		= 0;

			String	additional;
			OnUpdateFrameStat( OUT additional );

			_window->SetTitle( String(_title) << " [FPS: " << ToString(fps_value) <<
							   ", GPU: " << ToString(gpu_time) <<
							   ", CPU: " << ToString(cpu_time) <<
							   additional << ']' );
		}
	}
	
/*
=================================================
	Update
=================================================
*/
	bool  BaseSceneApp::Update ()
	{
		if ( _window )
		{
			if ( not _window->Update() )
				return false;
		}

		if ( _vrDevice )
			_vrDevice->Update();

		if ( not IsActiveVR() and Any( GetSurfaceSize() == uint2(0) ))
		{
			std::this_thread::sleep_for(SecondsF{0.01f});	// ~100 fps
			return true;
		}

		// wait frame-2 for double buffering
		_frameGraph->Wait({ _submittedBuffers[_frameId] });
		_submittedBuffers[_frameId] = null;
		
		auto	time	= TimePoint_t::clock::now();
		_timeDelta		= std::chrono::duration_cast<SecondsF>( time - _lastUpdateTime );
		_lastUpdateTime = time;

		if ( not DrawScene() )
			return true;

		++_frameId;
		
		CHECK_ERR( _frameGraph->Flush() );

		_UpdateFrameStat();
		return true;
	}
	
/*
=================================================
	_SetLastCommandBuffer
=================================================
*/
	void  BaseSceneApp::_SetLastCommandBuffer (const CommandBuffer &cmdbuf)
	{
		_submittedBuffers[_frameId] = cmdbuf;
	}
	
/*
=================================================
	AfterRender
=================================================
*/
	void  BaseSceneApp::AfterRender (const CommandBuffer &cmdbuf, Present &&task)
	{
		cmdbuf->AddTask( task.SetSwapchain( GetSwapchain() ));

		_SetLastCommandBuffer( cmdbuf );
	}

/*
=================================================
	_OnShaderTraceReady
=================================================
*/
	void  BaseSceneApp::_OnShaderTraceReady (StringView name, ArrayView<String> output) const
	{
	#ifdef FS_HAS_FILESYSTEM
		const auto	IsExists = [] (StringView path) { return FS::exists(FS::path{ path }); };
	#else
		const auto	IsExists = [] (StringView path) { return false; };	// TODO
	#endif
		
		String	fname;

		const auto	BuildName = [this, &fname, &name] (uint index)
		{
			fname = String(_debugOutputPath) << '/' << name << '_' << ToString(index) << ".glsl_dbg";
		};

		for (auto& str : output)
		{
			uint		min_index	= 0;
			uint		max_index	= 1;
			const uint	step		= 100;

			for (; min_index < max_index;)
			{
				BuildName( max_index );

				if ( not IsExists( fname ))
					break;

				min_index = max_index;
				max_index += step;
			}

			for (uint index = min_index; index <= max_index; ++index)
			{
				BuildName( index );

				if ( IsExists( fname ))
					continue;

				FileWStream		file{ fname };
				
				if ( file.IsOpen() )
					CHECK( file.Write( str ));
				
				FG_LOGI( "Shader trace saved to '"s << fname << "'" );
				
				// for quick access from VS
				#if defined(COMPILER_MSVC) and defined(FG_DEBUG)
				::OutputDebugStringA( (String{fname} << "(1): trace saved\n").c_str() );
				#endif
				break;
			}
		}
	
		//CHECK(!"shader trace is ready");
	}

}	// FG
