# FrameGraph
Work in progress.

## Features
* multithreaded command buffer building and submitting.
* frame graph visualization.

## Suported Platforms
* Windows (with MSVC 2017)
* Linux (with GCC 7.3)

## Branches
`dev` - development branch<br/>
`master` - contains last stable version<br/>
`single_threaded` - deprecated single threaded stable version

## Building
Generate project with CMake and build.<br/>
Required C++17 standard support.

CMake version 3.11 and greater will download all dependencies during configuration time.<br/>
If it didn't, manualy download dependencies into 'external' directory or in cmake specify FG_EXTERNAL_*** pathes for each dependency.

Dependencies:<br/>
[Vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers) or [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) - required.<br/>
[glfw](https://github.com/glfw/glfw) or [SDL2](https://www.libsdl.org) or [SFML](https://github.com/SFML/SFML) - required for framework and some tests.<br/>
[glslang](https://github.com/KhronosGroup/glslang) - required for glsl compiler.<br/>
[SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools), [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers) and Python - optional, for spirv optimization.<br/>
[VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - required.<br/>
[lodepng](https://github.com/lvandeve/lodepng) - optional.<br/>
[graphviz](https://www.graphviz.org/) - (optional) for graph visualization.<br/>

## Roadmap
### Stage 1
- glsl compilation to spirv + reflection. (done)<br/>
- auto pipeline barrier placement. (WIP)<br/>
- render pass optimization. (WIP)<br/>
- unit tests: stl, resource barriers, glsl compiler, resource cache, ... (WIP)<br/>
- graph node tests: buffer copy, image copy, draw triangle, dispatch, ... (WIP)<br/>
- implementation tests:<br/>
    correct and optimal barrier placement. (WIP)<br/>
    correct render pass optimization. (WIP)

### Stage 2
- multithreading. (WIP)<br/>
- performance tests. (WIP)<br/>
- logical resources.<br/>
- advanced gpu memory managment.<br/>
- samples.<br/>

### Stage 3
- multi-gpu.<br/>
- WebGL 2.0 (or WebGL-next) support.<br/>

## References
1. FrameGraph in Frostbite.<br/>
https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in

2. Handles vs pointers.<br/>
https://floooh.github.io/2018/06/17/handles-vs-pointers.html
