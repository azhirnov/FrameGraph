// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "UIApp.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "imgui_internal.h"


namespace FG
{
	
/*
=================================================
	constructor
=================================================
*/
	UIApp::UIApp () :
		_clearColor{ 0.45f, 0.55f, 0.60f, 1.00f }
	{
		_mouseJustPressed.fill( EKeyAction::Up );
	}

/*
=================================================
	OnResize
=================================================
*/
	void UIApp::OnResize (const uint2 &size)
	{
		VulkanSwapchainCreateInfo	swapchain_info;
		swapchain_info.surface		= BitCast<SurfaceVk_t>( _vulkan.GetVkSurface() );
		swapchain_info.surfaceSize  = size;

		CHECK_FATAL( _frameGraph->RecreateSwapchain( swapchain_info ));

		_frameGraph->ReleaseResource( INOUT _renderTarget );
	}

/*
=================================================
	_Initialize
=================================================
*/
	bool UIApp::_Initialize (WindowPtr &&wnd)
	{
		const uint2		wnd_size{ 800, 600 };

		// initialize window
		{
			_window = std::move(wnd);
			CHECK_ERR( _window->Create( wnd_size, "UITest" ) );
			_window->AddListener( this );
		}

		// initialize vulkan device
		{
			CHECK_ERR( _vulkan.Create( _window->GetVulkanSurface(), "UITest", "FrameGraph", VK_API_VERSION_1_1,
									   "",
									   {},
									   VulkanDevice::GetRecomendedInstanceLayers(),
									   VulkanDevice::GetRecomendedInstanceExtensions(),
									   VulkanDevice::GetAllDeviceExtensions()
									));
			//_vulkan.CreateDebugReportCallback( DebugReportFlags_All );
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
			//_fgInstance->SetCompilationFlags( ECompilationFlags::Unknown );

			ThreadDesc	desc{ EThreadUsage::Present | EThreadUsage::Graphics | EThreadUsage::Transfer };

			_frameGraph = _fgInstance->CreateThread( desc );
			CHECK_ERR( _frameGraph );
			CHECK_ERR( _frameGraph->Initialize( &swapchain_info ));
		}
		
		// add glsl pipeline compiler
		{
			auto	ppln_compiler = MakeShared<VPipelineCompiler>( vulkan_info.physicalDevice, vulkan_info.device );

			ppln_compiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations );

			_fgInstance->AddPipelineCompiler( ppln_compiler );
		}

		// initialize imgui and renderer
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGui::StyleColorsDark();

			CHECK_ERR( _uiRenderer.Initialize( _frameGraph, GImGui ));

			_lastUpdateTime = TimePoint_t::clock::now();
		}
		return true;
	}

/*
=================================================
	_Update
=================================================
*/
	bool UIApp::_Update ()
	{
		if ( not _window->Update() )
			return false;
		
		if ( not _UpdateUI() )
			return false;

		_Draw();

		return true;
	}
	
/*
=================================================
	_UpdateUI
=================================================
*/
	bool UIApp::_UpdateUI ()
	{
		ImGuiIO &	io = ImGui::GetIO();
		CHECK_ERR( io.Fonts->IsBuilt() );

		const TimePoint_t	curr_time	= TimePoint_t::clock::now();
		const SecondsF		dt			= std::chrono::duration_cast<SecondsF>( curr_time - _lastUpdateTime );
		_lastUpdateTime = curr_time;

		io.DisplaySize	= ImVec2{float(_window->GetSize().x), float(_window->GetSize().y)};
		io.DeltaTime	= dt.count();

		CHECK_ERR( _UpdateInput() );
		
		bool	show_demo_window	= true;
		bool	show_another_window	= false;
		auto&	clear_color			= BitCast<ImVec4>( _clearColor );

		ImGui::NewFrame();
		
		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if ( show_demo_window )
			ImGui::ShowDemoWindow( OUT &show_demo_window );

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");							// Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");				// Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);		// Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", OUT &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);			// Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", &clear_color.x);		// Edit 3 floats representing a color

			if (ImGui::Button("Button"))							// Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if ( show_another_window )
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		ImGui::Render();

		//if ( not show_demo_window )
		//	return false;

		return true;
	}

