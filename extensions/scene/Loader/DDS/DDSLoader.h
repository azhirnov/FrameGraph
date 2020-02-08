// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/IImageLoader.h"

namespace FG
{
	
	//
	// DDS Loader
	//

	class DDSLoader final : public IImageLoader
	{
	// methods
	public:
		DDSLoader () {}

		bool LoadImage (INOUT IntermImagePtr &image, ArrayView<StringView> directories, const ImageCachePtr &imgCache, bool flipY) override;
	};


}	// FG
