### Copy from host to device memory
| OpenGL | FrameGraph |
|---|---|
| glBufferData, glBufferSubData | UpdateBuffer task |
| glTexImage*, glTexSubImage*, glCompressedTexImage*. glCompressedTexSubImage* | UpdateImage task |
 
<br/><br/>
 
### Copy to host visible buffer
| OpenGL | FrameGraph |
|---|---|
| glMapBuffer, glMapBufferRange | FrameGraphThread::UpdateHostBuffer() |
 
<br/><br/>
 
### Copy from device to host memory
| OpenGL | FrameGraph |
|---|---|
| glGetBufferSubData | ReadBuffer task |
| glGetTextureSubImage | ReadImage task |
| glGetCompressedTextureSubImage | ReadImage task |
 
<br/><br/>
 
### Copy from buffer/image to image/buffer, clear buffer/image
| OpenGL | FrameGraph |
|---|---|
| glCopyBufferSubData | CopyBuffer task |
| glCopyImageSubData | CopyImage task |
| glBindBuffer(GL_PIXEL_UNPACK_BUFFER, ...) + glTexSubImage* | CopyBufferToImage task |
| glBindBuffer(GL_PIXEL_PACK_BUFFER, ...) + glGetTextureSubImage | CopyImageToBuffer task |
| glClearBufferData | FillBuffer task |
| glClearTexImage, glClearTexSubImage | ClearColorImage or ClearDepthStencilImage tasks |
| glBlitFramebuffer | BlitImage or ResolveImage tasks |
 
<br/><br/>
 
### Framebuffer and renderbuffer
| OpenGL | FrameGraph |
|---|---|
| glCreateFramebuffers, glCreateRenderbuffers | FrameGraphThread::CreateRenderPass |
| glFramebufferTexture* | RenderPassDesc::AddTarget |
| glClear, glClearColor, glClearDepth, glClearBuffer*, glClearNamedFramebuffer* | `clearValue` arg in RenderPassDesc::AddTarget |

<br/><br/>
 
### Render states
| OpenGL | FrameGraph |
|---|---|
| **color** |  |
| glBlendColor | RenderPassDesc::SetBlendColor |
| glBlendEquation*, glBlendEquationSeparate* | RenderPassDesc::AddColorBuffer or DrawTask::AddColorBuffer |
| glBlendFunc*, glBlendFuncSeparate* | RenderPassDesc::AddColorBuffer or DrawTask::AddColorBuffer |
| glColorMask | RenderPassDesc::AddColorBuffer or DrawTask::AddColorBuffer |
| glLogicOp | RenderPassDesc::SetLogicOp |
| **depth** | |
| glEnable(GL_DEPTH_TEST) | RenderPassDesc::SetDepthTestEnabled or DrawTask::SetDepthTestEnabled |
| glDepthMask | RenderPassDesc::SetDepthWriteEnabled or DrawTask::SetDepthWriteEnabled |
| glDepthFunc | RenderPassDesc::SetDepthCompareOp or DrawTask::SetDepthCompareOp |
| **stencil** | |
| glEnable(GL_STENCIL_TEST) | RenderPassDesc::SetStencilTestEnabled or DrawTask::SetStencilTestEnabled |
| glStencilOp, glStencilOpSeparate | RenderPassDesc::SetStencilFrontFaceFailOp, SetStencilFrontFaceDepthFailOp,  SetStencilFrontFacePassOp, SetStencilBackFaceFailOp, SetStencilBackFaceDepthFailOp, SetStencilBackFacePassOp |
| glStencilOp | DrawTask::SetStencilFailOp, SetStencilDepthFailOp, SetStencilPassOp |
| glStencilFunc, glStencilFuncSeparate | RenderPassDesc::SetStencilFrontFaceCompareOp, SetStencilFrontFaceReference, SetStencilFrontFaceCompareMask |
| glStencilFunc | DrawTask::SetStencilRef, SetStencilCompareMask |
| glStencilMask, glStencilMaskSeparate | RenderPassDesc::SetStencilFrontFaceWriteMask, SetStencilBackFaceWriteMask |
| glStencilMask | DrawTask::SetStencilWriteMask |
| **rasterizer** | |
| glEnable(GL_DEPTH_CLAMP) | RenderPassDesc::SetDepthClampEnabled |
| glPolygonOffset | RenderPassDesc::SetDepthBias or SetDepthBiasConstFactor, SetDepthBiasClamp, SetDepthBiasSlopeFactor |
| glPolygonMode | RenderPassDesc::SetPolygonMode |
| glLineWidth | RenderPassDesc::SetLineWidth |
| glEnable(GL_RASTERIZER_DISCARD) | RenderPassDesc::SetRasterizerDiscard or DrawTask::SetRasterizerDiscard |
| glCullFace | RenderPassDesc::SetCullMode or DrawTask::SetCullMode |
| glFrontFace | RenderPassDesc::SetFrontFaceCCW or DrawTask::SetFrontFaceCCW |
| **multisample** | |
| glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE) | RenderPassDesc::SetAlphaToCoverageEnabled |
| glEnable(GL_SAMPLE_ALPHA_TO_ONE) | RenderPassDesc::SetAlphaToOneEnabled |
| glEnable(GL_SAMPLE_SHADING) | RenderPassDesc::SetSampleShadingEnabled |
| glSampleMaski | RenderPassDesc::SetSampleMask, second parameter in SetMultiSamples |
| glMinSampleShading | RenderPassDesc::SetMinSampleShading |
| **viewport & scissor** | |
| glViewport*, glDepthRange* | RenderPassDesc::AddViewport |
| glScissor* | DrawTask::AddScissor |
 
