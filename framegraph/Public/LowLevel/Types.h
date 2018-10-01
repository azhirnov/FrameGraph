// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/CompileTime/DefaultType.h"
#include "stl/Containers/ArrayView.h"
#include "stl/Containers/StaticString.h"
#include "stl/Containers/FixedArray.h"
#include "stl/Containers/FixedMap.h"
#include "stl/Containers/Union.h"
#include "stl/Math/Bytes.h"
#include "stl/Math/Color.h"
#include "stl/Math/Math.h"
#include "stl/Math/Rectangle.h"
#include "stl/Math/Vec.h"
#include "stl/Algorithms/Cast.h"
#include "stl/Algorithms/EnumUtils.h"
#include "stl/Algorithms/MemUtils.h"


// virtual / non-virtual version
#if 1
#	define FG_IS_VIRTUAL	1
#	define FG_OVERRIDE		override final
#else
#	define FG_IS_VIRTUAL	0
#	define FG_OVERRIDE
#endif


namespace FG
{

	using ImagePtr					= SharedPtr< class ImageResource >;
	using BufferPtr					= SharedPtr< class BufferResource >;
	using SamplerPtr				= SharedPtr< class SamplerResource >;

	using PipelinePtr				= SharedPtr< class Pipeline >;
	using DescriptorSetLayoutPtr	= SharedPtr< class DescriptorSetLayout >;
	using PipelineResourcesPtr		= SharedPtr< class PipelineResources >;
	using IPipelineCompilerPtr		= SharedPtr< class IPipelineCompiler >;

	using FrameGraphPtr				= SharedPtr< class FrameGraph >;

	using Task						= class IFrameGraphTask *;
	using RenderPass				= class LogicalRenderPass *;


}	// FG
