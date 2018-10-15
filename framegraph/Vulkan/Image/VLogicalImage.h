// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VImage.h"

namespace FG
{

	//
	// Vulkan Logical Image
	//

	class VLogicalImage final : public ImageResource
	{
		friend class VFrameGraph;

	// types
	private:
		

	// variables
	protected:


	// methods
	protected:
		explicit VLogicalImage (const VFrameGraphWeak &fg);
		~VLogicalImage ();

	private:
		void _DestroyLogical ();
		
		void SetDebugName (StringView name) FG_OVERRIDE;

		ND_ ImagePtr	GetReal (Task task, EResourceState state) FG_OVERRIDE;
	};


}	// FG
