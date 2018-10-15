// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VLogicalImage.h"
#include "VFrameGraph.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VLogicalImage::VLogicalImage (const VFrameGraphWeak &fg)
	{}
	
/*
=================================================
	destructor
=================================================
*/
	VLogicalImage::~VLogicalImage ()
	{}
	
/*
=================================================
	_DestroyLogical
=================================================
*/
	void VLogicalImage::_DestroyLogical ()
	{
	}
	
/*
=================================================
	GetReal
=================================================
*/
	ImagePtr  VLogicalImage::GetReal (Task task, EResourceState state)
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
	void VLogicalImage::SetDebugName (StringView name)
	{
	}

}	// FG
