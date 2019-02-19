// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/ScenePreRender.h"
#include "scene/Renderer/RenderQueue.h"
#include "scene/Shaders/ShaderCache.h"

namespace FG
{

	//
	// Render Technique interface
	//

	class IRenderTechnique
	{
	// types
	public:
		using GraphicsPipelineInfo		= ShaderCache::GraphicsPipelineInfo;
		using RayTracingPipelineInfo	= ShaderCache::RayTracingPipelineInfo;
		using ComputePipelineInfo		= ShaderCache::ComputePipelineInfo;


	protected:
		class RenderQueueImpl final : public RenderQueue
		{
		public:
			RenderQueueImpl () {}
			
			void Create (const FGThreadPtr &fg, const CameraInfo &camera)					{ return _Create( fg, camera ); }

			ND_ Task  Submit (ArrayView<Task> dependsOn = Default)							{ return _Submit( dependsOn ); }

			void AddLayer (ERenderLayer layer, LogicalPassID passId,
						   const PipelineResources& pplnRes, StringView dbgName = Default)	{ return _AddLayer( layer, passId, &pplnRes, dbgName ); }

			void AddLayer (ERenderLayer layer, const RenderPassDesc &desc,
						   const PipelineResources& pplnRes, StringView dbgName = Default)	{ return _AddLayer( layer, desc, &pplnRes, dbgName ); }
		};

		using CameraData_t	= ScenePreRender::CameraData;


	// interface
	public:
		virtual void Destroy () = 0;

		virtual bool Render (const ScenePreRender &) = 0;

		virtual bool GetPipeline (ERenderLayer, INOUT GraphicsPipelineInfo &, OUT RawGPipelineID &) = 0;
		virtual bool GetPipeline (ERenderLayer, INOUT GraphicsPipelineInfo &, OUT RawMPipelineID &) = 0;
		virtual bool GetPipeline (ERenderLayer, INOUT RayTracingPipelineInfo &, OUT RawRTPipelineID &) = 0;
		virtual bool GetPipeline (ERenderLayer, INOUT ComputePipelineInfo &, OUT RawCPipelineID &) = 0;

		ND_ virtual Ptr<ShaderCache>	GetShaderBuilder () = 0;
		ND_ virtual FGInstancePtr		GetFrameGraphInstance () = 0;
		ND_ virtual FGThreadPtr			GetFrameGraphThread () = 0;


	protected:
		ND_ static ScenePreRender::CameraArray_t const&  _GetCameras (const ScenePreRender &preRender)		{ return preRender._cameras; }
	};


}	// FG
