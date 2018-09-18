// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/include/ArrayView.h"
#include "stl/include/Bytes.h"
#include <stdio.h>
#include <filesystem>

namespace FG
{

	//
	// Read-only File
	//
	
	class RFile : public std::enable_shared_from_this<RFile>
	{
	public:
		ND_ virtual bool	IsOpen ()	const = 0;
		ND_ virtual BytesU	Position ()	const = 0;
		ND_ virtual BytesU	Size ()		const = 0;

			virtual bool	SeekSet (BytesU pos) = 0;
		ND_ virtual BytesU	Read2 (OUT void *buffer, BytesU size) = 0;
		
		bool  Read (OUT void *buffer, BytesU size);
		bool  Read (size_t length, OUT String &str);
		bool  Read (size_t count, OUT Array<uint8_t> &buf);


		template <typename T>
		std::enable_if_t<std::is_pod_v<T>, bool>  Read (OUT T &data)
		{
			return Read2( std::addressof(data), BytesU::SizeOf(data) ) == BytesU::SizeOf(data);
		}

		template <typename T>
		std::enable_if_t<std::is_pod_v<T>, bool>  Read (size_t count, OUT Array<T> &arr)
		{
			arr.resize( count );
			
			BytesU	size = BytesU::SizeOf(arr[0]) * arr.size();

			return Read2( arr.data(), size ) == size;
		}
	};



	//
	// Write-only File
	//
	
	class WFile : public std::enable_shared_from_this<WFile>
	{
	public:
		ND_ virtual bool	IsOpen ()	const = 0;
		ND_ virtual BytesU	Position ()	const = 0;
		ND_ virtual BytesU	Size ()		const = 0;
		
		ND_ virtual BytesU	Write2 (const void *buffer, BytesU size) = 0;

		bool  Write (const void *buffer, BytesU size);
		bool  Write (StringView str);
		bool  Write (ArrayView<uint8_t> buf);
		
		template <typename T>
		std::enable_if_t<std::is_pod_v<T>, bool>  Write (const T &data)
		{
			return Write2( std::addressof(data), BytesU::SizeOf(data) ) == BytesU::SizeOf(data);
		}
	};




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
