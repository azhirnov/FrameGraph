// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "FGApp.h"
#include "graphviz/GraphViz.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "stl/Stream/FileStream.h"
#include "stl/Algorithms/StringParser.h"
#include <thread>

#ifdef FG_ENABLE_LODEPNG
#	include "lodepng/lodepng.h"
#endif

extern void UnitTest_VResourceManager (const FG::FGThreadPtr &fg);

namespace FG
{
namespace {
	static constexpr uint	UpdateAllReferenceDumps = false;
}

/*
=================================================
	constructor
=================================================
*/
	FGApp::FGApp ()
	{	
		_tests.push_back({ &FGApp::Test_CopyBuffer1,	1 });
		_tests.push_back({ &FGApp::Test_CopyImage1,		1 });
		_tests.push_back({ &FGApp::Test_CopyImage2,		1 });
		_tests.push_back({ &FGApp::Test_CopyImage3,		1 });
		_tests.push_back({ &FGApp::Test_CopyImage4,		1 });
		_tests.push_back({ &FGApp::Test_PushConst1,		1 });
		_tests.push_back({ &FGApp::Test_Compute1,		1 });
		_tests.push_back({ &FGApp::Test_Compute2,		1 });
		_tests.push_back({ &FGApp::Test_DynamicOffset,	1 });
		_tests.push_back({ &FGApp::Test_Draw1,			1 });
		_tests.push_back({ &FGApp::Test_Draw2,			1 });
		_tests.push_back({ &FGApp::Test_Draw3,			1 });
		_tests.push_back({ &FGApp::Test_Draw4,			1 });
		_tests.push_back({ &FGApp::Test_ExternalCmdBuf1,	1 });
		_tests.push_back({ &FGApp::Test_ReadAttachment1,	1 });
		_tests.push_back({ &FGApp::Test_AsyncCompute1,		1 });
		_tests.push_back({ &FGApp::Test_AsyncCompute2,		1 });
		_tests.push_back({ &FGApp::Test_ShaderDebugger1,	1 });
		_tests.push_back({ &FGApp::Test_ShaderDebugger2,	1 });
		_tests.push_back({ &FGApp::Test_ArrayOfTextures1,	1 });
		_tests.push_back({ &FGApp::Test_ArrayOfTextures2,	1 });
		
		_tests.push_back({ &FGApp::ImplTest_Scene1,			 1 });
		_tests.push_back({ &FGApp::ImplTest_Multithreading1, 1 });
		_tests.push_back({ &FGApp::ImplTest_Multithreading2, 1 });
		_tests.push_back({ &FGApp::ImplTest_Multithreading3, 1 });
		
		// RTX only
		_tests.push_back({ &FGApp::Test_DrawMeshes1,		1 });
		_tests.push_back({ &FGApp::Test_TraceRays1,			1 });
		_tests.push_back({ &FGApp::Test_TraceRays2,			1 });
		_tests.push_back({ &FGApp::Test_TraceRays3,			1 });
		_tests.push_back({ &FGApp::Test_ShadingRate1,		1 });
		_tests.push_back({ &FGApp::Test_RayTracingDebugger1, 1 });
		
		// very slow
		//_tests.push_back({ &FGApp::ImplTest_CacheOverflow1,	1 });
		
		// should not crash
		//_tests.push_back({ &FGApp::Test_InvalidID,	1 });
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
		VulkanSwapchainCreateInfo	swapchain_info;
		swapchain_info.surface		= BitCast<SurfaceVk_t>( _vulkan.GetVkSurface() );
		swapchain_info.surfaceSize  = size;

		CHECK_FATAL( _fgThreads[0]->RecreateSwapchain( swapchain_info ));
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
			CHECK_ERR( _vulkan.Create( _window->GetVulkanSurface(), "Test", "FrameGraph", VK_API_VERSION_1_1,
									   "",
									   {{ VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_SPARSE_BINDING_BIT | VK_QUEUE_PRESENT_BIT, 0.0f },
									    { VK_QUEUE_COMPUTE_BIT,  0.0f },
									    { VK_QUEUE_TRANSFER_BIT, 0.0f }},
									   VulkanDevice::GetRecomendedInstanceLayers(),
									   VulkanDevice::GetRecomendedInstanceExtensions(),
									   VulkanDevice::GetAllDeviceExtensions()
									));

			// this is a test and the test should fail for any validation error
			_vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All,
											  [] (const VulkanDeviceExt::DebugReport &rep) { CHECK_FATAL(not rep.isError); });
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
			_fgInstance->SetCompilationFlags( ECompilationFlags::EnableDebugger, ECompilationDebugFlags::Default );

			_fgThreads[0] = _fgInstance->CreateThread( ThreadDesc{ EThreadUsage::Transfer });
			CHECK_ERR( _fgThreads[0] );
			CHECK_ERR( _fgThreads[0]->Initialize( &swapchain_info ));

