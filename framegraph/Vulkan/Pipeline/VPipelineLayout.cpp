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
	destructor
=================================================
*/
	VPipelineLayout::~VPipelineLayout ()
	{
	}

/*
=================================================
	Initialize
=================================================
*/
	void VPipelineLayout::Initialize (const PipelineDescription::PipelineLayout &ppln, DSLayoutArray_t sets)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( ppln.descriptorSets.size() == sets.size() );
		ASSERT( _layout == VK_NULL_HANDLE );

		_AddDescriptorSets( ppln, sets, INOUT _hash, OUT _descriptorSets );
		_AddPushConstants( ppln, INOUT _hash );
	}
	
/*
=================================================
	_AddDescriptorSets
=================================================
*/
	void VPipelineLayout::_AddDescriptorSets (const PipelineDescription::PipelineLayout &ppln, DSLayoutArray_t sets,
											  INOUT HashVal &hash, OUT DescriptorSets_t &setsInfo) const
	{
		setsInfo.clear();

		for (size_t i = 0; i < ppln.descriptorSets.size(); ++i)
		{
			auto&	ds = ppln.descriptorSets[i];

			setsInfo.insert({ ds.id, DescriptorSet{ sets[i].first, sets[i].second->Handle(), ds.bindingIndex }});
			
			// calculate hash
			hash << HashOf( ds.id );
			hash << HashOf( ds.bindingIndex );
			hash << sets[i].second->GetHash();
		}

		setsInfo.sort();
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
	Create
=================================================
*/
	bool VPipelineLayout::Create (const VDevice &dev)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( GetState() == EState::Initial );
		CHECK_ERR( _layout == VK_NULL_HANDLE );
		
		VkDescriptorSetLayouts_t	vk_sets;		vk_sets.resize( _descriptorSets.size() );
		VkPushConstantRanges_t		vk_ranges;		// TODO

		auto	ds_iter = _descriptorSets.begin();

		for (size_t i = 0; ds_iter != _descriptorSets.end(); ++i, ++ds_iter)
		{
			auto&	ds = ds_iter->second;

			ASSERT( ds.index < vk_sets.size() );
			ASSERT( vk_sets[ ds.index ] == VK_NULL_HANDLE );

			vk_sets[ ds.index ] = ds.layout;
		}
		
		VkPipelineLayoutCreateInfo			layout_info = {};
		layout_info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount			= uint(vk_sets.size());
		layout_info.pSetLayouts				= vk_sets.data();
		layout_info.pushConstantRangeCount	= uint(vk_ranges.size());
		layout_info.pPushConstantRanges		= vk_ranges.data();

		VK_CHECK( dev.vkCreatePipelineLayout( dev.GetVkDevice(), &layout_info, null, OUT &_layout ) );
		
		_OnCreate();
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VPipelineLayout::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		SCOPELOCK( _rcCheck );

		if ( _layout ) {
			readyToDelete.emplace_back( VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, uint64_t(_layout) );
		}

		for (auto& ds : _descriptorSets) {
			unassignIDs.push_back( ds.second.layoutId.Release() );
		}

		_descriptorSets.clear();
		_layout = VK_NULL_HANDLE;
		_hash	= Default;
		
		_OnDestroy();
	}

/*
=================================================
	operator ==
=================================================
*/
	bool VPipelineLayout::operator == (const VPipelineLayout &rhs) const
	{
		SHAREDLOCK( _rcCheck );
		SHAREDLOCK( rhs._rcCheck );

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

			if ( not (lhs_ds->second.layoutId == rhs_ds->second.layoutId) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	GetDescriptorSetLayout
=================================================
*/
	bool VPipelineLayout::GetDescriptorSetLayout (const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const
	{
		SHAREDLOCK( _rcCheck );

		auto	iter = _descriptorSets.find( id );

		if ( iter != _descriptorSets.end() )
		{
			layout	= iter->second.layoutId.Get();
			binding	= iter->second.index;
			return true;
		}

		return false;
	}


}	// FG
