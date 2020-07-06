// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "GraphViz.h"

#if defined(FG_GRAPHVIZ_DOT_EXECUTABLE) && defined(FS_HAS_FILESYSTEM)

#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Platforms/WindowsHeader.h"
#include <thread>

namespace FG
{
	
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
		
		::CloseHandle( proc_info.hProcess );

		//std::this_thread::sleep_for( std::chrono::milliseconds(1) );
		return true;
#	else
		return false;
#	endif
	}

/*
=================================================
	Visualize
=================================================
*/
	bool GraphViz::Visualize (StringView graph, const FS::path &filepath, StringView format, bool autoOpen, bool deleteOrigin)
	{
		CHECK_ERR( filepath.extension() == ".dot" );

		FS::create_directories( filepath.parent_path() );
		
		const String	path		= filepath.string();
		const FS::path	graph_path	= FS::path{filepath}.concat( "."s << format );

		// space in path is not supported
		CHECK_ERR( path.find( ' ' ) == String::npos );

		// save to '.dot' file
		{
			FileWStream		wfile{ filepath };
			CHECK_ERR( wfile.IsOpen() );
			CHECK_ERR( wfile.Write( graph ));
		}

		// generate graph
		{
			// delete previous version
			if ( FS::exists( graph_path ) )
			{
				CHECK( FS::remove( graph_path ));
				std::this_thread::sleep_for( std::chrono::milliseconds(1) );
			}
			
			CHECK_ERR( Execute( "\""s << FG_GRAPHVIZ_DOT_EXECUTABLE << "\" -T" << format << " -O " << path, 30'000 ));

			if ( deleteOrigin )
			{
				CHECK( FS::remove( filepath ));
				std::this_thread::sleep_for( std::chrono::milliseconds(1) );
			}
		}

		if ( autoOpen )
		{
			CHECK( Execute( graph_path.string(), 1000 ));
		}
		return true;
	}
	
/*
=================================================
	Visualize
=================================================
*/
	bool GraphViz::Visualize (const FrameGraph &instance, const FS::path &filepath, StringView format, bool autoOpen, bool deleteOrigin)
	{
		String	str;
		CHECK_ERR( instance->DumpToGraphViz( OUT str ));

		CHECK_ERR( Visualize( str, filepath, "png", autoOpen, deleteOrigin ));
		return true;
	}


}	// FG

#endif	// FG_GRAPHVIZ_DOT_EXECUTABLE and FS_HAS_FILESYSTEM
