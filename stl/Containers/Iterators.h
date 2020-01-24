// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace FGC
{
namespace _fgc_hidden_
{
	template <typename T>
	struct _ReverseWrapper
	{
		T &		container;

		ND_ auto	begin ()	{ return std::rbegin(container); }
		ND_ auto	end ()		{ return std::rend(container); }
	};

}	// _fgc_hidden_

/*
=================================================
	Reverse
=================================================
*/
	template <typename Container>
	ND_ _fgc_hidden_::_ReverseWrapper<Container>  Reverse (Container &&arr)
	{
		return { arr };
	}


}	// FGC
