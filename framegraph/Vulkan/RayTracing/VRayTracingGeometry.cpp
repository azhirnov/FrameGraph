// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingGeometry.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VRayTracingGeometry::~VRayTracingGeometry ()
	{
		ASSERT( not _bottomLevelAS );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VRayTracingGeometry::Create (const VDevice &dev, const RayTracingGeometryDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
									  EQueueFamily queueFamily, StringView dbgName)
	{
		return false;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VRayTracingGeometry::Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t)
	{
	}


}	// FG
