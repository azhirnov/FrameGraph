// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/ResourceEnums.h"

namespace FG
{

	enum class EMemoryTypeExt : uint
	{
		HostRead		= uint(EMemoryType::HostRead),
		HostWrite		= uint(EMemoryType::HostWrite),
		Dedicated		= uint(EMemoryType::Dedicated),
		AllowAliasing	= uint(EMemoryType::AllowAliasing),
		Sparse			= uint(EMemoryType::Sparse),
		_Offset			= uint(EMemoryType::_Last)-1,

		LocalInGPU		= _Offset << 1,
		HostCoherent	= _Offset << 2,
		HostCached		= _Offset << 3,
		ForBuffer		= _Offset << 4,
		ForImage		= _Offset << 5,
		Virtual			= _Offset << 6,
		_Last,
		
		All				= ((_Last-1) << 1) - 1,
		HostVisible		= HostRead | HostWrite,
	};
	FG_BIT_OPERATORS( EMemoryTypeExt );


	enum class EMemoryMapFlags : uint
	{
		Read,
		Write,
		WriteDiscard,
	};

}	// FG
