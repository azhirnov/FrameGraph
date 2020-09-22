# Performance

## CPU overhead for barrier placement
For each render pass FrameGraph walk though the all draw tasks and accumulate pipeline barriers from resources in descriptor sets (`PipelineResources`), then put barriers before beginging render pass. Then FrameGraph walk though all draw tasks again and draw them.
So there is two cycles for all draw tasks that increaces CPU time around 30%, which can significantly reduce performance on thousands of draw calls. To avoid that use `CustomDraw` task and manually specify resources that have been modified before and will be used in draw commands.

## CPU overhead for pipeline creation
FrameGraph uses OpenGL-style pipelines that allows you to change render states for each draw call. FrameGraph calculates hash of render state, search for existing vulkan pipeline or create new pipeline if it doesn't exist. There are two bottlenecks, first is hashing and searching, second is pipeline creation that can lead to small lags, but desktop drivers always caches pipelines and second creation will be more faster.

## CPU overhead for descriptor set creation
FrameGraph allows you to change resources in `PipelineResources` as many times as you need, but for each draw task FrameGraph calculates hash of resources inside `PipelineResources` and searches for existing vulkan descriptor set or create new descriptor set.
The `PipelineResources` caches the last used descriptor set, so don't change state of `PipelineResources` and you will get maximum CPU performance.

## Immutable resources
Immutable resources allows FrameGraph to ignore them when it place pipeline barriers, this can improve CPU performance.</br>
All images are mutable because they require image layout transition for better performance.</br>
Buffers are mutable only if they have writable access in usage flags (`EBufferUsage`), these are `Storage`, `StorageTexel`, `TransferDst` and `RayTracing` flags.</br>
Ray tracing scene and geometry are always mutable.</br>
Descriptor set is immutable if it contains only immutable resources.</br>

## GPU overhead
FrameGraph tracks access to buffer ranges and put barriers only if you accesses the range that were changed before.</br>
FrameGraph tracks access to image array layers and mipmap levels, but not to 2D regions, so if you accesses the same layer and level but different 2D region FrameGraph will put barrier anyway. There is no specific functions to disable this kind of barriers, the only way is to merge copy, update, dispatch compute commands into a single task.</br>
Pipeline barriers prevent GPU command parallelization that increases execution time, so you should avoid unnecessary barriers.

## Memory managment overhead
FrameGraph uses [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) which has some specific behaviours. VMA immediatly releases memory page if all suballocations have been freed. If you frequently create and destroy buffers or images this may lead to frequently memory reallocations and hit performance. For example dedicated allocation on NVidia has very big CPU overhead.

## Future optimizations
1. Will be added support to secondary command buffers which speedup barrier placement for draw tasks by using single pass through all draw tasks instead of two.</br>
2. May be will be added deferred destruction for memory pages to avoid frequent reallocations.</br>
