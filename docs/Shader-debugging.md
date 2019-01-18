Shader debugging supported only for GLSL source. But you can use SPIRV-Cross to convert SPIRV binary into GLSL source.<br/>
For each shader, that you want to debug, add `EnableDebugTrace` flag:
```cpp
desc.AddShader( ... | EShaderLangFormat::EnableDebugTrace, ...
```
Then setup debug callback:
```cpp
FrameGraphThread::SetShaderDebugCallback( ... );
```
For task that you want to debug add:
```cpp
// record if {thread_x, thread_y, thread_z} == gl_GlobalInvocationID
DispatchCompute().EnableDebugTrace({ thread_x, thread_y, thread_z });

// record if {pixel_x, pixel_y} == gl_FragCoord.xy
DrawIndexed().EnableFragmentDebugTrace( pixel_x, pixel_y );

// record if vertex == gl_VertexIndex && instance == gl_InstanceIndex
DrawIndexed().EnableVertexDebugTrace( vertex, instance );

// record if invocation == gl_InvocationID && primitive == gl_PrimitiveID
DrawIndexed().EnableTessControlDebugTrace( invocation, primitive );

// record if primitive == gl_PrimitiveID
DrawIndexed().EnableTessEvaluationDebugTrace( primitive );

// record if invocation == gl_InvocationID && primitive == gl_PrimitiveID
DrawIndexed().EnableGeometryDebugTrace( invocation, primitive );

// record if invocation == gl_GlobalInvocationID.x
DrawMeshes().EnableMeshTaskDebugTrace( invocation );
DrawMeshes().EnableMeshDebugTrace( invocation );
```
Shader trace will be recorded only for selected thread/pixel/...
<br/> 
At the end of frame device synchronized with host, shader trace parsed and callback will be called anyway.
If selected thread/pixel/... is never dispatched or drawed then `output` array in callback will be empty.
<br/>
Example of shader trace:
```cpp
//> gl_GlobalInvocationID: uint3 {8, 8, 0}
no source

//> gl_LocalInvocationID: uint3 {0, 0, 0}
no source

//> gl_WorkGroupID: uint3 {1, 1, 0}
no source

//> index: uint {136}
//  gl_GlobalInvocationID: uint3 {8, 8, 0}
11. index	= gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;

//> size: uint {256}
12. size	= gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_WorkGroupSize.x * gl_WorkGroupSize.y;

//> value: float {0.506611}
//  index: uint {136}
//  size: uint {256}
13. value	= sin( float(index) / size );

//> imageStore(): void
//  gl_GlobalInvocationID: uint3 {8, 8, 0}
//  value: float {0.506611}
14. 	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), vec4(value) );
```
