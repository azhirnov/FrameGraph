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
	VDescriptorManager::VDescriptorManager ()
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
	bool VDescriptorManager::Initialize (const VDevice &dev)
	{
		CHECK_ERR( _CreateDescriptorPool( dev ));
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VDescriptorManager::Deinitialize (const VDevice &dev)
	{
		for (auto& pool : _descriptorPools) {
			dev.vkDestroyDescriptorPool( dev.GetVkDevice(), pool, null );
		}
		_descriptorPools.clear();
	}
	
/*
=================================================
	AllocDescriptorSet
=================================================
*/
	bool  VDescriptorManager::AllocDescriptorSet (const VDevice &dev, VkDescriptorSetLayout layout, OUT VkDescriptorSet &ds)
	{
		VkDescriptorSetAllocateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorSetCount	= 1;
		info.pSetLayouts		= &layout;

		for (auto& pool : _descriptorPools)
		{
			info.descriptorPool = pool;
			
			if ( dev.vkAllocateDescriptorSets( dev.GetVkDevice(), &info, OUT &ds ) == VK_SUCCESS )
				return true;
		}

		CHECK_ERR( _CreateDescriptorPool( dev ));
		
		info.descriptorPool = _descriptorPools.back();
		VK_CHECK( dev.vkAllocateDescriptorSets( dev.GetVkDevice(), &info, OUT &ds ));

		return true;
	}

/*
=================================================
	_CreateDescriptorPool
=================================================
*/
	bool  VDescriptorManager::_CreateDescriptorPool (const VDevice &dev)
	{
		FixedArray< VkDescriptorPoolSize, 32 >	pool_sizes;

		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER,						MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		MaxDescriptorPoolSize * 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,				MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,				MaxDescriptorPoolSize });

		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,				MaxDescriptorPoolSize * 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,				MaxDescriptorPoolSize * 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,		MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,		MaxDescriptorPoolSize });
		
		if ( dev.HasDeviceExtension( VK_NV_RAY_TRACING_EXTENSION_NAME ) )
		{
			pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, MaxDescriptorPoolSize });
		}

		
		VkDescriptorPoolCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount	= uint(pool_sizes.size());
		info.pPoolSizes		= pool_sizes.data();
		info.maxSets		= MaxDescriptorSets;

		VkDescriptorPool			ds_pool;
		VK_CHECK( dev.vkCreateDescriptorPool( dev.GetVkDevice(), &info, null, OUT &ds_pool ));

		_descriptorPools.push_back( ds_pool );
		return true;
	}


}	// FG