`DrawTask` - may be any of DrawVertices, DrawIndexed, DrawVerticesIndirect, DrawIndexedIndirect, DrawMeshes, DrawMeshesIndirect.
<br/><br/>
 
### Drawing
| OpenGL | FrameGraph |
|---|---|
| glDrawArrays, glDrawArraysInstanced, glDrawArraysInstancedBaseInstance | DrawVertices task |
| glDrawElements, glDrawElementsInstanced, glDrawElementsBaseVertex | DrawIndexed task |
| glDrawElementsInstancedBaseInstance, glDrawElementsInstancedBaseVertex | DrawIndexed task |
| glDrawElementsInstancedBaseVertexBaseInstance | DrawIndexed task |
| glDrawRangeElements, glDrawRangeElementsBaseVertex | DrawIndexed task |
| glDrawArraysIndirect | DrawVerticesInstanced task |
| glDrawElementsIndirect | DrawIndexedInstanced task |
| glMultiDrawArrays | DrawVertices task |
| glMultiDrawArraysIndirect, glMultiDrawElementsIndirect | none |
| glMultiDrawElements, glMultiDrawElementsBaseVertex | DrawIndexed task |
 
<br/><br/>
 
### Computing
| OpenGL | FrameGraph |
|---|---|
| glDispatchCompute | DispatchCompute task |
| glDispatchComputeIndirect | DispatchComputeIndirect task |
 
<br/><br/>
 
### Uniforms
| OpenGL | FrameGraph |
|---|---|
| glUniform* | not supported |
| glUniformBlockBinding | not supported |
| glBindBufferBase, glBindBufferRange | PipelineResources::BindBuffer |
| glBindTextureUnit | PipelineResources::BindTexture |
| glBindImageTexture | PipelineResources::BindImage |
| glBindSampler | PipelineResources::BindSampler, BindTexture |

use `DrawTask::AddResources` or `ComputeTask::AddResources` for applying uniforms to draw/compute command.
<br/><br/>
 
### Vertex & index buffers, vertex attributes
| OpenGL | FrameGraph |
|---|---|
| glBindVertexArray | DrawTask::SetVertexInput |
| glBindVertexBuffer | DrawTask::AddBuffer |
| glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...) | DrawTask::SetIndexBuffer |
 
`DrawTask` - may be any of DrawVertices, DrawIndexed, DrawVerticesIndirect, DrawIndexedIndirect.
<br/>
