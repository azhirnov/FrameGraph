// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/Renderer/Prototype/RendererPrototype.h"
#include "scene/Renderer/RenderQueue.h"
#include "scene/SceneManager/IViewport.h"
#include "stl/Algorithms/StringUtils.h"

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
	bool RendererPrototype::Initialize (const FGInstancePtr &inst, const FGThreadPtr &worker, StringView)
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

		_graphicsShaderSource = _shaderBuilder.CacheShaderSource(
										#include "RendererPrototypeGraphics.glsl"
									);
		
		_rayTracingShaderSource = _shaderBuilder.CacheShaderSource(
										#include "RendererPrototypeRaytracing.glsl"
									);

		CHECK_ERR( _CreateUniformBuffer() );
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void RendererPrototype::Deinitialize ()
	{
		if ( _frameGraph )
		{
			for (auto& ppln : _gpipelineCache) {
				_frameGraph->ReleaseResource( ppln.second );
			}
			for (auto& ppln : _mpipelineCache) {
				_frameGraph->ReleaseResource( ppln.second );
			}
			for (auto& ppln : _rtpipelineCache) {
				_frameGraph->ReleaseResource( ppln.second );
			}
		}

		_frameGraph	= null;
		_fgInstance	= null;
	}

/*
=================================================
	BuildShaderDefines
=================================================
*/
	ND_ static String  BuildShaderDefines (const IRenderTechnique::PipelineInfo &info)
	{
		String	result;

		for (ETextureType t = ETextureType(1); t <= info.textures; t = ETextureType(uint(t) << 1))
		{
			if ( not EnumEq( info.textures, t ) )
				continue;

			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case ETextureType::Albedo :		result << "#define ALBEDO_MAP\n";		break;
				case ETextureType::Normal :		result << "#define NORMAL_MAP\n";		break;
				case ETextureType::Height :		result << "#define HEIGHT_MAP\n";		break;
				case ETextureType::Reflection :	result << "#define REFLECTION_MAP\n";	break;
				case ETextureType::Specular :	result << "#define SPECULAR_MAP\n";		break;
				case ETextureType::Roughtness :	result << "#define ROUGHTNESS_MAP\n";	break;
				case ETextureType::Metallic :	result << "#define METALLIC_MAP\n";		break;
				case ETextureType::Unknown :
				default :						CHECK(!"unknown texture type");			break;
			}
			DISABLE_ENUM_CHECKS();
		}
		
		ENABLE_ENUM_CHECKS();
		switch ( info.layer )
		{
			case ERenderLayer::Shadow :			result << "#define LAYER_SHADOWMAP\n";		break;
			case ERenderLayer::DepthOnly :		result << "#define LAYER_DEPTHPREPASS\n";	break;
			case ERenderLayer::Opaque_1 :
			case ERenderLayer::Opaque_2 :
			case ERenderLayer::Opaque_3 :		result << "#define LAYER_OPAQUE\n";			break;
			case ERenderLayer::Translucent :	result << "#define LAYER_TRANSLUCENT\n";	break;
				
			case ERenderLayer::Background :		//result << "#define LAYER_BACKGROUND\n";	break;
			case ERenderLayer::Foreground :		//result << "#define LAYER_FOREGROUND\n";	break;
			case ERenderLayer::Emission :		//result << "#define LAYER_EMISSION\n";		break;
			case ERenderLayer::RayTracing :
			case ERenderLayer::Unknown :
			case ERenderLayer::Layer_32 :
			case ERenderLayer::HUD :
			case ERenderLayer::_Count :
			default :							CHECK(!"unknown render layer");				break;
		}
		DISABLE_ENUM_CHECKS();

		result << "#define DETAIL_LEVEL " << ToString( uint(info.detailLevel) ) << "\n";

		return result;
	}

