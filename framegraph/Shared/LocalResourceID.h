// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/IDs.h"

namespace FG
{

	using RawMemoryID				= _fg_hidden_::ResourceID<12>;
	using RawPipelineLayoutID		= _fg_hidden_::ResourceID<13>;
	using RawRenderPassID			= _fg_hidden_::ResourceID<14>;
	using RawFramebufferID			= _fg_hidden_::ResourceID<15>;
	//using DescriptorPoolID		= _fg_hidden_::ResourceID<17>;

	using LocalBufferID				= _fg_hidden_::ResourceID<1001>;
	using LocalImageID				= _fg_hidden_::ResourceID<1002>;
	using LogicalRenderPassID		= _fg_hidden_::ResourceID<1003>;
	
	using MemoryID					= _fg_hidden_::ResourceIDWrap< RawMemoryID >;
	using PipelineLayoutID			= _fg_hidden_::ResourceIDWrap< RawPipelineLayoutID >;
	using RenderPassID				= _fg_hidden_::ResourceIDWrap< RawRenderPassID >;
	using FramebufferID				= _fg_hidden_::ResourceIDWrap< RawFramebufferID >;
	using PipelineResourcesID		= _fg_hidden_::ResourceIDWrap< RawPipelineResourcesID >;
	using DescriptorSetLayoutID		= _fg_hidden_::ResourceIDWrap< RawDescriptorSetLayoutID >;

}	// FG
