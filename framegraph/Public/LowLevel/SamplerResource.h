// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/SamplerEnums.h"
#include "framegraph/Public/LowLevel/RenderStateEnums.h"

namespace FG
{

	//
	// Sampler Description
	//

	struct SamplerDesc
	{
	// types
		using Self			= SamplerDesc;
		using AddressMode3	= Vec< EAddressMode, 3 >;


	// variables
		EFilter					magFilter				= EFilter::Nearest;
		EFilter					minFilter				= EFilter::Nearest;
		EMipmapFilter			mipmapMode				= EMipmapFilter::Nearest;
		AddressMode3			addressMode				= { EAddressMode::Repeat, EAddressMode::Repeat, EAddressMode::Repeat };
		float					mipLodBias				= 0.0f;
		Optional<float>			maxAnisotropy;
		Optional<ECompareOp>	compareOp;
		float					minLod					= 0.0f;
		float					maxLod					= 0.0f;
		EBorderColor			borderColor				= EBorderColor::FloatTransparentBlack;
		bool					unnormalizedCoordinates	= false;


	// methods
		SamplerDesc () {}

		Self&  SetFilter (EFilter mag, EFilter min, EMipmapFilter mipmap)		{ magFilter = mag;  minFilter = min;  mipmapMode = mipmap;  return *this; }
		Self&  SetAddressMode (EAddressMode uvw)								{ addressMode = {uvw, uvw, uvw};  return *this; }
		Self&  SetAddressMode (EAddressMode u, EAddressMode v, EAddressMode w)	{ addressMode = {u, v, w};  return *this; }
		Self&  SetMipLodBias (float value)										{ mipLodBias = value;  return *this; }
		Self&  SetLodRange (float min, float max)								{ minLod = min;  maxLod = max;  return *this; }
		Self&  SetAnisotropy (float value)										{ maxAnisotropy = value;  return *this; }
		Self&  SetCompareOp (ECompareOp value)									{ compareOp = value;  return *this; }
		Self&  SetBorderColor (EBorderColor value)								{ borderColor = value;  return *this; }
		Self&  SetNormCoordinates (bool value)									{ unnormalizedCoordinates = not value;  return *this; }
	};



	//
	// Sampler Resource
	//

	class SamplerResource : public std::enable_shared_from_this<SamplerResource>
	{
	// methods
	public:
		virtual ~SamplerResource () {}
	};


}	// FG
