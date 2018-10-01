// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Math/Bytes.h"

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


}	// FG

