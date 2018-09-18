// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/include/File.h"
#include "stl/include/StringUtils.h"

#ifdef PLATFORM_WINDOWS
#	define fread	_fread_nolock
#	define fwrite	_fwrite_nolock
#	define fflush	_fflush_nolock
#	define fclose	_fclose_nolock
#	define ftell	_ftelli64_nolock
#	define fseek	_fseeki64_nolock
#endif

#if defined(PLATFORM_LINUX) or defined(PLATFORM_ANDROID)
#	define fseek	fseeko
#	define ftell	ftello
#endif


namespace FG
{
	
/*
=================================================
    Read
=================================================
*/
	bool  RFile::Read (void *buffer, BytesU size)
	{
		return Read2( buffer, size ) == size;
	}
	
/*
=================================================
    Read
=================================================
*/
	bool  RFile::Read (size_t length, OUT String &str)
	{
		str.resize( length );

		BytesU	expected_size	= BytesU::SizeOf(str[0]) * str.length();
		BytesU	current_size	= Read2( str.data(), expected_size );
		
		str.resize( size_t(current_size / sizeof(str[0])) );

		return str.length() == length;
	}
	
/*
=================================================
    Read
=================================================
*/
	bool  RFile::Read (size_t count, OUT Array<uint8_t> &buf)
	{
		buf.resize( count );

		BytesU	expected_size	= BytesU::SizeOf(buf[0]) * buf.size();
		BytesU	current_size	= Read2( buf.data(), expected_size );
		
		buf.resize( size_t(current_size / sizeof(buf[0])) );

		return buf.size() == count;
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
    Write
=================================================
*/
	bool  WFile::Write (const void *buffer, BytesU size)
	{
		return Write2( buffer, size ) == size;
	}
	
/*
=================================================
    Write
=================================================
*/
	bool  WFile::Write (StringView str)
	{
		BytesU	size = BytesU::SizeOf(str[0]) * str.length();

		return Write2( str.data(), size ) == size;
	}
	
/*
=================================================
    Write
=================================================
*/
	bool  WFile::Write (ArrayView<uint8_t> buf)
	{
		BytesU	size = BytesU::SizeOf(buf[0]) * buf.size();

		return Write2( buf.data(), size ) == size;
	}
//-----------------------------------------------------------------------------



/*
=================================================
    constructor
=================================================
*/
	HddRFile::HddRFile (StringView filename)
	{
		fopen_s( OUT &_file, filename.data(), "rb" );
		
		if ( _file )
			_fileSize = _GetSize();
		else
			FG_LOGE( "Can't open file: "s << filename );
	}
	
	HddRFile::HddRFile (const char *filename) : HddRFile{ StringView{filename} }
	{}

	HddRFile::HddRFile (const String &filename) : HddRFile{ StringView{filename} }
	{}

/*
=================================================
    constructor
=================================================
*/
	HddRFile::HddRFile (const std::filesystem::path &path)
	{
#	ifdef PLATFORM_WINDOWS
		_wfopen_s( OUT &_file, path.c_str(), L"rb" );
#	else
		fopen_s( OUT &_file, path.c_str(), "rb" );
#	endif

		if ( _file )
			_fileSize = _GetSize();
		else
			FG_LOGE( "Can't open file: "s << path.string() );
	}
	
/*
=================================================
    destructor
=================================================
*/
	HddRFile::~HddRFile ()
	{
		if ( _file ) {
			fclose( _file );
		}
	}
	
/*
=================================================
    Position
=================================================
*/
	BytesU  HddRFile::Position () const
	{
		ASSERT( IsOpen() );

		return BytesU(ftell( _file ));
	}
	
/*
=================================================
    _GetSize
=================================================
*/
	BytesU  HddRFile::_GetSize () const
	{
		ASSERT( IsOpen() );

		const int64_t	curr = ftell( _file );
		CHECK( fseek( _file, 0, SEEK_END ) == 0 );

		const int64_t	size = ftell( _file );
		CHECK( fseek( _file, curr, SEEK_SET ) == 0 );

		return BytesU(size);
	}
	
/*
=================================================
    SeekSet
=================================================
*/
	bool  HddRFile::SeekSet (BytesU pos)
	{
		ASSERT( IsOpen() );

		return (fseek( _file, int64_t(pos), SEEK_SET ) == 0);
	}
	
/*
=================================================
    Read2
=================================================
*/
	BytesU  HddRFile::Read2 (OUT void *buffer, BytesU size)
	{
		ASSERT( IsOpen() );

		return BytesU(fread( buffer, 1, size_t(size), _file ));
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
    constructor
=================================================
*/
	HddWFile::HddWFile (StringView filename)
	{
		if ( fopen_s( OUT &_file, filename.data(), "wb" ) != 0 )
		{
			FG_LOGE( "Can't open file: "s << filename );
		}
	}
	
	HddWFile::HddWFile (const char *filename) : HddWFile{ StringView{filename} }
	{}

	HddWFile::HddWFile (const String &filename) : HddWFile{ StringView{filename} }
	{}

/*
=================================================
    constructor
=================================================
*/
	HddWFile::HddWFile (const std::filesystem::path &path)
	{
#	ifdef PLATFORM_WINDOWS
		_wfopen_s( OUT &_file, path.c_str(), L"wb" );
#	else
		fopen_s( OUT &_file, path.c_str(), "wb" );
#	endif
		
		if ( not _file )
			FG_LOGE( "Can't open file: "s << path.string() );
	}
	
/*
=================================================
    destructor
=================================================
*/
	HddWFile::~HddWFile ()
	{
		if ( _file ) {
			fclose( _file );
		}
	}
	
/*
=================================================
    Position
=================================================
*/
	BytesU  HddWFile::Position () const
	{
		ASSERT( IsOpen() );

		return BytesU(ftell( _file ));
	}
	
/*
=================================================
    Size
=================================================
*/
	BytesU  HddWFile::Size () const
	{
		ASSERT( IsOpen() );

		const int64_t	curr = ftell( _file );
		CHECK( fseek( _file, 0, SEEK_END ) == 0 );

		const int64_t	size = ftell( _file );
		CHECK( fseek( _file, curr, SEEK_SET ) == 0 );

		return BytesU(size);
	}
	
/*
=================================================
    Write2
=================================================
*/
	BytesU  HddWFile::Write2 (const void *buffer, BytesU size)
	{
		ASSERT( IsOpen() );

		return BytesU(fwrite( buffer, 1, size_t(size), _file ));
	}

}	// FG
