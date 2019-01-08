// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphInstance.h"

namespace FG
{

/*
=================================================
	CreateFrameGraph
=================================================
*/
	FGInstancePtr  FrameGraphInstance::CreateFrameGraph (const DeviceInfo_t &ci)
	{
		FGInstancePtr	result;

		CHECK_ERR( Visit( ci,
				[&result] (const VulkanDeviceInfo &vdi) -> bool
				{
					CHECK_ERR( vdi.instance and vdi.physicalDevice and vdi.device and not vdi.queues.empty() );
					CHECK_ERR( VulkanLoader::Initialize() );

					result = FGInstancePtr{ new VFrameGraphInstance( vdi )};
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
