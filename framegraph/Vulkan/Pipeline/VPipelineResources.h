// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/PipelineResources.h"
#include "framegraph/Public/LowLevel/Pipeline.h"
#include "framegraph/Shared/ResourceBase.h"
#include "framegraph/Shared/PipelineResourcesInitializer.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Pipeline Resources
	//

	class VPipelineResources final : public ResourceBase
	{
	// types
	private:
		struct UpdateDescriptors
		{
			Deque< VkDescriptorBufferInfo >		buffers;
			Deque< VkDescriptorImageInfo >		images;
			Deque< VkBufferView >				bufferViews;
			Array< VkWriteDescriptorSet >		descriptors;
		};

		using ResourceSet_t		= PipelineResourcesInitializer::Resources_t;


	// variables
	private:
		VkDescriptorSet				_descriptorSet		= VK_NULL_HANDLE;
		DescriptorSetLayoutID		_layoutId;
		//DescriptorPoolID			_descriptorPoolId;
		HashVal						_hash;
		ResourceSet_t				_resources;			// all resource ids has a weak reference


	// methods
	public:
		VPipelineResources () {}
		~VPipelineResources ();

		void Initialize (const PipelineResources &desc);
		bool Create (VResourceManagerThread &, VPipelineCache &);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		void Replace (INOUT VPipelineResources &&other);

		ND_ bool	operator == (const VPipelineResources &rhs) const;

		ND_ VkDescriptorSet				Handle ()		const	{ return _descriptorSet; }
		ND_ RawDescriptorSetLayoutID	GetLayoutID ()	const	{ return _layoutId.Get(); }
		ND_ HashVal						GetHash ()		const	{ return _hash; }


	private:
		bool _AddResource (VResourceManagerThread &, const PipelineResourcesInitializer::Buffer &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, const PipelineResourcesInitializer::Image &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, const PipelineResourcesInitializer::Texture &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, const PipelineResourcesInitializer::Sampler &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, const std::monostate &, INOUT UpdateDescriptors &);
	};


}	// FG


namespace std
{

	template <>
	struct hash< FG::VPipelineResources >
	{
		ND_ size_t  operator () (const FG::VPipelineResources &value) const noexcept {
			return size_t(value.GetHash());
		}
	};

}	// std
