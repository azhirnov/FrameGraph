// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_ASSIMP

#include "scene/Loader/Intermediate/IntermScene.h"

namespace Assimp {
	class Importer;
}

namespace FG
{

	//
	// Assimp Scene Loader
	//

	class AssimpLoader final
	{
	// types
	public:
		struct Config
		{
			uint	maxTrianglesPerMesh	= UMax;
			uint	maxVerticesPerMesh	= UMax;

			bool	calculateTBN		= false;
			bool	smoothNormals		= false;
			bool	splitLargeMeshes	= false;
			bool	optimize			= false;
		};

		using AssimpImporter_t = UniquePtr< Assimp::Importer >;


	// variables
	private:
		AssimpImporter_t			_importerPtr;


	// methods
	public:
		AssimpLoader ();
		~AssimpLoader ();

		ND_ IntermScenePtr  Load (const Config &config, StringView filename);
	};


}	// FG

#endif	// FG_ENABLE_ASSIMP
