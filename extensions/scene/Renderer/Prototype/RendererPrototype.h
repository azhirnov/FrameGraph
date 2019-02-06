// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/ThreadPool/ThreadPool.h"
#include "scene/Renderer/IRenderTechnique.h"

namespace FG
{

	//
	// Renderer Prototype
	//

	class RendererPrototype final : public IRenderTechnique
	{
	// types
	private:
		using SourceID			= ShaderBuilder::ShaderSourceID;
		using ShaderOutputs_t	= StaticArray< PipelineResources, uint(ERenderLayer::_Count) >;

		template <typename T>
		using TPipelineCache	= HashMap< PipelineInfo, T, PipelineInfoHash >;
		using GPipelineCache_t	= TPipelineCache< GPipelineID >;
		using MPipelineCache_t	= TPipelineCache< MPipelineID >;
		using RTPipelineCache_t	= TPipelineCache< RTPipelineID >;

		struct CameraUB
		{
			mat4x4		viewProj;
			vec4		position;
			vec3		frustumRayLeftBottom;
			float		_padding1;
			vec3		frustumRayRightBottom;
			float		_padding2;
			vec3		frustumRayLeftTop;
			float		_padding3;
			vec3		frustumRayRightTop;
			float		_padding4;
			vec2		clipPlanes;
		};


	// variables
	private:
		FGInstancePtr		_fgInstance;
		FGThreadPtr			_frameGraph;

		SubmissionGraph		_submissionGraph;
		ShaderBuilder		_shaderBuilder;

		SourceID			_graphicsShaderSource		= Default;
		SourceID			_rayTracingShaderSource		= Default;

		GPipelineCache_t	_gpipelineCache;
		MPipelineCache_t	_mpipelineCache;
		RTPipelineCache_t	_rtpipelineCache;

		PipelineResources	_perPassResources;
		ShaderOutputs_t		_shaderOutputResources;		// for compute / ray-tracing
		BufferID			_cameraUB;


	// methods
	public:
		RendererPrototype ();
		~RendererPrototype ();

		bool Initialize (const FGInstancePtr &, const FGThreadPtr &, StringView);
		void Deinitialize ();

		bool GetPipeline (const PipelineInfo &, OUT RawGPipelineID &) override;
		bool GetPipeline (const PipelineInfo &, OUT RawMPipelineID &) override;
		bool GetPipeline (const PipelineInfo &, OUT RawRTPipelineID &) override;

		bool Render (const ScenePreRender &) override;
		
		ShaderBuilder*  GetShaderBuilder ()			override	{ return &_shaderBuilder; }
		
		FGInstancePtr	GetFrameGraphInstance ()	override	{ return _fgInstance; }
		FGThreadPtr		GetFrameGraphThread ()		override	{ return _frameGraph; }


	private:
		bool _SetupShadowPass (const CameraData_t &, INOUT RenderQueueImpl &, OUT ImageID &);
		bool _SetupColorPass (const CameraData_t &, INOUT RenderQueueImpl &, OUT ImageID &);
		bool _SetupRayTracingPass (const CameraData_t &, INOUT RenderQueueImpl &, OUT ImageID &);

		bool _CreateUniformBuffer ();
		void _UpdateUniformBuffer (ERenderLayer firstLayer, const CameraData_t &cameraData, INOUT RenderQueueImpl &queue);
	};


}	// FG
