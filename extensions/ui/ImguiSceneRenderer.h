// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/FG.h"
#include "imgui.h"
#include "scene/Renderer/RenderQueue.h"

namespace FG
{

	//
	// Imgui Scene Renderer
	//

	class ImguiSceneRenderer
	{
	// variables
	private:
		ImageID				_fontTexture;
		SamplerID			_fontSampler;
		GPipelineID			_pipeline;

		BufferID			_vertexBuffer;
		BufferID			_indexBuffer;
		BufferID			_uniformBuffer;

		BytesU				_vertexBufSize;
		BytesU				_indexBufSize;

		PipelineResources	_resources;

		Ptr<ImGuiContext>	_context;


	// methods
	public:
		ImguiSceneRenderer ();
		~ImguiSceneRenderer ();

		bool  Initialize (const FrameGraph &fg, ImGuiContext *ctx);
		void  Deinitialize (const FrameGraph &fg);

		bool  Draw (RenderQueue &rq, ERenderLayer layer);


	private:
		bool  _CreatePipeline (const FrameGraph &);
		bool  _CreateSampler (const FrameGraph &);

		bool  _RecreateBuffers (RenderQueue &rq, ERenderLayer layer);
		bool  _CreateFontTexture (RenderQueue &rq, ERenderLayer layer);
		bool  _UpdateUniformBuffer (RenderQueue &rq, ERenderLayer layer);
	};


}	// FG
