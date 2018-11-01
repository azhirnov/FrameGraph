// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/PipelineResources.h"

namespace FG
{

	//
	// Pipeline Resources Initializer
	//

	class PipelineResourcesInitializer
	{
	// types
	public:
		using Image			= PipelineResources::Image;
		using Buffer		= PipelineResources::Buffer;
		using Texture		= PipelineResources::Texture;
		using Subpass		= PipelineResources::Subpass;
		using Sampler		= PipelineResources::Sampler;
		using Resources_t	= PipelineResources::ResourceSet_t;


	// methods
	public:
		static bool Initialize (OUT PipelineResources &res, RawDescriptorSetLayoutID layoutId, const PipelineDescription::UniformMapPtr &uniforms, uint count)
		{
			res._layoutId	= layoutId;
			res._uniforms	= uniforms;
			res._resources.resize( count );
			return true;
		}
	};


}	// FG
