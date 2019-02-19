// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SceneApp.h"
#include "scene/Loader/Assimp/AssimpLoader.h"
#include "scene/Loader/DevIL/DevILLoader.h"
#include "scene/Renderer/Prototype/RendererPrototype.h"
#include "scene/SceneManager/Simple/SimpleRayTracingScene.h"
#include "scene/SceneManager/DefaultSceneManager.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	SceneApp::SceneApp ()
	{
		_frameStat.lastUpdateTime = TimePoint_t::clock::now();
	}
	
/*
=================================================
	destructor
=================================================
*/
	SceneApp::~SceneApp ()
	{
		if ( _scene )
		{
			_scene->Destroy( _frameGraph );
			_scene = null;
		}

		if ( _renderTech )
		{
			_renderTech->Destroy();
			_renderTech = null;
		}

		if ( _frameGraph )
		{
			_frameGraph->Deinitialize();
			_frameGraph = null;
		}

		if ( _fgInstance )
		{
			_fgInstance->Deinitialize();
			_fgInstance = null;
		}
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool SceneApp::Initialize ()
	{
		// will crash if it is not created as shader pointer
		ASSERT( shared_from_this() );

		CHECK_ERR( _CreateFrameGraph() );

		auto	renderer	= MakeShared<RendererPrototype>();
				_scene		= MakeShared<DefaultSceneManager>();
				
		CHECK_ERR( renderer->Create( _fgInstance, _frameGraph ));
		_renderTech = renderer;

		CHECK_ERR( _scene->Add( _LoadScene2() ));
		CHECK_ERR( _scene->Build( _renderTech ));
		
		_lastUpdateTime = TimePoint_t::clock::now();
		return true;
	}
	
/*
=================================================
	_LoadScene2
=================================================
*/
	SceneHierarchyPtr  SceneApp::_LoadScene2 () const
	{
		CommandBatchID		batch_id {"upload"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );

		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
		CHECK_ERR( _frameGraph->Begin( batch_id, 0, EQueueUsage::Graphics ));

		AssimpLoader			loader;
		AssimpLoader::Config	cfg;

		IntermScenePtr	temp_scene = loader.Load( cfg, FG_DATA_PATH R"(..\_data\sponza\sponza.obj)" );
		CHECK_ERR( temp_scene );
		
		DevILLoader		img_loader;
		CHECK_ERR( img_loader.Load( temp_scene, {FG_DATA_PATH R"(..\_data\sponza)"}, _scene->GetImageCache() ));
		
		Transform	transform;
		transform.scale	= 0.01f;

		auto		hierarchy = MakeShared<SimpleRayTracingScene>();
		CHECK_ERR( hierarchy->Create( _frameGraph, temp_scene, _scene->GetImageCache(), transform ));

		CHECK_ERR( _frameGraph->Execute() );
		CHECK_ERR( _fgInstance->EndFrame() );

		return hierarchy;
	}

/*
=================================================
	_CreateFrameGraph
=================================================
*/
	bool SceneApp::_CreateFrameGraph ()
	{
		const uint2		wnd_size{ 1280, 960 };

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

			CHECK_ERR( _window->Create( wnd_size, "Ray tracing" ) );
			_window->AddListener( this );
		}

		// initialize vulkan device
		{
			CHECK_ERR( _vulkan.Create( _window->GetVulkanSurface(), "Ray tracing", "FrameGraph", VK_API_VERSION_1_1,
									   "",
									   {},
									   VulkanDevice::GetRecomendedInstanceLayers(),
									   VulkanDevice::GetRecomendedInstanceExtensions(),
									   VulkanDevice::GetAllDeviceExtensions()
									));
			_vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );
		}

		// setup device info
		VulkanDeviceInfo						vulkan_info;
		FrameGraphThread::SwapchainCreateInfo	swapchain_info;
		{
			vulkan_info.instance		= BitCast<InstanceVk_t>( _vulkan.GetVkInstance() );
			vulkan_info.physicalDevice	= BitCast<PhysicalDeviceVk_t>( _vulkan.GetVkPhysicalDevice() );
			vulkan_info.device			= BitCast<DeviceVk_t>( _vulkan.GetVkDevice() );
			
			VulkanSwapchainCreateInfo	swapchain_ci;
			swapchain_ci.surface		= BitCast<SurfaceVk_t>( _vulkan.GetVkSurface() );
			swapchain_ci.surfaceSize	= _window->GetSize();
			//swapchain_ci.presentModes.push_back( BitCast<PresentModeVk_t>(VK_PRESENT_MODE_MAILBOX_KHR) );
			swapchain_ci.presentModes.push_back( BitCast<PresentModeVk_t>(VK_PRESENT_MODE_FIFO_KHR) );	// enable vsync
			swapchain_info				= swapchain_ci;

			for (auto& q : _vulkan.GetVkQuues())
			{
				VulkanDeviceInfo::QueueInfo	qi;
				qi.id			= BitCast<QueueVk_t>( q.id );
				qi.familyFlags	= BitCast<QueueFlagsVk_t>( q.flags );
				qi.familyIndex	= q.familyIndex;
				qi.priority		= q.priority;
				qi.debugName	= "";

				vulkan_info.queues.push_back( qi );
			}
		}

		// initialize framegraph
		{
			_fgInstance = FrameGraphInstance::CreateFrameGraph( vulkan_info );
			CHECK_ERR( _fgInstance );
			CHECK_ERR( _fgInstance->Initialize( 2 ));

			ThreadDesc	desc{ EThreadUsage::Transfer };

			_frameGraph = _fgInstance->CreateThread( desc );
			CHECK_ERR( _frameGraph );
			CHECK_ERR( _frameGraph->Initialize( &swapchain_info ));
		}

		// add glsl pipeline compiler
		{
			auto	compiler = MakeShared<VPipelineCompiler>( vulkan_info.physicalDevice, vulkan_info.device );
			compiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations |
										   EShaderCompilationFlags::Quiet			 |
										   EShaderCompilationFlags::ParseAnnoations );

			_fgInstance->AddPipelineCompiler( compiler );
		}

		return true;
	}

