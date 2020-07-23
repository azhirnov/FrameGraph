// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Stream/FileStream.h"

#if defined(FG_GRAPHVIZ_DOT_EXECUTABLE) && defined(FS_HAS_FILESYSTEM)

#include "framegraph/FG.h"

namespace FG
{

	//
	// GraphViz
	//

	struct GraphViz
	{
		static bool Visualize (StringView graph, const FS::path &filepath,
							   StringView format = "png", bool autoOpen = false, bool deleteOrigin = false);

		static bool Visualize (const FrameGraph &instance, const FS::path &filepath,
							   StringView format = "png", bool autoOpen = false, bool deleteOrigin = false);
	};


}	// FG

#endif	// FG_GRAPHVIZ_DOT_EXECUTABLE and FS_HAS_FILESYSTEM
