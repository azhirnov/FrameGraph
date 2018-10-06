// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Stream/Stream.h"
#include <stdio.h>
#include <filesystem>

namespace FG
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


	// methods
	public:
		FileRStream () {}
		FileRStream (StringView filename);
		FileRStream (const char *filename);
		FileRStream (const String &filename);
		FileRStream (const std::filesystem::path &path);
		~FileRStream ();

		bool	IsOpen ()	const override		{ return _file != null; }
		BytesU	Position ()	const override;
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
		FileWStream (StringView filename);
		FileWStream (const char *filename);
		FileWStream (const String &filename);
		FileWStream (const std::filesystem::path &path);
		~FileWStream ();
		
		bool	IsOpen ()	const override		{ return _file != null; }
		BytesU	Position ()	const override;
		BytesU	Size ()		const override;
		
		BytesU	Write2 (const void *buffer, BytesU size) override;
		void	Flush () override;
	};

}	// FG


// check definitions
#if defined (COMPILER_MSVC) or defined (COMPILER_CLANG)

# ifdef _FILE_OFFSET_BITS
#  if _FILE_OFFSET_BITS == 64
#	pragma detect_mismatch( "_FILE_OFFSET_BITS", "64" )
#  else
#	pragma detect_mismatch( "_FILE_OFFSET_BITS", "32" )
#  endif
# endif

#endif	// COMPILER_MSVC or COMPILER_CLANG
