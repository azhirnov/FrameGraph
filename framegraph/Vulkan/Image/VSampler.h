// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/SamplerDesc.h"
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
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VSampler () {}
		VSampler (VSampler &&) = delete;
		VSampler (const VSampler &) = delete;
		VSampler (const VDevice &dev, const SamplerDesc &desc);
		~VSampler ();

		bool Create (const VDevice &dev, StringView dbgName);
		void Destroy (VResourceManager &);

		ND_ bool	operator == (const VSampler &rhs) const;

		ND_ VkSampler					Handle ()		const	{ SHAREDLOCK( _drCheck );  return _sampler; }
		ND_ HashVal						GetHash ()		const	{ SHAREDLOCK( _drCheck );  return _hash; }
		//ND_ VkSamplerCreateInfo const&	CreateInfo ()	const	{ SHAREDLOCK( _drCheck );  return _createInfo; }


	private:
		ND_ static HashVal  _CalcHash (const VkSamplerCreateInfo &ci);
	};
	

}	// FG


namespace std
{
	template <>
	struct hash< FG::VSampler > {
		ND_ size_t  operator () (const FG::VSampler &value) const {
			return size_t(value.GetHash());
		}
	};

}	// std
