// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/SamplerDesc.h"
#include "framegraph/Shared/ResourceBase.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Sampler immutable data
	//

	class VSampler final : public ResourceBase
	{
	// variables
	private:
		VkSampler				_sampler	= VK_NULL_HANDLE;
		HashVal					_hash;
		VkSamplerCreateInfo		_createInfo	= {};


	// methods
	public:
		VSampler () {}
		~VSampler ();

		void Initialize (const VDevice &dev, const SamplerDesc &desc);
		bool Create (const VDevice &dev);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		void Replace (INOUT VSampler &&other);

		ND_ bool	operator == (const VSampler &rhs) const;

		ND_ VkSampler					Handle ()		const	{ return _sampler; }
		ND_ HashVal						GetHash ()		const	{ return _hash; }
		//ND_ VkSamplerCreateInfo const&	CreateInfo ()	const	{ return _createInfo; }


	private:
		ND_ static HashVal  _CalcHash (const VkSamplerCreateInfo &ci);
	};
	

}	// FG


namespace std
{
	template <>
	struct hash< FG::VSampler > {
		ND_ size_t  operator () (const FG::VSampler &value) const noexcept {
			return size_t(value.GetHash());
		}
	};

}	// std
