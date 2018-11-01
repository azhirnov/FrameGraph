// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/Pipeline.h"
#include "framegraph/Shared/ResourceBase.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Descriptor Set Layout
	//

	class VDescriptorSetLayout final : public ResourceBase
	{
	// types
	public:
		using DescriptorBinding_t	= Array< VkDescriptorSetLayoutBinding >;

	private:
		using UniformMapPtr			= PipelineDescription::UniformMapPtr;
		using PoolSizeArray_t		= FixedArray< VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE >;


	// variables
	private:
		HashVal					_hash;
		VkDescriptorSetLayout	_layout		= VK_NULL_HANDLE;
		UniformMapPtr			_uniforms;
		PoolSizeArray_t			_poolSize;
		uint					_maxIndex	= 0;


	// methods
	public:
		VDescriptorSetLayout () {}
		~VDescriptorSetLayout ();

		void Initialize (const UniformMapPtr &uniforms, OUT DescriptorBinding_t &binding);
		bool Create (const VDevice &dev, const DescriptorBinding_t &binding);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		void Replace (INOUT VDescriptorSetLayout &&other);

		ND_ bool	operator == (const VDescriptorSetLayout &rhs) const;

		ND_ VkDescriptorSetLayout	Handle ()		const	{ return _layout; }
		ND_ HashVal					GetHash ()		const	{ return _hash; }
		ND_ UniformMapPtr const&	GetUniforms ()	const	{ return _uniforms; }
		ND_ uint const				GetMaxIndex ()	const	{ return _maxIndex; }


	private:
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

	};

}	// FG


namespace std
{

	template <>
	struct hash< FG::VDescriptorSetLayout >
	{
		ND_ size_t  operator () (const FG::VDescriptorSetLayout &value) const noexcept {
			return size_t(value.GetHash());
		}
	};

}	// std
