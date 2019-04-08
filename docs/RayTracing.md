# How to use ray tracing extension

## Create pipeline

```cpp
// setup pipeline description
RayTracingPipelineDesc  ppln;

const String header = R"#(
#version 460 core
#extension GL_NV_ray_tracing : require
#define PRIMARY_RAY_LOC	0
#define SHADOW_RAY_LOC	1

struct PrimaryRayPayload {
    vec4  color;
};
struct ShadowRayPayload {
    float  distance;
};
)#";

// add ray-generation shader
ppln.AddShader( RTShaderID("Main"), EShader::RayGen, EShaderLangFormat::VKSL_110, "main", R"#(
layout(binding = 0) uniform accelerationStructureNV  un_RtScene;
layout(binding = 1, rgba8) writeonly uniform image2D  un_Output;
layout(location = PRIMARY_RAY_LOC) rayPayloadNV PrimaryRayPayload  PrimaryRay;
layout(location = SHADOW_RAY_LOC)  rayPayloadNV ShadowRayPayload   ShadowRay;

layout (constant_id = 0) const int sbtRecordStride = 1;
	
void main ()
{
    const vec2 uv = vec2(gl_LaunchIDNV.xy) / vec2(gl_LaunchSizeNV.xy - 1);

    const vec3 origin = vec3(uv.x, 1.0f - uv.y, -1.0f);
    const vec3 direction = vec3(0.0f, 0.0f, 1.0f);

    traceNV( /*topLevel*/un_RtScene, /*rayFlags*/gl_RayFlagsNoneNV, /*cullMask*/0xFF,
             /*sbtRecordOffset*/0, sbtRecordStride, /*missIndex*/0,
             /*origin*/origin, /*Tmin*/0.0f, /*direction*/direction, /*Tmax*/10.0f,
             /*payload*/PRIMARY_RAY_LOC );

    imageStore( un_Output, ivec2(gl_LaunchIDNV), PrimaryRay.color );
}
)#");

// add ray-miss shader for primary ray
ppln.AddShader( RTShaderID("PrimaryMiss"), EShader::RayMiss, EShaderLangFormat::VKSL_110, "main", R"#(
layout(location = PRIMARY_RAY_LOC) rayPayloadInNV PrimaryRayPayload  PrimaryRay;

void main ()
{
    PrimaryRay.color = vec4(0.0f);
}
)#");

// add ray-miss shader for shadow ray
ppln.AddShader( RTShaderID("ShadowMiss"), EShader::RayMiss, EShaderLangFormat::VKSL_110, "main", R"#(
layout(location = SHADOW_RAY_LOC) rayPayloadInNV ShadowRayPayload  ShadowRay;

void main ()
{
	ShadowRay.distance = gl_RayTmaxNV;
}
)#");

// add closest-hit shader for primary ray
ppln.AddShader( RTShaderID("PrimaryHit1"), EShader::RayClosestHit, EShaderLangFormat::VKSL_110, "main", R"#(
layout(location = PRIMARY_RAY_LOC) rayPayloadInNV PrimaryRayPayload  PrimaryRay;

hitAttributeNV vec2  hitAttribs;

void main ()
{
    const vec3 barycentrics = vec3(1.0f - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);
    PrimaryRay.color = vec4(barycentrics, 1.0);
}
)#");

ppln.AddShader( RTShaderID("PrimaryHit2"), EShader::RayClosestHit, EShaderLangFormat::VKSL_110, "main", R"#(
layout(location = PRIMARY_RAY_LOC) rayPayloadInNV PrimaryRayPayload  PrimaryRay;

void main ()
{
    PrimaryRay.color = vec4(1.0f);
}
)#");

// add closest-hit shader for shadow ray
ppln.AddShader( RTShaderID("ShadowHit"), EShader::RayClosestHit, EShaderLangFormat::VKSL_110, "main", R"#(
layout(location = SHADOW_RAY_LOC) rayPayloadInNV ShadowRayPayload  ShadowRay;

hitAttributeNV vec2  hitAttribs;

void main ()
{
	ShadowRay.distance = gl_HitTNV;
}
)#");

// create ray tracing pipeline
RTPipelineID  pipeline = frameGraph->CreatePipeline( ppln );

// initialize pipeline resources
PipelineResources  resources;
frameGraph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources );
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
triangle1_info.SetIndices< uint >( 3 );

// this geometry does not invoke the any-hit shaders
triangle1_info.AddFlags( ERayTracingGeometryFlags::Opaque );

// setup second geometry
RayTracingGeometryDesc::Triangles  triangle2_info;
triangle2_info.SetID( GeometryID{"Triangle2"} );
triangle2_info.SetVertices< float3 >( 3 );
triangle2_info.SetIndices( 3, EIndex::UInt );
triangle2_info.AddFlags( ERayTracingGeometryFlags::Opaque );

// create geometry for ray tracing scene
RTGeometryID  rt_geometry = frameGraph->CreateRayTracingGeometry(
                                 RayTracingGeometryDesc{{ triangle1_info, triangle2_info }} );

// create scene with one instance
RTSceneID  rt_scene = frameGraph->CreateRayTracingScene( RayTracingSceneDesc{ 1 });
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
const auto  indices = ArrayView<uint>{ 0, 1, 2 };
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
Task  t_build_geom = cmdBuffer->AddTask( build_blas );
```

