// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VSampler.h"
#include "VEnumCast.h"
#include "VDevice.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VSampler::~VSampler ()
	{
		ASSERT( _sampler == VK_NULL_HANDLE );
	}
	
/*
=================================================
	constructor
=================================================
*/
	VSampler::VSampler (const VDevice &dev, const SamplerDesc &desc)
	{
		EXLOCK( _rcCheck );

		_createInfo = {};
		_createInfo.sType					= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		_createInfo.flags					= 0;
		_createInfo.magFilter				= VEnumCast( desc.magFilter );
		_createInfo.minFilter				= VEnumCast( desc.minFilter );
		_createInfo.mipmapMode				= VEnumCast( desc.mipmapMode );
		_createInfo.addressModeU			= VEnumCast( desc.addressMode.x );
		_createInfo.addressModeV			= VEnumCast( desc.addressMode.y );
		_createInfo.addressModeW			= VEnumCast( desc.addressMode.z );
		_createInfo.mipLodBias				= desc.mipLodBias;
		_createInfo.anisotropyEnable		= desc.maxAnisotropy.has_value() ? VK_TRUE : VK_FALSE;
		_createInfo.maxAnisotropy			= desc.maxAnisotropy.value_or( 0.0f );
		_createInfo.compareEnable			= desc.compareOp.has_value() ? VK_TRUE : VK_FALSE;
		_createInfo.compareOp				= VEnumCast( desc.compareOp.value_or( ECompareOp::Always ));
		_createInfo.minLod					= desc.minLod;
		_createInfo.maxLod					= desc.maxLod;
		_createInfo.borderColor				= VEnumCast( desc.borderColor );
		_createInfo.unnormalizedCoordinates	= desc.unnormalizedCoordinates ? VK_TRUE : VK_FALSE;


		// validate
		const VkPhysicalDeviceLimits&	limits	= dev.GetDeviceProperties().limits;
		const VkPhysicalDeviceFeatures&	feat	= dev.GetDeviceFeatures();

		if ( _createInfo.unnormalizedCoordinates )
		{
			ASSERT( _createInfo.minFilter == _createInfo.magFilter );
			ASSERT( _createInfo.mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST );
			ASSERT( _createInfo.minLod == 0.0f and _createInfo.maxLod == 0.0f );
			ASSERT( _createInfo.addressModeU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE or
					_createInfo.addressModeU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER );
			ASSERT( _createInfo.addressModeV == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE or
					_createInfo.addressModeV == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER );

			_createInfo.magFilter			= _createInfo.minFilter;
			_createInfo.mipmapMode			= VK_SAMPLER_MIPMAP_MODE_NEAREST;
			_createInfo.minLod				= _createInfo.maxLod = 0.0f;
			_createInfo.anisotropyEnable	= VK_FALSE;
			_createInfo.compareEnable		= VK_FALSE;

			if ( _createInfo.addressModeU != VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE and
				 _createInfo.addressModeU != VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER )
			{
				ASSERT( false );
				_createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}

			if ( _createInfo.addressModeV != VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE and
				 _createInfo.addressModeV != VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER )
			{
				ASSERT( false );
				_createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}
		}

		if ( _createInfo.mipLodBias > limits.maxSamplerLodBias )
		{
			ASSERT( _createInfo.mipLodBias <= limits.maxSamplerLodBias );
			_createInfo.mipLodBias = limits.maxSamplerLodBias;
		}

		if ( _createInfo.maxLod < _createInfo.minLod )
		{
			ASSERT( _createInfo.maxLod >= _createInfo.minLod );
			_createInfo.maxLod = _createInfo.minLod;
		}

		if ( not feat.samplerAnisotropy )
		{
			ASSERT( not _createInfo.anisotropyEnable );
			_createInfo.anisotropyEnable = VK_FALSE;
		}

		if ( _createInfo.minFilter == VK_FILTER_CUBIC_IMG or _createInfo.magFilter == VK_FILTER_CUBIC_IMG )
		{
			ASSERT( not _createInfo.anisotropyEnable );
			_createInfo.anisotropyEnable = VK_FALSE;
		}

		if ( not _createInfo.anisotropyEnable )
			_createInfo.maxAnisotropy = 0.0f;
		else
			_createInfo.maxAnisotropy = Clamp( _createInfo.maxAnisotropy, 1.0f, limits.maxSamplerAnisotropy );

		if ( not _createInfo.compareEnable )
			_createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		if ( _createInfo.addressModeU != VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER and
			 _createInfo.addressModeV != VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER and
			 _createInfo.addressModeW != VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER )
		{
			// reset border color, because it is unused
			_createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		}
		
		if ( not dev.IsSamplerMirrorClampEnabled() )
		{
			if ( _createInfo.addressModeU == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE )
			{
				ASSERT( false );
				_createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}

			if ( _createInfo.addressModeV == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE )
			{
				ASSERT( false );
				_createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}

			if ( _createInfo.addressModeW == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE )
			{
				ASSERT( false );
				_createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}
		}


		// calculate hash
		_hash = _CalcHash( _createInfo );
	}
	
