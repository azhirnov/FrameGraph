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
	// variables
	private:
		FGInstancePtr		_fgInstance;
		FGThreadPtr			_frameGraph;

		SubmissionGraph		_submissionGraph;


	// methods
	public:
		RendererPrototype ();
		~RendererPrototype ();

		bool Initialize (const FGInstancePtr &, const FGThreadPtr &, StringView);

		bool GetPipeline (const PipelineInfo &, OUT GPipelineID &) override;
		bool GetPipeline (const PipelineInfo &, OUT MPipelineID &) override;

		bool Render (const ScenePreRender &) override;


	private:
		bool _SetupShadowPass (const CameraData_t &, INOUT RenderQueueImpl &, OUT ImageID &);
		bool _SetupColorPass (const CameraData_t &, INOUT RenderQueueImpl &, OUT ImageID &);

		bool _LoadPipelines (StringView path);
	};


}	// FG
