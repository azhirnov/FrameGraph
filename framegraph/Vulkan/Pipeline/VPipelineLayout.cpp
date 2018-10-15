// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VPipelineLayout.h"
#include "VDevice.h"

namespace FG
{
	
/*
=================================================
	PushConstantEquals
=================================================
*/
	ND_ inline bool PushConstantEquals (const PipelineDescription::PushConstant &lhs, const PipelineDescription::PushConstant &rhs) noexcept
	{
		return	lhs.offset	== rhs.offset;
	}
	
/*
=================================================
	PushConstantGreater
=================================================
*/
	ND_ inline bool PushConstantGreater (const PipelineDescription::PushConstant &lhs, const PipelineDescription::PushConstant &rhs) noexcept
	{
		return	lhs.offset	> rhs.offset;
	}
	
/*
=================================================
	PushConstantHash
=================================================
*/
	ND_ inline HashVal PushConstantHash (const PipelineDescription::PushConstant &pc)
	{
		return HashOf( pc.offset );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
----
	used to create temporary object for searching
	in pipeline layout cache
=================================================
*/
	VPipelineLayout::VPipelineLayout (const PipelineDescription::PipelineLayout &ppln, ArrayView<VDescriptorSetLayoutPtr> sets) :
		_hash{ 0 },
		_layout{ VK_NULL_HANDLE }
	{
		CHECK( _Setup( ppln, sets ) );
	}
	
/*
=================================================
	constructor
=================================================
*/
	VPipelineLayout::VPipelineLayout (const VDevice &dev, VPipelineLayout &&other) :
		_hash{ other._hash },
		_layout{ VK_NULL_HANDLE },
		_descriptorSets{ std::move(other._descriptorSets) }
	{
		CHECK( _Create( dev ) );
		//CHECK( _hash == other._hash );
	}

/*
=================================================
	destructor
=================================================
*/
	VPipelineLayout::~VPipelineLayout ()
	{
		CHECK( _layout == VK_NULL_HANDLE );
	}
	
/*
=================================================
	_Setup
=================================================
*/
	bool VPipelineLayout::_Setup (const PipelineDescription::PipelineLayout &ppln, ArrayView<VDescriptorSetLayoutPtr> sets)
	{
		CHECK_ERR( ppln.descriptorSets.size() == sets.size() );
		CHECK_ERR( _layout == VK_NULL_HANDLE );

		_AddDescriptorSets( ppln, sets, INOUT _hash, OUT _descriptorSets );
		_AddPushConstants( ppln, INOUT _hash );

		return true;
	}

/*
=================================================
	_Create
=================================================
*/
	bool VPipelineLayout::_Create (const VDevice &dev)
	{
		CHECK_ERR( _layout == VK_NULL_HANDLE );
		
		VkDescriptorSetLayouts_t	vk_sets;		vk_sets.resize( _descriptorSets.size() );
		VkPushConstantRanges_t		vk_ranges;		// TODO

		auto	ds_iter = _descriptorSets.begin();

		for (size_t i = 0; ds_iter != _descriptorSets.end(); ++i, ++ds_iter)
		{
			auto&	ds = ds_iter->second;

			ASSERT( ds.index < vk_sets.size() );
			ASSERT( vk_sets[ ds.index ] == VK_NULL_HANDLE );

			vk_sets[ ds.index ] = ds.layout->GetLayoutID();
		}
		
		VkPipelineLayoutCreateInfo			layout_info = {};
		layout_info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount			= uint(vk_sets.size());
		layout_info.pSetLayouts				= vk_sets.data();
		layout_info.pushConstantRangeCount	= uint(vk_ranges.size());
		layout_info.pPushConstantRanges		= vk_ranges.data();

		VK_CHECK( dev.vkCreatePipelineLayout( dev.GetVkDevice(), &layout_info, null, OUT &_layout ) );
		return true;
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void VPipelineLayout::_Destroy (const VDevice &dev)
	{
		if ( _layout )
		{
			dev.vkDestroyPipelineLayout( dev.GetVkDevice(), _layout, null );
			_layout = VK_NULL_HANDLE;
		}

		_hash			= Default;
		_descriptorSets = Default;
	}

/*
=================================================
	_AddDescriptorSets
=================================================
*/
	void VPipelineLayout::_AddDescriptorSets (const PipelineDescription::PipelineLayout &ppln, ArrayView<VDescriptorSetLayoutPtr> sets,
											  INOUT HashVal &hash, OUT DescriptorSets_t &setsInfo) const
	{
		setsInfo.clear();

		for (size_t i = 0; i < ppln.descriptorSets.size(); ++i)
		{
			auto&	ds = ppln.descriptorSets[i];

			setsInfo.insert({ ds.id, DescriptorSet{ sets[i], ds.bindingIndex }});
			
			// calculate hash
			hash << HashOf( ds.id );
			hash << HashOf( ds.bindingIndex );
			hash << sets[i]->GetHash();
		}

		//setsInfo.sort();
	}
	
/*
=================================================
	_AddPushConstants
=================================================
*/
	void VPipelineLayout::_AddPushConstants (const PipelineDescription::PipelineLayout &ppln, INOUT HashVal &hash) const
	{
		for (auto& pc : ppln.pushConstants)
		{
			// calculate hash
			hash << PushConstantHash( pc );
		}
	}

/*
=================================================
	operator ==
=================================================
*/
	bool VPipelineLayout::operator == (const VPipelineLayout &rhs) const
	{
		if ( _hash != rhs._hash )
			return false;

		if ( _descriptorSets.size() != rhs._descriptorSets.size() )
			return false;

		const size_t	ds_count	= _descriptorSets.size();
		auto			lhs_ds		= _descriptorSets.begin();
		auto			rhs_ds		= rhs._descriptorSets.begin();

		for (size_t i = 0; i < _descriptorSets.size(); ++i, ++lhs_ds, ++rhs_ds)
		{
			if ( lhs_ds->second.index != rhs_ds->second.index )
				return false;

			ASSERT( lhs_ds->second.layout );
			ASSERT( rhs_ds->second.layout );

			if ( not (*lhs_ds->second.layout == *rhs_ds->second.layout) )
				return false;
		}
		return true;
	}

/*
=================================================
	GetDescriptorSet
=================================================
*/
	VDescriptorSetLayoutPtr  VPipelineLayout::GetDescriptorSet (const DescriptorSetID &id) const
	{
		auto	iter = _descriptorSets.find( id );

		if ( iter != _descriptorSets.end() )
			return iter->second.layout;

		ASSERT(false);
		return null;
	}
	
/*
=================================================
	GetBindingIndex
=================================================
*/
	uint  VPipelineLayout::GetBindingIndex (const DescriptorSetID &id) const
	{
		auto	iter = _descriptorSets.find( id );

		if ( iter != _descriptorSets.end() )
			return iter->second.index;

		ASSERT(false);
		return 0;
	}


}	// FG
