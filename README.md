# FrameGraph
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](paypal.me/azhirnovgithub)<br/>

Work in progress.

## Features
* multithreaded command buffer building and submitting.
* simple API, hides memory allocation, host to/from device transfers, synchronizations.
* frame graph visualization.
* tested on DOOM trace (see [vkTraceConverter examples](https://github.com/azhirnov/vkTraceConverter/tree/dev/examples#convert-to-framegraph-trace)).
* glsl debugging.
* supports RTX extensions.

## Documentation
* [Overview](docs/Overview.md)
* [Porting from OpenGL](docs/Porting-from-OpenGL.md)
* [Porting from Vulkan](docs/Porting-from-Vulkan.md)
* [Extensions overview](extensions/Readme.md)
* [Shader debugging](docs/Shader-debugging.md)
* [Graph visualization](docs/Graph-visualization.md)

## Suported Platforms
* Windows (with MSVC 2017)
* Linux (with GCC 8.2)

## Building
Generate project with CMake and build.<br/>
Required C++17 standard support.

CMake version 3.11 and greater will download all dependencies during configuration time.<br/>
If it didn't, manualy download dependencies into 'external' directory or in cmake specify `FG_EXTERNAL_***` pathes for each dependency.

Dependencies:<br/>
[Vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers) or [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) - required.<br/>
[glfw](https://github.com/glfw/glfw) or [SDL2](https://www.libsdl.org) or [SFML](https://github.com/SFML/SFML) - required for framework and some tests.<br/>
[glslang](https://github.com/KhronosGroup/glslang) - required for glsl compiler.<br/>
[SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools), [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers) and Python - optional, for spirv optimization.<br/>
[VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - required.<br/>
[lodepng](https://github.com/lvandeve/lodepng) - optional.<br/>
[graphviz](https://www.graphviz.org/) - (optional) for graph visualization.<br/>
[Assimp](https://github.com/assimp/assimp) - (optional) for Scene extension.<br/>
[DevIL](http://openil.sourceforge.net/) - (optional) for Scene extension.<br/>
[FreeImage](http://freeimage.sourceforge.net/) - (optional) for Scene extension.<br/>
[imgui](https://github.com/ocornut/imgui) - (optional) for UI extension.<br/>

## Roadmap
### Stage 1
- glsl compilation to spirv + reflection. (done)<br/>
- automatic placement of pipeline barrier. (done<br/>
- unit tests: stl, resource barriers, glsl compiler, resource cache, ... (done)<br/>
- graph node tests: buffer copy, image copy, draw triangle, dispatch, ... (done)<br/>
- implementation tests:<br/>
    correct and optimal barrier placement. (done)<br/>
    correct render pass optimization. (WIP)
- multithreading. (done)<br/>
- render pass subpasses. (WIP)<br/>
- queue family transfer. (WIP)<br/>
- graph visualizations. (done, but should be improved)<br/>

### Stage 2
- mesh shader support. (done)<br/>
- ray tracing support. (done)<br/>
- glsl debugging. (done)<br/>
- sparse memory support.<br/>
- custom RAM allocators.<br/>
- logical resources.<br/>
- advanced VRAM managment.<br/>
- optimization for mobile and integrated GPUs.<br/>
- documentation.<br/>
- samples.<br/>
- performance tests:<br/>
	for samples.<br/>
	use [vkTraceConverter](https://github.com/azhirnov/vkTraceConverter) to generate FG API calls and run perftests with any vktrace. (WIP)<br/>
- multithreading optimizations:<br/>
	remove synchronization stage<br/>
	lockfree image view map<br/>
	lockfree pipelines, descriptor set, layout, ... maps<br/>

### Stage 3
- multi-gpu.<br/>
- WebGL 2.0 (or WebGL-next) support.<br/>

## References
1. [FrameGraph in Frostbite](https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in).<br/>
2. [Handles vs pointers](https://floooh.github.io/2018/06/17/handles-vs-pointers.html)<br/>
3. [Porting engine to vulkan](https://gpuopen.com/presentation-porting-engine-to-vulkan-dx12/).<br/>
