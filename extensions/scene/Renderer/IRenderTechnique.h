// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/ScenePreRender.h"
#include "scene/Renderer/RenderQueue.h"
#include "scene/Renderer/ShaderBuilder.h"

namespace FG
{

	//
	// Render Technique interface
	//

	class IRenderTechnique
	{
	// types
	public:
		using ShaderSource_t = FixedArray< ShaderBuilder::ShaderSourceID, 8 >;

		struct PipelineInfo
		{
			ETextureType	textures		= Default;
			EMaterialType	materialFlags	= Default;
			ERenderLayer	layer			= Default;
			EDetailLevel	detailLevel		= Default;
			ShaderSource_t	sourceIDs;

			PipelineInfo () {}

			ND_ bool  operator == (const PipelineInfo &) const;
		};


	protected:
		struct PipelineInfoHash {
			ND_ size_t  operator () (const PipelineInfo &) const noexcept;
		};

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
		virtual bool GetPipeline (const PipelineInfo &, OUT RawGPipelineID &) = 0;
		virtual bool GetPipeline (const PipelineInfo &, OUT RawMPipelineID &) = 0;
		virtual bool GetPipeline (const PipelineInfo &, OUT RawRTPipelineID &) = 0;

		virtual bool Render (const ScenePreRender &) = 0;

		ND_ virtual ShaderBuilder*	GetShaderBuilder () = 0;
		ND_ virtual FGInstancePtr	GetFrameGraphInstance () = 0;
		ND_ virtual FGThreadPtr		GetFrameGraphThread () = 0;


	protected:
		ND_ static ScenePreRender::CameraArray_t const&  _GetCameras (const ScenePreRender &preRender)		{ return preRender._cameras; }
	};

	
	
	inline bool  IRenderTechnique::PipelineInfo::operator == (const PipelineInfo &rhs) const
	{
		return	textures		== rhs.textures			and
				materialFlags	== rhs.materialFlags	and
				layer			== rhs.layer			and
				detailLevel		== rhs.detailLevel		and
				sourceIDs		== rhs.sourceIDs;
	}

	inline size_t  IRenderTechnique::PipelineInfoHash::operator () (const PipelineInfo &x) const noexcept
	{
		return size_t(HashOf( x.textures ) + HashOf( x.materialFlags ) +
					  HashOf( x.layer )    + HashOf( x.detailLevel )   +
					  HashOf( x.sourceIDs ));
	}


}	// FG
