// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VLogicalBuffer.h"
#include "VFrameGraph.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VLogicalBuffer::VLogicalBuffer (const VFrameGraphWeak &fg)
	{}
	
/*
=================================================
	destructor
=================================================
*/
	VLogicalBuffer::~VLogicalBuffer ()
	{}
	
/*
=================================================
	_DestroyLogical
=================================================
*/
	void VLogicalBuffer::_DestroyLogical ()
	{
	}

/*
=================================================
	GetReal
=================================================
*/
	BufferPtr  VLogicalBuffer::GetReal (Task task, EResourceState state)
	{
		// TODO
		ASSERT(false);
		return null;
	}

/*
=================================================
	SetDebugName
=================================================
*/
	void VLogicalBuffer::SetDebugName (StringView name)
	{
	}

}	// FG
