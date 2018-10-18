// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace FG
{

	template <typename T>
	struct Appendable;


	//
	// Appendable (Array)
	//

	template <typename T, typename AllocT>
	struct Appendable< std::vector<T, AllocT> >
	{
	// types
	private:
		using Array_t	= std::vector<T, AllocT>;
		using Self		= Appendable< std::vector<T, AllocT> >;


	// variables
	private:
		Array_t &	_arr;


	// methods
	public:
		Appendable (Array_t &arr) : _arr{arr} {}

		void push_back (T &&value)			{ _arr.push_back( std::move(value) ); }
		void push_back (const T &value)		{ _arr.push_back( value ); }

		template <typename ...Args>
		void emplace_back (Args&& ...args)	{ _arr.emplace_back( std::forward<Args&&>(args)... ); }
	};


}	// FG
