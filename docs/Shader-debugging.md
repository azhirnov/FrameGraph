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
DispatchCompute().EnableDebugTrace({ thread_x, thread_y, 0 });

DrawIndexed().EnableFragmentDebugTrace( pixel_x, pixel_y );
```
Shader trace will be recorded only for selected thread/pixel/...
<br/> 
At the end of frame device synchronized with host, shader trace parsed and callback will be called anyway.
If selected thread/pixel/... is never dispatched or drawed then `output` array in callback will be empty.
