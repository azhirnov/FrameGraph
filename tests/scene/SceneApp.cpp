// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SceneApp.h"
//#include "scene/Loader/Assimp/AssimpLoader.h"
//#include "scene/Loader/DevIL/DevILLoader.h"
#include "scene/Renderer/Prototype/RendererPrototype.h"
#include "scene/SceneManager/Simple/SimpleScene.h"
#include "scene/SceneManager/DefaultSceneManager.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	SceneApp::SceneApp ()
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	SceneApp::~SceneApp ()
	{
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool SceneApp::Initialize ()
	{
		CHECK_ERR( _CreateFrameGraph() );

		auto	renderer	= MakeShared<RendererPrototype>();
				scene		= MakeShared<DefaultSceneManager>();
				
		CHECK_ERR( renderer->Create( frameGraph ));
		renderTech = renderer;

		// TODO
		return true;
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
				window.reset( new WindowGLFW() );

			#elif defined( FG_ENABLE_SDL2 )
				window.reset( new WindowSDL2() );
			
			#elif defined(FG_ENABLE_SFML)
				window.reset( new WindowSFML() );

			#else
			#	error Unknown window library!
			#endif

			CHECK_ERR( window->Create( wnd_size, "SceneTest" ) );
			window->AddListener( this );
		}

		// initialize vulkan device
		{
			CHECK_ERR( vulkan.Create( window->GetVulkanSurface(), "SceneTest", "FrameGraph", VK_API_VERSION_1_1,
									   "",
									   {},
									   VulkanDevice::GetRecomendedInstanceLayers(),
									   VulkanDevice::GetRecomendedInstanceExtensions(),
									   VulkanDevice::GetAllDeviceExtensions()
									));
			vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );
		}

		// setup device info
		VulkanDeviceInfo			vulkan_info;
		VulkanSwapchainCreateInfo	swapchain_info;
		{
			vulkan_info.instance		= BitCast<InstanceVk_t>( vulkan.GetVkInstance() );
			vulkan_info.physicalDevice	= BitCast<PhysicalDeviceVk_t>( vulkan.GetVkPhysicalDevice() );
			vulkan_info.device			= BitCast<DeviceVk_t>( vulkan.GetVkDevice() );

			swapchain_info.surface		= BitCast<SurfaceVk_t>( vulkan.GetVkSurface() );
			swapchain_info.surfaceSize	= window->GetSize();

			for (auto& q : vulkan.GetVkQuues())
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

		// initialize framegraph
		{
			frameGraph = IFrameGraph::CreateFrameGraph( vulkan_info );
			CHECK_ERR( frameGraph );

			swapchainId = frameGraph->CreateSwapchain( swapchain_info );
			CHECK_ERR( swapchainId );
		}

		// add glsl pipeline compiler
		{
			auto	compiler = MakeShared<VPipelineCompiler>( vulkan_info.physicalDevice, vulkan_info.device );
			compiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations | EShaderCompilationFlags::Quiet );

			pplnCompiler = compiler;
			frameGraph->AddPipelineCompiler( compiler );
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
		if ( not window->Update() )
			return false;

		//scene->Draw({ shared_from_this() });
		return true;
	}
	
/*
=================================================
	OnResize
=================================================
*/
	void SceneApp::OnResize (const uint2 &size)
	{
		if ( Any( size == uint2(0) ))
			return;

		VulkanSwapchainCreateInfo	swapchain_info;
		swapchain_info.surface		= BitCast<SurfaceVk_t>( vulkan.GetVkSurface() );
		swapchain_info.surfaceSize  = size;
		
		frameGraph->WaitIdle();

		swapchainId = frameGraph->CreateSwapchain( swapchain_info, swapchainId.Release() );
		CHECK_FATAL( swapchainId );
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void SceneApp::OnKey (StringView key, EKeyAction action)
	{
	}
	
/*
=================================================
	OnMouseMove
=================================================
*/
	void SceneApp::OnMouseMove (const float2 &pos)
	{
	}

/*
=================================================
	Prepare
=================================================
*/
	void SceneApp::Prepare (ScenePreRender &preRender)
	{
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
	void SceneApp::AfterRender (const CommandBuffer &cmdbuf, Present &&task)
	{
		FG_UNUSED( cmdbuf->AddTask( task.SetSwapchain( swapchainId )) );
	}


}	// FG
