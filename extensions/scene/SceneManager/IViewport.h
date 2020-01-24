// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/Enums.h"
#include "scene/Math/AABB.h"
#include "scene/Math/Sphere.h"

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

		virtual void AfterRender (const CommandBuffer &, Present &&) = 0;
	};


}	// FG
