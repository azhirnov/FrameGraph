// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/Public/BufferDesc.h"

namespace FG
{
	
/*
=================================================
	Validate
=================================================
*/
	void BufferViewDesc::Validate (const BufferDesc &desc)
	{
		ASSERT( offset < desc.size );
		ASSERT( format != Default );
	
		offset	= Min( offset, desc.size-1 );
		size	= Min( size, desc.size - offset );
	}
	
/*
=================================================
	operator ==
=================================================
*/
	bool  BufferViewDesc::operator == (const BufferViewDesc &rhs) const
	{
		return	(format	== rhs.format)	&
				(offset	== rhs.offset)	&
				(size	== rhs.size);
	}

}	// FG
