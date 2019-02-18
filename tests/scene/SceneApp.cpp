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
				
		CHECK_ERR( renderer->Initialize( fgInstance, frameGraph, "" ));
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
		VulkanDeviceInfo						vulkan_info;
		FrameGraphThread::SwapchainCreateInfo	swapchain_info;
		{
			vulkan_info.instance		= BitCast<InstanceVk_t>( vulkan.GetVkInstance() );
			vulkan_info.physicalDevice	= BitCast<PhysicalDeviceVk_t>( vulkan.GetVkPhysicalDevice() );
			vulkan_info.device			= BitCast<DeviceVk_t>( vulkan.GetVkDevice() );
			
			VulkanSwapchainCreateInfo	swapchain_ci;
			swapchain_ci.surface		= BitCast<SurfaceVk_t>( vulkan.GetVkSurface() );
			swapchain_ci.surfaceSize	= window->GetSize();
			swapchain_info				= swapchain_ci;

			for (auto& q : vulkan.GetVkQuues())
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
			fgInstance = FrameGraphInstance::CreateFrameGraph( vulkan_info );
			CHECK_ERR( fgInstance );
			CHECK_ERR( fgInstance->Initialize( 2 ));
			fgInstance->SetCompilationFlags( ECompilationFlags::EnableDebugger, ECompilationDebugFlags::Default );

			ThreadDesc	desc{ EThreadUsage::Transfer };

			frameGraph = fgInstance->CreateThread( desc );
			CHECK_ERR( frameGraph );
			CHECK_ERR( frameGraph->Initialize( &swapchain_info ));
		}

		// add glsl pipeline compiler
		{
			auto	compiler = MakeShared<VPipelineCompiler>( vulkan_info.physicalDevice, vulkan_info.device );
			compiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations | EShaderCompilationFlags::Quiet );

			pplnCompiler = compiler;
			fgInstance->AddPipelineCompiler( compiler );
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
		VulkanSwapchainCreateInfo	swapchain_info;
		swapchain_info.surface		= BitCast<SurfaceVk_t>( vulkan.GetVkSurface() );
		swapchain_info.surfaceSize  = size;

		CHECK_FATAL( frameGraph->RecreateSwapchain( swapchain_info ));
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
	void SceneApp::AfterRender (const FGThreadPtr &fg, const Present &task)
	{
		FG_UNUSED( fg->AddTask( task ));
	}


}	// FG
