// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/SamplerDesc.h"
#include "framegraph/Shared/ResourceBase.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Sampler immutable data
	//

	class VSampler final
	{
	// variables
	private:
		VkSampler				_sampler	= VK_NULL_HANDLE;
		HashVal					_hash;
		VkSamplerCreateInfo		_createInfo	= {};
		
		DebugName_t				_debugName;
		
		RWRaceConditionCheck	_rcCheck;


	// methods
	public:
		VSampler () {}
		VSampler (VSampler &&) = default;
		VSampler (const VDevice &dev, const SamplerDesc &desc);
		~VSampler ();

		bool Create (const VDevice &dev, StringView dbgName);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		ND_ bool	operator == (const VSampler &rhs) const;

		ND_ VkSampler					Handle ()		const	{ SHAREDLOCK( _rcCheck );  return _sampler; }
		ND_ HashVal						GetHash ()		const	{ SHAREDLOCK( _rcCheck );  return _hash; }
		//ND_ VkSamplerCreateInfo const&	CreateInfo ()	const	{ SHAREDLOCK( _rcCheck );  return _createInfo; }


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
