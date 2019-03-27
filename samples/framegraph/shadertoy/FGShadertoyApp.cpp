// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "FGShadertoyApp.h"
#include "graphviz/GraphViz.h"
#include "scene/Loader/DevIL/DevILLoader.h"
#include "scene/Loader/Intermediate/IntermImage.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "stl/Stream/FileStream.h"
#include "stl/Algorithms/StringUtils.h"
#include <thread>

#ifdef FG_STD_FILESYSTEM
#	include <filesystem>
	namespace fs = std::filesystem;
#endif

namespace FG
{
	
/*
=================================================
	constructor
=================================================
*/
	FGShadertoyApp::Shader::Shader (StringView name, ShaderDescr &&desc) :
		_name{ name },
		_pplnFilename{ std::move(desc._pplnFilename) },		_pplnDefines{ std::move(desc._pplnDefines) },
		_channels{ desc._channels },
		_surfaceScale{ desc._surfaceScale },				_surfaceSize{ desc._surfaceSize }
	{}
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	FGShadertoyApp::FGShadertoyApp () :
		_passIdx{0}
	{
		_frameStat.lastUpdateTime = TimePoint_t::clock::now();
	}
	
/*
=================================================
	destructor
=================================================
*/
	FGShadertoyApp::~FGShadertoyApp ()
	{
	}

/*
=================================================
	_InitSamples
=================================================
*/
	void FGShadertoyApp::_InitSamples ()
	{
		_samples.push_back( [this] ()
		{
			ShaderDescr	sh_main;
			sh_main.Pipeline( "st_shaders/Glowballs.glsl" );
			sh_main.InChannel( "main", 0 );
			CHECK( _AddShader( "main", std::move(sh_main) ));
		});

		_samples.push_back( [this] ()
		{
			ShaderDescr	sh_main;
			sh_main.Pipeline( "st_shaders/Skyline.glsl" );
			CHECK( _AddShader( "main", std::move(sh_main) ));
		});
		
		_samples.push_back( [this] ()
		{
			ShaderDescr	sh_main;
			sh_main.Pipeline( "st_shaders/Skyline2.glsl" );
			CHECK( _AddShader( "main", std::move(sh_main) ));
		});
	}
	
/*
=================================================
	OnResize
=================================================
*/
	void FGShadertoyApp::OnResize (const uint2 &size)
	{
		if ( Any( size == uint2(0) ))
			return;

		VulkanSwapchainCreateInfo	swapchain_info;
		swapchain_info.surface		= BitCast<SurfaceVk_t>( _vulkan.GetVkSurface() );
		swapchain_info.surfaceSize  = size;

		_swapchainId = _frameGraph->CreateSwapchain( swapchain_info, _swapchainId.Release() );
		CHECK_FATAL( _swapchainId );
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void FGShadertoyApp::OnKey (StringView key, EKeyAction action)
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
			if ( key == "[" )					--_nextSample;		else
			if ( key == "]" )					++_nextSample;

			if ( key == "R" )					_Recompile();
			if ( key == "T" )					_frameCounter = 0;
			if ( key == "U" )					_debugPixel = _lastMousePos / vec2{_window->GetSize().x, _window->GetSize().y};

			if ( key == "F" )					_freeze = not _freeze;
			if ( key == "space" )				_pause = not _pause;

			if ( key == "escape" and _window )	_window->Quit();
		}

		if ( key == "left mb" )		_mousePressed = (action != EKeyAction::Up);
	}
	
/*
=================================================
	OnMouseMove
=================================================
*/
	void FGShadertoyApp::OnMouseMove (const float2 &pos)
	{
		if ( _mousePressed )
		{
			vec2	delta = vec2{pos.x, pos.y} - _lastMousePos;
			_mouseDelta  += delta * 0.01f;
		}

		_lastMousePos = vec2{pos.x, pos.y};
	}

