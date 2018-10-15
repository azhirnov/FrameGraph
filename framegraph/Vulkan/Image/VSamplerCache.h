// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VSampler.h"

namespace FG
{

	//
	// Vulkan Sampler Cache
	//

	class VSamplerCache final
	{
	// types
	private:
		struct SamplerItem
		{
			VSamplerPtr		ptr;

			SamplerItem () {}
			explicit SamplerItem (const VSamplerPtr &p) : ptr{p} {}
			
			ND_ bool  operator == (const SamplerItem &rhs) const	{ return *ptr == *rhs.ptr; }
		};

		struct SamplerHash {
			ND_ size_t  operator () (const SamplerItem &value) const noexcept	{ return size_t(value.ptr->_hash); }
		};

		using SamplerMap_t	= HashSet< SamplerItem, SamplerHash >;


	// variables
	private:
		VDevice const&		_device;
		SamplerMap_t		_samplerCache;


	// methods
	public:
		VSamplerCache (const VDevice &dev);
		~VSamplerCache ();

		ND_ VSamplerPtr  CreateSampler (const SamplerDesc &desc);

	private:
		void _DestroyAll ();
	};


}	// FG
