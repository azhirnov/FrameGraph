// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "FGApp.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "stl/Algorithms/StringUtils.h"

#ifdef FG_ENABLE_LODEPNG
#	include "lodepng/lodepng.h"
#endif

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	FGApp::FGApp ()
	{
		_tests.push_back({ &FGApp::_DrawTest_CopyBuffer,	1 });
		_tests.push_back({ &FGApp::_DrawTest_CopyImage2D,	1 });
		_tests.push_back({ &FGApp::_DrawTest_GLSLCompute,	1 });
		//_tests.push_back({ &FGApp::_DrawTest_FirstTriangle,	1 });

		//_tests.push_back({ &FGApp::_ImplTest_Scene1, 1 });
		//_tests.push_back( &FGApp::_ImplTest_Scene2 );
	}
	
/*
=================================================
	destructor
=================================================
*/
	FGApp::~FGApp ()
	{
	}
	
/*
=================================================
	OnResize
=================================================
*/
	void FGApp::OnResize (const uint2 &size)
	{
		VulkanSwapchainInfo		swapchain_info;
		swapchain_info.surface		= BitCast<VkSurface_t>( _vulkan.GetVkSurface() );
		swapchain_info.preTransform = {};

		CHECK( _frameGraph->CreateSwapchain( swapchain_info ));
	}
	
/*
=================================================
	OnRefrash
=================================================
*/
	void FGApp::OnRefrash ()
	{
	}
	
/*
=================================================
	OnDestroy
=================================================
*/
	void FGApp::OnDestroy ()
	{
	}
	
/*
=================================================
	OnUpdate
=================================================
*/
	void FGApp::OnUpdate ()
	{
	}

/*
=================================================
	_Initialize
=================================================
*/
	bool FGApp::_Initialize (WindowPtr &&wnd)
	{
		const uint2		wnd_size{ 800, 600 };

		// initialize window
		{
			_window = std::move(wnd);
			CHECK_ERR( _window->Create( wnd_size, "Test" ) );
			_window->AddListener( this );
		}

		// initialize vulkan device
		{
			CHECK_ERR( _vulkan.Create( _window->GetVulkanSurface(), "Test", "FrameGraph", VK_API_VERSION_1_1, "" ));
			_vulkan.CreateDebugCallback( VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT );
		}

		// setup device info
		VulkanDeviceInfo		vulkan_info;
		VulkanSwapchainInfo		swapchain_info;
		{
			vulkan_info.instance		= BitCast<VkInstance_t>( _vulkan.GetVkInstance() );
			vulkan_info.physicalDevice	= BitCast<VkPhysicalDevice_t>( _vulkan.GetVkPhysicalDevice() );
			vulkan_info.device			= BitCast<VkDevice_t>( _vulkan.GetVkDevice() );

			swapchain_info.surface		= BitCast<VkSurface_t>( _vulkan.GetVkSurface() );
			swapchain_info.preTransform = {};

			for (auto& q : _vulkan.GetVkQuues())
			{
				VulkanDeviceInfo::QueueInfo	qi;
				qi.id			= BitCast<VkQueue_t>( q.id );
				qi.familyFlags	= BitCast<VkQueueFlags_t>( q.flags );
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
			_frameGraphInst->SetCompilationFlags( ECompilationFlags::EnableDebugger );

			ThreadDesc	desc{ /*EThreadUsage::Present |*/ EThreadUsage::Graphics | EThreadUsage::Transfer, CommandBatchID{"1"} };

			_frameGraph = _frameGraphInst->CreateThread( desc );
			CHECK_ERR( _frameGraph );
			//CHECK_ERR( _frameGraph->CreateSwapchain( swapchain_info ));
			CHECK_ERR( _frameGraph->Initialize() );
		}

		// add glsl pipeline compiler
		{
			auto	ppln_compiler = std::make_shared<VPipelineCompiler>( vulkan_info.physicalDevice, vulkan_info.device );

			ppln_compiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations | EShaderCompilationFlags::GenerateDebugInfo );

			_frameGraphInst->AddPipelineCompiler( ppln_compiler );
		}

		return true;
	}

