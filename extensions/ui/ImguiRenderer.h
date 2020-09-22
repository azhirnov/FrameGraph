// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_IMGUI

#include "ui/Common.h"

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
		~ImguiRenderer ();

		bool  Initialize (const FrameGraph &fg, ImGuiContext *ctx);
		void  Deinitialize (const FrameGraph &fg);

		Task  Draw (const CommandBuffer &cmdbuf, LogicalPassID passId, ArrayView<Task> dependencies = Default);


	private:
		bool _CreatePipeline (const FrameGraph &);
		bool _CreateSampler (const FrameGraph &);

		ND_ Task  _RecreateBuffers (const CommandBuffer &);
		ND_ Task  _CreateFontTexture (const CommandBuffer &);
		ND_ Task  _UpdateUniformBuffer (const CommandBuffer &);
	};


}	// FG

#endif	// FG_ENABLE_IMGUI