/*
=================================================
	_Draw
=================================================
*/
	bool UIApp::_Draw ()
	{
		auto&	draw_data = *ImGui::GetDrawData();

		if ( draw_data.TotalVtxCount == 0 )
			return false;

		if ( not _renderTarget )
		{
			_renderTarget = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{_window->GetSize().x, _window->GetSize().y, 1},
																 EPixelFormat::RGBA8_UNorm, EImageUsage::ColorAttachment | EImageUsage::Transfer },
													  Default, "UI.ColorRenderTarget" );
		}

		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
		CHECK_ERR( _frameGraph->Begin( batch_id, 0, EThreadUsage::Graphics ));
		{
			LogicalPassID	pass_id = _frameGraph->CreateRenderPass( RenderPassDesc{ int2{float2{ draw_data.DisplaySize.x, draw_data.DisplaySize.y }} }
											.AddViewport(float2{ draw_data.DisplaySize.x, draw_data.DisplaySize.y })
											.AddTarget( RenderTargetID("out_Color0"), _renderTarget, _clearColor, EAttachmentStoreOp::Store )
											.AddColorBuffer( RenderTargetID("out_Color0"), EBlendFactor::SrcAlpha, EBlendFactor::OneMinusSrcAlpha, EBlendOp::Add )
											.SetDepthTestEnabled( false ).SetCullMode( ECullMode::None ));

			Task	draw_ui	= _uiRenderer.Draw( _frameGraph, pass_id );
			Task	present	= _frameGraph->AddTask( Present{ _renderTarget }.DependsOn( draw_ui ));
			FG_UNUSED( present );
		}
		CHECK_ERR( _frameGraph->Execute() );
		CHECK_ERR( _fgInstance->EndFrame() );

		return true;
	}
	
/*
=================================================
	_UpdateInput
=================================================
*/
	bool UIApp::_UpdateInput ()
	{
		ImGuiIO &	io = ImGui::GetIO();

		for (size_t i = 0; i < _mouseJustPressed.size(); ++i)
		{
			io.MouseDown[i] = (_mouseJustPressed[i] != EKeyAction::Up);
		}

		io.MousePos = { _lastMousePos.x, _lastMousePos.y };

		memset( io.NavInputs, 0, sizeof(io.NavInputs) );
		return true;
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void UIApp::OnKey (StringView key, EKeyAction action)
	{
		if ( key == "left mb" )		_mouseJustPressed[0] = action;
		if ( key == "right mb" )	_mouseJustPressed[1] = action;
		if ( key == "middle mb" )	_mouseJustPressed[2] = action;
	}
	
/*
=================================================
	OnMouseMove
=================================================
*/
	void UIApp::OnMouseMove (const float2 &pos)
	{
		_lastMousePos = pos;
	}

/*
=================================================
	_Destroy
=================================================
*/
	void UIApp::_Destroy ()
	{
		if ( _frameGraph )
		{
			_frameGraph->ReleaseResource( _renderTarget );
			_uiRenderer.Deinitialize( _frameGraph );

			_frameGraph->Deinitialize();
			_frameGraph = null;
		}

		if ( _fgInstance )
		{
			_fgInstance->Deinitialize();
			_fgInstance = null;
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
	Run
=================================================
*/
	void UIApp::Run ()
	{
		UIApp				app;
		UniquePtr<IWindow>	wnd;
		
		#if defined( FG_ENABLE_GLFW )
			wnd.reset( new WindowGLFW() );

		#elif defined( FG_ENABLE_SDL2 )
			wnd.reset( new WindowSDL2() );
			
		#elif defined(FG_ENABLE_SFML)
			wnd.reset( new WindowSFML() );

		#else
		#	error Unknown window library!
		#endif

		CHECK_FATAL( app._Initialize( std::move(wnd) ));

		for (; app._Update(); ) {}

		app._Destroy();
	}


}	// FG
