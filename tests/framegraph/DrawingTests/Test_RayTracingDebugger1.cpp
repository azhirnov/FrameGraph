// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_RayTracingDebugger1 ()
	{
		if ( not _vulkan.HasDeviceExtension( VK_NV_RAY_TRACING_EXTENSION_NAME ) )
			return true;

		RayTracingPipelineDesc	ppln;

		ppln.AddShader( RTShaderID("Main"), EShader::RayGen, EShaderLangFormat::VKSL_110 | EShaderLangFormat::EnableDebugTrace, "main", R"#(
#version 460 core
#extension GL_NV_ray_tracing : require
layout(binding = 0) uniform accelerationStructureNV  un_RtScene;
layout(binding = 1, rgba8) writeonly uniform image2D  un_Output;
layout(location = 0) rayPayloadNV vec4  payload;

void main ()
{
	const vec2 uv = vec2(gl_LaunchIDNV.xy) / vec2(gl_LaunchSizeNV.xy - 1);
	const vec3 origin = vec3(uv.x, 1.0f - uv.y, -1.0f);
	const vec3 direction = vec3(0.0f, 0.0f, 1.0f);

	traceNV( /*topLevel*/un_RtScene, /*rayFlags*/gl_RayFlagsNoneNV, /*cullMask*/0xFF,
			 /*sbtRecordOffset*/0, /*sbtRecordStride*/1, /*missIndex*/0,
			 /*origin*/origin, /*Tmin*/0.0f, /*direction*/direction, /*Tmax*/10.0f,
			 /*payload*/0 );

	vec4 color = payload;
	imageStore( un_Output, ivec2(gl_LaunchIDNV), color );
}
)#", "MainRayGenShader" );
		
		ppln.AddShader( RTShaderID("PrimaryMiss"), EShader::RayMiss, EShaderLangFormat::VKSL_110 | EShaderLangFormat::EnableDebugTrace, "main", R"#(
#version 460 core
#extension GL_NV_ray_tracing : require
layout(location = 0) rayPayloadInNV vec4  payload;

void main ()
{
	payload = vec4(0.0f);
}
)#", "DefaultRayMissShader" );
		
		ppln.AddShader( RTShaderID("PrimaryHit"), EShader::RayClosestHit, EShaderLangFormat::VKSL_110 | EShaderLangFormat::EnableDebugTrace, "main", R"#(
#version 460 core
#extension GL_NV_ray_tracing : require
layout(location = 0) rayPayloadInNV vec4  payload;
					 hitAttributeNV vec2  hitAttribs;

void main ()
{
	const vec3 barycentrics = vec3(1.0f - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);
	payload = vec4(barycentrics, 1.0);
}
)#", "Closest hit shader for triangle" );

		FGThreadPtr		frame_graph	= _fgGraphics1;
		const uint2		view_size	= {800, 600};
		const uint2		debug_coord	= view_size / 2;

		ImageID			dst_image	= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																			EImageUsage::Storage | EImageUsage::TransferSrc },
															    Default, "OutputImage" );
		
		RTPipelineID	pipeline	= frame_graph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );
		
		const auto		vertices	= ArrayView<float3>{ { 0.25f, 0.25f, 0.0f }, { 0.75f, 0.25f, 0.0f }, { 0.50f, 0.75f, 0.0f } };
		const auto		indices		= ArrayView<uint>{ 0, 1, 2 };
		
		BuildRayTracingGeometry::Triangles	triangles;
		triangles.SetID( GeometryID{"Triangle"} ).SetVertexArray( vertices ).SetIndexArray( indices );

		RayTracingGeometryDesc::Triangles	triangles_info;
		triangles_info.SetID( GeometryID{"Triangle"} ).SetVertices< decltype(vertices[0]) >( vertices.size() )
					.SetIndices( indices.size(), EIndex::UInt ).AddFlags( ERayTracingGeometryFlags::Opaque );

		RTGeometryID	rt_geometry	= frame_graph->CreateRayTracingGeometry( RayTracingGeometryDesc{{ triangles_info }} );
		RTSceneID		rt_scene	= frame_graph->CreateRayTracingScene( RayTracingSceneDesc{ 1 }, Default, "Scene" );
		RTShaderTableID	rt_shaders	= frame_graph->CreateRayTracingShaderTable();

		PipelineResources	resources;
		CHECK_ERR( frame_graph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources ));
		
		
		bool	data_is_correct				= false;
		bool	shader_output_is_correct	= true;
		
		const auto	OnShaderTraceReady = [OUT &shader_output_is_correct] (StringView taskName, StringView shaderName, EShaderStages stages, ArrayView<String> output)
		{
			shader_output_is_correct &= (stages == EShaderStages::AllRayTracing);
			shader_output_is_correct &= (taskName == "DebuggableRayTracing");

			if ( shaderName == "MainRayGenShader" )
			{
				const char	ref[] = R"#(//> gl_LaunchIDNV: uint3 {400, 300, 0}
no source

//> uv: float2 {0.500626, 0.500835}
//  gl_LaunchIDNV: uint3 {400, 300, 0}
10. uv = vec2(gl_LaunchIDNV.xy) / vec2(gl_LaunchSizeNV.xy - 1);

//> origin: float3 {0.500626, 0.499165, -1.000000}
//  uv: float2 {0.500626, 0.500835}
11. origin = vec3(uv.x, 1.0f - uv.y, -1.0f);

//> traceNV(): void
//  origin: float3 {0.500626, 0.499165, -1.000000}
14. un_RtScene, /*rayFlags*/gl_RayFlagsNoneNV, /*cullMask*/0xFF,
15. 			 /*sbtRecordOffset*/0, /*sbtRecordStride*/1, /*missIndex*/0,
16. 			 /*origin*/origin, /*Tmin*/0.0f, /*direction*/direction, /*Tmax*/10.0f,
17. 			 /*payload*/0 );

//> color: float4 {0.249583, 0.252086, 0.498331, 1.000000}
19. color = payload;

//> imageStore(): void
//  gl_LaunchIDNV: uint3 {400, 300, 0}
//  color: float4 {0.249583, 0.252086, 0.498331, 1.000000}
20. 	imageStore( un_Output, ivec2(gl_LaunchIDNV), color );

)#";
				shader_output_is_correct &= (output.size() == 1 ? output[0] == ref : false);
			}
			else
			if ( shaderName == "DefaultRayMissShader" )
			{
				shader_output_is_correct &= output.empty();
			}
			else
			{
				const char	ref[] = R"#(//> gl_LaunchIDNV: uint3 {400, 300, 0}
no source

//> barycentrics: float3 {0.249583, 0.252086, 0.498331}
9. barycentrics = vec3(1.0f - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);

//> payload: float4 {0.249583, 0.252086, 0.498331, 1.000000}
//  barycentrics: float3 {0.249583, 0.252086, 0.498331}
10. payload = vec4(barycentrics, 1.0);

)#";
				shader_output_is_correct &= (shaderName == "Closest hit shader for triangle");
				shader_output_is_correct &= (output.size() == 1 ? output[0] == ref : false);
			}
			ASSERT( shader_output_is_correct );
		};
		frame_graph->SetShaderDebugCallback( OnShaderTraceReady );

		const auto	OnLoaded =	[debug_coord, OUT &data_is_correct] (const ImageView &imageData) {
			RGBA32f		dst;
			imageData.Load( uint3(debug_coord,0), OUT dst );
			data_is_correct = Equals( dst.r, 0.249583f, 0.01f ) and	// must contains same value as in trace
							  Equals( dst.g, 0.252086f, 0.01f ) and
							  Equals( dst.b, 0.498331f, 0.01f ) and
							  Equals( dst.a, 1.0f,      0.01f );
			ASSERT( data_is_correct );
		};


		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
		CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));
		
		resources.BindImage( UniformID("un_Output"), dst_image );
		resources.BindRayTracingScene( UniformID("un_RtScene"), rt_scene );
		
		BuildRayTracingScene::Instance		instance;
		instance.SetID( InstanceID{"0"} );
		instance.SetGeometry( rt_geometry );
		

		Task	t_build_geom	= frame_graph->AddTask( BuildRayTracingGeometry{}.SetTarget( rt_geometry ).Add( triangles ));
		Task	t_build_scene	= frame_graph->AddTask( BuildRayTracingScene{}.SetTarget( rt_scene ).Add( instance ).DependsOn( t_build_geom ));

		Task	t_update_table	= frame_graph->AddTask( UpdateRayTracingShaderTable{}
															.SetTarget( rt_shaders ).SetPipeline( pipeline ).SetScene( rt_scene )
															.SetRayGenShader( RTShaderID{"Main"} )
															.AddMissShader( RTShaderID{"PrimaryMiss"} )
															.AddTriangleHitShader( InstanceID{"0"}, GeometryID{"Triangle"}, 0, RTShaderID{"PrimaryHit"} )
															.DependsOn( t_build_scene ));

		Task	t_trace			= frame_graph->AddTask( TraceRays{}.AddResources( DescriptorSetID("0"), &resources )
															.SetShaderTable( rt_shaders )
															.SetGroupCount( view_size.x, view_size.y )
															.SetName( "DebuggableRayTracing" )
															.EnableDebugTrace(uint3{ debug_coord, 0 })
															.DependsOn( t_update_table ));

		Task	t_read			= frame_graph->AddTask( ReadImage{}.SetImage( dst_image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_trace ));
		FG_UNUSED( t_read );

		CHECK_ERR( frame_graph->Execute() );		
		CHECK_ERR( _fgInstance->EndFrame() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));

		CHECK_ERR( _fgInstance->WaitIdle() );
		
		CHECK_ERR( data_is_correct and shader_output_is_correct );
		
		DeleteResources( pipeline, dst_image, rt_geometry, rt_scene, rt_shaders );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG
