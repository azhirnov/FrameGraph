// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Containers/StringView.h"
#include "stl/Math/Bytes.h"
#include "stl/Memory/MemUtils.h"

namespace FGC
{

	//
	// Read-only Stream
	//
	
	class RStream : public std::enable_shared_from_this< RStream >
	{
	public:
		RStream () {}

		RStream (const RStream &) = delete;
		RStream (RStream &&) = delete;
		virtual ~RStream () {}

		RStream&  operator = (const RStream &) = delete;
		RStream&  operator = (RStream &&) = delete;

		ND_ virtual bool	IsOpen ()			const = 0;
		ND_ virtual BytesU	Position ()			const = 0;
		ND_ virtual BytesU	Size ()				const = 0;
		ND_ BytesU			RemainingSize ()	const { return Size() - Position(); }

			virtual bool	SeekSet (BytesU pos) = 0;
		ND_ virtual BytesU	Read2 (OUT void *buffer, BytesU size) = 0;
		
		bool  Read (OUT void *buffer, BytesU size);
		bool  Read (size_t length, OUT String &str);
		bool  Read (size_t count, OUT Array<uint8_t> &buf);


		template <typename T>
		EnableIf<IsPOD<T>, bool>  Read (OUT T &data)
		{
			return Read2( AddressOf(data), BytesU::SizeOf(data) ) == BytesU::SizeOf(data);
		}

		template <typename T>
		EnableIf<IsPOD<T>, bool>  Read (size_t count, OUT Array<T> &arr)
		{
			arr.resize( count );
			
			BytesU	size = BytesU::SizeOf(arr[0]) * arr.size();

			return Read2( arr.data(), size ) == size;
		}
	};



	//
	// Write-only Stream
	//
	
	class WStream : public std::enable_shared_from_this< WStream >
	{
	public:
		WStream () {}

		WStream (const WStream &) = delete;
		WStream (WStream &&) = delete;
		virtual ~WStream () {}

		WStream&  operator = (const WStream &) = delete;
		WStream&  operator = (WStream &&) = delete;

		ND_ virtual bool	IsOpen ()	const = 0;
		ND_ virtual BytesU	Position ()	const = 0;
		ND_ virtual BytesU	Size ()		const = 0;
		
		ND_ virtual BytesU	Write2 (const void *buffer, BytesU size) = 0;
			virtual void	Flush () = 0;

		bool  Write (const void *buffer, BytesU size);
		bool  Write (StringView str);
		bool  Write (ArrayView<uint8_t> buf);
		
		template <typename T>
		EnableIf<IsPOD<T>, bool>  Write (const T &data)
		{
			return Write2( AddressOf(data), BytesU::SizeOf(data) ) == BytesU::SizeOf(data);
		}
	};


}	// FGC

