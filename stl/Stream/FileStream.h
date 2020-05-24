// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Stream/Stream.h"
#include "stl/Containers/NtStringView.h"
#include <stdio.h>

#if defined(FG_STD_FILESYSTEM)
#	define FS_HAS_FILESYSTEM
#	include <filesystem>
	namespace FGC { namespace FS = std::filesystem; }

#elif defined(FG_ENABLE_FILESYSTEM)
#	define FS_HAS_FILESYSTEM
#	include "stl/Platforms/WindowsHeader.h"
#	include "ghc/filesystem.hpp"
	namespace FGC { namespace FS = ghc::filesystem; }

#endif


namespace FGC
{

	//
	// Read-only File Stream
	//

	class FileRStream final : public RStream
	{
	// variables
	private:
		FILE*		_file	= null;
		BytesU		_fileSize;
		BytesU		_position;


	// methods
	public:
		FileRStream () {}
		FileRStream (NtStringView filename);
		FileRStream (const char *filename);
		FileRStream (const String &filename);
	#ifdef FS_HAS_FILESYSTEM
		FileRStream (const FS::path &path);
	#endif
		~FileRStream ();

		bool	IsOpen ()	const override		{ return _file != null; }
		BytesU	Position ()	const override		{ return _position; }
		BytesU	Size ()		const override		{ return _fileSize; }
		
		bool	SeekSet (BytesU pos) override;
		BytesU	Read2 (OUT void *buffer, BytesU size) override;

	private:
		ND_ BytesU  _GetSize () const;
	};



	//
	// Write-only File Stream
	//

	class FileWStream final : public WStream
	{
	// variables
	private:
		FILE*		_file	= null;


	// methods
	public:
		FileWStream () {}
		FileWStream (NtStringView filename);
		FileWStream (const char *filename);
		FileWStream (const String &filename);
	#ifdef FS_HAS_FILESYSTEM
		FileWStream (const FS::path &path);
	#endif
		~FileWStream ();
		
		bool	IsOpen ()	const override		{ return _file != null; }
		BytesU	Position ()	const override;
		BytesU	Size ()		const override;
		
		BytesU	Write2 (const void *buffer, BytesU size) override;
		void	Flush () override;
	};

}	// FGC


// check definitions
#if defined (COMPILER_MSVC) or defined (COMPILER_CLANG)

# ifdef _FILE_OFFSET_BITS
#  if _FILE_OFFSET_BITS == 64
#	pragma detect_mismatch( "_FILE_OFFSET_BITS", "64" )
#  else
#	pragma detect_mismatch( "_FILE_OFFSET_BITS", "32" )
#  endif
# endif

#  ifdef FG_STD_FILESYSTEM
#	pragma detect_mismatch( "FG_STD_FILESYSTEM", "1" )
#  else
#	pragma detect_mismatch( "FG_STD_FILESYSTEM", "0" )
#  endif

#  ifdef FG_ENABLE_FILESYSTEM
#	pragma detect_mismatch( "FG_ENABLE_FILESYSTEM", "1" )
#  else
#	pragma detect_mismatch( "FG_ENABLE_FILESYSTEM", "0" )
#  endif

#  ifdef FS_HAS_FILESYSTEM
#	pragma detect_mismatch( "FS_HAS_FILESYSTEM", "1" )
#  else
#	pragma detect_mismatch( "FS_HAS_FILESYSTEM", "0" )
#  endif

#endif	// COMPILER_MSVC or COMPILER_CLANG
