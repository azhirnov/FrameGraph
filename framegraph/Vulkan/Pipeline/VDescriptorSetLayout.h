// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Pipeline.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Descriptor Set Layout
	//

	class VDescriptorSetLayout final
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
		DebugName_t				_debugName;
		
		RWRaceConditionCheck	_rcCheck;


	// methods
	public:
		VDescriptorSetLayout () {}
		VDescriptorSetLayout (VDescriptorSetLayout &&) = default;
		VDescriptorSetLayout (const UniformMapPtr &uniforms, OUT DescriptorBinding_t &binding);
		~VDescriptorSetLayout ();

		bool Create (const VDevice &dev, const DescriptorBinding_t &binding);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		ND_ bool	operator == (const VDescriptorSetLayout &rhs) const;

		ND_ VkDescriptorSetLayout	Handle ()		const	{ SHAREDLOCK( _rcCheck );  return _layout; }
		ND_ HashVal					GetHash ()		const	{ SHAREDLOCK( _rcCheck );  return _hash; }
		ND_ UniformMapPtr const&	GetUniforms ()	const	{ SHAREDLOCK( _rcCheck );  return _uniforms; }
		ND_ uint const				GetMaxIndex ()	const	{ SHAREDLOCK( _rcCheck );  return _maxIndex; }
		
		ND_ StringView				GetDebugName ()	const	{ SHAREDLOCK( _rcCheck );  return _debugName; }


	private:
		void _AddUniform (const PipelineDescription::Uniform &un, INOUT DescriptorBinding_t &binding);
		void _AddImage (const PipelineDescription::Image &img, uint bindingIndex, EShaderStages stageFlags, INOUT DescriptorBinding_t &binding);
		void _AddTexture (const PipelineDescription::Texture &tex, uint bindingIndex, EShaderStages stageFlags, INOUT DescriptorBinding_t &binding);
		void _AddSampler (const PipelineDescription::Sampler &samp, uint bindingIndex, EShaderStages stageFlags, INOUT DescriptorBinding_t &binding);
		void _AddSubpassInput (const PipelineDescription::SubpassInput &spi, uint bindingIndex, EShaderStages stageFlags, INOUT DescriptorBinding_t &binding);
		void _AddUniformBuffer (const PipelineDescription::UniformBuffer &ub, uint bindingIndex, EShaderStages stageFlags, INOUT DescriptorBinding_t &binding);
		void _AddStorageBuffer (const PipelineDescription::StorageBuffer &sb, uint bindingIndex, EShaderStages stageFlags, INOUT DescriptorBinding_t &binding);
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
