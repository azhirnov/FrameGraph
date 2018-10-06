// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/MemUtils.h"

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
		return AddressOf( inst );
	}

}	// FG
