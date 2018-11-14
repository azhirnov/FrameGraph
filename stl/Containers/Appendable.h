// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/FixedArray.h"

namespace FG
{


	//
	// Appendable (Array)
	//

	template <typename T>
	struct Appendable
	{
	// types
	private:
		using Self				= Appendable< T >;
		using PushBackFunc_t	= void (*) (void *container, T &&value);
		using Value_t			= std::remove_const_t< T >;


	// variables
	private:
		void *			_ref		= null;
		PushBackFunc_t	_pushBack	= null;


	// methods
	public:
		template <typename AllocT>
		Appendable (std::vector<T,AllocT> &arr) :
			_ref{ &arr },
			_pushBack{ &_ArrayPushBack< std::remove_reference_t<decltype(arr)> > }
		{}

		template <typename T, size_t ArraySize>
		Appendable (FixedArray<T,ArraySize> &arr) :
			_ref{ &arr },
			_pushBack{ &_ArrayPushBack< std::remove_reference_t<decltype(arr)> > }
		{}

		void push_back (Value_t &&value)		{ _pushBack( _ref, std::move(value) ); }
		void push_back (const Value_t &value)	{ _pushBack( _ref, std::move(Value_t{value}) ); }
		
		template <typename ...Args>
		void emplace_back (Args&& ...args)		{ _pushBack( _ref, Value_t{ std::forward<Args&&>(args)... }); }


	private:
		template <typename ArrayT>
		static void _ArrayPushBack (void *container, Value_t &&value)
		{
			BitCast< ArrayT *>( container )->push_back( std::move(value) );
		}
	};


}	// FG
