// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/CompileTime/DefaultType.h"
#include "stl/Containers/ArrayView.h"
#include "stl/Containers/StaticString.h"
#include "stl/Containers/FixedArray.h"
#include "stl/Containers/FixedMap.h"
#include "stl/Containers/Union.h"
#include "stl/Containers/Ptr.h"
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


namespace FG
{
	using FGThreadPtr				= SharedPtr< class FrameGraphThread >;
	using IPipelineCompilerPtr		= SharedPtr< class IPipelineCompiler >;
	using FrameGraphPtr				= SharedPtr< class FrameGraph >;

	using Task						= Ptr< class IFrameGraphTask >;


}	// FG