## Build top-level acceleration structure
```cpp
// setup geometry instance
BuildRayTracingScene::Instance  instance;
instance.SetID( InstanceID{"First"} );

// one instance with geometry
instance.SetGeometry( rt_geometry );

// setup task
BuildRayTracingScene  build_tlas;

// set the top-level acceleration structure (TLAS) where instances will be built
build_tlas.SetTarget( rt_scene );

// add instance data
build_tlas.Add( instance );

// add second instance
instance.SetID( InstanceID{"Second"} );
build_tlas.Add( instance );

// set number of hit shaders per geometry instance,
// for example: for primary ray and for shadow ray.
// specialization constant 'sbtRecordStride' in ray-gen shader will be initialized with same value.
build_tlas.SetHitShadersPerInstance( 2 );

// enqueue task that builds the TLAS.
// this task uses 'rt_geometry' and must wait until BLAS building is complete.
Task  t_build_scene = cmdBuffer->AddTask( build_tlas.DependsOn( t_build_geom ));
```

## Update shader binding table
```cpp
// create shader binding table
RTShaderTableID  rt_shaders = frameGraph->CreateRayTracingShaderTable();

// setup task
UpdateRayTracingShaderTable update_st;

// set the ray tracing pipeline where shaders is.
update_st.SetPipeline( pipeline );

// set the ray tracing scene for which shader table will be created
update_st.SetScene( rt_scene );

// set the shader binging table that will be updated
update_st.SetTraget( rt_shaders );

// bind ray-generation shader.
// this shader invoces for each thread
update_st.SetRayGenShader( RTShaderGroupID{"Main"} );

// bind ray-miss shaders.
// the current shader selected in the ray-gen shader by passing missIndex into the 'traceNV()'
update_st.AddMissShader( RTShaderID{"PrimaryMiss"}, 0 );  // for missIndex = 0
update_st.AddMissShader( RTShaderID{"ShadowMiss"},  1 );  // for missIndex = 1

// bind ray-hit shader for each geometry instance
update_st.AddHitShader( InstanceID{"First"},  GeometryID{"Triangle1"}, /*offset*/0, RTShaderID{"PrimaryHit1"} );  // (3)
update_st.AddHitShader( InstanceID{"First"},  GeometryID{"Triangle1"}, /*offset*/1, RTShaderID{"ShadowHit"} );    // (4)
update_st.AddHitShader( InstanceID{"First"},  GeometryID{"Triangle2"}, /*offset*/0, RTShaderID{"PrimaryHit2"} );  // (5)
update_st.AddHitShader( InstanceID{"First"},  GeometryID{"Triangle2"}, /*offset*/1, RTShaderID{"ShadowHit"} );    // (6)
update_st.AddHitShader( InstanceID{"Second"}, GeometryID{"Triangle1"}, /*offset*/0, RTShaderID{"PrimaryHit2"} );  // (7)
update_st.AddHitShader( InstanceID{"Second"}, GeometryID{"Triangle1"}, /*offset*/1, RTShaderID{"ShadowHit"} );    // (8)
update_st.AddHitShader( InstanceID{"Second"}, GeometryID{"Triangle2"}, /*offset*/0, RTShaderID{"PrimaryHit1"} );  // (9)
update_st.AddHitShader( InstanceID{"Second"}, GeometryID{"Triangle2"}, /*offset*/1, RTShaderID{"ShadowHit"} );    // (10)

// enqueue task, there is no dependency on the GPU side,
// but this task uses data that will be updated in the BuildRayTracingScene task, so dependency is needed.
Task  t_update_table = cmdBuffer->AddTask( update_st.DependsOn( t_build_scene ));
```

## Shader binding table
| Shader groups → | raygen group | miss group | miss group | hit group | hit group | hit group | hit group |
|---|---|---|---|---|---|---|---|
| Instance and Geometry ↓ | Main | PrimaryMiss | ShadowMiss | PrimaryHit1 | ShadowHit | PrimaryHit2 | ShadowHit |
| primary ray with `sbtRecordOffset=0` | 
| First, Triangle1  |   |  |  | (3) |     |     |     |
| First, Triangle2  |   |  |  |     |     | (5) |     |
| shadow ray with `sbtRecordOffset=1` |
| First, Triangle1  |   |  |  |     | (4) |     |     |
| First, Triangle2  |   |  |  |     |     |     | (6) |
| primary ray with `sbtRecordOffset=0` |
| Second, Triangle1 |   |  |  |     |     | (7) |     |
| Second, Triangle2 |   |  |  | (9) |     |     |     |
| shadow ray with `sbtRecordOffset=1` |
| Second, Triangle1 |   |  |  |     | (8) |     |     |
| Second, Triangle2 |   |  |  |     |     |     | (10) |
| each thread       | X |
| primary ray with `missIndex=0` |  | (1) |  |  |  |  |
| shadow ray with `missIndex=1` |  |  | (2) |  |  |  |

`sbtRecordOffset` and `missIndex` are the arguments to the `traceNV()` function.


## Trace rays
```cpp
resources.BindImage( UniformID("un_Output"), dst_image );
resources.BindRayTracingScene( UniformID("un_RtScene"), rt_scene );

TraceRays  trace_rays;
trace_rays.AddResources( DescriptorSetID("0"), &resources );

// set the work group size
trace_rays.SetGroupCount( 800, 600 );

// set ray tracing shaders and wait until shader table is updating
trace_rays.SetShaderTable( rt_shaders ).DependsOn( t_update_table );

Task  t_trace = cmdBuffer->AddTask( trace_rays );
```

## Performance
Method `CreateRayTracingGeometry` and tasks `UpdateRayTracingShaderTable`, `BuildRayTracingGeometry`, `BuildRayTracingScene`
has CPU overhead for sorting and searching operations. Task `TraceRays` works as fast as possible.
