// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace FG
{
namespace _fg_hidden_
{
	template <typename T>
	struct _ReverseWrapper
	{
		T &		container;

		ND_ auto	begin ()	{ return std::rbegin(container); }
		ND_ auto	end ()		{ return std::rend(container); }
	};

}	// _fg_hidden_

/*
=================================================
	Reverse
=================================================
*/
	template <typename Container>
	ND_ _fg_hidden_::_ReverseWrapper<Container>  Reverse (Container &&arr)
	{
		return { arr };
	}


}	// FG