/*
=================================================
	GetPipeline
=================================================
*/
	bool RendererPrototype::GetPipeline (const PipelineInfo &info, OUT RawGPipelineID &pipeline)
	{
		auto	iter = _gpipelineCache.find( info );

		if ( iter != _gpipelineCache.end() )
		{
			pipeline = iter->second.Get();
			return true;
		}

		const String	defines		= BuildShaderDefines( info );
		String			fs_source	= "#define FRAGMENT_SHADER\n" + defines;
		String			vs_source	= "#define VERTEX_SHADER\n" + defines;
		StringView		ubershader	= _shaderBuilder.GetCachedSource( _graphicsShaderSource );

		for (auto& id : info.sourceIDs)
		{
			StringView	src = _shaderBuilder.GetCachedSource( id );
			vs_source << src;
			fs_source << src;
		}

		fs_source << ubershader;
		vs_source << ubershader;

		GraphicsPipelineDesc	desc;
		desc.AddShader( EShader::Vertex,   EShaderLangFormat::VKSL_110, "main", std::move(vs_source) );
		desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110, "main", std::move(fs_source) );

		pipeline = _frameGraph->CreatePipeline( desc ).Release();
		CHECK_ERR( pipeline.IsValid() );

		if ( not _perPassResources.IsInitialized() )
		{
			CHECK( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID{"PerPass"}, OUT _perPassResources ));
			_perPassResources.BindBuffer( UniformID{"CameraUB"}, _cameraUB, 0_b, SizeOf<CameraUB> );
		}
		
		auto&	ppln_res = _shaderOutputResources[ uint(info.layer) ];
		if ( not ppln_res.IsInitialized() )
		{
			CHECK( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID{"RenderTargets"}, OUT ppln_res ));
		}

		_gpipelineCache.insert_or_assign( info, GPipelineID{pipeline} );
		return true;
	}
	
/*
=================================================
	GetPipeline
=================================================
*/
	bool RendererPrototype::GetPipeline (const PipelineInfo &, OUT RawMPipelineID &)
	{
		return false;
	}
	
