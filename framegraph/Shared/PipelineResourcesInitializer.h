// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
			uint	dbo_count	= 0;

			res._layoutId	= layoutId;
			res._uniforms	= uniforms;

			res._resources.clear();
			res._resources.resize( count );
			
			for (auto& un : *res._uniforms)
			{
				Visit(	un.second.data,
						[&] (const PipelineDescription::Texture &tex)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::Texture{{ un.second.index, tex.state, RawImageID(), {}, false }, RawSamplerID() };
						},

						[&] (const PipelineDescription::Sampler &)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::Sampler{ un.second.index, RawSamplerID() };
						},

						[&] (const PipelineDescription::SubpassInput &spi)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::SubpassInput{{ un.second.index, spi.state, RawImageID(), {}, false }, spi.attachmentIndex };
						},

						[&] (const PipelineDescription::Image &img)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::Image{ un.second.index, img.state, RawImageID(), {}, false };
						},

						[&] (const PipelineDescription::UniformBuffer &ubuf)
						{
							dbo_count += uint(ubuf.dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET);

							res._resources[un.second.index.Unique()].res =
								PipelineResources::Buffer{ un.second.index, ubuf.state, ubuf.dynamicOffsetIndex, RawBufferID(), 0_b, ubuf.size };
						},

						[&] (const PipelineDescription::StorageBuffer &sbuf)
						{
							dbo_count += uint(sbuf.dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET);

							res._resources[un.second.index.Unique()].res =
								PipelineResources::Buffer{ un.second.index, sbuf.state, sbuf.dynamicOffsetIndex, RawBufferID(), 0_b, sbuf.arrayStride > 0 ? sbuf.staticSize : 0_b };
						},

						[&] (const PipelineDescription::RayTracingScene &)
						{
							res._resources[un.second.index.Unique()].res =
								PipelineResources::RayTracingScene{ un.second.index, RawRTSceneID() };
						},

						[] (const std::monostate &) { ASSERT(false); }
					);
			}

			ASSERT( dbo_count <= FG_MaxBufferDynamicOffsets );

			res._dynamicOffsets.resize( dbo_count );
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
