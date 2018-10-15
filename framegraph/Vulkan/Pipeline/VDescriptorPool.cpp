// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VDescriptorPool.h"
#include "VFrameGraph.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VDescriptorPool::VDescriptorPool (const VFrameGraphWeak &fg) :
		_poolId{ VK_NULL_HANDLE },
		_availableSets{ 0 },
		_frameGraph{ fg }
	{
		_InitializePoolSizes( 64 );

		CHECK( _Create() );
	}
	
/*
=================================================
	constructor
=================================================
*/
	VDescriptorPool::VDescriptorPool (const VFrameGraphWeak &fg, const VDescriptorSetLayoutPtr &layout) :
		_poolId{ VK_NULL_HANDLE },
		_availableSets{ 0 },
		_frameGraph{ fg }
	{
		_InitializePoolSizes( layout, 64 );

		CHECK( _Create() );
	}

/*
=================================================
	destructor
=================================================
*/
	VDescriptorPool::~VDescriptorPool ()
	{
		_Destroy();
	}
	
/*
=================================================
	_InitializePoolSizes
=================================================
*/
	void VDescriptorPool::_InitializePoolSizes (uint count)
	{
		_availableSets = Max( count / 8, 4u );

		for (size_t i = 0; i < _availablePoolSizes.size(); ++i)
		{
			_availablePoolSizes[i].type = VkDescriptorType(i);
			_availablePoolSizes[i].descriptorCount = count;
		}

		_availablePoolSizes[ VK_DESCRIPTOR_TYPE_SAMPLER					].descriptorCount = count;
		_availablePoolSizes[ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER	].descriptorCount = count * 4;
		_availablePoolSizes[ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE			].descriptorCount = count * 2;
		_availablePoolSizes[ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER			].descriptorCount = count * 4;
		_availablePoolSizes[ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER			].descriptorCount = count * 2;
		_availablePoolSizes[ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT		].descriptorCount = count;
	}

/*
=================================================
	_InitializePoolSizes
=================================================
*/
	void VDescriptorPool::_InitializePoolSizes (const VDescriptorSetLayoutPtr &layout, uint count)
	{
		ArrayView<VkDescriptorPoolSize>	pool_size	= layout->GetPoolSizes();
		
		for (size_t i = 0; i < _availablePoolSizes.size(); ++i)
		{
			_availablePoolSizes[i].type = VkDescriptorType(i);
			_availablePoolSizes[i].descriptorCount = 0;
		}

		for (auto& pool : pool_size)
		{
			_availablePoolSizes[ pool.type ].descriptorCount = pool.descriptorCount * count;
		}

		_availableSets = count;
	}