/*
=================================================
	_Update
=================================================
*/
	bool FGApp::_Update ()
	{
		if ( not _window->Update() )
			return false;
		
		if ( not _tests.empty() )
		{
			TestFunc_t	func		= _tests.front().first;
			const uint	max_invoc	= _tests.front().second;
			bool		passed		= (this->*func)();

			if ( _testInvocations == 0 )
			{
				_testsPassed += uint(passed);
				_testsFailed += uint(not passed);
			}

			if ( (not passed) or (++_testInvocations >= max_invoc) )
			{
				_tests.pop_front();
				_testInvocations = 0;
			}
		}
		else
		{
			_window->Quit();

			FG_LOGI( "Tests passed: " + ToString( _testsPassed ) + ", failed: " + ToString( _testsFailed ) );
			CHECK_FATAL( _testsFailed == 0 );
		}
		return true;
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void FGApp::_Destroy ()
	{
		_frameGraph->Deinitialize();
		_frameGraph = null;

		_frameGraphInst->Deinitialize();
		_frameGraphInst = null;

		_vulkan.Destroy();
		_window->Destroy();
	}
	
/*
=================================================
	_SavePNG
=================================================
*/
#ifdef FG_ENABLE_LODEPNG
	bool FGApp::_SavePNG (const String &filename, const ImageView &imageData) const
	{
		LodePNGColorType	colortype	= LodePNGColorType(-1);
		uint				bitdepth	= 0;
		uint				bitperpixel	= 0;

		switch ( imageData.Format() )
		{
			case EPixelFormat::RGBA8_UNorm :
			case EPixelFormat::BGRA8_UNorm :
				colortype	= LodePNGColorType::LCT_RGBA;
				bitdepth	= 8;
				bitperpixel	= 4*8;
				break;

			case EPixelFormat::RGBA16_UNorm :
				colortype	= LodePNGColorType::LCT_RGBA;
				bitdepth	= 16;
				bitperpixel	= 4*16;
				break;

			case EPixelFormat::RGB8_UNorm :
			case EPixelFormat::BGR8_UNorm :
				colortype	= LodePNGColorType::LCT_RGB;
				bitdepth	= 8;
				bitperpixel	= 3*8;
				break;
				
			case EPixelFormat::RGB16_UNorm :
				colortype	= LodePNGColorType::LCT_RGB;
				bitdepth	= 16;
				bitperpixel	= 3*16;
				break;

			case EPixelFormat::R8_UNorm :
				colortype	= LodePNGColorType::LCT_GREY;
				bitdepth	= 8;
				bitperpixel	= 8;
				break;
				
			case EPixelFormat::R16_UNorm :
				colortype	= LodePNGColorType::LCT_GREY;
				bitdepth	= 16;
				bitperpixel	= 16;
				break;

			default :
				RETURN_ERR( "unsupported pixel format!" );
		}


		uint	err = 0;

		if ( imageData.Parts().size() == 1 and imageData.RowPitch() == imageData.RowSize() )
		{
			err = lodepng::encode( filename, imageData.data(), imageData.Dimension().x, imageData.Dimension().y, colortype, bitdepth );
		}
		else
		{
			const size_t	row_size	= size_t(imageData.RowSize());
			Array<uint8_t>	pixels;		pixels.resize( row_size * imageData.Dimension().y );

			for (uint y = 0; y < imageData.Dimension().y; ++y)
			{
				auto	row = imageData.GetRow( y );

				::memcpy( pixels.data() + (row_size * y), row.data(), row_size );
			}

			err = lodepng::encode( filename, pixels.data(), imageData.Dimension().x, imageData.Dimension().y, colortype, bitdepth );
		}

		CHECK_ERR( err == 0 );
		//const char * error_text = lodepng_error_text( err );

		return true;
	}
#endif	// FG_ENABLE_LODEPNG

/*
=================================================
	_Visualize
=================================================
*/
	bool FGApp::_Visualize (const FrameGraphPtr &fg, EGraphVizFlags flags, StringView name, bool autoOpen) const
	{
		// TODO
		return true;
	}

/*
=================================================
	_CreateData
=================================================
*/
	Array<uint8_t>	FGApp::_CreateData (BytesU size) const
	{
		Array<uint8_t>	arr;
		arr.resize( size_t(size) );

		return arr;
	}

/*
=================================================
	_CreateBuffer
=================================================
*/
	BufferID  FGApp::_CreateBuffer (BytesU size, StringView name) const
	{
		BufferID	res = _frameGraph->CreateBuffer( MemoryDesc{ EMemoryType::Default },
													 BufferDesc{ size, EBufferUsage::Transfer | EBufferUsage::Uniform | EBufferUsage::Storage |
																 EBufferUsage::Vertex | EBufferUsage::Index | EBufferUsage::Indirect });

		//if ( not name.empty() )
		//	res->SetDebugName( name );

		return res;
	}
	
/*
=================================================
	_CreateImage2D
=================================================
*/
	ImageID  FGApp::_CreateImage2D (uint2 size, EPixelFormat fmt, StringView name) const
	{
		ImageID		res = _frameGraph->CreateImage( MemoryDesc{ EMemoryType::Default },
												    ImageDesc{ EImage::Tex2D, uint3(size.x, size.y, 0), fmt,
															   EImageUsage::Transfer | EImageUsage::ColorAttachment | EImageUsage::Sampled | EImageUsage::Storage });
		
		//if ( not name.empty() )
		//	res->SetDebugName( name );

		return res;
	}
	
/*
=================================================
	_CreateLogicalImage2D
=================================================
*
	ImagePtr  FGApp::_CreateLogicalImage2D (uint2 size, EPixelFormat fmt, StringView name) const
	{
		ImagePtr	res = _frameGraphInst->CreateLogicalImage( EMemoryType::Default, EImage::Tex2D, uint3(size.x, size.y, 0), fmt );
		
		if ( not name.empty() )
			res->SetDebugName( name );

		return res;
	}

/*
=================================================
	_CreateSampler
=================================================
*
	SamplerPtr  FGApp::_CreateSampler () const
	{
		return _frameGraph->CreateSampler( SamplerDesc()
								.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )
								.SetAddressMode( EAddressMode::ClampToEdge, EAddressMode::ClampToEdge, EAddressMode::ClampToEdge )
					);
	}
	
/*
=================================================
	_CreatePipeline
=================================================
*
	PipelinePtr  FGApp::_CreatePipeline () const
	{
		GraphicsPipelineDesc	ppln;

		ppln.AddDescriptorSet(
				DescriptorSetID("1"), 0,
				{{ UniformID("un_ColorTexture"), EImage::Tex2D, BindingIndex{0,0}, EShaderStages::Fragment }},
				{},
				{},
				{},
				{{ UniformID("un_ConstBuf"), 256_b, BindingIndex{0,1}, EShaderStages::Fragment | EShaderStages::Vertex }},
				{} );

		ppln.SetVertexInput( VertexAttribs()
				.Add( "at_Position", EVertexType::Float2, 0, "" )
				.Add( "at_Texcoord", EVertexType::Float2, 1, "" ));
		
		ppln.SetFragmentOutput( FragmentOutput()
				.Add( RenderTargetID("out_Color"), EFragOutput::Float4, 0 ));
		
		ppln.AddTopology( EPrimitive::TriangleList )
			.AddTopology( EPrimitive::TriangleStrip );

		ppln.SetDynamicStates( EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor );

		ppln.SetRenderState( RenderState() );

		return _frameGraph->CreatePipeline( std::move(ppln) );
	}

/*
=================================================
	Run
=================================================
*/
	void FGApp::Run ()
	{
		FGApp				app;
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
