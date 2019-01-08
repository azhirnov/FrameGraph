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
		using DescriptorPoolArray_t		= Array< VkDescriptorPool >;


	// variables
	private:
		DescriptorPoolArray_t		_descriptorPools;


	// methods
	public:
		VDescriptorManager ();
		~VDescriptorManager ();
		
		bool Initialize (const VDevice &dev);
		void Deinitialize (const VDevice &dev);

		bool AllocDescriptorSet (const VDevice &dev, VkDescriptorSetLayout layout, OUT VkDescriptorSet &ds);

	private:
		bool _CreateDescriptorPool (const VDevice &dev);
	};


}	// FG