/*
=================================================
	Initialize
=================================================
*/
	bool FGShadertoyApp::Initialize (WindowPtr &&wnd)
	{
		const uint2		wnd_size{ 1280, 720 };

		// initialize window
		{
			_window = std::move(wnd);
			CHECK_ERR( _window->Create( wnd_size, "Shadertoy" ) );
			_window->AddListener( this );
		}

		// initialize vulkan device
		{
			CHECK_ERR( _vulkan.Create( _window->GetVulkanSurface(), "Test", "FrameGraph", VK_API_VERSION_1_1,
									   "",
									   {},
									   VulkanDevice::GetRecomendedInstanceLayers(),
									   VulkanDevice::GetRecomendedInstanceExtensions(),
									   VulkanDevice::GetAllDeviceExtensions()
									));
			_vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );
		}

		// setup device info
		VulkanDeviceInfo					vulkan_info;
		IFrameGraph::SwapchainCreateInfo_t	swapchain_info;
		{
			vulkan_info.instance		= BitCast<InstanceVk_t>( _vulkan.GetVkInstance() );
			vulkan_info.physicalDevice	= BitCast<PhysicalDeviceVk_t>( _vulkan.GetVkPhysicalDevice() );
			vulkan_info.device			= BitCast<DeviceVk_t>( _vulkan.GetVkDevice() );
			
			VulkanSwapchainCreateInfo	swapchain_ci;
			swapchain_ci.surface		= BitCast<SurfaceVk_t>( _vulkan.GetVkSurface() );
			swapchain_ci.surfaceSize	= _window->GetSize();
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
			_frameGraph = IFrameGraph::CreateFrameGraph( vulkan_info );
			CHECK_ERR( _frameGraph );
			CHECK_ERR( _frameGraph->Initialize() );
			
			_swapchainId = _frameGraph->CreateSwapchain( swapchain_info );
			CHECK_ERR( _swapchainId );

			_frameGraph->SetShaderDebugCallback([this] (auto name, auto, auto, auto output) { _OnShaderTraceReady(name, output); });
		}

		// add glsl pipeline compiler
		{
			auto	ppln_compiler = MakeShared<VPipelineCompiler>( vulkan_info.physicalDevice, vulkan_info.device );
			ppln_compiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations | EShaderCompilationFlags::Quiet );

			_frameGraph->AddPipelineCompiler( ppln_compiler );
		}
		
		// setup debug output
#		ifdef FG_STD_FILESYSTEM
		{
			fs::path	path{ FG_DATA_PATH "_debug_output" };
		
			if ( fs::exists( path ) )
				fs::remove_all( path );
		
			CHECK( fs::create_directory( path ));

			_debugOutputPath = path.string();
		}
#		endif	// FG_STD_FILESYSTEM

		_CreateSamplers();
		_InitSamples();
		
		return true;
	}

