## Copy to host visible buffer
| Vulkan | FrameGraph |
|---|---|
| vkMapMemory | FrameGraphThread::UpdateHostBuffer() |


## Copy data between resources
| Vulkan | FrameGraph |
|---|---|
| vkCmdCopyBuffer | CopyBuffer task |
| vkCmdCopyBufferToImage | CopyBufferToImage task |
| vkCmdCopyImageToBuffer | CopyImageToBuffer task |
| vkCmdCopyImage | CopyImage task |
| vkCmdBlitImage | BlitImage task |
| vkResolveImage | ResolveImage task |
| vkCmdCopyBuffer from host to device | UpdateBuffer task |
| vkCmdCopyImage from host to device | UpdateImage task |
| vkCmdCopyBuffer from device to host | ReadBuffer task |
| vkCmdCopyImage from device to host | ReadImage task |


## Pipelines
| Vulkan | FrameGraph |
|---|---|
| vkCreateGraphicsPipelines | FrameGraphThread::CreatePipeline with GraphicsPipelineDesc that contains shaders and some states, RenderPassDesc contains all other render states, draw tasks contains frequently changed render states and dynamic states |
| vkCreateComputePipelines | FrameGraphThread::CreatePipeline with ComputePipelineDesc, compute tasks may override local group size |
| vkCreateGraphicsPipelines with mesh shader | FrameGraphThread::CreatePipeline with MeshPipelineDesc |
| vkCreateRayTracingPipelinesNV | FrameGraphThread::CreatePipeline with RayTracingPipelineDesc |


## Drawing
| Vulkan | FrameGraph |
|---|---|
| vkCmdDraw | DrawVertices task |
| vkCmdDrawIndexed | DrawIndexed task |
| vkCmdDrawIndirect | DrawVerticesIndirect task |
| vkCmdDrawIndexedIndirect | DrawIndexedIndirect task |
| vkCmdBindVertexBuffers | DrawTask::AddBuffer |
| vkCmdBindIndexBuffer | DrawIndexed::SetIndexBuffer or DrawIndexedIndirect::SetIndexBuffer |
| vkCmdDrawMeshTasksNV | DrawMeshes task |
| vkCmdDrawMeshTasksIndirectNV | DrawMeshesIndirect task |
| vkCmdSetScissor | DrawTask::AddScissor |
| vkCmdPushConstants | DrawTask::AddPushConstant |
| vkCmdSetStencilCompareMask | DrawTask::SetStencilCompareMask |
| vkCmdSetStencilReference | DrawTask::SetStencilReference |
| vkCmdSetStencilWriteMask | DrawTask::SetStencilWriteMask |
| vkCmdSetViewport | RenderPassDesc::AddViewport |
| vkCmdBeginRenderPass, vkCmdNextSubpass, vkCmdEndRenderPass | SubmitRenderPass task |
| vkCmdBindPipeline | DrawTask::SetPipeline |
| vkCmdBindDescriptorSets | DrawTask::AddResources |

`DrawTask` - may be any of DrawVertices, DrawIndexed, DrawVerticesIndirect, DrawIndexedIndirect, DrawMeshes, DrawMeshesIndirect.


## Computing
| Vulkan | FrameGraph |
|---|---|


## Descriptor set
| Vulkan | FrameGraph |
|---|---|

