// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "framegraph/Public/LowLevel/Pipeline.h"

namespace FG
{

	//
	// Vulkan Descriptor Set Layout
	//

	class VDescriptorSetLayout final : public DescriptorSetLayout
	{
		friend class VPipelineCache;

	// types
	private:
		using UniformMap_t			= PipelineDescription::UniformMap_t;
		using DescriptorBinding_t	= Array< VkDescriptorSetLayoutBinding >;
		using PoolSizeArray_t		= FixedArray< VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE >;
		using DescriptorPoolPtr		= SharedPtr< class VReusableDescriptorPool >;
		using DestriptorPools_t		= Array< DescriptorPoolPtr >;


	// variables
	private:
		HashVal					_hash;
		VkDescriptorSetLayout	_layout;
		UniformMap_t			_uniforms;
		PoolSizeArray_t			_poolSize;
		DestriptorPools_t		_descriptorPools;
		uint					_maxIndex;

		static constexpr uint	_maxDescriptorSetsInPool = 64;


	// methods
	private:
		bool _Create (const VDevice &dev, const DescriptorBinding_t &binding, INOUT VkDescriptorSetLayout &layout) const;
		void _Destroy (const VDevice &dev);

		bool _CreateDescriptorSet (const VDevice &dev, OUT IDescriptorPoolPtr &pool, OUT VkDescriptorSet &set);

		static void _AddUniform (const PipelineDescription::Uniform_t &un, INOUT HashVal &hash, INOUT uint &maxIndex,
								 INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize);

		static void _AddImage (const PipelineDescription::Image &img, INOUT HashVal &hash, INOUT uint &maxIndex,
								INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize);

		static void _AddTexture (const PipelineDescription::Texture &tex, INOUT HashVal &hash, INOUT uint &maxIndex,
								 INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize);

		static void _AddSampler (const PipelineDescription::Sampler &samp, INOUT HashVal &hash, INOUT uint &maxIndex,
								 INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize);

		static void _AddSubpassInput (const PipelineDescription::SubpassInput &spi, INOUT HashVal &hash, INOUT uint &maxIndex,
									  INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize);

		static void _AddUniformBuffer (const PipelineDescription::UniformBuffer &ub, INOUT HashVal &hash, INOUT uint &maxIndex,
										INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize);

		static void _AddStorageBuffer (const PipelineDescription::StorageBuffer &sb, INOUT HashVal &hash, INOUT uint &maxIndex,
										INOUT DescriptorBinding_t &binding, INOUT PoolSizeArray_t &poolSize);


	public:
		explicit VDescriptorSetLayout (UniformMap_t &&uniforms);
		VDescriptorSetLayout (const VDevice &dev, VDescriptorSetLayout &&other);
		~VDescriptorSetLayout ();

		ND_ bool	operator == (const VDescriptorSetLayout &rhs) const;

		ND_ HashVal							GetHash ()		const		{ return _hash; }

		ND_ UniformMap_t const&				GetUniforms ()	const		{ return _uniforms; }

		ND_ ArrayView<VkDescriptorPoolSize>	GetPoolSizes ()	const		{ return _poolSize; }

		ND_ VkDescriptorSetLayout			GetLayoutID ()	const		{ ASSERT( _layout );  return _layout; }
	};


}	// FG
