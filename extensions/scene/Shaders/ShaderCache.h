// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/Enums.h"

namespace FG
{

	//
	// Shader Cache
	//

	class ShaderCache
	{
	// types
	public:
		enum class ShaderSourceID : uint { Unknown = ~0u };

		using ShaderSource_t	= FixedArray< ShaderSourceID, 8 >;
		using ConstName_t		= StaticString<32>;
		using ConstArray_t		= Array<Pair< ConstName_t, uint >>;

		struct GraphicsPipelineInfo
		{
			VertexAttributesPtr	attribs;
			BytesU				vertexStride	= ~0_b;		// (mesh or raytracing shader)
			ConstArray_t		constants;
			ETextureType		textures		= Default;
			EDetailLevel		detailLevel		= Default;
			ShaderSource_t		sourceIDs;

			GraphicsPipelineInfo () {}

			ND_ bool  operator == (const GraphicsPipelineInfo &) const noexcept;
		};


		struct RayTracingPipelineInfo : GraphicsPipelineInfo
		{
			using Shaders_t	= Array<Pair< RTShaderID, EShader >>;

			Shaders_t	shaders;

			RayTracingPipelineInfo () {}

			ND_ bool  operator == (const RayTracingPipelineInfo &) const noexcept;
		};


		struct ComputePipelineInfo
		{
			ShaderSource_t		sourceIDs;

			ComputePipelineInfo () {}

			ND_ bool  operator == (const ComputePipelineInfo &) const noexcept;
		};

		
	private:
		struct GraphicsPipelineInfoHash {
			ND_ size_t  operator () (const GraphicsPipelineInfo &) const noexcept;
		};
		
		struct RayTracingPipelineInfoHash {
			ND_ size_t  operator () (const RayTracingPipelineInfo &) const noexcept;
		};

		struct ComputePipelineInfoHash {
			ND_ size_t  operator () (const ComputePipelineInfo &) const noexcept;
		};

		using GPipelineCache_t	= HashMap< GraphicsPipelineInfo, GPipelineID, GraphicsPipelineInfoHash >;
		using MPipelineCache_t	= HashMap< GraphicsPipelineInfo, MPipelineID, GraphicsPipelineInfoHash >;
		using RTPipelineCache_t	= HashMap< RayTracingPipelineInfo, RTPipelineID, RayTracingPipelineInfoHash >;
		using CPipelineCache_t	= HashMap< ComputePipelineInfo, CPipelineID, ComputePipelineInfoHash >;
		
		using ShaderSourceCache_t	= HashMap< String, ShaderSourceID >;
		using ShaderSources_t		= Array< StringView >;


	// variables
	private:
		FrameGraph				_frameGraph;
		String					_filePath;

		ShaderSourceCache_t		_sourceCache;
		ShaderSources_t			_sources;
		String					_defaultDefines;

		GPipelineCache_t		_gpplnCache;
		MPipelineCache_t		_mpplnCache;
		RTPipelineCache_t		_rtpplnCache;
		CPipelineCache_t		_cpplnCache;

		ShaderSourceID			_sharedSource;


	// methods
	public:
		ShaderCache ();
		~ShaderCache ();

		bool Load (const FrameGraph &, StringView path);
		void Clear ();


		ND_ ShaderSourceID  CacheFileSource (StringView filename);
		
		bool GetPipeline (GraphicsPipelineInfo &, OUT RawGPipelineID &);
		bool GetPipeline (GraphicsPipelineInfo &, OUT RawMPipelineID &);
		bool GetPipeline (RayTracingPipelineInfo &, OUT RawRTPipelineID &);
		bool GetPipeline (ComputePipelineInfo &, OUT RawCPipelineID &);


	private:
		ND_ StringView		_GetCachedSource (ShaderSourceID id) const;
		ND_ ShaderSourceID  _CacheShaderSource (StringView src);
		ND_ ShaderSourceID  _CacheShaderSource (String &&src);

		ND_ String  _CacheVertexAttribs (const GraphicsPipelineInfo &);
		ND_ String  _CacheMeshBuffer (const GraphicsPipelineInfo &);
		ND_ String  _CacheRayTracingVertexBuffer (const GraphicsPipelineInfo &);
	};


}	// FG
