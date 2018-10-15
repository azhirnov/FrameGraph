// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VDescriptorSetLayout.h"
#include "VDescriptorPool.h"
#include "VEnumCast.h"
#include "VDevice.h"

namespace FG
{

/*
=================================================
	TextureEquals
=================================================
*/
	ND_ inline bool TextureEquals (const PipelineDescription::Texture &lhs, const PipelineDescription::Texture &rhs) noexcept
	{
		return	lhs.textureType			== rhs.textureType			and
				lhs.index.VKBinding()	== rhs.index.VKBinding()	and
				lhs.stageFlags			== rhs.stageFlags;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	SamplerEquals
=================================================
*/
	ND_ inline bool SamplerEquals (const PipelineDescription::Sampler &lhs, const PipelineDescription::Sampler &rhs) noexcept
	{
		return	lhs.index.VKBinding()	== rhs.index.VKBinding()	and
				lhs.stageFlags			== rhs.stageFlags;
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	SubpassInputEquals
=================================================
*/
	ND_ inline bool SubpassInputEquals (const PipelineDescription::SubpassInput &lhs, const PipelineDescription::SubpassInput &rhs) noexcept
	{
		return	lhs.attachmentIndex		== rhs.attachmentIndex		and
				lhs.isMultisample		== rhs.isMultisample		and
				lhs.index.VKBinding()	== rhs.index.VKBinding()	and
				lhs.stageFlags			== rhs.stageFlags;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	ImageEquals
=================================================
*/
	ND_ inline bool ImageEquals (const PipelineDescription::Image &lhs, const PipelineDescription::Image &rhs) noexcept
	{
		return	lhs.imageType			== rhs.imageType			and
				lhs.format				== rhs.format				and
				lhs.access				== rhs.access				and
				lhs.index.VKBinding()	== rhs.index.VKBinding()	and
				lhs.stageFlags			== rhs.stageFlags;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	UniformBufferEquals
=================================================
*/
	ND_ inline bool UniformBufferEquals (const PipelineDescription::UniformBuffer &lhs, const PipelineDescription::UniformBuffer &rhs) noexcept
	{
		return	lhs.size				== rhs.size					and
				lhs.index.VKBinding()	== rhs.index.VKBinding()	and
				lhs.stageFlags			== rhs.stageFlags;
	}
//-----------------------------------------------------------------------------

	
	
/*
=================================================
	StorageBufferEquals
=================================================
*/
	ND_ inline bool StorageBufferEquals (const PipelineDescription::StorageBuffer &lhs, const PipelineDescription::StorageBuffer &rhs) noexcept
	{
		return	lhs.staticSize			== rhs.staticSize			and
				lhs.arrayStride			== rhs.arrayStride			and
				lhs.access				== rhs.access				and
				lhs.index.VKBinding()	== rhs.index.VKBinding()	and
				lhs.stageFlags			== rhs.stageFlags;
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	UniformEquals
=================================================
*/
	ND_ inline bool UniformEquals (const PipelineDescription::Uniform_t &lhsUniform, const PipelineDescription::Uniform_t &rhsUniform) noexcept
	{
		bool	result = false;

		Visit( lhsUniform,
			[&rhsUniform, &result] (const PipelineDescription::Texture &lhs)
			{
				if ( auto* rhs = std::get_if<PipelineDescription::Texture>( &rhsUniform ) )
				{
					result = TextureEquals( lhs, *rhs );
				}
			},
				   
			[&rhsUniform, &result] (const PipelineDescription::Sampler &lhs)
			{
				if ( auto* rhs = std::get_if<PipelineDescription::Sampler>( &rhsUniform ) )
				{
					result = SamplerEquals( lhs, *rhs );
				}
			},
				
			[&rhsUniform, &result] (const PipelineDescription::SubpassInput &lhs)
			{
				if ( auto* rhs = std::get_if<PipelineDescription::SubpassInput>( &rhsUniform ) )
				{
					result = SubpassInputEquals( lhs, *rhs );
				}
			},
				
			[&rhsUniform, &result] (const PipelineDescription::Image &lhs)
			{
				if ( auto* rhs = std::get_if<PipelineDescription::Image>( &rhsUniform ) )
				{
					result = ImageEquals( lhs, *rhs );
				}
			},
				
			[&rhsUniform, &result] (const PipelineDescription::UniformBuffer &lhs)
			{
				if ( auto* rhs = std::get_if<PipelineDescription::UniformBuffer>( &rhsUniform ) )
				{
					result = UniformBufferEquals( lhs, *rhs );
				}
			},
				
			[&rhsUniform, &result] (const PipelineDescription::StorageBuffer &lhs)
			{
				if ( auto* rhs = std::get_if<PipelineDescription::StorageBuffer>( &rhsUniform ) )
				{
					result = StorageBufferEquals( lhs, *rhs );
				}
			},

			[] (const auto &) { ASSERT(false); }
		);

		return result;
	}
//-----------------------------------------------------------------------------
	
	

/*
=================================================
	constructor
----
	used to create temporary object for searching
	in descriptor set layout cache
=================================================
*/
	VDescriptorSetLayout::VDescriptorSetLayout (UniformMap_t &&uniforms) :
		_layout{ VK_NULL_HANDLE },
		_uniforms{ std::move(uniforms) },
		_maxIndex{ 0 }
	{
		// initialize pool size
		_poolSize.resize( VK_DESCRIPTOR_TYPE_RANGE_SIZE );

		for (size_t i = 0; i < _poolSize.size(); ++i)
		{
			_poolSize[i].type = VkDescriptorType(i);
			_poolSize[i].descriptorCount = 0;
		}

		// bind uniforms
		DescriptorBinding_t		binding;	binding.reserve( _uniforms.size() );

		for (auto& un : _uniforms)
		{
			_hash << HashOf( un.first );

			_AddUniform( un.second, INOUT _hash, INOUT _maxIndex, INOUT binding, INOUT _poolSize );
		}
	}
	
/*
=================================================
	constructor
=================================================
*/
	VDescriptorSetLayout::VDescriptorSetLayout (const VDevice &dev, VDescriptorSetLayout &&other) :
		_layout{ VK_NULL_HANDLE },
		_uniforms{ std::move(other._uniforms) },
		_maxIndex{ 0 }
	{
		// initialize pool size
		_poolSize.resize( VK_DESCRIPTOR_TYPE_RANGE_SIZE );

		for (size_t i = 0; i < _poolSize.size(); ++i)
		{
			_poolSize[i].type = VkDescriptorType(i);
			_poolSize[i].descriptorCount = 0;
		}

		// bind uniforms
		DescriptorBinding_t		binding;	binding.reserve( _uniforms.size() );

		for (auto& un : _uniforms)
		{
			_hash << HashOf( un.first );

			_AddUniform( un.second, INOUT _hash, INOUT _maxIndex, INOUT binding, INOUT _poolSize );
		}

		// remove empty pool sizes
		for (size_t i = 0; i < _poolSize.size(); ++i)
		{
			if ( _poolSize[i].descriptorCount == 0 )
			{
				std::swap( _poolSize[i], _poolSize.back() );
				_poolSize.pop_back();
				--i;
			}
		}

		CHECK( _hash == other._hash );
		CHECK( _Create( dev, binding, OUT _layout ));
	}

/*
=================================================
	destructor
=================================================
*/
	VDescriptorSetLayout::~VDescriptorSetLayout ()
	{
		CHECK( _layout == VK_NULL_HANDLE );
	}

/*
=================================================
	_Create
=================================================
*/
	bool VDescriptorSetLayout::_Create (const VDevice &dev, const DescriptorBinding_t &binding, INOUT VkDescriptorSetLayout &layout) const
	{
		VkDescriptorSetLayoutCreateInfo	descriptor_info = {};
		descriptor_info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_info.pBindings		= binding.data();
		descriptor_info.bindingCount	= uint(binding.size());

		VK_CHECK( dev.vkCreateDescriptorSetLayout( dev.GetVkDevice(), &descriptor_info, null, OUT &layout ) );
		return true;
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void VDescriptorSetLayout::_Destroy (const VDevice &dev)
	{
		if ( _layout )
		{
			dev.vkDestroyDescriptorSetLayout( dev.GetVkDevice(), _layout, null );
			_layout = VK_NULL_HANDLE;
		}

		for (auto& pool : _descriptorPools)
		{
			pool->_Destroy( dev );
		}
		_descriptorPools.clear();

		_hash = Default;
		_uniforms.clear();
		_poolSize.clear();
	}
	
/*
=================================================
	_CreateDescriptorSet
=================================================
*/
	bool VDescriptorSetLayout::_CreateDescriptorSet (const VDevice &dev, OUT IDescriptorPoolPtr &outPool, OUT VkDescriptorSet &outSet)
	{
		for (auto& pool : _descriptorPools)
		{
			if ( pool->_CanAllocate() )
			{
				CHECK_ERR( pool->_Allocate( dev, OUT outSet ) );

				outPool = pool;
				return true;
			}
		}

		// create new pool
		DescriptorPoolPtr	pool = MakeShared<VReusableDescriptorPool>( dev, Cast<VDescriptorSetLayout>( shared_from_this() ), _maxDescriptorSetsInPool );

		_descriptorPools.push_back( pool );
		
		CHECK_ERR( pool->_Allocate( dev, OUT outSet ) );

		outPool = pool;
		return true;
	}

/*
=================================================
	_AddUniform
=================================================
*/
	void VDescriptorSetLayout::_AddUniform (const PipelineDescription::Uniform_t &un, INOUT HashVal &hash, INOUT uint &maxIndex,
											INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize)
	{
		Visit( un,
			[&hash, &maxIndex, &binding, &poolSize] (const PipelineDescription::Texture &tex) {
				_AddTexture( tex, INOUT hash, INOUT maxIndex, INOUT binding, INOUT poolSize );
			},
			[&hash, &maxIndex, &binding, &poolSize] (const PipelineDescription::Sampler &samp) {
				_AddSampler( samp, INOUT hash, INOUT maxIndex, INOUT binding, INOUT poolSize );
			},
			[&hash, &maxIndex, &binding, &poolSize] (const PipelineDescription::SubpassInput &spi) {
				_AddSubpassInput( spi, INOUT hash, INOUT maxIndex, INOUT binding, INOUT poolSize );
			},
			[&hash, &maxIndex, &binding, &poolSize] (const PipelineDescription::Image &img)	{
				_AddImage( img, INOUT hash, INOUT maxIndex, INOUT binding, INOUT poolSize );
			},
			[&hash, &maxIndex, &binding, &poolSize] (const PipelineDescription::UniformBuffer &ub) {
				_AddUniformBuffer( ub, INOUT hash, INOUT maxIndex, INOUT binding, INOUT poolSize );
			},
			[&hash, &maxIndex, &binding, &poolSize] (const PipelineDescription::StorageBuffer &sb) {
				_AddStorageBuffer( sb, INOUT hash, INOUT maxIndex, INOUT binding, INOUT poolSize );
			},
			[] (const auto &) { ASSERT(false); }
		);
	}

/*
=================================================
	_AddImage
=================================================
*/
	void VDescriptorSetLayout::_AddImage (const PipelineDescription::Image &img, INOUT HashVal &hash, INOUT uint &maxIndex,
										  INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize)
	{
		// calculate hash
		hash << HashOf( img.imageType );
		hash << HashOf( img.format );
		hash << HashOf( img.access );
		hash << HashOf( img.index.VKBinding() );
		hash << HashOf( img.stageFlags );
		
		// add binding
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		bind.stageFlags			= VEnumCast( img.stageFlags );
		bind.binding			= img.index.VKBinding();
		bind.descriptorCount	= 1;

		maxIndex = Max( maxIndex, bind.binding );
		poolSize[ bind.descriptorType ].descriptorCount++;

		binding.push_back( std::move(bind) );
	}
	
/*
=================================================
	_AddTexture
=================================================
*/
	void VDescriptorSetLayout::_AddTexture (const PipelineDescription::Texture &tex, INOUT HashVal &hash, INOUT uint &maxIndex,
											INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize)
	{
		// calculate hash
		hash << HashOf( tex.textureType );
		hash << HashOf( tex.index.VKBinding() );
		hash << HashOf( tex.stageFlags );

		// add binding
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bind.stageFlags			= VEnumCast( tex.stageFlags );
		bind.binding			= tex.index.VKBinding();
		bind.descriptorCount	= 1;
		
		maxIndex = Max( maxIndex, bind.binding );
		poolSize[ bind.descriptorType ].descriptorCount++;

		binding.push_back( std::move(bind) );
	}
	
/*
=================================================
	_AddSampler
=================================================
*/
	void VDescriptorSetLayout::_AddSampler (const PipelineDescription::Sampler &samp, INOUT HashVal &hash, INOUT uint &maxIndex,
											INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize)
	{
		// calculate hash
		hash << HashOf( samp.index.VKBinding() );
		hash << HashOf( samp.stageFlags );
		
		// add binding
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLER;
		bind.stageFlags			= VEnumCast( samp.stageFlags );
		bind.binding			= samp.index.VKBinding();
		bind.descriptorCount	= 1;
		
		maxIndex = Max( maxIndex, bind.binding );
		poolSize[ bind.descriptorType ].descriptorCount++;

		binding.push_back( std::move(bind) );
	}
	
/*
=================================================
	_AddSubpassInput
=================================================
*/
	void VDescriptorSetLayout::_AddSubpassInput (const PipelineDescription::SubpassInput &spi, INOUT HashVal &hash, INOUT uint &maxIndex,
												 INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize)
	{
		// calculate hash
		hash << HashOf( spi.attachmentIndex );
		hash << HashOf( spi.isMultisample );
		hash << HashOf( spi.index.VKBinding() );
		hash << HashOf( spi.stageFlags );
		
		// add binding
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		bind.stageFlags			= VEnumCast( spi.stageFlags );
		bind.binding			= spi.index.VKBinding();
		bind.descriptorCount	= 1;
		
		maxIndex = Max( maxIndex, bind.binding );
		poolSize[ bind.descriptorType ].descriptorCount++;

		binding.push_back( std::move(bind) );
	}
	
/*
=================================================
	_AddUniformBuffer
=================================================
*/
	void VDescriptorSetLayout::_AddUniformBuffer (const PipelineDescription::UniformBuffer &ub, INOUT HashVal &hash, INOUT uint &maxIndex,
												  INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize)
	{
		// calculate hash
		hash << HashOf( ub.size );
		hash << HashOf( ub.index.VKBinding() );
		hash << HashOf( ub.stageFlags );

		// add binding
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bind.stageFlags			= VEnumCast( ub.stageFlags );
		bind.binding			= ub.index.VKBinding();
		bind.descriptorCount	= 1;
		
		maxIndex = Max( maxIndex, bind.binding );
		poolSize[ bind.descriptorType ].descriptorCount++;

		binding.push_back( std::move(bind) );
	}
	
/*
=================================================
	_AddStorageBuffer
=================================================
*/
	void VDescriptorSetLayout::_AddStorageBuffer (const PipelineDescription::StorageBuffer &sb, INOUT HashVal &hash, INOUT uint &maxIndex,
												  INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize)
	{
		// calculate hash
		hash << HashOf( sb.staticSize );
		hash << HashOf( sb.arrayStride );
		hash << HashOf( sb.access );
		hash << HashOf( sb.index.VKBinding() );
		hash << HashOf( sb.stageFlags );
		
		// add binding
		VkDescriptorSetLayoutBinding	bind = {};
		bind.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bind.stageFlags			= VEnumCast( sb.stageFlags );
		bind.binding			= sb.index.VKBinding();
		bind.descriptorCount	= 1;
		
		maxIndex = Max( maxIndex, bind.binding );
		poolSize[ bind.descriptorType ].descriptorCount++;

		binding.push_back( std::move(bind) );
	}

/*
=================================================
	operator ==
=================================================
*/
	bool VDescriptorSetLayout::operator == (const VDescriptorSetLayout &rhs) const
	{
		if ( _hash != rhs._hash )
			return false;

		if ( _uniforms.size() != rhs._uniforms.size() )
			return false;

		for (auto& un : _uniforms)
		{
			auto	iter = rhs._uniforms.find( un.first );

			if ( iter == rhs._uniforms.end() )
				return false;
			
			if ( not UniformEquals( un.second, iter->second ) )
				return false;
		}
		return true;
	}
	
}	// FG
