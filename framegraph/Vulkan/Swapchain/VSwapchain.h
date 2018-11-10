// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Swapchain interface
	//

	class VSwapchain
	{
	// interface
	public:
			virtual ~VSwapchain () {}

			virtual bool Acquire (VFrameGraphThread &) = 0;
			virtual bool Present (VFrameGraphThread &) = 0;

			virtual bool Initialize (VkQueue) = 0;
			virtual void Deinitialize () = 0;

		ND_ virtual bool IsCompatibleWithQueue (EQueueFamily familyIndex) const = 0;

		ND_ virtual RawImageID  GetImage (ESwapchainImage type) = 0;
	};


}	// FG
