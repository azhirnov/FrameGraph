// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VPipelineResources.h"
#include "VDescriptorSetLayout.h"
#include "VDescriptorPool.h"
#include "VSampler.h"
#include "Shared/EnumUtils.h"

namespace FG
{

/*
=================================================
	AddBuffer
=================================================
*/
	void VPipelineResources::UpdateDescriptors::AddBuffer (const Buffer &buf, VkBuffer id, VkDescriptorSet ds)
	{
		ASSERT( buf.binding != INVALID_BINDING );

		VkDescriptorBufferInfo	info = {};
		info.buffer	= id;
		info.offset	= VkDeviceSize(buf.offset);
		info.range	= VkDeviceSize(buf.size);

		buffers.push_back( std::move(info) );


		VkWriteDescriptorSet	wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= null;
		wds.descriptorType	= ((buf.state & EResourceState::_StateMask) == EResourceState::UniformRead) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		wds.descriptorCount	= 1;
		wds.dstArrayElement	= 0;
		wds.dstBinding		= buf.binding;
		wds.dstSet			= ds;
		wds.pBufferInfo		= &buffers.back();

		descriptors.push_back( std::move(wds) );
	}
	
/*
=================================================
	AddImage
=================================================
*/
	void VPipelineResources::UpdateDescriptors::AddImage (const Image &img, VkImageView id, VkDescriptorSet ds)
	{
		ASSERT( img.binding != INVALID_BINDING );

		VkDescriptorImageInfo	info = {};
		info.imageLayout	= img.layout;
		info.imageView		= id;
		info.sampler		= VK_NULL_HANDLE;

		images.push_back( std::move(info) );
		

		VkWriteDescriptorSet	wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= null;
		wds.descriptorType	= ((img.state & EResourceState::_StateMask) == EResourceState::InputAttachment) ? VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		wds.descriptorCount	= 1;
		wds.dstArrayElement	= 0;
		wds.dstBinding		= img.binding;
		wds.dstSet			= ds;
		wds.pImageInfo		= &images.back();

		descriptors.push_back( std::move(wds) );
	}
	
/*
=================================================
	AddTexture
=================================================
*/
	void VPipelineResources::UpdateDescriptors::AddTexture (const Texture &tex, VkImageView img, VkSampler samp, VkDescriptorSet ds)
	{
		ASSERT( tex.binding != INVALID_BINDING );

		VkDescriptorImageInfo	info = {};
		info.imageLayout	= tex.layout;
		info.imageView		= img;
		info.sampler		= samp;

		images.push_back( std::move(info) );
		

		VkWriteDescriptorSet	wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= null;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		wds.descriptorCount	= 1;
		wds.dstArrayElement	= 0;
		wds.dstBinding		= tex.binding;
		wds.dstSet			= ds;
		wds.pImageInfo		= &images.back();

		descriptors.push_back( std::move(wds) );
	}
	
/*
=================================================
	AddSampler
=================================================
*/
	void VPipelineResources::UpdateDescriptors::AddSampler (const Sampler &samp, VkSampler id, VkDescriptorSet ds)
	{
		ASSERT( samp.binding != INVALID_BINDING );

		VkDescriptorImageInfo	info = {};
		info.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.imageView		= VK_NULL_HANDLE;
		info.sampler		= id;

		images.push_back( std::move(info) );
		

		VkWriteDescriptorSet	wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= null;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_SAMPLER;
		wds.descriptorCount	= 1;
		wds.dstArrayElement	= 0;
		wds.dstBinding		= samp.binding;
		wds.dstSet			= ds;
		wds.pImageInfo		= &images.back();

		descriptors.push_back( std::move(wds) );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VPipelineResources::VPipelineResources (const VDescriptorSetLayoutPtr &layout) :
		_descriptorSet{ VK_NULL_HANDLE },
		_layout{ layout },
		_hasLogical{ false }
	{
		_resources.resize( layout->GetUniforms().size() );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VPipelineResources::~VPipelineResources ()
	{
		if ( _descriptorPool and _descriptorSet )
		{
			_descriptorPool->FreeDescriptorSet( _layout, _descriptorSet );
		}
	}
	
/*
=================================================
	_IsCreated
=================================================
*/
	bool VPipelineResources::_IsCreated () const
	{
		return _descriptorSet != VK_NULL_HANDLE;
	}

/*
=================================================
	_Create
=================================================
*/
	bool VPipelineResources::_Create (const IDescriptorPoolPtr &pool, VkDescriptorSet descriptorSet)
	{
		CHECK_ERR( _layout and pool and descriptorSet );

		_descriptorSet	= descriptorSet;
		_descriptorPool	= pool;
		return true;
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void VPipelineResources::_Destroy ()
	{
		// descriptor set will be destroyed together with descriptor pool
		//ASSERT( not _descriptorPool and _descriptorPool->GetPoolID() == VK_NULL_HANDLE );	// TODO

		_descriptorSet	= VK_NULL_HANDLE;
		_descriptorPool	= null;

		_resources.clear();
	}

/*
=================================================
	_GetRes
=================================================
*/
	template <typename T>
	inline T const&  VPipelineResources::_GetRes (const UniformID &id) const
	{
		const auto	res_iter = _layout->GetUniforms().find( id );
		ASSERT( res_iter != _layout->GetUniforms().end() );

		const auto	info = std::get_if< T >( &res_iter->second );
		ASSERT( info );

		return *info;
	}

/*
=================================================
	BindImage
=================================================
*/
	VPipelineResources::Base_t&
		VPipelineResources::BindImage (const UniformID &id, const ImagePtr &image)
	{
		return BindImage( id, image, ImageViewDesc{image->Description()} );
	}
	
/*
=================================================
	BindImage
=================================================
*/
	VPipelineResources::Base_t&
		VPipelineResources::BindImage (const UniformID &id, const ImagePtr &image, const ImageViewDesc &desc)
	{
		ASSERT( image );
		ASSERT( not image->IsImmutable() );

		_hasLogical |= image->IsLogical();
		
		Image		img;
		img.image	= image;
		img.desc	= desc;

		const auto	res_iter = _layout->GetUniforms().find( id );
		ASSERT( res_iter != _layout->GetUniforms().end() );

		// image storage
		if ( auto* img_info = std::get_if< PipelineDescription::Image >( &res_iter->second ) )
		{
			image->AddUsage( EImageUsage::Storage );
			ASSERT( EnumEq( image->Description().usage, EImageUsage::Storage ));

			img.state	= EResourceState_FromShaders( img_info->stageFlags );
			img.layout	= VK_IMAGE_LAYOUT_GENERAL;
			img.binding	= img_info->index.VKBinding();

			ENABLE_ENUM_CHECKS();
			switch ( img_info->access ) {
				case EShaderAccess::ReadOnly :		img.state |= EResourceState::ShaderRead;		break;
				case EShaderAccess::WriteOnly :		img.state |= EResourceState::ShaderWrite;		break;
				case EShaderAccess::ReadWrite :		img.state |= EResourceState::ShaderReadWrite;	break;
				case EShaderAccess::WriteDiscard :	img.state |= EResourceState::ShaderWrite | EResourceState::InvalidateBefore;	break;
			}
			DISABLE_ENUM_CHECKS();

			_resources[ img.binding ] = std::move(img);
		}
		else
		// subpass input
		if ( auto* spi_info = std::get_if< PipelineDescription::SubpassInput >( &res_iter->second ) )
		{
			image->AddUsage( EImageUsage::InputAttachment );
			ASSERT( EnumEq( image->Description().usage, EImageUsage::InputAttachment ));

			img.state	= EResourceState::InputAttachment | EResourceState_FromShaders( spi_info->stageFlags );
			img.layout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			img.binding	= spi_info->index.VKBinding();

			_resources[ img.binding ] = std::move(img);
		}
		else
		{
			RETURN_ERR( "unsupported image type", *this );
		}

		return *this;
	}
	
/*
=================================================
	BindTexture
=================================================
*/
	VPipelineResources::Base_t&
		VPipelineResources::BindTexture (const UniformID &id, const ImagePtr &image, const SamplerPtr &sampler)
	{
		return BindTexture( id, image, sampler, ImageViewDesc{image->Description()} );
	}
	
/*
=================================================
	BindTexture
=================================================
*/
	VPipelineResources::Base_t&
		VPipelineResources::BindTexture (const UniformID &id, const ImagePtr &image, const SamplerPtr &sampler, const ImageViewDesc &desc)
	{
		ASSERT( image and sampler );

		image->AddUsage( EImageUsage::Sampled );

		_hasLogical |= image->IsLogical();
		
		const auto&	info = _GetRes< PipelineDescription::Texture >( id );

		ASSERT( image->Description().imageType == info.textureType );
		ASSERT( EnumEq( image->Description().usage, EImageUsage::Sampled ));

		Texture		tex;
		tex.image	= image;
		tex.desc	= desc;
		tex.sampler	= Cast<VSampler>( sampler );
		tex.state	= EResourceState::ShaderSample | EResourceState_FromShaders( info.stageFlags );
		tex.layout	= image->IsImmutable() ? VK_IMAGE_LAYOUT_GENERAL :VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		tex.binding	= info.index.VKBinding();

		_resources[ tex.binding ] = std::move(tex);
		return *this;
	}

/*
=================================================
	BindSampler
=================================================
*/
	VPipelineResources::Base_t&
		VPipelineResources::BindSampler (const UniformID &id, const SamplerPtr &sampler)
	{
		ASSERT( sampler );
		
		const auto&	info = _GetRes< PipelineDescription::Sampler >( id );

		Sampler		samp;
		samp.sampler	= Cast<VSampler>( sampler );
		samp.binding	= info.index.VKBinding();

		_resources[ samp.binding ] = std::move(samp);
		return *this;
	}

/*
=================================================
	BindBuffer
=================================================
*/
	VPipelineResources::Base_t&
		VPipelineResources::BindBuffer (const UniformID &id, const BufferPtr &buffer)
	{
		return BindBuffer( id, buffer, 0_b, buffer->Description().size );
	}
	
/*
=================================================
	BindBuffer
=================================================
*/
	VPipelineResources::Base_t&
		VPipelineResources::BindBuffer (const UniformID &id, const BufferPtr &buffer, const BytesU offset, const BytesU size)
	{
		ASSERT( buffer );

		_hasLogical |= buffer->IsLogical();

		Buffer		buf;
		buf.buffer	= buffer;
		buf.offset	= VkDeviceSize(offset);
		buf.size	= VkDeviceSize(size);

		const auto	res_iter = _layout->GetUniforms().find( id );
		ASSERT( res_iter != _layout->GetUniforms().end() );

		// uniform buffer
		if ( auto* ubuf = std::get_if< PipelineDescription::UniformBuffer >( &res_iter->second ) )
		{
			buffer->AddUsage( EBufferUsage::Uniform );
			
			ASSERT( ubuf->size == size );
			ASSERT( EnumEq( buffer->Description().usage, EBufferUsage::Uniform ));

			buf.state	= EResourceState::UniformRead | EResourceState_FromShaders( ubuf->stageFlags );
			buf.binding	= ubuf->index.VKBinding();

			_resources[ buf.binding ] = std::move(buf);
		}
		else
		// storage buffer
		if ( auto* sbuf = std::get_if< PipelineDescription::StorageBuffer >( &res_iter->second ) )
		{
			buffer->AddUsage( EBufferUsage::Storage );
			
			ASSERT(	(size >= sbuf->staticSize) and
					(sbuf->arrayStride == 0 or (size - sbuf->staticSize) % sbuf->arrayStride == 0) );
			ASSERT( EnumEq( buffer->Description().usage, EBufferUsage::Storage ));

			buf.state	= EResourceState_FromShaders( sbuf->stageFlags );
			buf.binding	= sbuf->index.VKBinding();

			ENABLE_ENUM_CHECKS();
			switch ( sbuf->access ) {
				case EShaderAccess::ReadOnly :		buf.state |= EResourceState::ShaderRead;		break;
				case EShaderAccess::WriteOnly :		buf.state |= EResourceState::ShaderWrite;		break;
				case EShaderAccess::ReadWrite :		buf.state |= EResourceState::ShaderReadWrite;	break;
				case EShaderAccess::WriteDiscard :	buf.state |= EResourceState::ShaderWrite | EResourceState::InvalidateBefore;	break;
			}
			DISABLE_ENUM_CHECKS();

			_resources[ buf.binding ] = std::move(buf);
		}
		else
		{
			RETURN_ERR( "unsupported buffer type", *this );
		}

		return *this;
	}


}	// FG
