// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "stl/include/Common.h"

namespace FG
{

/*
=================================================
	Singleton
=================================================
*/
	template <typename T>
	ND_ inline static T*  Singleton () noexcept
	{
		static T inst;
		return std::addressof( inst );
	}

}	// FG
