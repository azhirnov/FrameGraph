// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/PipelineResources.h"
#include "framegraph/Public/Pipeline.h"
#include "VDescriptorSetLayout.h"

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

		using Element_t			= Union< VkDescriptorBufferInfo, VkDescriptorImageInfo, VkAccelerationStructureNV >;
		using DynamicDataPtr	= PipelineResources::DynamicDataPtr;
		using DescriptorSet		= VDescriptorSetLayout::DescriptorSet;


	// variables
	private:
		DescriptorSet				_descriptorSet;
		RawDescriptorSetLayoutID	_layoutId;
		HashVal						_hash;
		DynamicDataPtr				_dataPtr;
		const bool					_allowEmptyResources;
		
		DebugName_t					_debugName;
		
		RWRaceConditionCheck		_rcCheck;


	// methods
	public:
		VPipelineResources () : _allowEmptyResources{false} {}
		VPipelineResources (VPipelineResources &&) = default;
		explicit VPipelineResources (const PipelineResources &desc);
		explicit VPipelineResources (INOUT PipelineResources &desc);
		~VPipelineResources ();

			bool Create (VResourceManager &);
			void Destroy (VResourceManager &);

		ND_ bool  IsAllResourcesAlive (const VResourceManager &) const;

		ND_ bool  operator == (const VPipelineResources &rhs) const;
		
			template <typename Fn>
			void ForEachUniform (Fn&& fn) const					{ SHAREDLOCK( _rcCheck );  ASSERT( _dataPtr );  _dataPtr->ForEachUniform( fn ); }

		ND_ VkDescriptorSet				Handle ()		const	{ SHAREDLOCK( _rcCheck );  return _descriptorSet.first; }
		ND_ RawDescriptorSetLayoutID	GetLayoutID ()	const	{ SHAREDLOCK( _rcCheck );  return _layoutId; }
		ND_ HashVal						GetHash ()		const	{ SHAREDLOCK( _rcCheck );  return _hash; }

		ND_ StringView					GetDebugName ()	const	{ SHAREDLOCK( _rcCheck );  return _debugName; }


	private:
		bool _AddResource (VResourceManager &, INOUT PipelineResources::Buffer &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, INOUT PipelineResources::Image &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, INOUT PipelineResources::Texture &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, const PipelineResources::Sampler &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, const PipelineResources::RayTracingScene &, INOUT UpdateDescriptors &);
		bool _AddResource (VResourceManager &, const NullUnion &, INOUT UpdateDescriptors &);
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
