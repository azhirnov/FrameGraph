// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VDescriptorSetLayout.h"

namespace FG
{

	//
	// Vulkan Pipeline Layout
	//

	class VPipelineLayout final : public std::enable_shared_from_this<VPipelineLayout>
	{
		friend class VPipelineCache;

	// types
	private:
		struct DescriptorSet
		{
			VDescriptorSetLayoutPtr		layout;
			uint						index;
		};

		using DescriptorSets_t			= FixedMap< DescriptorSetID, DescriptorSet, FG_MaxDescriptorSets >;
		using VkDescriptorSetLayouts_t	= FixedArray< VkDescriptorSetLayout, FG_MaxDescriptorSets >;
		using VkPushConstantRanges_t	= FixedArray< VkPushConstantRange, FG_MaxPushConstants >;


	// variables
	private:
		HashVal					_hash;
		VkPipelineLayout		_layout;
		DescriptorSets_t		_descriptorSets;


	// methods
	protected:
		bool _Setup (const PipelineDescription::PipelineLayout &ppln, ArrayView<VDescriptorSetLayoutPtr> sets);
		bool _Create (const VDevice &dev);
		void _Destroy (const VDevice &dev);

		void _AddDescriptorSets (const PipelineDescription::PipelineLayout &ppln, ArrayView<VDescriptorSetLayoutPtr> sets,
								 INOUT HashVal &hash, OUT DescriptorSets_t &setsInfo) const;
		void _AddPushConstants (const PipelineDescription::PipelineLayout &ppln, 
								INOUT HashVal &hash) const;

	public:
		VPipelineLayout (const PipelineDescription::PipelineLayout &ppln, ArrayView<VDescriptorSetLayoutPtr> sets);
		VPipelineLayout (const VDevice &dev, VPipelineLayout &&other);
		~VPipelineLayout ();

		ND_ bool	operator == (const VPipelineLayout &rhs) const;

		ND_ HashVal					GetHash ()		const		{ return _hash; }

		ND_ VkPipelineLayout		GetLayoutID ()	const		{ ASSERT( _layout );  return _layout; }

		ND_ VDescriptorSetLayoutPtr	GetDescriptorSet (const DescriptorSetID &id) const;
		ND_ uint					GetBindingIndex (const DescriptorSetID &id) const;
	};


}	// FG
