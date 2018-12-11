// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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

		bool Load (const IntermediateImagePtr &image, ArrayView<StringView> directories);
		bool Load (const IntermediateMaterialPtr &material, ArrayView<StringView> directories);
		bool Load (ArrayView<IntermediateMaterialPtr> materials, ArrayView<StringView> directories);
		bool Load (const IntermediateScenePtr &scene, ArrayView<StringView> directories);
	};


}	// FG

#endif	// FG_ENABLE_DEVIL
