// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Saver/ImageSaver.h"

namespace FG
{

	//
	// Image Saver interface
	//

	class DDSSaver final : public IImageSaver
	{
	// methods
	public:
		bool  SaveImage (StringView filename, const IntermImagePtr &image) override;
	};


}	// FG
