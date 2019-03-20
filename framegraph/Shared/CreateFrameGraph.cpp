// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
		FrameGraph	result;

		CHECK_ERR( Visit( ci,
				[&result] (const VulkanDeviceInfo &vdi) -> bool
				{
					CHECK_ERR( vdi.instance and vdi.physicalDevice and vdi.device and not vdi.queues.empty() );
					CHECK_ERR( VulkanLoader::Initialize() );

					result = MakeShared<VFrameGraph>( vdi );
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
