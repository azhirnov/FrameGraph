// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "BaseSceneApp.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "framework/VR/OpenVRDevice.h"
#include "framework/VR/VRDeviceEmulator.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"

#ifdef FG_STD_FILESYSTEM
#	include <filesystem>
	namespace FS = std::filesystem;
#endif

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
	bool BaseSceneApp::_CreateFrameGraph (const uint2 &windowSize, StringView windowTitle,
										  ArrayView<StringView> shaderDirectories, StringView dbgOutputPath)
	{
		// initialize window
		{
			#if defined( FG_ENABLE_GLFW )
				_window.reset( new WindowGLFW() );

			#elif defined( FG_ENABLE_SDL2 )
				_window.reset( new WindowSDL2() );
			
			#elif defined(FG_ENABLE_SFML)
				_window.reset( new WindowSFML() );

			#else
			#	error Unknown window library!
			#endif

			_title = windowTitle;

			CHECK_ERR( _window->Create( windowSize, _title ));
			_window->AddListener( this );
		}

		// initialize VR
		{
			#if defined(FG_ENABLE_OPENVR)
				_vrDevice.reset( new OpenVRDevice{});
			#elif 0
			{
				WindowPtr	wnd{ new WindowGLFW{}};
				CHECK_ERR( wnd->Create( { 1024, 512 }, "emulator" ));
				_vrDevice.reset( new VRDeviceEmulator{ std::move(wnd) });
			}
			#endif

			if ( _vrDevice and not _vrDevice->Create() ) {
				_vrDevice.reset();
			}
		}

		// initialize vulkan device
		{
			Array<const char*>	inst_ext	{ VulkanDevice::GetRecomendedInstanceExtensions() };
			Array<const char*>	dev_ext		{ VulkanDevice::GetAllDeviceExtensions() };
			Array<String>		vr_inst_ext	= _vrDevice ? _vrDevice->GetRequiredInstanceExtensions() : Default;
			Array<String>		vr_dev_ext	= _vrDevice ? _vrDevice->GetRequiredDeviceExtensions() : Default;

			for (auto& ext : vr_inst_ext) {
				inst_ext.push_back( ext.data() );
			}
			for (auto& ext : vr_dev_ext) {
				dev_ext.push_back( ext.data() );
			}

			CHECK_ERR( _vulkan.Create( _window->GetVulkanSurface(), _title, "FrameGraph", VK_API_VERSION_1_1,
									   "",
									   {},
									   {}, //VulkanDevice::GetRecomendedInstanceLayers(),
									   inst_ext,
									   dev_ext
									));
			_vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );
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

			if ( _vrDevice )
				swapchain_info.presentModes.push_back( BitCast<PresentModeVk_t>(VK_PRESENT_MODE_MAILBOX_KHR) );
			else
				swapchain_info.presentModes.push_back( BitCast<PresentModeVk_t>(VK_PRESENT_MODE_FIFO_KHR) );	// enable vsync

			for (auto& q : _vulkan.GetVkQueues())
			{
				VulkanDeviceInfo::QueueInfo	qi;
				qi.handle		= BitCast<QueueVk_t>( q.handle );
				qi.familyFlags	= BitCast<QueueFlagsVk_t>( q.flags );
				qi.familyIndex	= q.familyIndex;
				qi.priority		= q.priority;
				qi.debugName	= "";

				vulkan_info.queues.push_back( qi );
			}
		}

		if ( _vrDevice ) {
			CHECK_ERR( _vrDevice->SetVKDevice( _vulkan.GetVkInstance(), _vulkan.GetVkPhysicalDevice(), _vulkan.GetVkDevice() ));
		}

		// initialize framegraph
		{
			_frameGraph = IFrameGraph::CreateFrameGraph( vulkan_info );
			CHECK_ERR( _frameGraph );

			_swapchainId = _frameGraph->CreateSwapchain( swapchain_info );
			CHECK_ERR( _swapchainId );

			_frameGraph->SetShaderDebugCallback([this] (auto name, auto, auto, auto output) { _OnShaderTraceReady(name, output); });
		}

		// add glsl pipeline compiler
		// TODO: ShaderCache also adds shader compiler, so remove one of them
		{
			auto	compiler = MakeShared<VPipelineCompiler>( vulkan_info.physicalDevice, vulkan_info.device );
			compiler->SetCompilationFlags( EShaderCompilationFlags::Quiet			|
										   EShaderCompilationFlags::ParseAnnoations	|
										   EShaderCompilationFlags::UseCurrentDeviceLimits );

			for (auto& dir : shaderDirectories) {
				compiler->AddDirectory( dir );
			}

			_frameGraph->AddPipelineCompiler( compiler );
		}
		
		// setup debug output
#		ifdef FG_STD_FILESYSTEM
		if ( dbgOutputPath.size() )
		{
			FS::path	path{ dbgOutputPath };
		
			if ( FS::exists( path ) )
				FS::remove_all( path );
		
			CHECK( FS::create_directory( path ));

			_debugOutputPath = path.string();
		}
#		endif	// FG_STD_FILESYSTEM

		_SetupCamera( _cameraFov, _viewRange );
		return true;
	}
	
/*
=================================================
	_DestroyFrameGraph
=================================================
*/
	void BaseSceneApp::_DestroyFrameGraph ()
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

		_vulkan.Destroy();

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
	void BaseSceneApp::_SetupCamera (Rad fovY, const vec2 &viewRange)
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
	_UpdateCamera
