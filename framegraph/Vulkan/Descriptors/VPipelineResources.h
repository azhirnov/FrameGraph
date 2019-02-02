// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/PipelineResources.h"
#include "framegraph/Public/Pipeline.h"
#include "framegraph/Shared/PipelineResourcesInitializer.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Pipeline Resources
	//

	class VPipelineResources final
	{
	// types
	private:
		struct UpdateDescriptors
		{
			LinearAllocator<>			allocator;
			VkWriteDescriptorSet *		descriptors;
			uint						descriptorIndex;
		};

		using ResourceSet_t		= PipelineResources::ResourceSet_t;


	// variables
	private:
		VkDescriptorSet				_descriptorSet		= VK_NULL_HANDLE;
		RawDescriptorSetLayoutID	_layoutId;
		//DescriptorPoolID			_descriptorPoolId;
		HashVal						_hash;
		ResourceSet_t				_resources;				// all resource ids has a weak reference
		const bool					_allowEmptyResources;
		
		DebugName_t					_debugName;
		
		RWRaceConditionCheck		_rcCheck;


	// methods
	public:
		VPipelineResources () : _allowEmptyResources{false} {}
		VPipelineResources (VPipelineResources &&) = default;
		VPipelineResources (const PipelineResources &desc);
		~VPipelineResources ();

		bool Create (VResourceManagerThread &);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		ND_ bool  IsAllResourcesAlive (const VResourceManagerThread &) const;

		ND_ bool  operator == (const VPipelineResources &rhs) const;

		ND_ VkDescriptorSet				Handle ()		const	{ SHAREDLOCK( _rcCheck );  return _descriptorSet; }
		ND_ RawDescriptorSetLayoutID	GetLayoutID ()	const	{ SHAREDLOCK( _rcCheck );  return _layoutId; }
		ND_ HashVal						GetHash ()		const	{ SHAREDLOCK( _rcCheck );  return _hash; }
		ND_ ResourceSet_t const&		GetResources ()	const	{ SHAREDLOCK( _rcCheck );  return _resources; }
		
		ND_ StringView					GetDebugName ()	const	{ SHAREDLOCK( _rcCheck );  return _debugName; }


	private:
		bool _AddResource (VResourceManagerThread &, INOUT PipelineResources::Buffer &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, INOUT PipelineResources::Image &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, INOUT PipelineResources::Texture &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, const PipelineResources::Sampler &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, const PipelineResources::RayTracingScene &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManagerThread &, const NullUnion &, INOUT UpdateDescriptors &);
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
