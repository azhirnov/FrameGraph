// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/File/File.h"
#include <stdio.h>
#include <filesystem>

namespace FG
{

	//
	// HDD Read-only File
	//

	class HddRFile final : public RFile
	{
	// variables
	private:
		FILE*		_file	= null;
		BytesU		_fileSize;


	// methods
	public:
		HddRFile () {}
		HddRFile (StringView filename);
		HddRFile (const char *filename);
		HddRFile (const String &filename);
		HddRFile (const std::filesystem::path &path);
		~HddRFile ();

		bool	IsOpen ()	const override		{ return _file != null; }
		BytesU	Position ()	const override;
		BytesU	Size ()		const override		{ return _fileSize; }
		
		bool	SeekSet (BytesU pos) override;
		BytesU	Read2 (OUT void *buffer, BytesU size) override;

	private:
		ND_ BytesU  _GetSize () const;
	};



	//
	// HDD Write-only File
	//

	class HddWFile final : public WFile
	{
	// variables
	private:
		FILE*		_file	= null;


	// methods
	public:
		HddWFile () {}
		HddWFile (StringView filename);
		HddWFile (const char *filename);
		HddWFile (const String &filename);
		HddWFile (const std::filesystem::path &path);
		~HddWFile ();
		
		bool	IsOpen ()	const override		{ return _file != null; }
		BytesU	Position ()	const override;
		BytesU	Size ()		const override;
		
		BytesU	Write2 (const void *buffer, BytesU size) override;
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