/*
=================================================
	Update
=================================================
*/
	bool FGShadertoyApp::Update ()
	{
		if ( not _window->Update() )
			return false;

		if ( _pause or Any(_window->GetSize() == uint2(0)) )
		{
			std::this_thread::sleep_for(SecondsF{0.01f});
			return true;
		}

		_UpdateFrameStat();
		
		// select sample
		if ( _currSample != _nextSample )
		{
			_nextSample = _nextSample % _samples.size();
			_currSample = _nextSample;

			_ResetShaders();
			_samples[_currSample]();
		}

		_cmdBuffer = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });

		if ( Any(_surfaceSize != _window->GetSize()) )
		{
			CHECK_ERR( _RecreateShaders( _window->GetSize() ));

			_camera.SetPerspective( _cameraFov, float(_surfaceSize.x) / _surfaceSize.y, 0.1f, 100.0f );
		}
		
		if ( _ordered.size() )
		{
			_UpdateShaderData();
		
			const uint	pass_idx = _passIdx;

			// run shaders
			for (size_t i = 0; i < _ordered.size(); ++i)
			{
				_DrawWithShader( _ordered[i], pass_idx, i+1 == _ordered.size() );
			}
		
			// present
			{
				ShadersMap_t::iterator	iter = _shaders.find( "main" );
				CHECK_ERR( iter != _shaders.end() );

				const auto&	image	= iter->second->_perPass[pass_idx].renderTarget;
				const auto&	desc	= _frameGraph->GetDescription( image );
				const uint2	point	= { uint((desc.dimension.x * _lastMousePos.x) / _window->GetSize().x + 0.5f),
										uint((desc.dimension.y * _lastMousePos.y) / _window->GetSize().y + 0.5f) };

				if ( point.x < desc.dimension.x and point.y < desc.dimension.y )
				{
					_cmdBuffer->AddTask( ReadImage{}.SetImage( image, point, uint2{1,1} )
													 .SetCallback( [this, point] (const ImageView &view) { _OnPixelReadn( point, view ); })
													 .DependsOn( _currTask ));
				}

				_currTask = _cmdBuffer->AddTask( Present{ _swapchainId, image }.DependsOn( _currTask ));
			}
		}

		CHECK_ERR( _frameGraph->Execute( _cmdBuffer ));
		CHECK_ERR( _frameGraph->Flush() );

		_currTask = null;
		++_passIdx;

		return true;
	}
	
/*
=================================================
	_UpdateShaderData
=================================================
*/
	void FGShadertoyApp::_UpdateShaderData ()
	{
		const auto	time		= TimePoint_t::clock::now();
		const auto	velocity	= 1.0f;

		if ( _frameCounter == 0 )
			_startTime = time;

		const float	app_dt		= std::chrono::duration_cast<SecondsF>( (_freeze ? _lastUpdateTime : time) - _startTime ).count();
		const float	frame_dt	= std::chrono::duration_cast<SecondsF>( time - _lastUpdateTime ).count();
		
		_camera.Rotate( -_mouseDelta.x, _mouseDelta.y );

		if ( length2( _positionDelta ) > 0.01f ) {
			_positionDelta = normalize(_positionDelta) * velocity * frame_dt;
			_camera.Move2( _positionDelta );
		}

		_ubData.iTime				= app_dt;
		_ubData.iTimeDelta			= frame_dt;
		_ubData.iFrame				= _frameCounter;
		_ubData.iMouse				= vec4( _mousePressed ? _lastMousePos : vec2{}, vec2{} );	// TODO: click
		//_ubData.iDate				= float4(uint3( date.Year(), date.Month(), date.DayOfMonth()).To<float3>(), float(date.Second()) + float(date.Milliseconds()) * 0.001f);
		_ubData.iSampleRate			= 0.0f;	// not supported yet
		_ubData.iCameraPos			= _camera.GetCamera().transform.position;
		_ubData.iCameraFovY			= float(_cameraFov);
		_ubData.iCameraOrientation	= _camera.GetCamera().transform.orientation;

		_camera.GetFrustum().GetRays( OUT _ubData.iCameraFrustumRayLB, OUT _ubData.iCameraFrustumRayLT,
									  OUT _ubData.iCameraFrustumRayRB, OUT _ubData.iCameraFrustumRayRT );

		if ( not _freeze ) {
			_lastUpdateTime	= time;
			++_frameCounter;
		}

		_positionDelta = vec3{0.0f};
		_mouseDelta    = vec2{0.0f};
	}