			_fgThreads[1] = _fgInstance->CreateThread( ThreadDesc{ EThreadUsage::Transfer });
			CHECK_ERR( _fgThreads[1] );
			CHECK_ERR( _fgThreads[1]->Initialize() );
			
			_fgThreads[2] = _fgInstance->CreateThread( ThreadDesc{ EThreadUsage::Transfer });
			CHECK_ERR( _fgThreads[2] );
			CHECK_ERR( _fgThreads[2]->Initialize() );
			
			_fgThreads[3] = _fgInstance->CreateThread( ThreadDesc{ EThreadUsage::Transfer });
			CHECK_ERR( _fgThreads[3] );
			CHECK_ERR( _fgThreads[3]->Initialize() );
		}

		// add glsl pipeline compiler
		{
			_pplnCompiler = MakeShared<VPipelineCompiler>( vulkan_info.physicalDevice, vulkan_info.device );
			_pplnCompiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations	|
												EShaderCompilationFlags::Quiet				|
												EShaderCompilationFlags::ParseAnnoations	|
												EShaderCompilationFlags::UseCurrentDeviceLimits );

			_fgInstance->AddPipelineCompiler( _pplnCompiler );
		}
		
		UnitTest_VResourceManager( _fgThreads[0] );
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
		_pplnCompiler = null;

		for (auto& fg : _fgThreads)
		{
			if ( fg ) fg->Deinitialize();
			fg = null;
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
	SavePNG
=================================================
*/
#ifdef FG_ENABLE_LODEPNG
	bool FGApp::SavePNG (const String &filename, const ImageView &imageData) const
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
	Visualize
=================================================
*/
	bool FGApp::Visualize (StringView name) const
	{
#	if defined(FG_GRAPHVIZ_DOT_EXECUTABLE) and defined(FG_STD_FILESYSTEM)

		String	str;
		CHECK_ERR( _fgInstance->DumpToGraphViz( OUT str ));
		
		auto	path = std::filesystem::path{ FG_TEST_GRAPHS_DIR }.append( name.data() ).replace_extension( "dot" );

		CHECK( GraphViz::Visualize( str, path, "png", false, true ));

#	else
		// not supported
#	endif
		return true;
	}
	
/*
=================================================
	CompareDumps
=================================================
*/
	bool FGApp::CompareDumps (StringView filename) const
	{
		String	fname {FG_TEST_DUMPS_DIR};	fname << '/' << filename << ".txt";

		String	right;
		CHECK_ERR( _fgInstance->DumpToString( OUT right ));
		
		// override dump
		if ( UpdateAllReferenceDumps )
		{
			FileWStream		wfile{ fname };
			CHECK_ERR( wfile.IsOpen() );
			CHECK_ERR( wfile.Write( StringView{right} ));
			return true;
		}

		// read from file
		String	left;
		{
			FileRStream		rfile{ fname };
			CHECK_ERR( rfile.IsOpen() );
			CHECK_ERR( rfile.Read( size_t(rfile.Size()), OUT left ));
		}

		size_t		l_pos	= 0;
		size_t		r_pos	= 0;
		uint2		line_number;
		StringView	line_str[2];

		const auto	LeftValid	= [&l_pos, &left ] ()	{ return l_pos < left.length(); };
		const auto	RightValid	= [&r_pos, &right] ()	{ return r_pos < right.length(); };
		
		const auto	IsEmptyLine	= [] (StringView str)
		{
			for (auto& c : str) {
				if ( c != '\n' and c != '\r' and c != ' ' and c != '\t' )
					return false;
			}
			return true;
		};


		// compare line by line
		for (; LeftValid() and RightValid(); )
		{
			// read left line
			do {
				StringParser::ReadLineToEnd( left, INOUT l_pos, OUT line_str[0] );
				++line_number[0];
			}
			while ( IsEmptyLine( line_str[0] ) and LeftValid() );

			// read right line
			do {
				StringParser::ReadLineToEnd( right, INOUT r_pos, OUT line_str[1] );
				++line_number[1];
			}
			while ( IsEmptyLine( line_str[1] ) and RightValid() );

			if ( line_str[0] != line_str[1] )
			{
				RETURN_ERR( "in: "s << filename << "\n\n"
							<< "line mismatch:" << "\n(" << ToString( line_number[0] ) << "): " << line_str[0]
							<< "\n(" << ToString( line_number[1] ) << "): " << line_str[1] );
			}
		}

		if ( LeftValid() != RightValid() )
		{
			RETURN_ERR( "in: "s << filename << "\n\n" << "sizes of dumps are not equal!" );
		}
		
		return true;
	}

/*
=================================================
	CreateData
=================================================
*/
	Array<uint8_t>	FGApp::CreateData (BytesU size) const
	{
		Array<uint8_t>	arr;
		arr.resize( size_t(size) );

		return arr;
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
