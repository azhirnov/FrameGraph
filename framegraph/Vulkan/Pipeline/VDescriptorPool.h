// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VDescriptorSetLayout.h"

namespace FG
{

	//
	// Descriptor Pool interface
	//

	class IDescriptorPool : public std::enable_shared_from_this<IDescriptorPool>
	{
	// interface
	public:
		virtual void FreeDescriptorSet (const VDescriptorSetLayoutPtr &layout, VkDescriptorSet ds) = 0;
	};



	//
	// Vulkan Descriptor Pool
	//

	class VDescriptorPool final : public IDescriptorPool
	{
	// types
	private:
		using PoolSizeArray_t	= StaticArray< VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE >;


	// variables
	private:
		VkDescriptorPool		_poolId;
		PoolSizeArray_t			_availablePoolSizes;
		uint					_availableSets;

		VFrameGraphWeak			_frameGraph;

		DEBUG_ONLY(
			mutable PoolSizeArray_t		_releasedPoolSizes;
			mutable uint				_releasedSets;
		)


	// methods
	private:
		explicit VDescriptorPool (const VFrameGraphWeak &fg);
		VDescriptorPool (const VFrameGraphWeak &fg, const VDescriptorSetLayoutPtr &layout);
		~VDescriptorPool ();

		void _InitializePoolSizes (uint count);
		void _InitializePoolSizes (const VDescriptorSetLayoutPtr &layout, uint count);

		bool _Create ();
		void _Destroy ();

		bool Allocate (const VDevice &dev, const VDescriptorSetLayoutPtr &layout, OUT VkDescriptorSet &descriptorSetId);


	public:
		ND_ bool CanAllocate (ArrayView<VkDescriptorPoolSize> poolSize) const;
		ND_ bool CanAllocate (const VDescriptorSetLayoutPtr &layout) const;
			
			void FreeDescriptorSet (const VDescriptorSetLayoutPtr &layout, VkDescriptorSet ds) override;

		ND_ VkDescriptorPool	GetPoolID ()	const	{ return _poolId; }
	};
	


	//
	// Vulkan Reusable Descriptor Pool
	//

	class VReusableDescriptorPool final : public IDescriptorPool
	{
		friend class VDescriptorSetLayout;

	// types
	private:
		using PoolSizeArray_t	= FixedArray< VkDescriptorPoolSize, VK_DESCRIPTOR_TYPE_RANGE_SIZE >;
		using DescriptorSets_t	= Array< VkDescriptorSet >;


	// variables
	private:
		VkDescriptorPool			_poolId;
		uint						_availableSetCount;
		VDescriptorSetLayoutWeak	_layout;
		DescriptorSets_t			_releasedSets;


	// methods
	private:
		void _InitializePoolSizes (const VDescriptorSetLayoutPtr &layout, uint count);

		bool _Create ();
		void _Destroy (const VDevice &dev);

		bool _Allocate (const VDevice &dev, OUT VkDescriptorSet &descriptorSetId);
		bool _CanAllocate () const;


	public:
		VReusableDescriptorPool (const VDevice &dev, const VDescriptorSetLayoutPtr &layout, uint count);
		~VReusableDescriptorPool ();
			
		void FreeDescriptorSet (const VDescriptorSetLayoutPtr &layout, VkDescriptorSet ds) override;
	};


}	// FG