=================================================
*/
	void BaseSceneApp::_UpdateCamera ()
	{
		auto	time	= TimePoint_t::clock::now();
		auto	dt		= std::chrono::duration_cast<SecondsF>( time - _lastUpdateTime ).count();
		_lastUpdateTime = time;
		
		if ( _vrDevice and _vrDevice->GetHmdStatus() == IVRDevice::EHmdStatus::Mounted )
		{
			IVRDevice::VRCamera	cam;
			_vrDevice->GetCamera( OUT cam );
			
			if ( length2( _positionDelta ) > 0.01f ) {
				_positionDelta = normalize(_positionDelta) * _cameraVelocity * dt;
				_vrCamera.Move2( _positionDelta );
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

			_camera.SetPerspective( _cameraFov, view_size.x / view_size.y, _viewRange.x, _viewRange.y );
			_camera.Rotate( -_mouseDelta.x, _mouseDelta.y );

			if ( length2( _positionDelta ) > 0.01f ) {
				_positionDelta = normalize(_positionDelta) * _cameraVelocity * dt;
				_camera.Move2( _positionDelta );
			}
			_vrCamera.SetPosition( _camera.GetCamera().transform.position );
		}

		// reset
		_positionDelta	= vec3{0.0f};
		_mouseDelta		= vec2{0.0f};
	}

/*
=================================================
	OnResize
=================================================
*/
	void BaseSceneApp::OnResize (const uint2 &size)
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
	void BaseSceneApp::OnKey (StringView key, EKeyAction action)
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
			if ( key == "arrow up" )	_mouseDelta.y -= 0.01f;		else
			if ( key == "arrow down" )	_mouseDelta.y += 0.01f;

			// rotate left/right
			if ( key == "arrow right" )	_mouseDelta.x += 0.01f;		else
			if ( key == "arrow left" )	_mouseDelta.x -= 0.01f;
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
	void BaseSceneApp::OnMouseMove (const float2 &pos)
	{
		if ( _mousePressed )
		{
			vec2	delta = vec2{pos.x, pos.y} - _lastMousePos;
			_mouseDelta   += delta * 0.01f;
		}
		_lastMousePos = vec2{pos.x, pos.y};
	}
	
/*
=================================================
	_UpdateFrameStat
=================================================
*/
	void BaseSceneApp::_UpdateFrameStat (StringView additionalParams)
	{
		using namespace std::chrono;

		++_frameStat.frameCounter;

		TimePoint_t		now			= TimePoint_t::clock::now();
		int64_t			duration	= duration_cast<milliseconds>(now - _frameStat.lastUpdateTime).count();

		IFrameGraph::Statistics		stat;
		_frameGraph->GetStatistics( OUT stat );
		_frameStat.gpuTimeSum += stat.renderer.gpuTime;
		_frameStat.cpuTimeSum += stat.renderer.cpuTime;

		if ( duration > _frameStat.UpdateIntervalMillis )
		{
			uint		fps_value	= uint(float(_frameStat.frameCounter) / float(duration) * 1000.0f + 0.5f);
			Nanoseconds	gpu_time	= _frameStat.gpuTimeSum / _frameStat.frameCounter;
			Nanoseconds	cpu_time	= _frameStat.cpuTimeSum / _frameStat.frameCounter;

			_frameStat.lastUpdateTime	= now;
			_frameStat.gpuTimeSum		= Default;
			_frameStat.cpuTimeSum		= Default;
			_frameStat.frameCounter		= 0;

			_window->SetTitle( String(_title) << " [FPS: " << ToString(fps_value) <<
							   ", GPU: " << ToString(gpu_time) <<
							   ", CPU: " << ToString(cpu_time) <<
							   additionalParams << ']' );
		}
	}
	
/*
=================================================
	Update
=================================================
*/
	bool BaseSceneApp::Update ()
	{
		if ( not GetWindow()->Update() )
			return false;

		if ( Any( GetSurfaceSize() == uint2(0) ))
		{
			std::this_thread::sleep_for(SecondsF{0.01f});	// ~100 fps
			return true;
		}

		if ( _vrDevice ) {
			_vrDevice->Update();
		}

		// wait frame-2 for double buffering
		_frameGraph->Wait({ _submittedBuffers[_frameId] });
		_submittedBuffers[_frameId] = null;

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
	void BaseSceneApp::_SetLastCommandBuffer (const CommandBuffer &cmdbuf)
	{
		_submittedBuffers[_frameId] = cmdbuf;
	}
	
/*
=================================================
	AfterRender
=================================================
*/
	void BaseSceneApp::AfterRender (const CommandBuffer &cmdbuf, Present &&task)
	{
		FG_UNUSED( cmdbuf->AddTask( task.SetSwapchain( GetSwapchain() ) ));

		_SetLastCommandBuffer( cmdbuf );
	}

/*
=================================================
	_OnShaderTraceReady
=================================================
*/
	void BaseSceneApp::_OnShaderTraceReady (StringView name, ArrayView<String> output) const
	{
	#	ifdef FG_STD_FILESYSTEM
		const auto	IsExists = [] (StringView path) { return FS::exists(FS::path{ path }); };
	#	else
		// TODO
	#	endif

		String	fname;

		for (auto& str : output)
		{
			for (uint index = 0; index < 100; ++index)
			{
				fname = String(_debugOutputPath) << '/' << name << '_' << ToString(index) << ".glsl_dbg";

				if ( IsExists( fname ) )
					continue;

				FileWStream		file{ fname };
				
				if ( file.IsOpen() )
					CHECK( file.Write( str ));
				
				FG_LOGI( "Shader trace saved to '"s << fname << "'" );
				break;
			}
		}
	
		//CHECK(!"shader trace is ready");
	}

}	// FG
