// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "FGApp.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "stl/Stream/FileStream.h"

#ifdef FG_ENABLE_LODEPNG
#	include "lodepng/lodepng.h"
#endif

#include "stl/Platforms/WindowsHeader.h"

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
		_tests.push_back({ &FGApp::Test_Compute1,		1 });
		_tests.push_back({ &FGApp::Test_Draw1,			1 });

		//_tests.push_back({ &FGApp::ImplTest_Scene1, 1 });
		//_tests.push_back( &FGApp::ImplTest_Scene2 );
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
        swapchain_info.surfaceSize  = size;
		swapchain_info.preTransform = {};

		CHECK_FATAL( _frameGraph1->CreateSwapchain( swapchain_info ));
	}
	
/*
=================================================
	OnRefresh
=================================================
*/
	void FGApp::OnRefresh ()
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
			CHECK_ERR( _vulkan.Create( _window->GetVulkanSurface(), "Test", "FrameGraph", VK_API_VERSION_1_1,
									   "",
									   {},
									   VulkanDevice::GetRecomendedInstanceLayers(),
									   VulkanDevice::GetRecomendedInstanceExtensions(),
									   VulkanDevice::GetRecomendedDeviceExtensions()
									));
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
			_frameGraphInst->SetCompilationFlags( ECompilationFlags::EnableDebugger, ECompilationDebugFlags::Default );

			ThreadDesc	desc{ /*EThreadUsage::Present |*/ EThreadUsage::Graphics | EThreadUsage::Transfer };

			_frameGraph1 = _frameGraphInst->CreateThread( desc );
			CHECK_ERR( _frameGraph1 );
			//CHECK_ERR( frame_graph->CreateSwapchain( swapchain_info ));
			CHECK_ERR( _frameGraph1->Initialize() );

			desc.relative = _frameGraph1;
			_frameGraph2 = _frameGraphInst->CreateThread( desc );
			CHECK_ERR( _frameGraph2 );
			CHECK_ERR( _frameGraph2->Initialize() );

			CHECK_ERR( _frameGraph1->IsCompatibleWith( _frameGraph2, EThreadUsage::Graphics ));
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
		_frameGraph1->Deinitialize();
		_frameGraph2->Deinitialize();

		_frameGraph1 = null;
		_frameGraph2 = null;

		_frameGraphInst->Deinitialize();
		_frameGraphInst = null;

		_vulkan.Destroy();
		_window->Destroy();
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
	DeleteFile
=================================================
*/
	static bool  DeleteFile (StringView fname)
	{
#	ifdef FG_STD_FILESYSTEM
		namespace fs = std::filesystem;

		const fs::path	path{ fname };

		if ( fs::exists( path ) )
		{
			bool	result = fs::remove( path );
			std::this_thread::sleep_for( std::chrono::milliseconds(1) );
			return result;
		}
		return true;
#	else
		// TODO
#	endif
	}
	
/*
=================================================
	CreateDirectory
=================================================
*/
	static bool  CreateDirectory (StringView fname)
	{
#	ifdef FG_STD_FILESYSTEM
		namespace fs = std::filesystem;

		fs::path	path{ fname };

		if ( not fs::exists( path ) )
			return fs::create_directory( path );
		
		return true;
#	else
		// TODO
#	endif
	}
	
/*
=================================================
	Execute
=================================================
*/
	static bool Execute (StringView commands, uint timeoutMS)
	{
#	ifdef PLATFORM_WINDOWS
		char	buf[MAX_PATH] = {};
		::GetSystemDirectoryA( buf, UINT(CountOf(buf)) );
		
		String		command_line;
		command_line << '"' << buf << "\\cmd.exe\" /C " << commands;

		STARTUPINFOA			startup_info = {};
		PROCESS_INFORMATION		proc_info	 = {};
		
		bool process_created = ::CreateProcessA(
			NULL,
			command_line.data(),
			NULL,
			NULL,
			FALSE,
			CREATE_NO_WINDOW,
			NULL,
			NULL,
			OUT &startup_info,
			OUT &proc_info
		);

		if ( not process_created )
			return false;

		if ( ::WaitForSingleObject( proc_info.hThread, timeoutMS ) != WAIT_OBJECT_0 )
			return false;
		
		DWORD process_exit;
		::GetExitCodeProcess( proc_info.hProcess, OUT &process_exit );

		//std::this_thread::sleep_for( std::chrono::milliseconds(1) );
		return true;
#	else
		// TODO
#	endif
	}

