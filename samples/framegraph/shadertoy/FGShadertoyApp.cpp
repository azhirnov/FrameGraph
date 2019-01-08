// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "FGShadertoyApp.h"
#include "graphviz/GraphViz.h"
#include "scene/Loader/DevIL/DevILLoader.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "stl/Stream/FileStream.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

	struct ShadertoyUB
	{
		float3		iResolution;			// offset: 0, align: 16
		float		iTime;					// offset: 12, align: 4
		float		iTimeDelta;				// offset: 16, align: 4
		int			iFrame;					// offset: 20, align: 4
		float2		_padding0;				// offset: 24, align: 8
		float4		iChannelTime[4];		// offset: 32, align: 16
		float4		iChannelResolution[4];	// offset: 96, align: 16
		float4		iMouse;					// offset: 160, align: 16
		float4		iDate;					// offset: 176, align: 16
		float		iSampleRate;			// offset: 192, align: 4
		float		_padding1;				// offset: 196, align: 4
		float		_padding2;				// offset: 200, align: 4
		float		_padding3;				// offset: 204, align: 4
		float4		iCameraFrustum[4];		// offset: 208, align: 16
		float4		iCameraPos;				// offset: 272, align: 16
	};
	
/*
=================================================
	constructor
=================================================
*/
	FGShadertoyApp::Shader::Shader (StringView name, ShaderDescr &&desc) :
		_name{ name },
		_pplnFilename{ std::move(desc._pplnFilename) },
		_pplnDefines{ std::move(desc._pplnDefines) },
		_channels{ desc._channels }
	{
	}
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	FGShadertoyApp::FGShadertoyApp () :
		_passIdx{0}
	{
		_fps.lastUpdateTime = TimePoint_t::clock::now();
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
			sh_main.Pipeline( R"#(Glowballs.glsl)#" );
			sh_main.InChannel( "main", 0 );
			CHECK( _AddShader( "main", std::move(sh_main) ));
		});

		_samples.push_back( [this] ()
		{
			ShaderDescr	sh_main;
			sh_main.Pipeline( R"(Skyline.glsl)" );
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

		CHECK_FATAL( _frameGraph->RecreateSwapchain( swapchain_info ));
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void FGShadertoyApp::OnKey (StringView key, EKeyAction action)
	{
		if ( action != EKeyAction::Down )
			return;

		if ( key == "[" )	--_nextSample;	else
		if ( key == "]" )	++_nextSample;

		if ( key == "R" )
			_Recompile();

		if ( key == "space" )
			_pause = not _pause;

		if ( key == "escape" and _window )
			_window->Quit();
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
			_frameGraphInst = FrameGraph::CreateFrameGraph( vulkan_info );
			CHECK_ERR( _frameGraphInst );
			CHECK_ERR( _frameGraphInst->Initialize( 2 ));

			//_frameGraphInst->SetCompilationFlags( ECompilationFlags::EnableDebugger, ECompilationDebugFlags::Default );

			ThreadDesc	desc{ EThreadUsage::Present | EThreadUsage::Graphics | EThreadUsage::Transfer };

			_frameGraph = _frameGraphInst->CreateThread( desc );
			CHECK_ERR( _frameGraph );
			CHECK_ERR( _frameGraph->Initialize( &swapchain_info ));
		}

		// add glsl pipeline compiler
		{
			auto	ppln_compiler = MakeShared<VPipelineCompiler>( vulkan_info.physicalDevice, vulkan_info.device );
			ppln_compiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations | EShaderCompilationFlags::Quiet );

			_frameGraphInst->AddPipelineCompiler( ppln_compiler );
		}

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

		if ( _pause )
		{
			std::this_thread::sleep_for(SecondsF{0.01f});
			return true;
		}

		_UpdateFPS();
		
		// select sample
		if ( _currSample != _nextSample )
		{
			_nextSample = _nextSample % _samples.size();
			_currSample = _nextSample;

			_ResetShaders();
			_samples[_currSample]();
		}

		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );

		CHECK_ERR( _frameGraphInst->BeginFrame( submission_graph ));
		CHECK_ERR( _frameGraph->Begin( batch_id, 0, EThreadUsage::Graphics ));

		if ( Any(_surfaceSize != _window->GetSize()) )
		{
			CHECK_ERR( _RecreateShaders( _window->GetSize() ));
		}
		
		if ( _ordered.size() )
		{
			_UpdateShaderData();
		
			const uint	pass_idx = _passIdx;

			// run shaders
			for (auto& sh : _ordered)
			{
				_DrawWithShader( sh, pass_idx );
			}
		
			// present
			{
				ShadersMap_t::iterator	iter = _shaders.find( "main" );
				CHECK_ERR( iter != _shaders.end() );

				const auto&	image = iter->second->_perPass[pass_idx].renderTarget;

				_currTask = _frameGraph->AddTask( Present{ image }.DependsOn( _currTask ));
			}
		}

		CHECK_ERR( _frameGraph->Execute() );
		CHECK_ERR( _frameGraphInst->EndFrame() );

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
		const auto	time = TimePoint_t::clock::now();

		if ( _frameCounter == 0 )
			_startTime = time;

		const float		app_dt		= std::chrono::duration_cast<SecondsF>( time - _startTime ).count();
		const float		frame_dt	= std::chrono::duration_cast<SecondsF>( time - _lastUpdateTime ).count();
		_lastUpdateTime	= time;

		_ubData.iResolution		= float3{ float2(_scaledSurfaceSize), 0.0f };
		_ubData.iTime			= app_dt;
		_ubData.iTimeDelta		= frame_dt;
		_ubData.iFrame			= _frameCounter;
		//_ubData.iMouse		= float4( _mousePressed ? _mousePos : float2(), float2() );	// TODO: click
		//_ubData.iDate			= float4(uint3( date.Year(), date.Month(), date.DayOfMonth()).To<float3>(), float(date.Second()) + float(date.Milliseconds()) * 0.001f);
		_ubData.iSampleRate		= 0.0f;	// not supported yet
		//_ubData.iCameraPos	= state.transform.Position();

		//state.frustum.GetRays( OUT _ubData.iCameraFrustumRay2, OUT _ubData.iCameraFrustumRay0,
		//					   OUT _ubData.iCameraFrustumRay3, OUT _ubData.iCameraFrustumRay1 );

		++_frameCounter;
	}

