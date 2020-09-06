// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/Public/FrameGraph.h"

#ifdef FG_ENABLE_VULKAN
#	include "framegraph/Vulkan/Instance/VFrameGraph.h"
#endif

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

			#ifdef FG_ENABLE_VULKAN
				[] (const VulkanDeviceInfo &vdi) -> FrameGraph
				{
					CHECK_ERR( vdi.instance and vdi.physicalDevice and vdi.device and not vdi.queues.empty() );
					CHECK_ERR( VulkanLoader::Initialize() );

					auto  fg = MakeShared<VFrameGraph>( vdi );
					CHECK_ERR( fg->Initialize() );

					return fg;
				},
			#endif

				[] (const auto &) -> FrameGraph
				{
					ASSERT(false);
					return null;
				}
			);

		return result;
	}
	
/*
=================================================
	GetVersion
=================================================
*/
	const char*  IFrameGraph::GetVersion ()
	{
		static constexpr char version[] = "FrameGraph v" FG_VERSION_STR " (" FG_COMMIT_HASH ")";
		return version;
	}


}	// FG
