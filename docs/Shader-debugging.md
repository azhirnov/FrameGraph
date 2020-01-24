Shader debugging and profiling supported only for GLSL source. But you can use SPIRV-Cross to convert SPIRV binary into GLSL source.<br/>

## Shader debugging

For each shader, that you want to debug, add `EnableDebugTrace` flag:
```cpp
desc.AddShader( ... | EShaderLangFormat::EnableDebugTrace, ... );
```
Then setup debug callback:
```cpp
frameGraph.SetShaderDebugCallback( ... );
```
For task that you want to debug add one of this:
```cpp
// record if {thread_x, thread_y, thread_z} == gl_GlobalInvocationID
DispatchCompute().EnableDebugTrace({ thread_x, thread_y, thread_z });

// record if {pixel_x, pixel_y} == gl_FragCoord.xy
DrawIndexed().EnableFragmentDebugTrace( pixel_x, pixel_y );

// record if {launch_x, launch_y, launch_z} == gl_LaunchIDNV
TraceRays().EnableDebugTrace({ launch_x, launch_y, launch_z });
```
Shader trace will be recorded only for selected thread/pixel/launch.
<br/>
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

TraceRays().EnableDebugTrace();

DispatchCompute().EnableDebugTrace();
```

<br/> 
At the end of frame device synchronized with host, shader trace parsed and callback will be called anyway.
If selected thread/pixel/launch is never dispatched or drawed then `output` array in callback parameters will be empty.
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

The `//>` symbol marks the modified variable or function result.


## Shader profiling

For each shader, that you want to profile, add `EnableProfiling` flag:
```cpp
desc.AddShader( ... | EShaderLangFormat::EnableProfiling, ... );
```
Then setup debug callback:
```cpp
frameGraph.SetShaderDebugCallback( ... );
```
For task that you want to profile add one of this:
```cpp
// record if {thread_x, thread_y, thread_z} == gl_GlobalInvocationID
DispatchCompute().EnableShaderProfiling({ thread_x, thread_y, thread_z });

// record if {pixel_x, pixel_y} == gl_FragCoord.xy
DrawIndexed().EnableFragmentProfiling( pixel_x, pixel_y );

// record if {launch_x, launch_y, launch_z} == gl_LaunchIDNV
TraceRays().EnableShaderProfiling({ launch_x, launch_y, launch_z });
```
Shader profiling will be recorded only for selected thread/pixel/launch.
<br/>
<br/>
Aloso you can enable shader profiling during shader execution:
```cpp
desc.AddShader( ... | EShaderLangFormat::EnableProfiling, "main", R"#(
    // empty function will be replaced during shader compilation
    void dbg_EnableProfiling (bool b) {}
    
    void main ()
    {
        bool condition = ...
        
        // if condition is true then profiling will started here
        dbg_EnableProfiling( condition );
        ...
    }
)#" );

// supports any shader stage
DrawIndexed().EnableShaderProfiling( EShaderStages::Vertex | EShaderStages::Fragment );

TraceRays().EnableShaderProfiling();

DispatchCompute().EnableShaderProfiling();
```

Example of shader profiling output:
```cpp
//> gl_GlobalInvocationID: uint3 {512, 512, 0}
//> gl_LocalInvocationID: uint3 {0, 0, 0}
//> gl_WorkGroupID: uint3 {64, 64, 0}
no source

// subgroup total: 100.00%,  avr: 100.00%,  (95108.00)
// device   total: 100.00%,  avr: 100.00%,  (2452.00)
// invocations:    1
106. void main ()

// subgroup total: 89.57%,  avr: 89.57%,  (85192.00)
// device   total: 89.56%,  avr: 89.56%,  (2196.00)
// invocations:    1
29. float FBM (in float3 coord)

// subgroup total: 84.67%,  avr: 12.10%,  (11504.57)
// device   total: 84.18%,  avr: 12.03%,  (294.86)
// invocations:    7
56. float GradientNoise (const float3 pos)

// subgroup total: 45.15%,  avr: 0.81%,  (766.86)
// device   total: 44.54%,  avr: 0.80%,  (19.50)
// invocations:    56
72. float3 DHash33 (const float3 p)
```