/*
=================================================
	_DrawWithShader
=================================================
*/
	bool FGShadertoyApp::_DrawWithShader (const ShaderPtr &shader, uint passIndex)
	{
		auto&	pass		= shader->_perPass[passIndex];
		auto	view_size	= _frameGraph->GetDescription( pass.renderTarget ).dimension.xy();

		// update uniform buffer
		{
			ShadertoyUB		ub_data	= {};

			for (size_t i = 0; i < CountOf(ub_data.iChannelResolution); ++i)
			{
				if ( pass.resources.HasTexture(UniformID{ "iChannel"s << ToString(i) }) )
				{
					ub_data.iChannelResolution[i]	= float4{ float(view_size.x), float(view_size.y), 0.0f, 0.0f };		// TODO
					ub_data.iChannelTime[i]			= float4{ _ubData.iTime };
				}
			}

			ub_data.iDate				= _ubData.iDate;
			ub_data.iFrame				= _ubData.iFrame;
			ub_data.iMouse				= _ubData.iMouse;
			ub_data.iResolution			= _ubData.iResolution;
			ub_data.iSampleRate			= _ubData.iSampleRate;
			ub_data.iTime				= _ubData.iTime;
			ub_data.iTimeDelta			= _ubData.iTimeDelta;
			ub_data.iCameraFrustum[0]	= float4(_ubData.iCameraFrustumRay0, 0.0f);
			ub_data.iCameraFrustum[1]	= float4(_ubData.iCameraFrustumRay1, 0.0f);
			ub_data.iCameraFrustum[2]	= float4(_ubData.iCameraFrustumRay2, 0.0f);
			ub_data.iCameraFrustum[3]	= float4(_ubData.iCameraFrustumRay3, 0.0f);
			ub_data.iCameraPos			= float4(_ubData.iCameraPos, 0.0f);

			_currTask = _frameGraph->AddTask( UpdateBuffer{}.SetBuffer( shader->_ubuffer ).AddData( &ub_data, 1 ).DependsOn( _currTask ));
		}


		auto	pass_id = _frameGraph->CreateRenderPass( RenderPassDesc( view_size )
									.AddTarget( RenderTargetID("out_Color"), pass.renderTarget, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
									.AddViewport( view_size ) );

		_frameGraph->AddTask( pass_id, DrawVertices{}.SetPipeline( shader->_pipeline ).AddResources( 0, &pass.resources )
													 .AddDrawCmd( 4 ).SetTopology( EPrimitive::TriangleStrip ));

		_currTask = _frameGraph->AddTask( SubmitRenderPass{ pass_id }.DependsOn( _currTask ));
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

			_frameGraph->Deinitialize();
			_frameGraph = null;
		}

		if ( _frameGraphInst )
		{
			_frameGraphInst->Deinitialize();
			_frameGraphInst = null;
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
			ShadersMap_t::iterator	iter = _shaders.find( ch.first );
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
			if ( _HasImage( ch.first ) )
				continue;

			RETURN_ERR( "unknown channel type, it is not a shader pass and not a file" );
		}
		
		// create render targets
		for (auto& pass : shader->_perPass)
		{
			pass.viewport = _scaledSurfaceSize;

			ImageDesc	desc;
			desc.imageType	= EImage::Tex2D;
			desc.dimension	= uint3{ _scaledSurfaceSize, 1 };
			desc.format		= EPixelFormat::RGBA8_UNorm;
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
			auto&						pass	= shader->_perPass[i];
			RawDescriptorSetLayoutID	ds_layout;
			uint						ds_binding;

			CHECK( _frameGraph->GetDescriptorSet( shader->_pipeline, DescriptorSetID{"0"}, OUT ds_layout, OUT ds_binding ));
			CHECK( _frameGraph->InitPipelineResources( ds_layout, OUT pass.resources ));
			CHECK_ERR( ds_binding == 0 );

			pass.resources.BindBuffer( UniformID{"ShadertoyUB"}, shader->_ubuffer );

			for (size_t j = 0; j < shader->_channels.size(); ++j)
			{
				auto&		ch   = shader->_channels[j];
				UniformID	name {"iChannel"s << ToString(j)};
				
				// find channel in shader passes
				ShadersMap_t::iterator	iter = _shaders.find( ch.first );
				if ( iter != _shaders.end() )
				{
					if ( iter->second == shader )
						// use image from previous pass
						pass.resources.BindTexture( name, shader->_perPass[(i-1) & 1].renderTarget, _linearClampSampler );	// TODO: sampler
					else
						pass.resources.BindTexture( name, iter->second->_perPass[i].renderTarget, _linearClampSampler );	// TODO: sampler

					continue;
				}
				
				// find channel in loadable images
				ImageID		image_id;
				if ( _LoadImage( ch.first, OUT image_id ) )
				{
					pass.resources.BindTexture( name, image_id, _linearClampSampler );
					image_id.Release();
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
				vec3	iCameraFrustum[4];		// frustum rays (left bottom, right bottom, left top, right top)
				vec3	iCameraPos;				// camera position in world space
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
			FileRStream		file{ name };
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
		desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", std::move(src0) );

		return _frameGraph->CreatePipeline( std::move(desc) );
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

		_surfaceSize = Default;
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
	_UpdateFPS
=================================================
*/
	void FGShadertoyApp::_UpdateFPS ()
	{
		using namespace std::chrono;

		++_fps.frameCounter;

		TimePoint_t		now			= TimePoint_t::clock::now();
		int64_t			duration	= duration_cast<milliseconds>(now - _fps.lastUpdateTime).count();

		if ( duration > _fps.UpdateIntervalMillis )
		{
			uint	value		= uint(float(_fps.frameCounter) / float(duration) * 1000.0f + 0.5f);
			_fps.lastUpdateTime	= now;
			_fps.frameCounter	= 0;

			_window->SetTitle( "Shadertoy [FPS: "s << ToString(value) << ']' );
		}
	}
	
/*
=================================================
	_LoadImage
=================================================
*/
	bool FGShadertoyApp::_LoadImage (const String &filename, OUT ImageID &id)
	{
	#ifdef FG_ENABLE_DEVIL
		auto	iter = _imageCache.find( filename );
		
		if ( iter != _imageCache.end() )
		{
			id = ImageID{ iter->second.Get() };
			return true;
		}

		DevILLoader		loader;
		auto			image	= MakeShared<IntermediateImage>( filename );

		CHECK_ERR( loader.Load( image, {} ));

		auto&	level = image->GetData()[0][0];
		String	fname;
		
		#ifdef FG_STD_FILESYSTEM
			fname = std::filesystem::path{filename}.filename().string();
		#endif

		id = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, level.dimension, level.format, EImageUsage::TransferDst | EImageUsage::Sampled },
									   Default, fname );

		_currTask = _frameGraph->AddTask( UpdateImage{}.SetImage( id ).SetData( level.pixels, level.dimension, level.rowPitch, level.slicePitch ).DependsOn( _currTask ));

		_imageCache.insert_or_assign( filename, ImageID{id.Get()} );
		return true;
	#else
		return false;
	#endif
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
