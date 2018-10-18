// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/IDs.h"
#include "framegraph/Public/LowLevel/ImageDesc.h"

namespace FG
{

	//
	// Pipeline Resources
	//

	struct PipelineResources final
	{
	// types
	public:
		using Self	= PipelineResources;

	private:
		struct Buffer
		{
			//EResourceState		state		= Default;
			BufferID				buffer;
			BytesU					offset;
			BytesU					size;
		};

		struct Image
		{
			//EResourceState		state		= Default;
			ImageID					image;
			Optional<ImageViewDesc>	desc;
		};

		struct Texture final : Image
		{
			SamplerID				sampler;
		};

		struct Subpass final : Image
		{
			uint					attachmentIndex	= ~0u;
		};

		struct Sampler
		{
			SamplerID				sampler;
		};

		using Resource_t	= Union< std::monostate, Buffer, Image, Texture, Sampler, Subpass >;

		struct ResourceInfo
		{
			HashVal			hash;
			Resource_t		res;

			ResourceInfo (BufferID buf, BytesU offset, BytesU size) : hash{HashOf(buf) + HashOf(offset) + HashOf(size)}, res{Buffer{ buf, offset, size }} {}
			ResourceInfo (ImageID img) : hash{HashOf(img)}, res{Image{ img }} {}
			ResourceInfo (ImageID img, const ImageViewDesc &desc) : hash{HashOf(img) + HashOf(desc)}, res{Image{ img, desc }} {}
			ResourceInfo (ImageID img, SamplerID samp) : hash{HashOf(img) + HashOf(samp)}, res{Texture{ img, {}, samp }} {}
			ResourceInfo (ImageID img, const ImageViewDesc &desc, SamplerID samp) : hash{HashOf(img) + HashOf(desc) + HashOf(samp)}, res{Texture{ img, desc, samp }} {}
			ResourceInfo (SamplerID samp) : hash{HashOf(samp)}, res{Sampler{ samp }} {}
			ResourceInfo (ImageID img, uint attachmentIdx) : hash{HashOf(img) + HashOf(attachmentIdx)}, res{Subpass{ img, {}, attachmentIdx }} {}
			ResourceInfo (ImageID img, const ImageViewDesc &desc, uint attachmentIdx) : hash{HashOf(img) + HashOf(desc) + HashOf(attachmentIdx)}, res{Subpass{ img, desc, attachmentIdx }} {}
		};

		using ResourceSet_t	= HashMap< UniformID, ResourceInfo >;


	// variables
	private:
		DescriptorSetLayoutID		_layoutId;
		ResourceSet_t				_resources;


	// methods
	public:
		explicit PipelineResources (DescriptorSetLayoutID layout) : _layoutId{layout} {}

		PipelineResources (PipelineResources &&) = default;

		Self&	BindImage (const UniformID &id, ImageID image);
		Self&	BindImage (const UniformID &id, ImageID image, const ImageViewDesc &desc);

		Self&	BindTexture (const UniformID &id, ImageID image, SamplerID sampler);
		Self&	BindTexture (const UniformID &id, ImageID image, SamplerID sampler, const ImageViewDesc &desc);

		Self&	BindSampler (const UniformID &id, SamplerID sampler);

		Self&	BindBuffer (const UniformID &id, BufferID buffer);
		Self&	BindBuffer (const UniformID &id, BufferID buffer, BytesU offset, BytesU size);

		ND_ DescriptorSetLayoutID  GetLayout () const	{ return _layoutId; }
	};


	using PipelineResourceSet	= FixedMap< DescriptorSetID, PipelineResources const*, FG_MaxDescriptorSets >;

	
/*
=================================================
	BindImage
=================================================
*/
	inline PipelineResources&  PipelineResources::BindImage (const UniformID &id, ImageID image)
	{
		_resources.insert({ id, ResourceInfo( image ) });
		return *this;
	}

	inline PipelineResources&  PipelineResources::BindImage (const UniformID &id, ImageID image, const ImageViewDesc &desc)
	{
		_resources.insert({ id, ResourceInfo( image, desc ) });
		return *this;
	}
	
/*
=================================================
	BindTexture
=================================================
*/
	inline PipelineResources&  PipelineResources::BindTexture (const UniformID &id, ImageID image, SamplerID sampler)
	{
		_resources.insert({ id, ResourceInfo( image, sampler ) });
		return *this;
	}

	inline PipelineResources&  PipelineResources::BindTexture (const UniformID &id, ImageID image, SamplerID sampler, const ImageViewDesc &desc)
	{
		_resources.insert({ id, ResourceInfo( image, desc, sampler ) });
		return *this;
	}
	
/*
=================================================
	BindSampler
=================================================
*/
	inline PipelineResources&  PipelineResources::BindSampler (const UniformID &id, SamplerID sampler)
	{
		_resources.insert({ id, ResourceInfo( sampler ) });
		return *this;
	}
	
/*
=================================================
	BindBuffer
=================================================
*/
	inline PipelineResources&  PipelineResources::BindBuffer (const UniformID &id, BufferID buffer)
	{
		_resources.insert({ id, ResourceInfo( buffer, 0_b, ~0_b ) });
		return *this;
	}

	inline PipelineResources&  PipelineResources::BindBuffer (const UniformID &id, BufferID buffer, BytesU offset, BytesU size)
	{
		_resources.insert({ id, ResourceInfo( buffer, offset, size ) });
		return *this;
	}


}	// FG