/*
=================================================
	_DrawWithShader
=================================================
*/
	bool FGShadertoyApp::_DrawWithShader (const ShaderPtr &shader, uint passIndex, bool isLast)
	{
		auto&	pass		= shader->_perPass[passIndex];
		auto	view_size	= _frameGraph->GetDescription( pass.renderTarget ).dimension.xy();

		// update uniform buffer
		{
			for (size_t i = 0; i < pass.images.size(); ++i)
			{
				if ( pass.resources.HasTexture(UniformID{ "iChannel"s << ToString(i) }) )
				{
					auto	dim	= _frameGraph->GetDescription( pass.images[i] ).dimension;

					_ubData.iChannelResolution[i]	= vec4{ float(dim.x), float(dim.y), 0.0f, 0.0f };
					_ubData.iChannelTime[i]			= vec4{ _ubData.iTime };
				}
			}
		
			_ubData.iResolution = vec3{ float(view_size.x), float(view_size.y), 0.0f };

			_currTask = _cmdBuffer->AddTask( UpdateBuffer{}.SetBuffer( shader->_ubuffer ).AddData( &_ubData, 1 ).DependsOn( _currTask ));
		}


		auto	pass_id = _cmdBuffer->CreateRenderPass( RenderPassDesc( view_size )
									.AddTarget( RenderTargetID(0), pass.renderTarget, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
									.AddViewport( view_size ) );

		DrawVertices	draw_task;
		draw_task.SetPipeline( shader->_pipeline );
		draw_task.AddResources( DescriptorSetID{"0"}, &pass.resources );
		draw_task.Draw( 4 ).SetTopology( EPrimitive::TriangleStrip );

		if ( isLast and _debugPixel.has_value() )
		{
			const vec2	coord = vec2{pass.viewport.x, pass.viewport.y} * (*_debugPixel) + 0.5f;

			draw_task.EnableFragmentDebugTrace( uint(coord.x), uint(coord.y) );
			_debugPixel.reset();
		}

		_cmdBuffer->AddTask( pass_id, draw_task );

		_currTask = _cmdBuffer->AddTask( SubmitRenderPass{ pass_id }.DependsOn( _currTask ));
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void FGShadertoyApp::Destroy ()
	{
		if ( _frameGraph )
		{
			_ResetShaders();
			_samples.clear();
			
			_frameGraph->ReleaseResource( INOUT _nearestClampSampler );
			_frameGraph->ReleaseResource( INOUT _linearClampSampler );
			_frameGraph->ReleaseResource( INOUT _nearestRepeatSampler );
			_frameGraph->ReleaseResource( INOUT _linearRepeatSampler );
			_frameGraph->ReleaseResource( INOUT _swapchainId );

			_frameGraph->Deinitialize();
			_frameGraph = null;
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
	_RecreateShaders
=================================================
*/
	bool FGShadertoyApp::_RecreateShaders (const uint2 &newSize)
	{
		CHECK_ERR( not _shaders.empty() );

		_surfaceSize		= newSize;
		_scaledSurfaceSize	= uint2( float2(newSize) * _sufaceScale + 0.5f );
		
		Array<ShaderPtr>	sorted;

		// destroy all
		for (auto& sh : _shaders)
		{
			sorted.push_back( sh.second );
			_DestroyShader( sh.second );
		}
		_ordered.clear();

		// create all
		while ( not sorted.empty() )
		{
			for (auto iter = sorted.begin(); iter != sorted.end();)
			{
				if ( _CreateShader( *iter ) )
				{
					_ordered.push_back( *iter );
					iter = sorted.erase( iter );
				}
				else
					++iter;
			}
		}
		return true;
	}
	
/*
=================================================
	_CreateShader
=================================================
*/
	bool FGShadertoyApp::_CreateShader (const ShaderPtr &shader)
	{
		// compile pipeline
		if ( not shader->_pipeline )
		{
			shader->_pipeline = _Compile( shader->_pplnFilename, shader->_pplnDefines );
			CHECK_ERR( shader->_pipeline );
		}

		// check dependencies
		for (auto& ch : shader->_channels)
		{
			// find channel in shader passes
			ShadersMap_t::iterator	iter = _shaders.find( ch.name );
			if ( iter != _shaders.end() )
			{
				if ( iter->second == shader )
					continue;

				for (auto& pass : iter->second->_perPass)
				{
					// wait until all dependencies will be initialized
					if ( not pass.renderTarget.IsValid() )
						return false;
				}
				continue;
			}

			// find channel in loadable images
			if ( _HasImage( ch.name ) )
				continue;

			RETURN_ERR( "unknown channel type, it is not a shader pass and not a file" );
		}
		
		// create render targets
		for (auto& pass : shader->_perPass)
		{
			if ( shader->_surfaceSize.has_value() )
				pass.viewport = shader->_surfaceSize.value();
			else
				pass.viewport = uint2( float2(_scaledSurfaceSize) * shader->_surfaceScale.value_or(1.0f) + 0.5f );

			ImageDesc	desc;
			desc.imageType	= EImage::Tex2D;
			desc.dimension	= uint3{ pass.viewport, 1 };
			desc.format		= ImageFormat;
			desc.usage		= EImageUsage::Transfer | EImageUsage::Sampled | EImageUsage::ColorAttachment;

			pass.renderTarget = _frameGraph->CreateImage( desc, Default, String(shader->Name()) << "-RT-" << ToString(Distance( shader->_perPass.data(), &pass )) );
		}
		
		// create uniform buffers
		{
			BufferDesc	desc;
			desc.size	= SizeOf<ShadertoyUB>;
			desc.usage	= EBufferUsage::Uniform | EBufferUsage::TransferDst;

			shader->_ubuffer = _frameGraph->CreateBuffer( desc, Default, "ShadertoyUB" );
		}
		
		// setup pipeline resource table
		for (size_t i = 0; i < shader->_perPass.size(); ++i)
		{
			auto&	pass = shader->_perPass[i];
			
			CHECK( _frameGraph->InitPipelineResources( shader->_pipeline, DescriptorSetID{"0"}, OUT pass.resources ));

			pass.resources.BindBuffer( UniformID{"ShadertoyUB"}, shader->_ubuffer );
			pass.images.resize( shader->_channels.size() );

			for (size_t j = 0; j < shader->_channels.size(); ++j)
			{
				auto&		ch		= shader->_channels[j];
				auto&		image	= pass.images[j];
				UniformID	name	{"iChannel"s << ToString(j)};

				// find channel in shader passes
				ShadersMap_t::iterator	iter = _shaders.find( ch.name );
				if ( iter != _shaders.end() )
				{
					if ( iter->second == shader )
						// use image from previous pass
						image = ImageID{ shader->_perPass[(i-1) & 1].renderTarget.Get() };
					else
						image = ImageID{ iter->second->_perPass[i].renderTarget.Get() };
					
					pass.resources.BindTexture( name, image, _linearClampSampler );	// TODO: sampler
					continue;
				}
				
				// find channel in loadable images
				if ( _LoadImage( ch.name, OUT image ) )
				{
					pass.resources.BindTexture( name, image, _linearClampSampler );
					continue;
				}
				
				RETURN_ERR( "unknown channel type, it is not a shader pass and not a file" );
			}
		}
		return true;
	}
	
/*
=================================================
	_DestroyShader
=================================================
*/
	void FGShadertoyApp::_DestroyShader (const ShaderPtr &shader)
	{
		for (auto& pass : shader->_perPass)
		{
			_frameGraph->ReleaseResource( INOUT pass.renderTarget );
			
			for (auto& img : pass.images)
			{
				if ( img.IsValid() )
					FG_UNUSED( img.Release() );
			}
		}

		_frameGraph->ReleaseResource( INOUT shader->_ubuffer );
	}

/*
=================================================
	_Compile
=================================================
*/
	GPipelineID  FGShadertoyApp::_Compile (StringView name, StringView defs) const
	{
		const char	vs_source[] = R"#(
			const vec2	g_Positions[] = {
				{ -1.0f,  1.0f },  { -1.0f, -1.0f },
				{  1.0f,  1.0f },  {  1.0f, -1.0f }
			};

			void main() {
				gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0f, 1.0f );
			}
		)#";

		const char	fs_source[] = R"#(
			layout(binding=0, std140) uniform  ShadertoyUB
			{
				vec3	iResolution;			// viewport resolution (in pixels)
				float	iTime;					// shader playback time (in seconds)
				float	iTimeDelta;				// render time (in seconds)
				int		iFrame;					// shader playback frame
				float	iChannelTime[4];		// channel playback time (in seconds)
				vec3	iChannelResolution[4];	// channel resolution (in pixels)
				vec4	iMouse;					// mouse pixel coords. xy: current (if MLB down), zw: click
				vec4	iDate;					// (year, month, day, time in seconds)
				float	iSampleRate;			// sound sample rate (i.e., 44100)
				vec3	iCameraFrustumLB;		// frustum rays (left bottom, right bottom, left top, right top)
				vec3	iCameraFrustumRB;
				vec3	iCameraFrustumLT;
				vec3	iCameraFrustumRT;
				vec4	iCameraOrientation;
				vec3	iCameraPos;				// camera position in world space
				float	iCameraFovY;
			};

			layout(location=0) out vec4	out_Color;

			void mainImage (out vec4 fragColor, in vec2 fragCoord);

			void main ()
			{
				vec2 coord = gl_FragCoord.xy;
				coord.y = iResolution.y - coord.y;

				mainImage( out_Color, coord );
			}
		)#";

		String	src0, src1;
		{
			FileRStream		file{ String{FG_DATA_PATH} << name };
			CHECK_ERR( file.IsOpen() );
			CHECK_ERR( file.Read( size_t(file.Size()), OUT src1 ));
		}

		if ( not defs.empty() )
			src0 << defs;

		if ( HasSubString( src1, "iChannel0" ) )
			src0 << "layout(binding=1) uniform sampler2D  iChannel0;\n";

		if ( HasSubString( src1, "iChannel1" ) )
			src0 << "layout(binding=2) uniform sampler2D  iChannel1;\n";

		if ( HasSubString( src1, "iChannel2" ) )
			src0 << "layout(binding=3) uniform sampler2D  iChannel2;\n";

		src0 << fs_source;
		src0 << src1;

		GraphicsPipelineDesc	desc;
		desc.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", vs_source );
		desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100 | EShaderLangFormat::EnableDebugTrace, "main", std::move(src0), name );

		return _frameGraph->CreatePipeline( desc, name );
	}
	
