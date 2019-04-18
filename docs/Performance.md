# Performance

## CPU overhead for the automatic placement of the pipeline barriers
For each render pass FrameGraph walk through the all draw tasks and place barriers for resources in the descriptor sets (PipelineResources) if it needed.
To optimize this case use immutable resources (see below) or use `CustomDraw` task.

## Imutable resources
All images are mutable because they require image layout transition for better performance.</br>
Buffers are mutable only if they have writable access in usage flags (`EBufferUsage`), there are `Storage` and `TransferDst` flags.</br>
Ray tracing scene and geometry are already mutable.</br>
Descriptor set is immutable if it contains only immutable resources (work in progress).</br>
