# Roadmap

## FrameGraph
### Stage 1
- [x] glsl compilation to spirv + reflection.<br/>
- [x] automatic placement of pipeline barrier.<br/>
- [x] unit tests: stl, resource barriers, glsl compiler, resource cache, ...<br/>
- [x] graph node tests: buffer copy, image copy, draw triangle, dispatch, ...<br/>
- [x] implementation tests for correct and optimal barrier placement.<br/>
- [x] multithreading.<br/>
- [x] different queue families support.<br/>
- [x] graph visualizations.<br/>

### Stage 2
- [x] mesh shader support.<br/>
- [x] ray tracing support.<br/>
- [x] glsl debugging.<br/>
- [x] dependencies between command buffers.<br/>
- [ ] custom RAM allocators.<br/>
- [ ] logical resources.<br/>
- [ ] advanced VRAM managment.<br/>
- [ ] render pass subpasses.<br/>
- [ ] immutable descriptor sets.<br/>

### Stage 3
- [ ] sparse memory support.<br/>
- [ ] optimization for mobile and integrated GPUs.<br/>


## Extensions
- [ ] Shader graph
- [ ] Script bindings
- [ ] Static analyzer for shader source