// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/PipelineResources.h"

namespace FG
{

	//
	// Pipeline Resources Initializer
	//

	class PipelineResourcesInitializer
	{
	public:
		static bool Initialize (OUT PipelineResources &res, RawDescriptorSetLayoutID layoutId, const PipelineDescription::UniformMapPtr &uniforms, uint count)
		{
			res._layoutId	= layoutId;
			res._uniforms	= uniforms;
			res._resources.resize( count );
			
			for (auto& un : *res._uniforms)
			{
				Visit(	un.second.data,
						[&] (const PipelineDescription::Texture &tex)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::Texture{{ un.second.index, tex.state, RawImageID(), std::nullopt }, RawSamplerID() };
						},
						[&] (const PipelineDescription::Sampler &)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::Sampler{ un.second.index, RawSamplerID() };
						},
						[&] (const PipelineDescription::SubpassInput &spi)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::SubpassInput{{ un.second.index, spi.state, RawImageID(), std::nullopt }, spi.attachmentIndex };
						},
						[&] (const PipelineDescription::Image &img)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::Image{ un.second.index, img.state, RawImageID(), std::nullopt };
						},
						[&] (const PipelineDescription::UniformBuffer &ubuf)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::Buffer{ un.second.index, ubuf.state, RawBufferID(), 0_b, ubuf.size };
						},
						[&] (const PipelineDescription::StorageBuffer &sbuf)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::Buffer{ un.second.index, sbuf.state, RawBufferID(), 0_b, sbuf.arrayStride > 0 ? sbuf.staticSize : 0_b };
						},
						[] (const PipelineDescription::AccelerationStructure &) { ASSERT(false); },
						[] (const std::monostate &) { ASSERT(false); }
					);
			}
			return true;
		}

		ND_ static RawPipelineResourcesID  GetCached (const PipelineResources &res)
		{
			return res._GetCachedID();
		}
		
		static void SetCache (const PipelineResources &res, const RawPipelineResourcesID &id)
		{
			res._SetCachedID( id );
		}
	};


}	// FG
