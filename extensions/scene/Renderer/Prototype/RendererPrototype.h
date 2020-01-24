// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

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
		using SourceID			= ShaderCache::ShaderSourceID;
		using ShaderOutputs_t	= StaticArray< PipelineResources, uint(ERenderLayer::_Count) >;

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

		struct LightsUB
		{
			struct Light
			{
				vec3	position;
				float	radius;
				vec4	color;
				vec4	attenuation;
			};

			uint		lightCount	= 0;
			int			_padding[3];
			Light		lights [32];
		};


	// variables
	private:
		FrameGraph			_frameGraph;
		ShaderCache			_shaderCache;

		SourceID			_graphicsShaderSource		= Default;
		SourceID			_rayTracingShaderSource		= Default;

		PipelineResources	_perPassResources;
		ShaderOutputs_t		_shaderOutputResources;		// for compute / ray-tracing
		BufferID			_cameraUB;
		BufferID			_lightsUB;

		ImageID				_colorTarget;
		ImageID				_depthTarget;


	// methods
	public:
		RendererPrototype ();
		~RendererPrototype ();

		bool Create (const FrameGraph &);
		void Destroy () override;

		bool Render (const ScenePreRender &) override;

		bool GetPipeline (ERenderLayer, INOUT GraphicsPipelineInfo &, OUT RawGPipelineID &) override;
		bool GetPipeline (ERenderLayer, INOUT GraphicsPipelineInfo &, OUT RawMPipelineID &) override;
		bool GetPipeline (ERenderLayer, INOUT RayTracingPipelineInfo &, OUT RawRTPipelineID &) override;
		bool GetPipeline (ERenderLayer, INOUT ComputePipelineInfo &, OUT RawCPipelineID &) override;
		
		Ptr<ShaderCache>	GetShaderBuilder ()	override	{ return &_shaderCache; }
		FrameGraph			GetFrameGraph ()	override	{ return _frameGraph; }


	private:
		//bool _SetupShadowPass (const CameraData_t &, INOUT RenderQueueImpl &, OUT RawImageID &);
		bool _SetupColorPass (const CameraData_t &, INOUT RenderQueueImpl &, OUT RawImageID &);
		bool _SetupRayTracingPass (const CameraData_t &, INOUT RenderQueueImpl &, OUT RawImageID &);

		ND_ RawImageID  _CreateColorTarget (const uint2 &dim);
		ND_ RawImageID  _CreateDepthTarget (const uint2 &dim);

		bool _CreateUniformBuffer ();
		void _UpdateUniformBuffer (ERenderLayer firstLayer, const CameraData_t &cameraData, INOUT RenderQueueImpl &queue);
	};


}	// FG
