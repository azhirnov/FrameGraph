// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This test affects:
		...
*/

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_TraceRays2 ()
	{
		if ( not _vulkan.HasDeviceExtension( VK_NV_RAY_TRACING_EXTENSION_NAME ) )
			return true;

		RayTracingPipelineDesc	ppln;

		ppln.AddShader( RTShaderID("Main"), EShader::RayGen, EShaderLangFormat::VKSL_110, "main", R"#(
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

	imageStore( un_Output, ivec2(gl_LaunchIDNV), payload );
}
)#");
		
		ppln.AddShader( RTShaderID("PrimiryMiss"), EShader::RayMiss, EShaderLangFormat::VKSL_110, "main", R"#(
#version 460 core
#extension GL_NV_ray_tracing : require
layout(location = 0) rayPayloadInNV vec4  payload;

void main ()
{
	payload = vec4(0.0f);
}
)#");
		
		ppln.AddShader( RTShaderID("PrimiryHit"), EShader::RayClosestHit, EShaderLangFormat::VKSL_110, "main", R"#(
#version 460 core
#extension GL_NV_ray_tracing : require
layout(location = 0) rayPayloadInNV vec4  payload;
					 hitAttributeNV vec2  hitAttribs;

void main ()
{
	const vec3 barycentrics = vec3(1.0f - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);
	payload = vec4(barycentrics, 1.0);
}
)#");
		ppln.AddRayGenShader( RTShaderGroupID("Main"), RTShaderID("Main") );
		ppln.AddRayMissShader( RTShaderGroupID("PrimiryMiss"), RTShaderID("PrimiryMiss") );
		ppln.AddTriangleHitShaders( RTShaderGroupID("TriangleHit"), RTShaderID("PrimiryHit") );


		FGThreadPtr		frame_graph	= _frameGraph1;
		const uint2		view_size	= {800, 600};
		ImageID			dst_image	= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																			EImageUsage::Storage | EImageUsage::TransferSrc },
															    Default, "OutputImage" );
		BufferID		sbt_buffer	= frame_graph->CreateBuffer( BufferDesc{ 4_Kb, EBufferUsage::RayTracing | EBufferUsage::TransferDst } );
		
		RTPipelineID	pipeline	= frame_graph->CreatePipeline( std::move(ppln) );
		
		const auto		vertices	= ArrayView<float3>{ { 0.25f, 0.25f, 0.0f }, { 0.75f, 0.25f, 0.0f }, { 0.50f, 0.75f, 0.0f } };
		const auto		indices		= ArrayView<uint>{ 0, 1, 2 };
		
		BuildRayTracingGeometry::Triangles	triangles;
		triangles.SetID( GeometryID{"Triangle"} ).SetVertices( vertices ).SetIndices( indices ).AddFlags( ERayTracingGeometryFlags::Opaque );

		RayTracingGeometryDesc::Triangles	triangles_info;
		triangles_info.SetID( GeometryID{"Triangle"} ).SetVertices< decltype(vertices[0]) >( vertices.size() )
					.SetIndices( indices.size(), EIndex::UInt ).AddFlags( ERayTracingGeometryFlags::Opaque );

		RTGeometryID	rt_geometry	= frame_graph->CreateRayTracingGeometry( RayTracingGeometryDesc{{ triangles_info }} );
		RTSceneID		rt_scene	= frame_graph->CreateRayTracingScene( RayTracingSceneDesc{ 1 });

		RawDescriptorSetLayoutID	ds_layout;
		uint						ds_index;
		CHECK_ERR( frame_graph->GetDescriptorSet( pipeline, DescriptorSetID("0"), OUT ds_layout, OUT ds_index ));

		PipelineResources			resources;
		CHECK_ERR( frame_graph->InitPipelineResources( ds_layout, OUT resources ));
		
		
		bool	data_is_correct = false;
		
		const auto	OnLoaded =	[this, OUT &data_is_correct] (const ImageView &imageData)
		{
			const auto	TestPixel = [&imageData] (float x, float y, const RGBA32f &color)
			{
				uint	ix	 = uint( (x + 1.0f) * 0.5f * float(imageData.Dimension().x) + 0.5f );
				uint	iy	 = uint( (y + 1.0f) * 0.5f * float(imageData.Dimension().y) + 0.5f );

				RGBA32f	col;
				imageData.Load( uint3(ix, iy, 0), OUT col );

				bool	is_equal	= Equals( col.r, color.r, 0.1f ) and
									  Equals( col.g, color.g, 0.1f ) and
									  Equals( col.b, color.b, 0.1f ) and
									  Equals( col.a, color.a, 0.1f );
				ASSERT( is_equal );
				return is_equal;
			};

			data_is_correct  = true;
			data_is_correct &= TestPixel( 0.00f, -0.49f, RGBA32f{0.0f, 0.0f, 1.0f, 1.0f} );
			data_is_correct &= TestPixel( 0.49f,  0.49f, RGBA32f{0.0f, 1.0f, 0.0f, 1.0f} );
			data_is_correct &= TestPixel(-0.49f,  0.49f, RGBA32f{1.0f, 0.0f, 0.0f, 1.0f} );
			
			data_is_correct &= TestPixel( 0.00f, -0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.51f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel(-0.51f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.00f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.51f, -0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel(-0.51f, -0.51f, RGBA32f{0.0f} );
		};


		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id, 1 );
		
		TraceRays::ShaderTable		shader_table;

		// frame 1
		{
			CHECK_ERR( _frameGraphInst->BeginFrame( submission_graph ));
			CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));
		
			resources.BindImage( UniformID("un_Output"), dst_image );
			resources.BindRayTracingScene( UniformID("un_RtScene"), rt_scene );
		
			BuildRayTracingScene::Instance		instance;
			instance.SetGeometry( rt_geometry );

			Task	t_build_geom	= frame_graph->AddTask( BuildRayTracingGeometry{}.SetTarget( rt_geometry ).Add( triangles ));
			Task	t_build_scene	= frame_graph->AddTask( BuildRayTracingScene{}.SetTarget( rt_scene ).Add( instance ).DependsOn( t_build_geom ));

			Task	t_update_table	= frame_graph->AddTask( UpdateRayTracingShaderTable{ OUT shader_table }.SetPipeline( pipeline ).SetScene( rt_scene )
																.SetBuffer( sbt_buffer ).SetRayGenShader(RTShaderGroupID{"Main"})
																.AddMissShader(RTShaderGroupID{"PrimiryMiss"})
																.AddHitShader( GeometryID{"Triangle"}, RTShaderGroupID{"TriangleHit"} )
																.DependsOn( t_build_scene ));
			FG_UNUSED( t_update_table );

			CHECK_ERR( frame_graph->Execute() );
			CHECK_ERR( _frameGraphInst->EndFrame() );
		}

		// frame 2
		{
			CHECK_ERR( _frameGraphInst->BeginFrame( submission_graph ));
			CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));
			
			Task	t_build_geom	= frame_graph->AddTask( BuildRayTracingGeometry{}.SetTarget( rt_geometry ).Add( triangles ));
			Task	t_trace			= frame_graph->AddTask( TraceRays{}.SetPipeline( pipeline ).AddResources( ds_index, &resources )
																.SetGroupCount( view_size.x, view_size.y ).SetShaderTable( shader_table ).DependsOn( t_build_geom ));
			FG_UNUSED( t_trace );
			
			CHECK_ERR( frame_graph->Execute() );
			CHECK_ERR( _frameGraphInst->EndFrame() );
		}

		// frame 3
		{
			CHECK_ERR( _frameGraphInst->BeginFrame( submission_graph ));
			CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));
			
			Task	t_build_geom	= frame_graph->AddTask( BuildRayTracingGeometry{}.SetTarget( rt_geometry ).Add( triangles ));
			Task	t_trace			= frame_graph->AddTask( TraceRays{}.SetPipeline( pipeline ).AddResources( ds_index, &resources )
																.SetGroupCount( view_size.x, view_size.y ).SetShaderTable( shader_table ).DependsOn( t_build_geom ));
			Task	t_read			= frame_graph->AddTask( ReadImage{}.SetImage( dst_image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_trace ));
			FG_UNUSED( t_read );
			
			CHECK_ERR( frame_graph->Execute() );
			CHECK_ERR( _frameGraphInst->EndFrame() );
		}
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( _frameGraphInst->WaitIdle() );

		CHECK_ERR( data_is_correct );
		
		DeleteResources( pipeline, dst_image, rt_geometry, rt_scene, sbt_buffer );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG
