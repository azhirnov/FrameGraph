// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VDescriptorManager.h"
#include "VDevice.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VDescriptorManager::VDescriptorManager (const VDevice &dev) :
		_device{ dev }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VDescriptorManager::~VDescriptorManager ()
	{
		CHECK( _descriptorPools.empty() );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VDescriptorManager::Initialize ()
	{
		EXLOCK( _guard );

		CHECK_ERR( _CreateDescriptorPool() );
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VDescriptorManager::Deinitialize ()
	{
		EXLOCK( _guard );

		for (auto& item : _descriptorPools)
		{
			//EXLOCK( item.guard );

			if ( item.pool )
			{
				_device.vkDestroyDescriptorPool( _device.GetVkDevice(), item.pool, null );
				item.pool = VK_NULL_HANDLE;
			}
		}
		_descriptorPools.clear();
	}
	
/*
=================================================
	AllocDescriptorSet
=================================================
*/
	bool  VDescriptorManager::AllocDescriptorSet (VkDescriptorSetLayout layout, OUT DescriptorSet &ds)
	{
		EXLOCK( _guard );

		VkDescriptorSetAllocateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorSetCount	= 1;
		info.pSetLayouts		= &layout;

		for (auto& item : _descriptorPools)
		{
			//EXLOCK( item.guard );
			info.descriptorPool = item.pool;
			
			if ( _device.vkAllocateDescriptorSets( _device.GetVkDevice(), &info, OUT &ds.first ) == VK_SUCCESS )
			{
				ds.second = uint8_t(Distance( _descriptorPools.data(), &item ));
				return true;
			}
		}

		CHECK_ERR( _CreateDescriptorPool() );
		
		info.descriptorPool = _descriptorPools.back().pool;
		VK_CHECK( _device.vkAllocateDescriptorSets( _device.GetVkDevice(), &info, OUT &ds.first ));
		ds.second = uint8_t(_descriptorPools.size() - 1);

		return true;
	}
	
/*
=================================================
	DeallocDescriptorSet
=================================================
*/
	bool  VDescriptorManager::DeallocDescriptorSet (const DescriptorSet &ds)
	{
		EXLOCK( _guard );
		CHECK_ERR( ds.second < _descriptorPools.size() );

		VK_CALL( _device.vkFreeDescriptorSets( _device.GetVkDevice(), _descriptorPools[ds.second].pool, 1, &ds.first ));
		return true;
	}
	
/*
=================================================
	DeallocDescriptorSets
=================================================
*/
	bool  VDescriptorManager::DeallocDescriptorSets (ArrayView<DescriptorSet> descSets)
	{
		EXLOCK( _guard );

		FixedArray< VkDescriptorSet, 32 >	temp;
		uint8_t								last_idx = UMax;

		for (auto& ds : descSets)
		{
			if ( last_idx != ds.second and temp.size() )
			{
				VK_CALL( _device.vkFreeDescriptorSets( _device.GetVkDevice(), _descriptorPools[last_idx].pool, uint(temp.size()), temp.data() ));
				temp.clear();
			}

			last_idx = ds.second;
			temp.push_back( ds.first );
		}

		if ( temp.size() )
		{
			VK_CALL( _device.vkFreeDescriptorSets( _device.GetVkDevice(), _descriptorPools[last_idx].pool, uint(temp.size()), temp.data() ));
		}
		return true;
	}

/*
=================================================
	_CreateDescriptorPool
=================================================
*/
	bool  VDescriptorManager::_CreateDescriptorPool ()
	{
		CHECK_ERR( _descriptorPools.size() < _descriptorPools.capacity() );

		FixedArray< VkDescriptorPoolSize, 32 >	pool_sizes;

		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER,						MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		MaxDescriptorPoolSize * 4 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,				MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,				MaxDescriptorPoolSize });

		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,				MaxDescriptorPoolSize * 4 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,				MaxDescriptorPoolSize * 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,		MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,		MaxDescriptorPoolSize });
		
		if ( _device.IsRayTracingEnabled() ) {
			pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, MaxDescriptorPoolSize });
		}

		
		VkDescriptorPoolCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount	= uint(pool_sizes.size());
		info.pPoolSizes		= pool_sizes.data();
		info.maxSets		= MaxDescriptorSets;
		info.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkDescriptorPool	ds_pool;
		VK_CHECK( _device.vkCreateDescriptorPool( _device.GetVkDevice(), &info, null, OUT &ds_pool ));

		_descriptorPools.push_back({ ds_pool });
		return true;
	}


}	// FG