/*
=================================================
	_CalcHash
=================================================
*/
	HashVal  VSampler::_CalcHash (const VkSamplerCreateInfo &ci)
	{
		HashVal	result;

		// ignore 'ci.sType'
		// ignore 'ci.pNext'
		result << HashOf( ci.flags );
		result << HashOf( ci.magFilter );
		result << HashOf( ci.minFilter );
		result << HashOf( ci.mipmapMode );
		result << HashOf( ci.addressModeU );
		result << HashOf( ci.addressModeV );
		result << HashOf( ci.addressModeW );
		result << HashOf( ci.mipLodBias );
		result << HashOf( ci.anisotropyEnable );
		result << HashOf( ci.compareEnable );
		result << HashOf( ci.minLod );
		result << HashOf( ci.maxLod );
		result << HashOf( ci.borderColor );
		result << HashOf( ci.unnormalizedCoordinates );

		if ( ci.anisotropyEnable )
			result << HashOf( ci.maxAnisotropy );

		if ( ci.compareEnable )
			result << HashOf( ci.compareOp );

		return result;
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VSampler::Create (const VDevice &dev, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _sampler == VK_NULL_HANDLE );

		VK_CHECK( dev.vkCreateSampler( dev.GetVkDevice(), &_createInfo, null, OUT &_sampler ));
		
		_debugName = dbgName;

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VSampler::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t)
	{
		EXLOCK( _rcCheck );

		if ( _sampler ) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_SAMPLER, uint64_t(_sampler) );
		}

		_sampler	= VK_NULL_HANDLE;
		_hash		= Default;
		_createInfo	= Default;
		_debugName.clear();
	}
	
/*
=================================================
	operator ==
=================================================
*/
	bool VSampler::operator == (const VSampler &rhs) const
	{
		SHAREDLOCK( _rcCheck );
		SHAREDLOCK( rhs._rcCheck );

		if ( _hash != rhs._hash )
			return false;
		
		const auto&	lci = _createInfo;
		const auto&	rci = rhs._createInfo;

		return	lci.flags				==	rci.flags				and
				lci.magFilter			==	rci.magFilter			and
				lci.minFilter			==	rci.minFilter			and
				lci.mipmapMode			==	rci.mipmapMode			and
				lci.addressModeU		==	rci.addressModeU		and
				lci.addressModeV		==	rci.addressModeV		and
				lci.addressModeW		==	rci.addressModeW		and
				Equals( lci.mipLodBias,		rci.mipLodBias )		and

				lci.anisotropyEnable	==	rci.anisotropyEnable	and
				(not lci.anisotropyEnable	or
				 Equals( lci.maxAnisotropy,	rci.maxAnisotropy))		and

				lci.compareEnable		==	rci.compareEnable		and
				(not lci.compareEnable		or
				 lci.compareOp			==	rci.compareOp)			and

				Equals( lci.minLod,			rci.minLod )			and
				Equals( lci.maxLod,			rci.maxLod )			and
				lci.borderColor			==	rci.borderColor			and
				lci.unnormalizedCoordinates	==	rci.unnormalizedCoordinates;
	}


}	// FG
