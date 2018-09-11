// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "stl/include/ArrayView.h"
#include "stl/include/Bytes.h"
#include "stl/include/Cast.h"
#include "stl/include/Color.h"
#include "stl/include/DefaultType.h"
#include "stl/include/EnumUtils.h"
#include "stl/include/FixedArray.h"
#include "stl/include/FixedMap.h"
#include "stl/include/Math.h"
#include "stl/include/MemUtils.h"
#include "stl/include/Rectangle.h"
#include "stl/include/StaticString.h"
#include "stl/include/StringUtils.h"
#include "stl/include/Union.h"
#include "stl/include/Vec.h"


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
