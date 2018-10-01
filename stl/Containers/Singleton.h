// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

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
