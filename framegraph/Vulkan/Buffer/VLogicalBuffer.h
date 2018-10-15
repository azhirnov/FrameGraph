// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VBuffer.h"

namespace FG
{

	//
	// Vulkan Logical Buffer
	//

	class VLogicalBuffer final : public BufferResource
	{
		friend class VFrameGraph;

	// types
	private:


	// variables
	protected:


	// methods
	protected:
		explicit VLogicalBuffer (const VFrameGraphWeak &fg);
		~VLogicalBuffer ();

	private:
		void _DestroyLogical ();
		
		void SetDebugName (StringView name) FG_OVERRIDE;

		ND_ BufferPtr	GetReal (Task task, EResourceState state) FG_OVERRIDE;
	};


}	// FG
