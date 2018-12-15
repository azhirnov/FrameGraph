// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

#if defined(FG_GRAPHVIZ_DOT_EXECUTABLE) && defined(FG_STD_FILESYSTEM)
#include <filesystem>

#include "framegraph/VFG.h"

namespace FG
{

	//
	// GraphViz
	//

	struct GraphViz
	{
		static bool Visualize (StringView graph, const std::filesystem::path &filepath,
							   StringView format = "png", bool autoOpen = false, bool deleteOrigin = false);

		static bool Visualize (const FrameGraphPtr &instance, const std::filesystem::path &filepath,
							   StringView format = "png", bool autoOpen = false, bool deleteOrigin = false);
	};


}	// FG

#endif	// FG_GRAPHVIZ_DOT_EXECUTABLE and FG_STD_FILESYSTEM
