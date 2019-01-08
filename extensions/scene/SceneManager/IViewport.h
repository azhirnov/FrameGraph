// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/Enums.h"
#include "scene/Utils/Math/AABB.h"
#include "scene/Utils/Math/Sphere.h"

namespace FG
{

	//
	// Viewport interface
	//

	class IViewport : public std::enable_shared_from_this<IViewport>
	{
	// interface
	public:
		// add cameras
		virtual void Prepare (ScenePreRender &) = 0;

		// draw UI or something else,
		// must be thread safe
		virtual void Draw (RenderQueue &) const = 0;

		virtual void AfterRender (const FGThreadPtr &, const Present &) = 0;
	};


}	// FG
