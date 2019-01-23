Shader debugging supported only for GLSL source. But you can use SPIRV-Cross to convert SPIRV binary into GLSL source.<br/>
For each shader, that you want to debug, add `EnableDebugTrace` flag:
```cpp
desc.AddShader( ... | EShaderLangFormat::EnableDebugTrace, ... );
```
Then setup debug callback:
```cpp
FrameGraphThread::SetShaderDebugCallback( ... );
```
For task that you want to debug add one of this:
```cpp
// record if {thread_x, thread_y, thread_z} == gl_GlobalInvocationID
DispatchCompute().EnableDebugTrace({ thread_x, thread_y, thread_z });

// record if {pixel_x, pixel_y} == gl_FragCoord.xy
DrawIndexed().EnableFragmentDebugTrace( pixel_x, pixel_y );

// record if launchID == gl_LaunchIDNV
TraceRays().EnableDebugTrace( EShaderStages::RayGen, { x, y, z });
```
Shader trace will be recorded only for selected thread/pixel/...
<br/>
Aloso you can enable shader recording during shader execution:
```cpp
desc.AddShader( ... | EShaderLangFormat::EnableDebugTrace, "main", R"#(
    // empty function will be replaced during shader compilation
    void dbg_EnableTraceRecording (bool b) {}
    
    void main ()
    {
        bool condition = ...
        
        // if condition is true then trace recording will started here
        dbg_EnableTraceRecording( condition );
        ...
    }
)#" );

// supports any shader stage
DrawIndexed().EnableDebugTrace( EShaderStages::Vertex | EShaderStages::Fragment );

TraceRays().EnableDebugTrace( EShaderStages::RayGen );

DispatchCompute().EnableDebugTrace();
```
<br/> 
At the end of frame device synchronized with host, shader trace parsed and callback will be called anyway.
If selected thread/pixel/... is never dispatched or drawed then `output` array in callback parameters will be empty.
<br/>
Example of shader trace:
```cpp
//> gl_GlobalInvocationID: uint3 {8, 8, 0}
//> gl_LocalInvocationID: uint3 {0, 0, 0}
//> gl_WorkGroupID: uint3 {1, 1, 0}
no source

//> index: uint {136}
//  gl_GlobalInvocationID: uint3 {8, 8, 0}
11. index = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;

//> size: uint {256}
12. size = gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_WorkGroupSize.x * gl_WorkGroupSize.y;

//> value: float {0.506611}
//  index: uint {136}
//  size: uint {256}
13. value = sin( float(index) / size );

//> imageStore(): void
//  gl_GlobalInvocationID: uint3 {8, 8, 0}
//  value: float {0.506611}
14.     imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), vec4(value) );
```
The `//>` symbol marks the modified variable.
