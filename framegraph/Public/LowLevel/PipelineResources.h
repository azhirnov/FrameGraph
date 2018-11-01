// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/IDs.h"
#include "framegraph/Public/LowLevel/ImageDesc.h"
#include "framegraph/Public/LowLevel/Pipeline.h"

namespace FG
{

	//
	// Pipeline Resources
	//

	struct PipelineResources final
	{
		friend class PipelineResourcesInitializer;

	// types
	public:
		using Self	= PipelineResources;

	private:
		struct Buffer
		{
			BindingIndex			index;
			EResourceState			state;
			RawBufferID				buffer;
			BytesU					offset;
			BytesU					size;

			ND_ bool  operator == (const Buffer &rhs) const {
				return	state	== rhs.state	and 
						buffer	== rhs.buffer	and
						offset	== rhs.offset	and
						size	== rhs.size;
			}
		};

		struct Image
		{
			BindingIndex			index;
			EResourceState			state;
			RawImageID				image;
			Optional<ImageViewDesc>	desc;

			ND_ bool  operator == (const Image &rhs) const {
				return	state	== rhs.state	and
						image	== rhs.image	and
						desc	== rhs.desc;
			}
		};

		struct Texture final : Image
		{
			RawSamplerID			sampler;

			ND_ bool  operator == (const Texture &rhs) const {
				return	Image::operator== (rhs) and
						sampler == rhs.sampler;
			}
		};

		struct Subpass final : Image
		{
			uint					attachmentIndex	= ~0u;

			ND_ bool  operator == (const Subpass &rhs) const {
				return	Image::operator== (rhs) and
						attachmentIndex == rhs.attachmentIndex;
			}
		};

		struct Sampler
		{
			BindingIndex			index;
			RawSamplerID			sampler;

			ND_ bool  operator == (const Sampler &rhs) const {
				return	sampler == rhs.sampler;
			}
		};

		using Resource_t	= Union< std::monostate, Buffer, Image, Texture, Sampler, Subpass >;

		struct ResourceInfo
		{
			HashVal			hash;
			Resource_t		res;

			ResourceInfo ()
				{}
			ResourceInfo (const BindingIndex &index, EResourceState state, RawBufferID buf, BytesU offset, BytesU size) :
				hash{HashOf(index) + HashOf(state) + HashOf(buf) + HashOf(offset) + HashOf(size)},
				res{Buffer{ index, state, buf, offset, size }}
				{}
			ResourceInfo (const BindingIndex &index, EResourceState state, RawImageID img) :
				hash{HashOf(index) + HashOf(state) + HashOf(img)},
                res{Image{ index, state, img, {} }}
				{}
			ResourceInfo (const BindingIndex &index, EResourceState state, RawImageID img, const ImageViewDesc &desc) :
				hash{HashOf(index) + HashOf(state) + HashOf(img) + HashOf(desc)},
				res{Image{ index, state, img, desc }}
				{}
			ResourceInfo (const BindingIndex &index, EResourceState state, RawImageID img, RawSamplerID samp) :
				hash{HashOf(index) + HashOf(state) + HashOf(img) + HashOf(samp)},
                res{Texture{{ index, state, img, {} }, samp }}
				{}
			ResourceInfo (const BindingIndex &index, EResourceState state, RawImageID img, const ImageViewDesc &desc, RawSamplerID samp) :
				hash{HashOf(index) + HashOf(state) + HashOf(img) + HashOf(desc) + HashOf(samp)},
                res{Texture{{ index, state, img, desc }, samp }}
				{}
			ResourceInfo (const BindingIndex &index, RawSamplerID samp) :
				hash{HashOf(index) + HashOf(samp)},
				res{Sampler{ index, samp }}
				{}
			ResourceInfo (const BindingIndex &index, EResourceState state, RawImageID img, uint attachmentIdx) :
				hash{HashOf(index) + HashOf(state) + HashOf(img) + HashOf(attachmentIdx)},
                res{Subpass{{ index, state, img, {} }, attachmentIdx }}
				{}
			ResourceInfo (const BindingIndex &index, EResourceState state, RawImageID img, const ImageViewDesc &desc, uint attachmentIdx) :
				hash{HashOf(index) + HashOf(state) + HashOf(img) + HashOf(desc) + HashOf(attachmentIdx)},
                res{Subpass{{ index, state, img, desc }, attachmentIdx }}
				{}
		};

