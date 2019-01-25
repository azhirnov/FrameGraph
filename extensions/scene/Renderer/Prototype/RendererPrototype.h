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
		using PipelineCache_t	= HashMap< PipelineInfo, GPipelineID, PipelineInfoHash >;

		struct CameraUB
		{
			mat4x4		viewProj;
			vec4		cameraPos;
		};


	// variables
	private:
		FGInstancePtr		_fgInstance;
		FGThreadPtr			_frameGraph;

		SubmissionGraph		_submissionGraph;
		ShaderBuilder		_shaderBuilder;
		PipelineCache_t		_pipelineCache;

		PipelineResources	_perPassResources;
		BufferID			_cameraUB;


	// methods
	public:
		RendererPrototype ();
		~RendererPrototype ();

		bool Initialize (const FGInstancePtr &, const FGThreadPtr &, StringView);

		bool GetPipeline (const PipelineInfo &, OUT RawGPipelineID &) override;
		bool GetPipeline (const PipelineInfo &, OUT RawMPipelineID &) override;

		bool Render (const ScenePreRender &) override;
		
		ShaderBuilder*  GetShaderBuilder () override	{ return &_shaderBuilder; }


	private:
		bool _SetupShadowPass (const CameraData_t &, INOUT RenderQueueImpl &, OUT ImageID &);
		bool _SetupColorPass (const CameraData_t &, INOUT RenderQueueImpl &, OUT ImageID &);

		bool _CreateUniformBuffer ();
	};


}	// FG
