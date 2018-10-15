// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VSamplerCache.h"
#include "VDevice.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VSamplerCache::VSamplerCache (const VDevice &dev) :
		_device{ dev }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VSamplerCache::~VSamplerCache ()
	{
		_DestroyAll();
	}
	
/*
=================================================
	_DestroyAll
=================================================
*/
	void VSamplerCache::_DestroyAll ()
	{
		for (auto& samp : _samplerCache)
		{
			samp.ptr->_Destroy( _device );
		}

		_samplerCache.clear();
	}

/*
=================================================
	CreateSampler
=================================================
*/
	VSamplerPtr  VSamplerCache::CreateSampler (const SamplerDesc &desc)
	{
		VSamplerPtr	samp = MakeShared<VSampler>();

		CHECK_ERR( samp->_Setup( desc, _device ));

		auto	iter = _samplerCache.find( SamplerItem(samp) );

		if ( iter != _samplerCache.end() )
			return iter->ptr;

		CHECK_ERR( samp->_Create( _device ));

		_samplerCache.insert( SamplerItem(samp) );
		return samp;
	}

}	// FG
