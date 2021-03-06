// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_DEVIL

#include "scene/Loader/IImageLoader.h"

namespace FG
{

	//
	// DevIL Loader
	//

	class DevILLoader final : public IImageLoader
	{
	// methods
	public:
		DevILLoader ();

		bool LoadImage (INOUT IntermImagePtr &image, ArrayView<StringView> directories, const ImageCachePtr &imgCache, bool flipY) override;
	};


}	// FG

#endif	// FG_ENABLE_DEVIL
