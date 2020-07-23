// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/ImageSaver.h"

namespace FG
{

	//
	// DDS Image Saver
	//

	class DDSSaver final : public IImageSaver
	{
	// methods
	public:
		bool  SaveImage (StringView filename, const IntermImagePtr &image) override;
	};


}	// FG
