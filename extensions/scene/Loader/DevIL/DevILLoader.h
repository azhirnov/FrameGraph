// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_DEVIL

#include "scene/Loader/Intermediate/IntermScene.h"

namespace FG
{

	//
	// DevIL Loader
	//

	class DevILLoader
	{
	// methods
	public:
		DevILLoader ();

		bool Load (INOUT IntermImagePtr &image, ArrayView<StringView> directories, const ImageCachePtr &imgCache = null);

		bool Load (const IntermMaterialPtr &material, ArrayView<StringView> directories, const ImageCachePtr &imgCache = null);
		bool Load (ArrayView<IntermMaterialPtr> materials, ArrayView<StringView> directories, const ImageCachePtr &imgCache = null);
		bool Load (const IntermScenePtr &scene, ArrayView<StringView> directories, const ImageCachePtr &imgCache = null);
	};


}	// FG

#endif	// FG_ENABLE_DEVIL