/*
=================================================
	_Recompile
=================================================
*/
	bool FGShadertoyApp::_Recompile ()
	{
		for (auto& shader : _ordered)
		{
			GPipelineID		ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines );

			if ( ppln )
			{
				_frameGraph->ReleaseResource( INOUT shader->_pipeline );
				shader->_pipeline = std::move(ppln);
			}
		}
		return true;
	}

/*
=================================================
	_AddShader
=================================================
*/
	bool FGShadertoyApp::_AddShader (const String &name, ShaderDescr &&desc)
	{
		_shaders.insert_or_assign( name, MakeShared<Shader>( name, std::move(desc) ));
		return true;
	}
	
/*
=================================================
	_ResetShaders
=================================================
*/
	void FGShadertoyApp::_ResetShaders ()
	{
		for (auto& sh : _shaders)
		{
			_DestroyShader( sh.second );
			_frameGraph->ReleaseResource( INOUT sh.second->_pipeline );
		}
		_shaders.clear();
		_ordered.clear();

		_surfaceSize	= Default;
		_frameCounter	= 0;
	}
	
/*
=================================================
	_CreateSamplers
=================================================
*/
	void FGShadertoyApp::_CreateSamplers ()
	{
		SamplerDesc		desc;
		desc.SetAddressMode( EAddressMode::ClampToEdge );
		desc.SetFilter( EFilter::Nearest, EFilter::Nearest, EMipmapFilter::Nearest );
		_nearestClampSampler = _frameGraph->CreateSampler( desc, "NearestClamp" );
		
		desc.SetAddressMode( EAddressMode::ClampToEdge );
		desc.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear );
		_linearClampSampler = _frameGraph->CreateSampler( desc, "LinearClamp" );
		
		desc.SetAddressMode( EAddressMode::Repeat );
		desc.SetFilter( EFilter::Nearest, EFilter::Nearest, EMipmapFilter::Nearest );
		_nearestRepeatSampler = _frameGraph->CreateSampler( desc, "NearestRepeat" );
		
		desc.SetAddressMode( EAddressMode::Repeat );
		desc.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear );
		_linearRepeatSampler = _frameGraph->CreateSampler( desc, "LinearRepeat" );
	}

