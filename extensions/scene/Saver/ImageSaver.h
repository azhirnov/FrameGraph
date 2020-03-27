// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Common.h"

namespace FG
{

	//
	// Image Saver interface
	//

	class IImageSaver
	{
	// methods
	public:
		virtual bool  SaveImage (StringView filename, const IntermImagePtr &image) = 0;
	};


}	// FG
