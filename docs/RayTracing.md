# How to use ray tracing extension

## Create pipeline

```cpp
// setup pipeline description
RayTracingPipelineDesc  ppln;

// add ray-generation shader
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
             /*sbtRecordOffset*/0, /*sbtRecordStride*/2, /*missIndex*/0,
             /*origin*/origin, /*Tmin*/0.0f, /*direction*/direction, /*Tmax*/10.0f,
             /*payload*/0 );

    imageStore( un_Output, ivec2(gl_LaunchIDNV), payload );
}
)#");

// add ray-miss shader
ppln.AddShader( RTShaderID("PrimiryMiss"), EShader::RayMiss, EShaderLangFormat::VKSL_110, "main", R"#(
#version 460 core
#extension GL_NV_ray_tracing : require
layout(location = 0) rayPayloadInNV vec4  payload;

void main ()
{
    payload = vec4(0.0f);
}
)#");

// add closest-hit shader
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

// setup shader groups
ppln.AddRayGenShader( RTShaderGroupID("Main"), RTShaderID("Main") );
ppln.AddRayMissShader( RTShaderGroupID("PrimiryMiss"), RTShaderID("PrimiryMiss") );
ppln.AddRayMissShader( RTShaderGroupID("SecondaryMiss"), RTShaderID("PrimiryMiss") );

// for triangle intersections you can set closest-hit shader and optionaly the any-hit shader
ppln.AddTriangleHitShaders( RTShaderGroupID("TriangleHit1"), RTShaderID("PrimiryHit") );
ppln.AddTriangleHitShaders( RTShaderGroupID("TriangleHit2"), RTShaderID("PrimiryHit") );

// create ray tracing pipeline
RTPipelineID  pipeline = fgThread->CreatePipeline( ppln );

// initialize pipeline resources
PipelineResources  resources;
fgThread->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources );
```

## Create scene for ray tracing

```cpp
// setup geometry description
RayTracingGeometryDesc::Triangles  triangle1_info;

// geometry ID needed to set hit shader for each geometry
triangle1_info.SetID( GeometryID{"Triangle1"} );

// one triangle with 'float3' vertex type
triangle1_info.SetVertices< float3 >( 3 );

// 3 indices with type 'uint'
triangle1_info.SetIndices( 3, EIndex::UInt );

// this geometry does not invoke the any-hit shaders
triangle1_info.AddFlags( ERayTracingGeometryFlags::Opaque );

// setup second geometry
RayTracingGeometryDesc::Triangles  triangle2_info;
triangle2_info.SetID( GeometryID{"Triangle2"} );
triangle2_info.SetVertices< float3 >( 3 );
triangle2_info.SetIndices( 3, EIndex::UInt );
triangle2_info.AddFlags( ERayTracingGeometryFlags::Opaque );

// create geometry for ray tracing scene
RTGeometryID rt_geometry = fgThread->CreateRayTracingGeometry(
                                 RayTracingGeometryDesc{{ triangle1_info, triangle2_info }} );

// create scene with one instance
RTSceneID rt_scene = fgThread->CreateRayTracingScene( RayTracingSceneDesc{ 1 });
```

## Build bottom-level acceleration structure
```cpp
// setup geometry data
BuildRayTracingGeometry::Triangles  triangle1;

// set geometry ID to mutch geometry data with a geometry description in 'rt_geometry'
triangle1.SetID( GeometryID{"Triangle1"} );

// set triangle vertices, must be no more than number of vertices in the geometry description.
// vertices will be copied to the staging buffer and used for acceleration structure building.
const auto  vertices = ArrayView<float3>{ { 0.25f, 0.25f, 0.0f }, { 0.75f, 0.25f, 0.0f }, { 0.50f, 0.75f, 0.0f } };
triangle1.SetVertices( vertices );

// set triangle indices, must be no more than number of indices in the geometry description.
// indices will be copied to the staging buffer too.
triangle1.SetIndices( indices );

// setup second geometry data
BuildRayTracingGeometry::Triangles  triangle2 = triangle1;
triangle2.SetID( GeometryID{"Triangle2"} );

// setup task
BuildRayTracingGeometry  build_blas;

// set the bottom-level acceleration structure (BLAS) where geometry data will be built
build_blas.SetTarget( rt_geometry );

// add geometries
build_blas.Add( triangle1 ).Add( triangle2 );

// enqueue task that builds the BLAS
Task t_build_geom = fgThread->AddTask( build_blas );
```