/*
=================================================
	GetPipeline
=================================================
*/
	bool RendererPrototype::GetPipeline (const PipelineInfo &info, OUT RawRTPipelineID &pipeline)
	{
		CHECK_ERR( info.layer == ERenderLayer::RayTracing );

		auto	iter = _rtpipelineCache.find( info );

		if ( iter != _rtpipelineCache.end() )
		{
			pipeline = iter->second.Get();
			return true;
		}

		String		header			= "#version 460\n#extension GL_NV_ray_tracing : require\n";
		String		raygen_source	= header + "#define RAYGEN_SHADER\n";
		String		miss1_source	= header + "#define PRIMARY_MISS_SHADER\n";
		String		miss2_source	= header + "#define SHADOW_MISS_SHADER\n";
		String		hit1_source		= header + "#define PRIMARY_CLOSEST_HIT_SHADER\n";
		String		hit2_source		= header + "#define SHADOW_CLOSEST_HIT_SHADER\n";
		StringView	ubershader		= _shaderBuilder.GetCachedSource( _rayTracingShaderSource );
		
		for (auto& id : info.sourceIDs)
		{
			StringView	src = _shaderBuilder.GetCachedSource( id );
			raygen_source << src;
			miss1_source << src;
			miss2_source << src;
			hit1_source << src;
			hit2_source << src;
		}

		raygen_source << ubershader;
		miss1_source << ubershader;
		miss2_source << ubershader;
		hit1_source << ubershader;
		hit2_source << ubershader;

		RayTracingPipelineDesc	desc;
		desc.AddShader( RTShaderID{"Main"}, EShader::RayGen, EShaderLangFormat::VKSL_110, "main", std::move(raygen_source) );
		desc.AddShader( RTShaderID{"PrimaryMiss"}, EShader::RayMiss, EShaderLangFormat::VKSL_110, "main", std::move(miss1_source) );
		desc.AddShader( RTShaderID{"ShadowMiss"}, EShader::RayMiss, EShaderLangFormat::VKSL_110, "main", std::move(miss2_source) );
		desc.AddShader( RTShaderID{"PrimaryHit"}, EShader::RayClosestHit, EShaderLangFormat::VKSL_110, "main", std::move(hit1_source) );
		desc.AddShader( RTShaderID{"ShadowHit"}, EShader::RayClosestHit, EShaderLangFormat::VKSL_110, "main", std::move(hit2_source) );


		pipeline = _frameGraph->CreatePipeline( desc ).Release();
		CHECK_ERR( pipeline.IsValid() );
		
		auto&	ppln_res = _shaderOutputResources[ uint(info.layer) ];
		if ( not ppln_res.IsInitialized() )
		{
			CHECK( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID{"RenderTargets"}, OUT ppln_res ));
			ppln_res.BindBuffer( UniformID{"CameraUB"}, _cameraUB, 0_b, SizeOf<CameraUB> );
		}

		_rtpipelineCache.insert_or_assign( info, RTPipelineID{pipeline} );
		return true;
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
		//_submissionGraph.AddBatch( ShadowMapBatch, uint(cameras.size()) );
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
			if ( cam.layers[uint(ERenderLayer::Opaque_1)] )
			{
				CHECK_ERR( _frameGraph->Begin( MainRendererBatch, mr_counter++, EThreadUsage::Graphics ));
				CHECK_ERR( _SetupColorPass( cam, INOUT queue, OUT image ));
			}
			else
			// detect ray trace technique
			if ( cam.layers[uint(ERenderLayer::RayTracing)] )
			{
				CHECK_ERR( _frameGraph->Begin( MainRendererBatch, mr_counter++, EThreadUsage::Graphics ));
				CHECK_ERR( _SetupRayTracingPass( cam, INOUT queue, OUT image ));
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

		RenderPassDesc	rp{ desc.dimension.xy() };
		rp.AddTarget( RenderTargetID("depth"), shadow_map );
		rp.AddViewport( desc.dimension.xy() );
		rp.AddResources( DescriptorSetID{"PerPass"}, &_perPassResources );

		auto&	res = _shaderOutputResources[uint(ERenderLayer::Shadow)];
		if ( res.IsInitialized() ) {
			res.BindImage( UniformID{"un_ShadowMap"}, shadow_map );
		}

		queue.AddLayer( ERenderLayer::Shadow, rp, res, "ShadowMap" );
		
		_UpdateUniformBuffer( ERenderLayer::Shadow, cameraData, queue );

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
			RenderPassDesc	rp{ desc.dimension.xy() };
			rp.AddTarget( RenderTargetID("out_Color"), color_target, RGBA32f{0.0f}, EAttachmentStoreOp::Store )
			  .AddTarget( RenderTargetID("depth"), depth_target, DepthStencil{1.0f}, EAttachmentStoreOp::Store );
			rp.AddViewport( desc.dimension.xy() );
			rp.SetDepthTestEnabled( true ).SetDepthWriteEnabled( true );
			rp.SetDepthCompareOp( ECompareOp::LEqual );	// for reverse depth buffer
			rp.AddResources( DescriptorSetID{"PerPass"}, &_perPassResources );
			
			auto&	res = _shaderOutputResources[uint(ERenderLayer::Opaque_1)];
			if ( res.IsInitialized() ) {
				res.BindImage( UniformID{"un_OutColor"}, color_target );
				res.BindImage( UniformID{"un_OutDepth"}, depth_target );
			}

			queue.AddLayer( ERenderLayer::Opaque_1, rp, res, "Opaque" );
		}

		// translucent pass
		/*{
			RenderPassDesc	rp{ desc.dimension.xy() };
			rp.AddTarget( RenderTargetID("out_Color"), color_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
			  .AddTarget( RenderTargetID("depth"), depth_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Invalidate );
			rp.AddViewport( desc.dimension.xy() );
			rp.SetDepthTestEnabled( true ).SetDepthWriteEnabled( false );
			rp.SetDepthCompareOp( ECompareOp::GEqual );	// for reverse depth buffer
			rp.AddColorBuffer( RenderTargetID("out_Color"), EBlendFactor::SrcAlpha, EBlendFactor::OneMinusSrcAlpha, EBlendOp::Add );
			rp.AddResources( DescriptorSetID{"PerPass"}, &_perPassResources );
			
			auto&	res = _shaderOutputResources[uint(ERenderLayer::Opaque_1)];
			if ( res.IsInitialized() ) {
				res.BindImage( UniformID{"un_OutColor"}, color_target );
				res.BindImage( UniformID{"un_OutDepth"}, depth_target );
			}

			queue.AddLayer( ERenderLayer::Translucent, rp, res, "Translucent" );
		}*/
		
		_UpdateUniformBuffer( ERenderLayer::Opaque_1, cameraData, queue );

		outImage = std::move(color_target);
		_frameGraph->ReleaseResource( depth_target );
		return true;
	}
	
