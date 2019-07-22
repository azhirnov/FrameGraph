// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "GenPlanetApp.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"
#include "scene/Loader/DevIL/DevILLoader.h"
#include "scene/Loader/Intermediate/IntermImage.h"

#ifdef FG_STD_FILESYSTEM
#	include <filesystem>
	namespace FS = std::filesystem;
#endif

namespace FG
{
namespace {
	static constexpr uint	Lod = 9;
}
	
/*
=================================================
	destructor
=================================================
*/
	GenPlanetApp::~GenPlanetApp ()
	{
		if ( _frameGraph )
		{
			_planet.cube.Destroy( _frameGraph );

			_frameGraph->ReleaseResource( _planet.pipeline );
			_frameGraph->ReleaseResource( _planet.heightMap );
			_frameGraph->ReleaseResource( _planet.normalMap );
			_frameGraph->ReleaseResource( _planet.dbgColorMap );
			_frameGraph->ReleaseResource( _planet.ubuffer );

			_frameGraph->ReleaseResource( _depthBuffer );
		}
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool GenPlanetApp::Initialize ()
	{
		CHECK_ERR( _CreateFrameGraph( uint2(1024, 768), "Planet generator", {FG_DATA_PATH "../shaderlib", FG_DATA_PATH "shaders"}, FG_DATA_PATH "_debug_output" ));
		
		_depthBuffer = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{GetSurfaceSize()}, EPixelFormat::Depth24_Stencil8, EImageUsage::DepthStencilAttachment },
												 Default, "DepthBuffer" );
		CHECK_ERR( _depthBuffer );

		_linearSampler = _frameGraph->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::Repeat )
								.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )).Release();

		// upload resource data
		auto	cmdbuf = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });
		{
			CHECK_ERR( _CreatePlanet( cmdbuf ));
			CHECK_ERR( _GenerateHeightMap( cmdbuf ));
			//CHECK_ERR( _CreateGenerators() );
		}
		CHECK_ERR( _frameGraph->Execute( cmdbuf ));
		CHECK_ERR( _frameGraph->Flush() );
		
		GetFPSCamera().SetPosition({ 0.0f, 0.0f, 2.0f });
		return true;
	}
	
