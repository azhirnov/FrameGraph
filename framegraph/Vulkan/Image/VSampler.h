// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/SamplerResource.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Sampler
	//

	class VSampler final : public SamplerResource
	{
		friend class VSamplerCache;

	// variables
	private:
		VkSampler			_samplerId;
		HashVal				_hash;
		VkSamplerCreateInfo	_createInfo;


	// methods
	private:
		bool _Setup (const SamplerDesc &desc, const VDevice &dev);

		bool _Create (const VDevice &dev);
		void _Destroy (const VDevice &dev);

		ND_ static HashVal  _CalcHash (const VkSamplerCreateInfo &ci);


	public:
		VSampler ();
		~VSampler ();

		ND_ bool	operator == (const VSampler &rhs) const;

		ND_ VkSampler					GetSamplerID ()		const	{ return _samplerId; }
		ND_ VkSamplerCreateInfo const&	GetCreateInfo ()	const	{ return _createInfo; }
	};


}	// FG
