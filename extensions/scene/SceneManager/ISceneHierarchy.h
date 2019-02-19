// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This is abstraction of different scene hierarchy algorithms,
	for example: octree, quadtree, BVH, indoor portals and other.

	Scene hierarchy controls:
	- static & dynamic objects visibility (culling)
	- static objects streaming
	- static objects memory managment (images, buffers)
*/

#pragma once

#include "scene/Common.h"
#include "scene/Utils/Math/AABB.h"

namespace FG
{

	//
	// Scene Hierarchy interface
	//

	class ISceneHierarchy : public std::enable_shared_from_this<ISceneHierarchy>
	{
	// interface
	public:
		// build materials with new render technique
		virtual bool Build (const FGThreadPtr &, const RenderTechniquePtr &) = 0;

		// add all cameras (main, reflection, shadow, etc),
		// add self or other hierarchies if visible for camera
		virtual void PreDraw (const CameraInfo &, ScenePreRender &) const = 0;

		// add draw commands into render queue,
		// this method must be thread safe,
		// use only immutable data or protect mutable data for race condition.
		virtual void Draw (RenderQueue &) const = 0;

		// returns scene bounding volume, the result is valid only for current frame
		ND_ virtual AABB  CalculateBoundingVolume () const = 0;
		
		virtual void Destroy (const FGThreadPtr &) = 0;
	};


}	// FG
