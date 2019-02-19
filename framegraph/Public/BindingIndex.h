// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	//
	// Binding Index
	//
	struct BindingIndex
	{
		friend struct std::hash< BindingIndex >;
		using Index_t = uint16_t;

	// variables
	private:
		Index_t		_index1		= UMax;	// resource dependend index, may be optimized to minimize resource switches between pipelines, used in OpenGL, DirectX
		Index_t		_index2		= UMax;	// resource unique index in current pipeline, used in Vulkan, OpenCL, software renderer

	// methods
	public:
		BindingIndex () {}

		explicit BindingIndex (uint perResourceIndex, uint uniqueIndex) : _index1{Index_t(perResourceIndex)}, _index2{Index_t(uniqueIndex)} {}

		ND_ bool		operator == (const BindingIndex &rhs) const		{ return _index1 == rhs._index1 and _index2 == rhs._index2; }
		ND_ bool		operator != (const BindingIndex &rhs) const		{ return not (*this == rhs); }

		ND_ Index_t		GLBinding ()	const	{ return _index1; }
		ND_ Index_t		DXBinding ()	const	{ return _index1; }
		ND_ Index_t		VKBinding ()	const	{ return _index2; }
		ND_ Index_t		CLBinding ()	const	{ return _index2; }
		ND_ Index_t		SWBinding ()	const	{ return _index2; }
		ND_ Index_t		Unique ()		const	{ return _index2; }
	};


}	// FG


namespace std
{
	template <>
	struct hash< FG::BindingIndex >
	{
		ND_ size_t  operator () (const FG::BindingIndex &value) const noexcept
		{
		#if FG_FAST_HASH
			return size_t(FG::HashOf( this, sizeof(this) ));
		#else
			return size_t(FG::HashOf( value._index1 ) + FG::HashOf( value._index2 ));
		#endif
		}
	};

}	// std
