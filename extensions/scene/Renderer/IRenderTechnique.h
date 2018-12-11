// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/ScenePreRender.h"
#include "scene/Renderer/RenderQueue.h"

namespace FG
{

	//
	// Render Technique interface
	//

	class IRenderTechnique
	{
	// types
	public:
		struct PipelineInfo
		{
			ETextureType	textures	= Default;
			EMaterialType	flags		= Default;
			ERenderLayer	layer		= Default;
			uint			detailLevel	= 0;
		};

	protected:
		class RenderQueueImpl final : public RenderQueue
		{
		public:
			RenderQueueImpl () {}
			
			void Create (const FGThreadPtr &fg, const CameraInfo &camera)									{ return _Create( fg, camera ); }
			void Submit (ArrayView<Task> dependsOn = Default)												{ return _Submit( dependsOn ); }

			void AddLayer (ERenderLayer layer, LogicalPassID passId, StringView dbgName = Default)			{ return _AddLayer( layer, passId, dbgName ); }
			void AddLayer (ERenderLayer layer, const RenderPassDesc &desc, StringView dbgName = Default)	{ return _AddLayer( layer, desc, dbgName ); }
		};

		using CameraData_t	= ScenePreRender::CameraData;


	// interface
	public:
		virtual bool GetPipeline (const PipelineInfo &, OUT GPipelineID &) = 0;
		virtual bool GetPipeline (const PipelineInfo &, OUT MPipelineID &) = 0;

		virtual bool Render (const ScenePreRender &) = 0;


	protected:
		ND_ static ScenePreRender::CameraArray_t const&  _GetCameras (const ScenePreRender &preRender)		{ return preRender._cameras; }
	};


}	// FG