/*
=================================================
	Visualize
=================================================
*/
	bool FGApp::Visualize (StringView name, EGraphVizFlags flags, bool autoOpen) const
	{
#	ifdef FG_GRAPHVIZ_DOT_EXECUTABLE
		String	str;
		CHECK_ERR( _frameGraphInst->DumpToGraphViz( flags, OUT str ));
		CHECK_ERR( CreateDirectory( FG_TEST_GRAPHS_DIR ));
		
		const StringView	format	= "png";
		String				path{ FG_TEST_GRAPHS_DIR };		path << '/' << name << ".dot";
		String				graph_path;						graph_path << path << '.' << format;

		// space in path is not supported
		CHECK_ERR( path.find( ' ' ) == String::npos );

		// save to '.dot' file
		{
			FileWStream		wfile{ path };
			CHECK_ERR( wfile.IsOpen() );
			CHECK_ERR( wfile.Write( StringView{str} ));
		}

		// generate graph
		{
			// delete previous version
			CHECK( DeleteFile( graph_path ));
			
			CHECK_ERR( Execute( "\""s << FG_GRAPHVIZ_DOT_EXECUTABLE << "\" -T" << format << " -O " << path, 30'000 ));

			// delete '.dot' file
			CHECK( DeleteFile( path ));
		}

		if ( autoOpen )
		{
			CHECK( Execute( graph_path, 1000 ));
		}
		return true;

#	else
		// not supported
		return true;
#	endif
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
		CHECK_ERR( _frameGraphInst->DumpToString( OUT right ));
		
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

		const auto	ReadLine	= [] (StringView str, INOUT size_t &pos, OUT StringView &result)
		{
			result = {};
			
			const size_t	prev_pos = pos;

			// move to end of line
			while ( pos < str.length() )
			{
				const char	n = (pos+1) >= str.length() ? 0 : str[pos+1];
				
				++pos;

				if ( n == '\n' or n == '\r' )
					break;
			}

			result = str.substr( prev_pos, pos - prev_pos );

			// move to next line
			while ( pos < str.length() )
			{
				const char	c = str[pos];
				const char	n = (pos+1) >= str.length() ? 0 : str[pos+1];
			
				++pos;

				// windows style "\r\n"
				if ( c == '\r' and n == '\n' ) {
					++pos;
					return;
				}

				// linux style "\n" (or mac style "\r")
				if ( c == '\n' or c == '\r' )
					return;
			}
		};


		// compare line by line
		for (; LeftValid() and RightValid(); )
		{
			// read left line
			do {
				ReadLine( left, INOUT l_pos, OUT line_str[0] );
				++line_number[0];
			}
			while ( IsEmptyLine( line_str[0] ) and LeftValid() );

			// read right line
			do {
				ReadLine( right, INOUT r_pos, OUT line_str[1] );
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
	CreateBuffer
=================================================
*/
	BufferID  FGApp::CreateBuffer (BytesU size, StringView name) const
	{
		BufferID	res = _frameGraph1->CreateBuffer( MemoryDesc{ EMemoryType::Default },
													  BufferDesc{ size, EBufferUsage::Transfer | EBufferUsage::Uniform | EBufferUsage::Storage |
																  EBufferUsage::Vertex | EBufferUsage::Index | EBufferUsage::Indirect },
													 name );
		return res;
	}
	
/*
=================================================
	CreateImage2D
=================================================
*/
	ImageID  FGApp::CreateImage2D (uint2 size, EPixelFormat fmt, StringView name) const
	{
		ImageID		res = _frameGraph1->CreateImage( MemoryDesc{ EMemoryType::Default },
												     ImageDesc{ EImage::Tex2D, uint3(size.x, size.y, 0), fmt,
															    EImageUsage::Transfer | EImageUsage::ColorAttachment | EImageUsage::Sampled | EImageUsage::Storage },
												    name );
		return res;
	}
	
/*
=================================================
	CreateLogicalImage2D
=================================================
*
	ImagePtr  FGApp::CreateLogicalImage2D (uint2 size, EPixelFormat fmt, StringView name) const
	{
		ImagePtr	res = _frameGraphInst->CreateLogicalImage( EMemoryType::Default, EImage::Tex2D, uint3(size.x, size.y, 0), fmt );
		
		if ( not name.empty() )
			res->SetDebugName( name );

		return res;
	}

/*
=================================================
	CreateSampler
=================================================
*
	SamplerPtr  FGApp::CreateSampler () const
	{
		return frame_graph->CreateSampler( SamplerDesc()
								.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )
								.SetAddressMode( EAddressMode::ClampToEdge, EAddressMode::ClampToEdge, EAddressMode::ClampToEdge )
					);
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
