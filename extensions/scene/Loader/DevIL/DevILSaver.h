// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_DEVIL

#include "scene/Loader/ImageSaver.h"

namespace FG
{

	//
	// DevIL Image Saver
	//

	class DevILSaver final : public IImageSaver
	{
	// methods
	public:
		bool  SaveImage (StringView filename, const IntermImagePtr &image) override;
	};


}	// FG

#endif	// FG_ENABLE_DEVIL
