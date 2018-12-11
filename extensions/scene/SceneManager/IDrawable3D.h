// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/RenderQueue.h"
#include "scene/Utils/Math/Transform.h"

namespace FG
{

	//
	// 3D Drawable interface
	//

	class IDrawable3D : public std::enable_shared_from_this<IDrawable3D>
	{
	// interface
	public:
		virtual ~IDrawable3D () {}
		
		virtual bool Build (const IRenderTechnique *) = 0;
		virtual void Draw (const RenderQueue &, const Transform &) const = 0;
	};


}	// FG
