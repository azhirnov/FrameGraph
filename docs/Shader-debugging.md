Shader debugging supported only for GLSL source. But you can use SPIRV-Cross to convert SPIRV binary into GLSL source.<br/>
For each shader you whant to debug add `EnableDebugTrace` flag:
```cpp
desc.AddShader( ... | EShaderLangFormat::EnableDebugTrace, ...
```
<br/>
Then setup debug callback:
```cpp
FrameGraphThread::SetShaderDebugCallback( ... );
```
<br/>
For task that you whant to debug add:
```cpp
DispatchCompute().EnableDebugTrace({ thread_x, thread_y, 0 });

DrawIndexed().EnableFragmentDebugTrace( pixel_x, pixel_y );
```
<br/>
Shader trace will be recorded only for selected thread/pixel/...
<br/> 
At the end of frame device synchronized with host, shader trace parsed and callback will be called anyway.
If selected thread/pixel/... is never dispatched or drawed then `output` array in callback will be empty.
