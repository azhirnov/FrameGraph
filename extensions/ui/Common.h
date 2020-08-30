// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_IMGUI

#include "framegraph/FG.h"
#include "imgui.h"

namespace FGC
{
	
/*
=================================================
	VecCast
=================================================
*/
	inline float2  VecCast (const ImVec2 &vec)
	{
		return float2{ vec.x, vec.y };
	}
	
	inline float4  VecCast (const ImVec4 &vec)
	{
		return float4{ vec.x, vec.y, vec.z, vec.w };
	}

}	// FGC

#endif	// FG_ENABLE_IMGUI
