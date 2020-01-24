// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraph.h"

namespace FG
{

/*
=================================================
	CreateFrameGraph
=================================================
*/
	FrameGraph  IFrameGraph::CreateFrameGraph (const DeviceInfo_t &ci)
	{
		FrameGraph	result = Visit( ci,
				[] (const VulkanDeviceInfo &vdi) -> FrameGraph
				{
					CHECK_ERR( vdi.instance and vdi.physicalDevice and vdi.device and not vdi.queues.empty() );
					CHECK_ERR( VulkanLoader::Initialize() );

					auto  fg = MakeShared<VFrameGraph>( vdi );
					CHECK_ERR( fg->Initialize() );

					return fg;
				},

				[] (const auto &) -> FrameGraph
				{
					ASSERT(false);
					return null;
				}
			);

		return result;
	}


}	// FG