/*
=================================================
	_SetupRayTracingPass
=================================================
*/
	bool RendererPrototype::_SetupRayTracingPass (const CameraData_t &cameraData, INOUT RenderQueueImpl &queue, OUT ImageID &outImage)
	{
		ASSERT( cameraData.layers.count() == 1 );	// other layers will be ignored

		ImageDesc	desc;
		desc.imageType	= EImage::Tex2D;
		desc.dimension	= uint3{float3{ cameraData.viewportSize.x + 0.5f, cameraData.viewportSize.y + 0.5f, 1.0f }};
		desc.format		= EPixelFormat::RGBA16F;	// HDR
		desc.usage		= EImageUsage::ColorAttachment | EImageUsage::Storage | EImageUsage::Transfer;

		ImageID		color_target = _frameGraph->CreateImage( desc );	// TODO: create virtual image
		
		RenderPassDesc	rp{ desc.dimension.xy() };
		rp.AddTarget( RenderTargetID("out_Color"), color_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store );
		rp.AddViewport( desc.dimension.xy() );

		auto&	res = _shaderOutputResources[uint(ERenderLayer::RayTracing)];
		if ( res.IsInitialized() ) {
			res.BindImage( UniformID{"un_OutColor"}, color_target );
		}
		
		queue.AddLayer( ERenderLayer::RayTracing, rp, res, "RayTracing" );

		_UpdateUniformBuffer( ERenderLayer::RayTracing, cameraData, queue );

		outImage = std::move(color_target);
		return true;
	}

/*
=================================================
	_CreateUniformBuffer
=================================================
*/
	bool RendererPrototype::_CreateUniformBuffer ()
	{
		_cameraUB = _frameGraph->CreateBuffer( BufferDesc{ SizeOf<CameraUB>, EBufferUsage::Uniform | EBufferUsage::TransferDst }, Default, "CameraUB" );
		CHECK_ERR( _cameraUB );

		return true;
	}
	
/*
=================================================
	_UpdateUniformBuffer
=================================================
*/
	void RendererPrototype::_UpdateUniformBuffer (ERenderLayer firstLayer, const CameraData_t &cameraData, INOUT RenderQueueImpl &queue)
	{
		CameraUB	data;
		data.viewProj	= cameraData.camera.ToViewProjMatrix();
		data.position	= vec4{ cameraData.camera.transform.position, 0.0f };
		data.clipPlanes	= cameraData.visibilityRange;

		cameraData.frustum.GetRays( OUT data.frustumRayLeftTop, OUT data.frustumRayLeftBottom,
								    OUT data.frustumRayRightTop, OUT data.frustumRayRightBottom );

		queue.AddTask( firstLayer, UpdateBuffer{}.SetBuffer( _cameraUB ).AddData( &data, 1 ));
	}


}	// FG
