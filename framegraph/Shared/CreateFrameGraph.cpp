// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraph.h"

namespace FG
{

/*
=================================================
	CreateFrameGraph
=================================================
*/
	FrameGraphPtr  FrameGraph::CreateFrameGraph (const DeviceInfo_t &ci)
	{
		FrameGraphPtr	result;

		CHECK_ERR( Visit( ci,
				[&result] (const VulkanDeviceInfo &vdi) -> bool
				{
					CHECK_ERR( vdi.instance and vdi.physicalDevice and vdi.device and not vdi.queues.empty() );
					CHECK_ERR( VulkanLoader::Initialize() );

					result = FrameGraphPtr{ new VFrameGraph( vdi )};
					return true;
				},

				[] (const auto &)
				{
					ASSERT(false);
					return false;
				}
			));

		return result;
	}


}	// FG