/*
=================================================
	_UpdateFrameStat
=================================================
*/
	void FGShadertoyApp::_UpdateFrameStat ()
	{
		using namespace std::chrono;

		++_frameStat.frameCounter;

		TimePoint_t		now			= TimePoint_t::clock::now();
		int64_t			duration	= duration_cast<milliseconds>(now - _frameStat.lastUpdateTime).count();

		IFrameGraph::Statistics	stat;
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

			String	str = "Shadertoy [FPS: ";
			str << ToString( fps_value );
			str << ", GPU: " << ToString( gpu_time );
			str << ", CPU: " << ToString( cpu_time );
			str << ", Pixel: " << ToString( _frameStat.selectedPixel );
			str << ']';

			_window->SetTitle( str );
		}
	}
	
/*
=================================================
	_LoadImage
=================================================
*/
	bool FGShadertoyApp::_LoadImage (const String &filename, OUT ImageID &id)
	{
#	ifdef FG_ENABLE_DEVIL
		auto	iter = _imageCache.find( filename );
		
		if ( iter != _imageCache.end() )
		{
			id = ImageID{ iter->second.Get() };
			return true;
		}

		DevILLoader		loader;
		auto			image	= MakeShared<IntermImage>( filename );

		CHECK_ERR( loader.LoadImage( image, {}, null ));

		auto&	level = image->GetData()[0][0];
		String	img_name;
		
		#ifdef FG_STD_FILESYSTEM
			img_name = std::filesystem::path{filename}.filename().string();
		#endif

		id = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, level.dimension, level.format, EImageUsage::TransferDst | EImageUsage::Sampled },
									   Default, img_name );

		_currTask = _cmdBuffer->AddTask( UpdateImage{}.SetImage( id ).SetData( level.pixels, level.dimension, level.rowPitch, level.slicePitch ).DependsOn( _currTask ));

		_imageCache.insert_or_assign( filename, ImageID{id.Get()} );
		return true;
#	else
		return false;
#	endif
	}
	
/*
=================================================
	_HasImage
=================================================
*/
	bool FGShadertoyApp::_HasImage (StringView filename) const
	{
	#ifdef FG_STD_FILESYSTEM
		return std::filesystem::exists({ filename });
	#else
		return true;
	#endif
	}
	
/*
=================================================
	_OnShaderTraceReady
=================================================
*/
	void FGShadertoyApp::_OnShaderTraceReady (StringView name, ArrayView<String> output) const
	{
	#	ifdef FG_STD_FILESYSTEM
		const auto	IsExists = [] (StringView path) { return fs::exists(fs::path{ path }); };
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
	
		CHECK(!"shader trace is ready");
	}
	
/*
=================================================
	_OnPixelReadn
=================================================
*/
	void FGShadertoyApp::_OnPixelReadn (const uint2 &, const ImageView &view)
	{
		view.Load( uint3{0,0,0}, OUT _frameStat.selectedPixel );
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

	FGShadertoyApp		app;
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

	CHECK_FATAL( app.Initialize( std::move(wnd) ));

	for (; app.Update(); ) {}

	app.Destroy();
	return 0;
}
