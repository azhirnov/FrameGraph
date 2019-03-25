// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VDescriptorSetLayout.h"

namespace FG
{

	//
	// Vulkan Descriptor Manager
	//

	class VDescriptorManager final
	{
	// types
	private:
		static constexpr uint			MaxDescriptorPoolSize	= 1024;
		static constexpr uint			MaxDescriptorSets		= 512;

		struct DSPool
		{
			//SpinLock			guard;
			VkDescriptorPool	pool	= VK_NULL_HANDLE;
		};

		using DescriptorPoolArray_t		= FixedArray< DSPool, 16 >;
		using DescriptorSet				= VDescriptorSetLayout::DescriptorSet;


	// variables
	private:
		VDevice const&				_device;

		std::mutex					_guard;
		DescriptorPoolArray_t		_descriptorPools;


	// methods
	public:
		explicit VDescriptorManager (const VDevice &);
		~VDescriptorManager ();
		
		bool Initialize ();
		void Deinitialize ();

		bool AllocDescriptorSet (VkDescriptorSetLayout layout, OUT DescriptorSet &ds);
		bool DeallocDescriptorSet (const DescriptorSet &ds);
		bool DeallocDescriptorSets (ArrayView<DescriptorSet> ds);

	private:
		bool _CreateDescriptorPool ();
	};


}	// FG