/*
=================================================
	Update
=================================================
*/
	bool SceneApp::Update ()
	{
		if ( not _window->Update() )
			return false;

		_scene->Draw({ shared_from_this() });

		_UpdateFrameStat();
		return true;
	}
	
/*
=================================================
	OnResize
=================================================
*/
	void SceneApp::OnResize (const uint2 &size)
	{
		VulkanSwapchainCreateInfo	swapchain_info;
		swapchain_info.surface		= BitCast<SurfaceVk_t>( _vulkan.GetVkSurface() );
		swapchain_info.surfaceSize  = size;

		CHECK_FATAL( _frameGraph->RecreateSwapchain( swapchain_info ));
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void SceneApp::OnKey (StringView key, EKeyAction action)
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
	void SceneApp::OnMouseMove (const float2 &pos)
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
	Prepare
=================================================
*/
	void SceneApp::Prepare (ScenePreRender &preRender)
	{
		const auto	time	= TimePoint_t::clock::now();
		const auto	dt		= std::chrono::duration_cast<SecondsF>( time - _lastUpdateTime ).count();
		_lastUpdateTime = time;
		
		const uint2	wnd_size	= _window->GetSize();
		const vec2	view_size	{ wnd_size.x, wnd_size.y };
		//const vec2	view_size	{ 3840.0f, 2160.0f };
		const vec2	range		{ 0.05f, 100.0f };

		_camera.SetPerspective( 70_deg, view_size.x / view_size.y, range.x, range.y );
		_camera.Rotate( -_mouseDelta.x, _mouseDelta.y );

		if ( length2( _positionDelta ) > 0.01f ) {
			_positionDelta = normalize(_positionDelta) * _velocity * dt;
			_camera.Move2( _positionDelta );
		}

		LayerBits	layers;
		layers[uint(ERenderLayer::RayTracing)] = true;

		preRender.AddCamera( _camera.GetCamera(), view_size, range, DetailLevelRange{}, ECameraType::Main, layers, shared_from_this() );

		// reset
		_positionDelta	= vec3{0.0f};
		_mouseDelta		= vec2{0.0f};
	}
	
/*
=================================================
	Draw
=================================================
*/
	void SceneApp::Draw (RenderQueue &) const
	{
	}
	
/*
=================================================
	AfterRender
=================================================
*/
	void SceneApp::AfterRender (const FGThreadPtr &fg, const Present &task)
	{
		FG_UNUSED( fg->AddTask( task ));
	}
	
/*
=================================================
	_UpdateFrameStat
=================================================
*/
	void SceneApp::_UpdateFrameStat ()
	{
		using namespace std::chrono;

		++_frameStat.frameCounter;

		TimePoint_t		now			= TimePoint_t::clock::now();
		int64_t			duration	= duration_cast<milliseconds>(now - _frameStat.lastUpdateTime).count();

		FrameGraphInstance::Statistics	stat;
		_fgInstance->GetStatistics( OUT stat );
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

			_window->SetTitle( "RayTracing [FPS: "s << ToString(fps_value) <<
							   ", GPU: " << ToString(gpu_time) <<
							   ", CPU: " << ToString(cpu_time) << ']' );
		}
	}

}	// FG

/*
=================================================
	main
=================================================
*/
int main ()
{
	using namespace FG;
	{
		auto	scene = MakeShared<SceneApp>();

		CHECK_ERR( scene->Initialize(), -1 );

		for (; scene->Update();) {}
	}
	return 0;
}
