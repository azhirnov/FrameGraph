// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/PipelineResources.h"
#include "framegraph/Public/Pipeline.h"
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

		using ResourceSet_t		= PipelineResources::ResourceSet_t;


	// variables
	private:
		VkDescriptorSet				_descriptorSet		= VK_NULL_HANDLE;
		DescriptorSetLayoutID		_layoutId;
		//DescriptorPoolID			_descriptorPoolId;
		HashVal						_hash;
		ResourceSet_t				_resources;			// all resource ids has a weak reference
		
		DebugName_t					_debugName;


	// methods
	public:
		VPipelineResources () {}
		~VPipelineResources ();

		void Initialize (const PipelineResources &desc);
		bool Create (VResourceManagerThread &, VPipelineCache &);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		ND_ bool	IsAllResourcesAlive (const VResourceManagerThread &) const;

		ND_ bool	operator == (const VPipelineResources &rhs) const;

		ND_ VkDescriptorSet				Handle ()		const	{ SHAREDLOCK( _rcCheck );  return _descriptorSet; }
		ND_ RawDescriptorSetLayoutID	GetLayoutID ()	const	{ SHAREDLOCK( _rcCheck );  return _layoutId.Get(); }
		ND_ HashVal						GetHash ()		const	{ SHAREDLOCK( _rcCheck );  return _hash; }
		ND_ ResourceSet_t const&		GetData ()		const	{ SHAREDLOCK( _rcCheck );  return _resources; }


	private:
		bool _AddResource (VResourceManagerThread &, INOUT PipelineResources::Buffer &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, INOUT PipelineResources::Image &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, INOUT PipelineResources::Texture &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, const PipelineResources::Sampler &, INOUT UpdateDescriptors &);
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