		using ResourceSet_t	= Array< ResourceInfo >;
		using UniformMapPtr	= PipelineDescription::UniformMapPtr;


	// variables
	private:
		RawDescriptorSetLayoutID	_layoutId;
		UniformMapPtr				_uniforms;
		ResourceSet_t				_resources;


	// methods
	public:
		PipelineResources () noexcept {}
		PipelineResources (PipelineResources &&) = default;

		Self&	BindImage (const UniformID &id, const ImageID &image) noexcept;
		Self&	BindImage (const UniformID &id, const ImageID &image, const ImageViewDesc &desc) noexcept;

		Self&	BindTexture (const UniformID &id, const ImageID &image, const SamplerID &sampler) noexcept;
		Self&	BindTexture (const UniformID &id, const ImageID &image, const SamplerID &sampler, const ImageViewDesc &desc) noexcept;

		Self&	BindSampler (const UniformID &id, const SamplerID &sampler) noexcept;

		Self&	BindBuffer (const UniformID &id, const BufferID &buffer) noexcept;
		Self&	BindBuffer (const UniformID &id, const BufferID &buffer, BytesU offset, BytesU size) noexcept;

		ND_ bool  HasImage (const UniformID &id)	const noexcept;
		ND_ bool  HasSampler (const UniformID &id)	const noexcept;
		ND_ bool  HasTexture (const UniformID &id)	const noexcept;
		ND_ bool  HasBuffer (const UniformID &id)	const noexcept;

		ND_ DescriptorSetLayoutID	GetLayout ()	const	{ return DescriptorSetLayoutID{ _layoutId }; }
		ND_ ResourceSet_t const&	GetData ()		const	{ return _resources; }
	};