## Built top-level acceleration structure
```cpp
// setup geometry instance
BuildRayTracingScene::Instance  instance;

// one instance with geometry
instance.SetGeometry( rt_geometry );

// setup task
BuildRayTracingScene  build_tlas;

// set the top-level acceleration structure (TLAS) where instances will be built
build_tlas.SetTarget( rt_scene );

// add instance data
build_tlas.Add( instance );

// set number of hit shaders per geometry instance,
// for example: for primary ray and for shadow ray.
// 'sbtRecordStride' in ray-gen shader must have same value
build_tlas.SetHitShadersPerInstance( 2 );

// enqueue task that builds the TLAS.
// this task uses 'rt_geometry' and must wait until BLAS building is complete.
Task t_build_scene = fgThread->AddTask( build_tlas.DependsOn( t_build_geom ));
```

## Update shader binding table
```cpp
// shader binding table contains ray tracing shader handles that stored into device local buffer.
// all fields of 'shader_table' will be initialized in 'AddTask' method,
// but buffer data will be copied when task executes in the GPU.
TraceRays::ShaderTable  shader_table;
UpdateRayTracingShaderTable update_st{ OUT shader_table };

// set the ray tracing pipeline where shaders is.
update_st.SetPipeline( pipeline );

// set the ray tracing scene for which shader table will be created
update_st.SetScene( rt_scene );

// set the destination buffer.
// buffer must have enough space for all shaders.
update_st.SetBuffer( sbt_buffer );

// bind ray-generation shader group.
// this shader invoces for each thread
update_st.SetRayGenShader( RTShaderGroupID{"Main"} );

// add miss shader groups.
// the current shader selected in the ray-gen shader by passing missIndex into traceNV()
update_st.AddMissShader( RTShaderGroupID{"PrimiryMiss"} );    // for missIndex = 0
update_st.AddMissShader( RTShaderGroupID{"SecondaryMiss"} );  // for missIndex = 1

// add hit shader groups for each geometry instance, see the table below
update_st.AddHitShader( GeometryID{"Triangle1"}, RTShaderGroupID{"TriangleHit1"}, /*offset*/0 );	// (1)
update_st.AddHitShader( GeometryID{"Triangle1"}, RTShaderGroupID{"TriangleHit2"}, /*offset*/1 );	// (2)
update_st.AddHitShader( GeometryID{"Triangle2"}, RTShaderGroupID{"TriangleHit2"}, /*offset*/0 );	// (3)
update_st.AddHitShader( GeometryID{"Triangle2"}, RTShaderGroupID{"TriangleHit1"}, /*offset*/1 );	// (4)

// enqueue task, there is no dependency on the GPU side,
// but this task uses data that will be updated in the BuildRayTracingScene task, so dependency is needed.
Task  t_update_table = fgThread->AddTask( update_st.DependsOn( t_build_scene ));
```

## Shader binding table
| Shader groups → | ray-gen | miss group | miss group | hit group | hit group |
|---|---|---|---|---|---|
| Geometry ↓ | Main | PrimiryMiss | SecondaryMiss | TriangleHit1 | TriangleHit2 |
| `sbtRecordOffset = 0` |  |  |  |  |  |
| Triangle1 |  |  |  | (1) |  |
| Triangle2 |  |  |  |  | (3) |
| `sbtRecordOffset = 1` |  |  |  |  |  |
| Triangle1 |  |  |  |  | (4) |
| Triangle2 |  |  |  | (2) |  |
| `gl_LaunchIDNV` each thread | X | X |  |  |  |

* only the `PrimiryMiss` shader group executed for each thread because `missIndex` is always 0.
<br/>

## Trace rays
```cpp
resources.BindImage( UniformID("un_Output"), dst_image );
resources.BindRayTracingScene( UniformID("un_RtScene"), rt_scene );

TraceRays trace_rays;
trace_rays.AddResources( DescriptorSetID("0"), &resources );

// set the work group size
trace_rays.SetGroupCount( 800, 600 );

// set ray tracing shaders and wait until shader table is updating
trace_rays.SetShaderTable( shader_table ).DependsOn( t_update_table );

Task  t_trace = fgThread->AddTask( trace_rays );
```

