// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_DEVIL

#include "scene/Loader/Intermediate/IntermediateScene.h"

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

		bool Load (INOUT IntermediateImagePtr &image, ArrayView<StringView> directories, const ImageCachePtr &imgCache = null);

		bool Load (const IntermediateMaterialPtr &material, ArrayView<StringView> directories, const ImageCachePtr &imgCache = null);
		bool Load (ArrayView<IntermediateMaterialPtr> materials, ArrayView<StringView> directories, const ImageCachePtr &imgCache = null);
		bool Load (const IntermediateScenePtr &scene, ArrayView<StringView> directories, const ImageCachePtr &imgCache = null);
	};


}	// FG

#endif	// FG_ENABLE_DEVIL
