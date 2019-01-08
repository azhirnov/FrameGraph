Copy from host to device memory
```cpp
glBufferData, glBufferSubData -> UpdateBuffer task
glTexImage*, glTexSubImage*, glCompressedTexImage*. glCompressedTexSubImage* -> UpdateImage task
```
<br/>
Copy to host visible buffer
```cpp
glMapBuffer, glMapBufferRange -> FrameGraphThread::UpdateHostBuffer()
```
<br/>
Copy from device to host memory
```cpp
glGetBufferSubData -> ReadBuffer task
glGetTextureSubImage -> ReadImage task
glGetCompressedTextureSubImage -> ReadImage task
```
<br/>
Copy from buffer/image to image/buffer and clear buffer/image
```cpp
glCopyBufferSubData -> CopyBuffer task
glCopyImageSubData -> CopyImage task
glBindBuffer(GL_PIXEL_UNPACK_BUFFER, ...) + glTexSubImage* -> CopyBufferToImage task
glBindBuffer(GL_PIXEL_PACK_BUFFER, ...) + glGetTextureSubImage -> CopyImageToBuffer task
glClearBufferData -> FillBuffer task
glClearTexImage, glClearTexSubImage -> ClearColorImage or ClearDepthStencilImage tasks
glBlitFramebuffer -> BlitImage or ResolveImage tasks
```
<br/>
Framebuffer and renderbuffer
```cpp
FrameGraphThread::CreateRenderPass & SubmitRenderPass task replaces:
glCreateFramebuffers, glBindFramebuffer,
glFramebufferTexture*,
glClear, glClearColor, glClearDepth, glClearBuffer*, glClearNamedFramebuffer*
```
<br/>
Render states
```cpp
// color
glBlendColor -> RenderPassDesc::SetBlendColor
glBlendEquation*, glBlendEquationSeparate* -> RenderPassDesc::AddColorBuffer or DrawTask::AddColorBuffer
glBlendFunc*, glBlendFuncSeparate* -> RenderPassDesc::AddColorBuffer or DrawTask::AddColorBuffer
glColorMask -> RenderPassDesc::AddColorBuffer or DrawTask::AddColorBuffer
glLogicOp -> RenderPassDesc::SetLogicOp
// depth
glEnable(GL_DEPTH_TEST) -> RenderPassDesc::SetDepthTestEnabled or DrawTask::SetDepthTestEnabled
glDepthMask -> RenderPassDesc::SetDepthWriteEnabled or DrawTask::SetDepthWriteEnabled
glDepthFunc -> RenderPassDesc::SetDepthCompareOp or DrawTask::SetDepthCompareOp
// stencil
glEnable(GL_STENCIL_TEST) -> RenderPassDesc::SetStencilTestEnabled or DrawTask::SetStencilTestEnabled
glStencilOp, glStencilOpSeparate -> RenderPassDesc::SetStencilFrontFaceFailOp, SetStencilFrontFaceDepthFailOp, SetStencilFrontFacePassOp, SetStencilBackFaceFailOp, SetStencilBackFaceDepthFailOp, SetStencilBackFacePassOp
glStencilOp -> DrawTask::SetStencilFailOp, SetStencilDepthFailOp, SetStencilPassOp
glStencilFunc, glStencilFuncSeparate -> RenderPassDesc::SetStencilFrontFaceCompareOp, SetStencilFrontFaceReference, SetStencilFrontFaceCompareMask
glStencilFunc -> DrawTask::SetStencilRef, SetStencilCompareMask
glStencilMask, glStencilMaskSeparate -> RenderPassDesc::SetStencilFrontFaceWriteMask, SetStencilBackFaceWriteMask
glStencilMask -> DrawTask::SetStencilWriteMask
// rasterizer
glEnable(GL_DEPTH_CLAMP) -> RenderPassDesc::SetDepthClampEnabled
glPolygonOffset -> RenderPassDesc::SetDepthBias or SetDepthBiasConstFactor, SetDepthBiasClamp, SetDepthBiasSlopeFactor
glPolygonMode -> RenderPassDesc::SetPolygonMode
glLineWidth -> RenderPassDesc::SetLineWidth
glEnable(GL_RASTERIZER_DISCARD) -> RenderPassDesc::SetRasterizerDiscard or DrawTask::SetRasterizerDiscard
glCullFace -> RenderPassDesc::SetCullMode or DrawTask::SetCullMode
glFrontFace -> RenderPassDesc::SetFrontFaceCCW or DrawTask::SetFrontFaceCCW
// multisample
glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE) -> RenderPassDesc::SetAlphaToCoverageEnabled
glEnable(GL_SAMPLE_ALPHA_TO_ONE) -> RenderPassDesc::SetAlphaToOneEnabled
glEnable(GL_SAMPLE_SHADING) -> RenderPassDesc::SetSampleShadingEnabled
glSampleMaski -> RenderPassDesc::SetSampleMask, second parameter in SetMultiSamples
glMinSampleShading -> RenderPassDesc::SetMinSampleShading
// viewport & scissor
glViewport*, glDepthRange* -> RenderPassDesc::AddViewport
glScissor* -> DrawTask::AddScissor
```
<br/>
Drawing
```cpp
glDrawArrays, glDrawArraysInstanced, glDrawArraysInstancedBaseInstance -> DrawVertices task
glDrawElements, glDrawElementsInstanced, glDrawElementsBaseVertex -> DrawIndexed task
glDrawElementsInstancedBaseInstance, glDrawElementsInstancedBaseVertex -> DrawIndexed task
glDrawElementsInstancedBaseVertexBaseInstance -> DrawIndexed task
glDrawRangeElements, glDrawRangeElementsBaseVertex -> DrawIndexed task
glDrawArraysIndirect -> DrawVerticesInstanced task
glDrawElementsIndirect -> DrawIndexedInstanced task
glMultiDrawArrays -> DrawVertices task
glMultiDrawArraysIndirect, glMultiDrawElementsIndirect -> none
glMultiDrawElements, glMultiDrawElementsBaseVertex -> DrawIndexed task
```
<br/>
Computing
```cpp
glDispatchCompute -> DispatchCompute task
glDispatchComputeIndirect -> DispatchComputeIndirect task
```
<br/>
Uniforms
```cpp
glUniform* -> not supported
glUniformBlockBinding -> not supported
glBindBufferBase, glBindBufferRange -> PipelineResources::BindBuffer
glBindTextureUnit -> PipelineResources::BindTexture
glBindImageTexture -> PipelineResources::BindImage
glBindSampler -> PipelineResources::BindSampler, BindTexture
// use DrawTask::AddResources or ComputeTask::AddResources for applying uniforms to draw/compute command.
```
<br/>
Vertex & index buffers, vertex attributes
```cpp
glBindVertexArray -> DrawTask::SetVertexInput
glBindVertexBuffer -> DrawTask::AddBuffer
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...) -> DrawTask::SetIndexBuffer
```