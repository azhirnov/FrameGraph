// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/VFG.h"
#include "imgui.h"

namespace FG
{

	//
	// Imgui Renderer
	//

	class ImguiRenderer
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
		ImguiRenderer ();

		bool Initialize (const FGThreadPtr &fg, ImGuiContext *ctx);
		void Deinitialize (const FGThreadPtr &fg);

		ND_ Task  Draw (const FGThreadPtr &fg, LogicalPassID passId, ArrayView<Task> dependencies = Default);


	private:
		bool _CreatePipeline (const FGThreadPtr &fg);
		bool _CreateSampler (const FGThreadPtr &fg);

		ND_ Task  _RecreateBuffers (const FGThreadPtr &fg);
		ND_ Task  _CreateFontTexture (const FGThreadPtr &fg);
		ND_ Task  _UpdateUniformBuffer (const FGThreadPtr &fg);
	};


}	// FG
