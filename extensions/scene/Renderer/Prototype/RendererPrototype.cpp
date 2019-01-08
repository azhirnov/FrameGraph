// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/Renderer/Prototype/RendererPrototype.h"
#include "scene/Renderer/RenderQueue.h"
#include "scene/SceneManager/IViewport.h"

namespace FG
{
namespace {
	static constexpr CommandBatchID		ShadowMapBatch {"ShadowMap"};
	static constexpr CommandBatchID		MainRendererBatch {"Main"};
}

/*
=================================================
	constructor
=================================================
*/
	RendererPrototype::RendererPrototype ()
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	RendererPrototype::~RendererPrototype ()
	{
	}
	
/*
=================================================
	GetRenderLayerMask
=================================================
*
	ERenderLayer  RendererPrototype::GetRenderLayerMask () const
	{
		return	ERenderLayer::Background	| ERenderLayer::Foreground	|
				ERenderLayer::Shadow		| ERenderLayer::DepthOnly	|
				ERenderLayer::Opaque_1		| ERenderLayer::Opaque_2	|
				ERenderLayer::Opaque_3		| ERenderLayer::Translucent;
	}

/*
=================================================
	Initialize
=================================================
*/
	bool RendererPrototype::Initialize (const FGInstancePtr &inst, const FGThreadPtr &worker, StringView pplnPath)
	{
		CHECK_ERR( inst );

		_fgInstance = inst;

		ThreadDesc	desc{ EThreadUsage::Graphics };

		_frameGraph = worker;
		
		if ( not _frameGraph )
		{
			_frameGraph = _fgInstance->CreateThread( desc );
			CHECK_ERR( _frameGraph );
			CHECK_ERR( _frameGraph->Initialize() );
		}

		CHECK_ERR( _LoadPipelines( pplnPath ));
		return true;
	}
	
/*
=================================================
	_LoadPipelines
=================================================
*/
	bool RendererPrototype::_LoadPipelines (StringView path)
	{
		return true;
	}

/*
=================================================
	GetPipeline
=================================================
*/
	bool RendererPrototype::GetPipeline (const PipelineInfo &, OUT GPipelineID &)
	{
		return false;
	}
	
/*
=================================================
	GetPipeline
=================================================
*/
	bool RendererPrototype::GetPipeline (const PipelineInfo &, OUT MPipelineID &)
	{
		return false;
	}
	
/*
=================================================
	Render
=================================================
*/
	bool RendererPrototype::Render (const ScenePreRender &preRender)
	{
		auto&	cameras		= _GetCameras( preRender );
		uint	sm_counter	= 0;
		uint	mr_counter	= 0;

		_submissionGraph.Clear();
		_submissionGraph.AddBatch( ShadowMapBatch, uint(cameras.size()) );
		_submissionGraph.AddBatch( MainRendererBatch, 1 );

		CHECK_ERR( _fgInstance->BeginFrame( _submissionGraph ));

		for (auto& cam : cameras)
		{
			if ( cam.scenes.empty() )
				continue;
			
			RenderQueueImpl		queue;
			queue.Create( _frameGraph, cam );

			ImageID		image;

			// detect shadow techniques
			if ( cam.layers[uint(ERenderLayer::Shadow)] )
			{
				CHECK_ERR( _frameGraph->Begin( ShadowMapBatch, sm_counter++, EThreadUsage::Graphics ));
				CHECK_ERR( _SetupShadowPass( cam, INOUT queue, OUT image ));
			}
			else
			// detect color technique
			{
				CHECK_ERR( _frameGraph->Begin( MainRendererBatch, mr_counter++, EThreadUsage::Graphics ));
				CHECK_ERR( _SetupColorPass( cam, INOUT queue, OUT image ));
			}

			// build render queue
			for (auto& scene : cam.scenes) {
				scene->Draw( INOUT queue );
			}

			if ( cam.viewport )
				cam.viewport->Draw( INOUT queue );

			// submit draw commands
			Task  last_task = queue.Submit();

			// present on viewport
			if ( cam.viewport )
				cam.viewport->AfterRender( _frameGraph, Present{ image }.DependsOn( last_task ));

			CHECK_ERR( _frameGraph->Execute() );
			_frameGraph->ReleaseResource( image );
		}

		CHECK_ERR( _fgInstance->EndFrame() );
		return true;
	}

/*
=================================================
	_SetupShadowPass
=================================================
*/
	bool RendererPrototype::_SetupShadowPass (const CameraData_t &cameraData, INOUT RenderQueueImpl &queue, OUT ImageID &outImage)
	{
		ASSERT( cameraData.layers.count() == 1 );	// other layers will be ignored

		ImageDesc	desc;
		desc.imageType	= EImage::Tex2D;
		desc.dimension	= uint3{float3{ cameraData.viewportSize.x + 0.5f, cameraData.viewportSize.y + 0.5f, 1.0f }};
		desc.format		= EPixelFormat::Depth32F;
		desc.usage		= EImageUsage::DepthStencilAttachment | EImageUsage::Transfer;

		ImageID		shadow_map = _frameGraph->CreateImage( desc );

		RenderPassDesc	rp;
		rp.AddTarget( RenderTargetID("depth"), shadow_map );
		rp.AddViewport( desc.dimension.xy() );

		queue.AddLayer( ERenderLayer::Shadow, rp, "ShadowMap" );

		outImage = std::move(shadow_map);
		return true;
	}
	
/*
=================================================
	_SetupColorPass
=================================================
*/
	bool RendererPrototype::_SetupColorPass (const CameraData_t &cameraData, INOUT RenderQueueImpl &queue, OUT ImageID &outImage)
	{
		ImageDesc	desc;
		desc.imageType	= EImage::Tex2D;
		desc.dimension	= uint3{float3{ cameraData.viewportSize.x + 0.5f, cameraData.viewportSize.y + 0.5f, 1.0f }};
		desc.format		= EPixelFormat::RGBA16F;	// HDR
		desc.usage		= EImageUsage::ColorAttachment | EImageUsage::Transfer;

		ImageID		color_target = _frameGraph->CreateImage( desc );	// TODO: create virtual image

		desc.format		= EPixelFormat::Depth32F;
		desc.usage		= EImageUsage::DepthStencilAttachment | EImageUsage::Transfer;

		ImageID		depth_target = _frameGraph->CreateImage( desc );

		// opaque pass
		{
			RenderPassDesc	rp;
			rp.AddTarget( RenderTargetID("out_Color"), color_target, RGBA32f{0.0f}, EAttachmentStoreOp::Store )
			  .AddTarget( RenderTargetID("depth"), depth_target, DepthStencil{1.0f}, EAttachmentStoreOp::Store );
			rp.AddViewport( desc.dimension.xy() );
			rp.SetDepthTestEnabled( true ).SetDepthWriteEnabled( true );
			rp.SetDepthCompareOp( ECompareOp::GEqual );	// for reverse depth buffer

			queue.AddLayer( ERenderLayer::Opaque_1, rp, "Opaque" );
		}

		// translucent pass
		{
			RenderPassDesc	rp;
			rp.AddTarget( RenderTargetID("out_Color"), color_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
			  .AddTarget( RenderTargetID("depth"), depth_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Invalidate );
			rp.AddViewport( desc.dimension.xy() );
			rp.SetDepthTestEnabled( true ).SetDepthWriteEnabled( false );
			rp.SetDepthCompareOp( ECompareOp::GEqual );	// for reverse depth buffer
			rp.AddColorBuffer( RenderTargetID("out_Color"), EBlendFactor::SrcAlpha, EBlendFactor::OneMinusSrcAlpha, EBlendOp::Add );

			queue.AddLayer( ERenderLayer::Translucent, rp, "Translucent" );
		}

		outImage = std::move(color_target);
		_frameGraph->ReleaseResource( depth_target );
		return true;
	}


}	// FG