/*
=================================================
	_CreatePlanet
=================================================
*/
	bool GenPlanetApp::_CreatePlanet (const CommandBuffer &cmdbuf)
	{
		const uint2		face_size { 1024, 1024 };

		CHECK_ERR( _planet.cube.Create( cmdbuf, Lod, Lod ));

		// create height map
		{
			_planet.heightMap = _frameGraph->CreateImage( ImageDesc{ EImage::TexCube, uint3{face_size}, EPixelFormat::R16F,
																	 EImageUsage::Storage | EImageUsage::Transfer | EImageUsage::Sampled, 6_layer },
														  Default, "Planet.HeightMap" );
			CHECK_ERR( _planet.heightMap );
		}
		
		// create normal map
		{
			_planet.normalMap = _frameGraph->CreateImage( ImageDesc{ EImage::TexCube, uint3{face_size}, EPixelFormat::RGBA16F,
																	 EImageUsage::Storage | EImageUsage::Transfer | EImageUsage::Sampled, 6_layer },
														  Default, "Planet.NormalMap" );
			CHECK_ERR( _planet.normalMap );
		}

		// create color map
		{
			_planet.dbgColorMap = _frameGraph->CreateImage( ImageDesc{ EImage::TexCube, uint3(4,4,1), EPixelFormat::RGBA8_UNorm,
																	   EImageUsage::TransferDst | EImageUsage::Sampled, 6_layer },
														    Default, "Planet.DbgColor" );
			CHECK_ERR( _planet.dbgColorMap );

			for (uint i = 0; i < 6; ++i) {
				cmdbuf->AddTask( ClearColorImage{}.SetImage( _planet.dbgColorMap ).Clear(RGBA32f(HSVColor{ float(i&3)/6 })).AddRange( 0_mipmap, 1, ImageLayer(i), 1 ));
			}
		}

		// create uniform buffer
		{
			_planet.ubuffer = _frameGraph->CreateBuffer( BufferDesc{ SizeOf<PlanetData>, EBufferUsage::Uniform | EBufferUsage::TransferDst },
														 Default, "Planet.UB" );
			CHECK_ERR( _planet.ubuffer );
		}

		// create pipeline and descriptor set
		{
			const String			shader = _LoadShader( "shaders/planet.glsl" );
			GraphicsPipelineDesc	ppln;

			ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_VERTEX\n"s + shader );
			ppln.AddShader( EShader::TessControl, EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_TESS_CONTROL\n"s + shader );
			ppln.AddShader( EShader::TessEvaluation, EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_TESS_EVALUATION\n"s + shader );
			ppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_FRAGMENT\n"s + shader );

			_planet.pipeline = _frameGraph->CreatePipeline( ppln );
			CHECK_ERR( _planet.pipeline );

			CHECK_ERR( _frameGraph->InitPipelineResources( _planet.pipeline, DescriptorSetID{"0"}, OUT _planet.resources ));

			_planet.resources.BindBuffer( UniformID{"un_PlanetData"}, _planet.ubuffer );
			_planet.resources.BindTexture( UniformID{"un_HeightMap"}, _planet.heightMap, _linearSampler );
			_planet.resources.BindTexture( UniformID{"un_NormalMap"}, _planet.normalMap, _linearSampler );
			_planet.resources.BindTexture( UniformID{"un_ColorMap"}, _planet.dbgColorMap, _linearSampler );
		}

		return true;
	}
	
/*
=================================================
	_GenerateHeightMap
=================================================
*/
	bool GenPlanetApp::_GenerateHeightMap (const CommandBuffer &cmdbuf)
	{
		ComputePipelineDesc	desc;
		desc.AddShader( EShaderLangFormat::VKSL_110 | EShaderLangFormat::EnableDebugTrace,
					    "main",
						"#define PROJECTION  CM_TangentialSC_Forward\n\n"s
							<< _LoadShader( "shaders/gen_height.glsl" )
						);

		CPipelineID	gen_height_ppln = _frameGraph->CreatePipeline( desc );
		CHECK_ERR( gen_height_ppln );

		PipelineResources	ppln_res;
		CHECK_ERR( _frameGraph->InitPipelineResources( gen_height_ppln, DescriptorSetID{"0"}, OUT ppln_res ));

		const uint2		local_size	{8,8};
		const uint2		face_size	= _frameGraph->GetDescription( _planet.heightMap ).dimension.xy();
		const uint2		group_count	= (face_size + local_size - 3) / (local_size - 2);

		for (uint face = 0; face < 6; ++face)
		{
			ppln_res.BindImage( UniformID{"un_OutHeight"}, _planet.heightMap, ImageViewDesc{}.SetArrayLayers( face, 1 ));
			ppln_res.BindImage( UniformID{"un_OutNormal"}, _planet.normalMap, ImageViewDesc{}.SetArrayLayers( face, 1 ));

			DispatchCompute	comp;
			const uint3		pc_data{ face_size.x, face_size.y, face };

			comp.AddPushConstant( PushConstantID{"PushConst"}, pc_data );
			comp.AddResources( DescriptorSetID{"0"}, &ppln_res );
			comp.SetLocalSize( local_size );
			comp.Dispatch( group_count );
			comp.SetPipeline( gen_height_ppln );

			cmdbuf->AddTask( comp );
		}

		_frameGraph->ReleaseResource( gen_height_ppln );
		return true;
	}

/*
=================================================
	_AddDecal
=================================================
*
	bool GenPlanetApp::_AddDecal (const vec2 &mousePos, const SphericalF &angularSize)
	{
		const vec3		dir		= GetFrustum().GetRay( mousePos / GetSurfaceSizeF() );
		const float3	begin	= VecCast( dir * GetViewRange().x );
		const float3	end		= VecCast( dir * GetViewRange().y );
		const float3	center	= VecCast( -GetCamera().transform.position );
		const float		radius	= 1.0f;
		float3			intersection;

		if ( not _planet.cube.RayCast( center, radius, begin, end, OUT intersection ))
			return false;

		const float2	angle	= float2{SphericalF::FromCartesian( intersection ).first};
		const float2	hsize	= float2{angularSize} * 0.5f;
		const RectF		region	= RectF{-hsize, hsize} + angle;

		const float3	points[] = {
			SphericalF{region.left, region.top}.ToCartesian(),
			SphericalF{region.left + region.Width()*0.25f, region.top}.ToCartesian(),
			SphericalF{region.left + region.Width()*0.5f, region.top}.ToCartesian(),
			SphericalF{region.left + region.Width()*0.75f, region.top}.ToCartesian(),
			SphericalF{region.right, region.top}.ToCartesian(),
			SphericalF{region.right, region.top + region.Height()*0.25f}.ToCartesian(),
			SphericalF{region.right, region.top + region.Height()*0.5f}.ToCartesian(),
			SphericalF{region.right, region.top + region.Height()*0.75f}.ToCartesian(),
			SphericalF{region.right, region.bottom}.ToCartesian(),
			SphericalF{region.right - region.Width()*0.25f, region.bottom}.ToCartesian(),
			SphericalF{region.right - region.Width()*0.5f, region.bottom}.ToCartesian(),
			SphericalF{region.right - region.Width()*0.75f, region.bottom}.ToCartesian(),
			SphericalF{region.left, region.bottom}.ToCartesian(),
			SphericalF{region.left, region.bottom - region.Height()*0.25f}.ToCartesian(),
			SphericalF{region.left, region.bottom - region.Height()*0.5f}.ToCartesian(),
			SphericalF{region.left, region.bottom - region.Height()*0.75f}.ToCartesian()
		};
		const bool		active_faces[6] = {};

		for (uint face = 0; face < 6; ++face)
		{
			//_planet.cube.ForwardTransform();
		}

		auto[coord, face] = _planet.cube.InverseProjection( intersection );

		//FG_LOGI( "Intersection: "s << ToString(intersection) << "; face: " << ToString(face) );

		return true;
	}

/*
=================================================
	_LoadShader
=================================================
*/
	String  GenPlanetApp::_LoadShader (StringView filename)
	{
		FileRStream		file{ String{FG_DATA_PATH} << filename };
		CHECK_ERR( file.IsOpen() );

		String	str;
		CHECK_ERR( file.Read( size_t(file.Size()), OUT str ));

		return str;
	}

/*
=================================================
	DrawScene
=================================================
*/
	bool GenPlanetApp::DrawScene ()
	{
		PlanetData	planet_data;

		// update
		{
			_UpdateCamera();

			planet_data.viewProj	= GetCamera().ToViewProjMatrix();
			planet_data.position	= vec4{ GetCamera().transform.position, 0.0f };
			planet_data.clipPlanes	= GetViewRange();

			planet_data.tessLevel		= 12.0f;
			planet_data.lightDirection	= normalize(vec3( 0.0f, -1.0f, 0.0f ));
		}

		// draw
		{
			CommandBuffer	cmdbuf		= _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });
			RawImageID		sw_image	= cmdbuf->GetSwapchainImage( GetSwapchain() );
			uint2			sw_dim		= _frameGraph->GetDescription( sw_image ).dimension.xy();

			LogicalPassID	pass_id		= cmdbuf->CreateRenderPass( RenderPassDesc{ sw_dim }
												.AddViewport( sw_dim )
												.AddTarget( RenderTargetID::Color_0, sw_image, /*RGBA32f{0.412f, 0.796f, 1.0f, 0.0f}*/ RGBA32f{0.0f}, EAttachmentStoreOp::Store )
												.AddTarget( RenderTargetID::Depth, _depthBuffer, DepthStencil{1.0f}, EAttachmentStoreOp::Store )
												.SetDepthCompareOp( ECompareOp::LEqual ).SetDepthTestEnabled( true ).SetDepthWriteEnabled( true )
												//.SetPolygonMode( EPolygonMode::Line )
											);

			cmdbuf->AddTask( pass_id, _planet.cube.Draw( Lod ).SetPipeline( _planet.pipeline )
								.AddResources( DescriptorSetID{"0"}, &_planet.resources ));

			cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _planet.ubuffer ).AddData( &planet_data, 1, 0_b ));

			cmdbuf->AddTask( SubmitRenderPass{ pass_id });


			CHECK_ERR( _frameGraph->Execute( cmdbuf ));

			_SetLastCommandBuffer( cmdbuf );
		}
		return true;
	}
	
/*
=================================================
	OnResize
=================================================
*/
	void GenPlanetApp::OnResize (const uint2 &size)
	{
		if ( Any( size == uint2(0) ))
			return;

		BaseSceneApp::OnResize( size );

		_frameGraph->ReleaseResource( _depthBuffer );

		_depthBuffer = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{size}, EPixelFormat::Depth24_Stencil8, EImageUsage::DepthStencilAttachment },
												 Default, "DepthBuffer" );
		CHECK( _depthBuffer );
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void GenPlanetApp::OnKey (StringView key, EKeyAction action)
	{
		BaseSceneApp::OnKey( key, action );

		if ( action == EKeyAction::Down )
		{
			//if ( key == "right mb" )	_AddDecal( GetMousePos(), 0.1f );
		}
	}


}	// FG


// unit tests
extern void UnitTest_SphericalCubeMath ();


/*
=================================================
	main
=================================================
*/
int main ()
{
	using namespace FG;

	UnitTest_SphericalCubeMath();

	auto	scene = MakeShared<GenPlanetApp>();

	CHECK_ERR( scene->Initialize(), -1 );

	for (; scene->Update();) {}
	
	return 0;
}
