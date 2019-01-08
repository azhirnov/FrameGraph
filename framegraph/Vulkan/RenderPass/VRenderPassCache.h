// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VRenderPass.h"

namespace FG
{

	//
	// Vulkan Render Pass Cache
	//

	class VRenderPassCache final
	{
	// types
	private:
		using FragmentOutput	= GraphicsPipelineDesc::FragmentOutput;

	// variables
	private:


	// methods
	public:
		VRenderPassCache ();
		~VRenderPassCache ();
		
		bool CreateRenderPasses (VResourceManagerThread &resMngr, ArrayView<VLogicalRenderPass*> logicalPasses,
								 ArrayView<FragmentOutput> fragOutput);

	private:
		ND_ RawFramebufferID  _CreateFramebuffer (VResourceManagerThread &resMngr, ArrayView<VLogicalRenderPass*> logicalPasses,
												  RawRenderPassID renderPassId);
	};


}	// FG
