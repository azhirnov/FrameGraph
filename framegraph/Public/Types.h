// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Containers/StaticString.h"
#include "stl/Containers/FixedArray.h"
#include "stl/Containers/FixedMap.h"
#include "stl/Containers/Union.h"
#include "stl/Containers/Ptr.h"
#include "stl/Containers/Optional.h"
#include "stl/Math/Bytes.h"
#include "stl/Math/Color.h"
#include "stl/Math/Math.h"
#include "stl/Math/Rectangle.h"
#include "stl/Math/Vec.h"
#include "stl/Algorithms/Cast.h"
#include "stl/Algorithms/EnumUtils.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Memory/MemUtils.h"

#include "framegraph/Public/Config.h"
#include <chrono>
#include <atomic>


namespace FG
{
	using IPipelineCompilerPtr	= SharedPtr< class IPipelineCompiler >;
	using FGThreadPtr			= SharedPtr< class FrameGraphThread >;
	using FGInstancePtr			= SharedPtr< class FrameGraphInstance >;

	using Task					= Ptr< class IFrameGraphTask >;
	
	using Nanoseconds			= std::chrono::nanoseconds;

}	// FG
