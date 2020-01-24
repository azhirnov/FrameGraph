// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This string_view type guaranties that contains non-null pointer to null-terminated string (C-style string).
	Use NtStringView only as function argument.
*/

#pragma once

#include "stl/Containers/StringView.h"
#include "stl/Memory/MemUtils.h"
#include "stl/Memory/UntypedAllocator.h"

namespace FGC
{

	//
	// Null-terminated String View
	//

	template <typename T>
	struct NtBasicStringView
	{
	// types
	public:
		using Value_t		= T;
		using Self			= NtBasicStringView< T >;
		using Allocator_t	= UntypedAllocator;

	private:
		static constexpr T	NullChar = T(0);


	// variables
	private:
		T const *	_data;
		size_t		_length;
		T			_buffer [32];
		bool		_isAllocated	= false;


	// methods
	public:
		NtBasicStringView ();
		NtBasicStringView (Self &&other);
		NtBasicStringView (const Self &other);
		NtBasicStringView (BasicStringView<T> str);
		NtBasicStringView (const std::basic_string<T> &str);
		NtBasicStringView (const T* str);
		NtBasicStringView (const T* str, size_t length);
		~NtBasicStringView ();

		// TODO: why they ignored by compiler ?
		Self& operator = (Self &&) = delete;
		Self& operator = (const Self &) = delete;
		Self& operator = (BasicStringView<T>) = delete;
		Self& operator = (const std::basic_string<T> &) = delete;
		Self& operator = (const T*) = delete;

		explicit operator StringView ()	const	{ return StringView{ _data, _length }; }

		ND_ T const*	c_str ()		const	{ return _data; }
		ND_ size_t		size ()			const	{ return _length; }
		ND_ size_t		length ()		const	{ return _length; }
		ND_ bool		empty ()		const	{ return _length == 0; }

	private:
		bool  _Validate ();
		bool  _IsStatic ()				const	{ return _data == &_buffer[0]; }
	};


	using NtStringView = NtBasicStringView< char >;
	


	template <typename T>
	inline NtBasicStringView<T>::NtBasicStringView () :
		_data{ _buffer }, _length{ 0 }, _buffer{ 0 }
	{}

	template <typename T>
	inline NtBasicStringView<T>::NtBasicStringView (BasicStringView<T> str) :
		_data{ str.data() }, _length{ str.size() }
	{
		_Validate();
	}

	template <typename T>
	inline NtBasicStringView<T>::NtBasicStringView (const std::basic_string<T> &str) :
		_data{ str.data() }, _length{ str.size() }
	{
		_Validate();
	}

	template <typename T>
	inline NtBasicStringView<T>::NtBasicStringView (const Self &other) :
		_data{ other._data }, _length{ other._length }
	{
		if ( other._IsStatic() )
		{
			_data = _buffer;
			std::memcpy( _buffer, other._buffer, sizeof(_buffer) );
		}
		else
			_Validate();
	}
	
	template <typename T>
	inline NtBasicStringView<T>::NtBasicStringView (Self &&other) :
		_data{ other._data }, _length{ other._length }, _isAllocated{ other._isAllocated }
	{
		if ( other._IsStatic() )
		{
			_data = _buffer;
			std::memcpy( _buffer, other._buffer, sizeof(_buffer) );
		}
		other._isAllocated = false;
	}

	template <typename T>
	inline NtBasicStringView<T>::NtBasicStringView (const T* str) :
		_data{ str }, _length{ str ? strlen(str) : 0 }
	{
		_Validate();
	}

	template <typename T>
	inline NtBasicStringView<T>::NtBasicStringView (const T* str, size_t length) :
		_data{ str }, _length{ length }
	{
		_Validate();
	}

	template <typename T>
	inline NtBasicStringView<T>::~NtBasicStringView ()
	{
		if ( _isAllocated )
			Allocator_t::Deallocate( const_cast<T *>(_data), SizeOf<T> * (_length+1) );
	}
		
	template <typename T>
	inline bool  NtBasicStringView<T>::_Validate ()
	{
		if ( not _data )
		{
			_buffer[0]	= 0;
			_data		= _buffer;
			_length		= 0;
			return false;
		}

		if ( _data[_length] == NullChar )
			return false;
		
		T *		new_data;
		BytesU	size	= SizeOf<T> * (_length+1);

		if ( size > sizeof(_buffer) )
		{
			_isAllocated= true;
			new_data	= Cast<T>( Allocator_t::Allocate( size ));
		}
		else
			new_data	= _buffer;

		memcpy( OUT new_data, _data, size_t(size) );
		new_data[_length] = NullChar;
		_data = new_data;

		return true;
	}

}   // FGC