	using PipelineResourceSet	= FixedMap< DescriptorSetID, PipelineResources const*, FG_MaxDescriptorSets >;

	
/*
=================================================
	BindImage
=================================================
*/
	inline PipelineResources&  PipelineResources::BindImage (const UniformID &id, const ImageID &image) noexcept
	{
		ASSERT( HasImage( id ) );
		auto	un = _uniforms->find( id );

		if ( auto* image_un = std::get_if<PipelineDescription::Image>( &un->second ) )
		{
			_resources[ image_un->index.Unique() ] = ResourceInfo{ image_un->index, image_un->state, image.Get() };
		}
		else
		if ( auto* subpass_un = std::get_if<PipelineDescription::SubpassInput>( &un->second ) )
		{
			_resources[ subpass_un->index.Unique() ] = ResourceInfo{ subpass_un->index, subpass_un->state, image.Get() };
		}
		return *this;
	}
	
/*
=================================================
	BindImage
=================================================
*/
	inline PipelineResources&  PipelineResources::BindImage (const UniformID &id, const ImageID &image, const ImageViewDesc &desc) noexcept
	{
		ASSERT( HasImage( id ) );
		auto	un = _uniforms->find( id );
		
		if ( auto* image_un = std::get_if<PipelineDescription::Image>( &un->second ) )
		{
			_resources[ image_un->index.Unique() ] = ResourceInfo{ image_un->index, image_un->state, image.Get(), desc };
		}
		else
		if ( auto* subpass_un = std::get_if<PipelineDescription::SubpassInput>( &un->second ) )
		{
			_resources[ subpass_un->index.Unique() ] = ResourceInfo{ subpass_un->index, subpass_un->state, image.Get(), desc };
		}
		return *this;
	}
	
/*
=================================================
	HasImage
=================================================
*/
	inline bool  PipelineResources::HasImage (const UniformID &id) const noexcept
	{
		if ( _uniforms ) {
			auto	un = _uniforms->find( id );
			return (un != _uniforms->end() and (std::holds_alternative<PipelineDescription::Image>( un->second ) or
												std::holds_alternative<PipelineDescription::SubpassInput>( un->second )) );
		}
		return false;
	}

/*
=================================================
	BindTexture
=================================================
*/
	inline PipelineResources&  PipelineResources::BindTexture (const UniformID &id, const ImageID &image, const SamplerID &sampler) noexcept
	{
		ASSERT( HasTexture( id ) );
		auto&	tex_un = std::get<PipelineDescription::Texture>( _uniforms->find( id )->second );
		
		_resources[ tex_un.index.Unique() ] = ResourceInfo{ tex_un.index, tex_un.state, image.Get(), sampler.Get() };
		return *this;
	}
	
/*
=================================================
	BindTexture
=================================================
*/
	inline PipelineResources&  PipelineResources::BindTexture (const UniformID &id, const ImageID &image, const SamplerID &sampler, const ImageViewDesc &desc) noexcept
	{
		ASSERT( HasTexture( id ) );
		auto&	tex_un = std::get<PipelineDescription::Texture>( _uniforms->find( id )->second );
		
		_resources[ tex_un.index.Unique() ] = ResourceInfo{ tex_un.index, tex_un.state, image.Get(), desc, sampler.Get() };
		return *this;
	}
	
/*
=================================================
	HasTexture
=================================================
*/
	inline bool  PipelineResources::HasTexture (const UniformID &id) const noexcept
	{
		if ( _uniforms ) {
			auto	un = _uniforms->find( id );
			return (un != _uniforms->end() and std::holds_alternative<PipelineDescription::Texture>( un->second ));
		}
		return false;
	}

/*
=================================================
	BindSampler
=================================================
*/
	inline PipelineResources&  PipelineResources::BindSampler (const UniformID &id, const SamplerID &sampler) noexcept
	{
		ASSERT( HasSampler( id ) );
		auto&	samp_un = std::get<PipelineDescription::Sampler>( _uniforms->find( id )->second );
		
		_resources[ samp_un.index.Unique() ] = ResourceInfo{ samp_un.index, sampler.Get() };
		return *this;
	}
	
/*
=================================================
	HasSampler
=================================================
*/
	inline bool  PipelineResources::HasSampler (const UniformID &id) const noexcept
	{
		if ( _uniforms ) {
			auto	un = _uniforms->find( id );
			return (un != _uniforms->end() and std::holds_alternative<PipelineDescription::Sampler>( un->second ));
		}
		return false;
	}

/*
=================================================
	BindBuffer
=================================================
*/
	inline PipelineResources&  PipelineResources::BindBuffer (const UniformID &id, const BufferID &buffer) noexcept
	{
		ASSERT( HasBuffer( id ) );
		auto	un = _uniforms->find( id );
		
		if ( auto* ubuf = std::get_if<PipelineDescription::UniformBuffer>( &un->second ) )
		{
			_resources[ ubuf->index.Unique() ] = ResourceInfo{ ubuf->index, ubuf->state, buffer.Get(), 0_b, ~0_b };
		}
		else
		if ( auto* sbuf = std::get_if<PipelineDescription::StorageBuffer>( &un->second ) )
		{
			_resources[ sbuf->index.Unique() ] = ResourceInfo{ sbuf->index, sbuf->state, buffer.Get(), 0_b, ~0_b };
		}
		return *this;
	}
	
/*
=================================================
	BindBuffer
=================================================
*/
	inline PipelineResources&  PipelineResources::BindBuffer (const UniformID &id, const BufferID &buffer, BytesU offset, BytesU size) noexcept
	{
		ASSERT( HasBuffer( id ) );
		auto	un = _uniforms->find( id );
		
		if ( auto* ubuf = std::get_if<PipelineDescription::UniformBuffer>( &un->second ) )
		{
			_resources[ ubuf->index.Unique() ] = ResourceInfo{ ubuf->index, ubuf->state, buffer.Get(), offset, size };
		}
		else
		if ( auto* sbuf = std::get_if<PipelineDescription::StorageBuffer>( &un->second ) )
		{
			_resources[ sbuf->index.Unique() ] = ResourceInfo{ sbuf->index, sbuf->state, buffer.Get(), offset, size };
		}
		return *this;
	}
	
/*
=================================================
	HasBuffer
=================================================
*/
	inline bool  PipelineResources::HasBuffer (const UniformID &id) const noexcept
	{
		if ( _uniforms ) {
			auto	un = _uniforms->find( id );
			return (un != _uniforms->end() and (std::holds_alternative<PipelineDescription::UniformBuffer>( un->second ) or
												std::holds_alternative<PipelineDescription::StorageBuffer>( un->second )) );
		}
		return false;
	}


}	// FG