/*
=================================================
	_Create
=================================================
*/
	bool VDescriptorPool::_Create ()
	{
		CHECK_ERR( not _poolId );

		VFrameGraphPtr	fg = _frameGraph.lock();
		CHECK_ERR( fg );

		VDevice const&	dev = fg->GetDevice();
		
		VkDescriptorPoolCreateInfo	pool_info = {};
		pool_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount		= uint(_availablePoolSizes.size());
		pool_info.pPoolSizes		= _availablePoolSizes.data();
		pool_info.maxSets			= _availableSets;

		VK_CHECK( dev.vkCreateDescriptorPool( dev.GetVkDevice(), &pool_info, null, OUT &_poolId ) );
		return true;
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void VDescriptorPool::_Destroy ()
	{
		if ( not _poolId )
			return;

		VFrameGraphPtr	fg = _frameGraph.lock();

		if ( not fg ) {
			ASSERT( not _poolId );
			return;
		}

		VDevice const&	dev = fg->GetDevice();

		dev.vkDestroyDescriptorPool( dev.GetVkDevice(), _poolId, null );

		_poolId = VK_NULL_HANDLE;
	}

/*
=================================================
	CanAllocate
=================================================
*/
	bool VDescriptorPool::CanAllocate (ArrayView<VkDescriptorPoolSize> poolSize) const
	{
		if ( _availableSets == 0 )
			return false;

		for (auto& pool : poolSize)
		{
			if ( pool.descriptorCount > _availablePoolSizes[pool.type].descriptorCount )
				return false;
		}
		return true;
	}
	
	bool VDescriptorPool::CanAllocate (const VDescriptorSetLayoutPtr &layout) const
	{
		return CanAllocate( layout->GetPoolSizes() );
	}

/*
=================================================
	Allocate
=================================================
*/
	bool VDescriptorPool::Allocate (const VDevice &dev, const VDescriptorSetLayoutPtr &layout, OUT VkDescriptorSet &descriptorSetId)
	{
		ASSERT( _availableSets > 0 );

		ArrayView<VkDescriptorPoolSize>	pool_size	= layout->GetPoolSizes();
		VkDescriptorSetLayout			layout_id	= layout->GetLayoutID();

		for (auto& pool : pool_size)
		{
			ASSERT( pool.descriptorCount <= _availablePoolSizes[pool.type].descriptorCount );

			_availablePoolSizes[pool.type].descriptorCount -= pool.descriptorCount;
		}

		--_availableSets;


		// create descriptor
		VkDescriptorSetAllocateInfo		alloc_info = {};
		alloc_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool		= GetPoolID();
		alloc_info.pSetLayouts			= &layout_id;
		alloc_info.descriptorSetCount	= 1u;

		VK_CHECK( dev.vkAllocateDescriptorSets( dev.GetVkDevice(), &alloc_info, OUT &descriptorSetId ) );
		return true;
	}
	
/*
=================================================
	FreeDescriptorSet
=================================================
*/
	void VDescriptorPool::FreeDescriptorSet (const VDescriptorSetLayoutPtr &layout, VkDescriptorSet)
	{
		DEBUG_ONLY(
			ArrayView<VkDescriptorPoolSize>	pool_size	= layout->GetPoolSizes();

			for (auto& pool : pool_size)
			{
				_releasedPoolSizes[pool.type].descriptorCount += pool.descriptorCount;
			}

			++_releasedSets;
		)
	}
//-----------------------------------------------------------------------------

	

/*
=================================================
	constructor
=================================================
*/
	VReusableDescriptorPool::VReusableDescriptorPool (const VDevice &dev, const VDescriptorSetLayoutPtr &layout, uint count) :
		_poolId{ VK_NULL_HANDLE },
		_availableSetCount{ count },
		_layout{ layout }
	{
		PoolSizeArray_t		pool_size = layout->GetPoolSizes();
		
		for (auto& pool : pool_size) {
			pool.descriptorCount *= count;
		}
		
		VkDescriptorPoolCreateInfo	pool_info = {};
		pool_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount		= uint(pool_size.size());
		pool_info.pPoolSizes		= pool_size.data();
		pool_info.maxSets			= _availableSetCount;

		VK_CALL( dev.vkCreateDescriptorPool( dev.GetVkDevice(), &pool_info, null, OUT &_poolId ) );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VReusableDescriptorPool::~VReusableDescriptorPool ()
	{
		CHECK( not _poolId );
	}
	
/*
=================================================
	_Allocate
=================================================
*/
	bool VReusableDescriptorPool::_Allocate (const VDevice &dev, OUT VkDescriptorSet &descriptorSetId)
	{
		if ( not _releasedSets.empty() )
		{
			descriptorSetId = _releasedSets.back();
			_releasedSets.pop_back();
			return true;
		}

		CHECK_ERR( _poolId );
		CHECK_ERR( _availableSetCount > 0 );

		VDescriptorSetLayoutPtr			layout		= _layout.lock();
		CHECK_ERR( layout );

		VkDescriptorSetLayout			layout_id	= layout->GetLayoutID();

		// create descriptor
		VkDescriptorSetAllocateInfo		alloc_info	= {};
		alloc_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool		= _poolId;
		alloc_info.pSetLayouts			= &layout_id;
		alloc_info.descriptorSetCount	= 1u;

		VK_CHECK( dev.vkAllocateDescriptorSets( dev.GetVkDevice(), &alloc_info, OUT &descriptorSetId ) );

		--_availableSetCount;
		return true;
	}
	
/*
=================================================
	_CanAllocate
=================================================
*/
	bool VReusableDescriptorPool::_CanAllocate () const
	{
		return _availableSetCount > 0 or _releasedSets.size() > 0;
	}

/*
=================================================
	_Destroy
=================================================
*/
	void VReusableDescriptorPool::_Destroy (const VDevice &dev)
	{
		if ( _poolId )
		{
			dev.vkDestroyDescriptorPool( dev.GetVkDevice(), _poolId, null );
			_poolId = VK_NULL_HANDLE;
		}

		_layout.reset();
		_releasedSets.clear();
	}

/*
=================================================
	FreeDescriptorSet
=================================================
*/
	void VReusableDescriptorPool::FreeDescriptorSet (const VDescriptorSetLayoutPtr &layout, VkDescriptorSet ds)
	{
		ASSERT( layout == _layout.lock() );
		ASSERT( _poolId );

		_releasedSets.push_back( ds );
	}


}	// FG
