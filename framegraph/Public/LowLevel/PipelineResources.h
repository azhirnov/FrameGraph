// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/IDs.h"
#include "framegraph/Public/LowLevel/BufferResource.h"
#include "framegraph/Public/LowLevel/ImageResource.h"
#include "framegraph/Public/LowLevel/SamplerResource.h"

namespace FG
{

	//
	// Descriptor Set Layout interface
	//

	class DescriptorSetLayout : public std::enable_shared_from_this<DescriptorSetLayout>
	{
	// interface
	public:
#	if FG_IS_VIRTUAL
		virtual ~DescriptorSetLayout () {}
#	endif
	};



	//
	// Pipeline Resources interface
	//

	class PipelineResources : public std::enable_shared_from_this<PipelineResources>
	{
	// types
	private:
		using Self	= PipelineResources;


	// interface
	public:
#	if FG_IS_VIRTUAL
		virtual ~PipelineResources () {}

		virtual Self&	BindImage (const UniformID &id, const ImagePtr &image) = 0;
		virtual Self&	BindImage (const UniformID &id, const ImagePtr &image, const ImageViewDesc &desc) = 0;

		virtual Self&	BindTexture (const UniformID &id, const ImagePtr &image, const SamplerPtr &sampler) = 0;
		virtual Self&	BindTexture (const UniformID &id, const ImagePtr &image, const SamplerPtr &sampler, const ImageViewDesc &desc) = 0;

		virtual Self&	BindSampler (const UniformID &id, const SamplerPtr &sampler) = 0;

		virtual Self&	BindBuffer (const UniformID &id, const BufferPtr &buffer) = 0;
		virtual Self&	BindBuffer (const UniformID &id, const BufferPtr &buffer, BytesU offset, BytesU size) = 0;

		ND_ virtual DescriptorSetLayoutPtr  GetLayout () const = 0;
#	endif
	};


	using PipelineResourceSet	= FixedMap< DescriptorSetID, PipelineResourcesPtr, FG_MaxDescriptorSets >;


}	// FG
