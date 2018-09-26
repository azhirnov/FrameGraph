# FrameGraph
Work in progress.

## Building
Generate project with CMake and build.<br/>
Required C++17 standard support.

CMake version 3.11 and greater will download all dependencies during configuration time.<br/>
If it didn't, manualy download dependencies into 'external' directory or in cmake specify FG_EXTERNAL_xxx pathes for each dependency.

Dependencies:<br/>
[Vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers) or [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) - required.<br/>
[glfw](https://github.com/glfw/glfw) or [SDL2](https://www.libsdl.org) or [SFML](https://github.com/SFML/SFML) - required for framework and some tests.<br/>
[glslang](https://github.com/KhronosGroup/glslang) - required for glsl compiler.<br/>
[SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools), [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers) and Python - optional, for spirv optimization.<br/>
[VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - will be required later.<br/>
[lodepng](https://github.com/lvandeve/lodepng) - optional.<br/>

## Roadmap
### Stage 1
- glsl compilation to spirv + reflection. (done)<br/>
- auto pipeline barrier placement. (done)<br/>
- render pass optimization. (WIP)<br/>
- unit tests: stl, resource barriers, glsl compiler, resource cache, ... (WIP)<br/>
- graph node tests: buffer copy, image copy, draw triangle, dispatch, ... (WIP)<br/>
- implementation tests:<br/>
    correct and optimal barrier placement. (WIP)<br/>
    correct render pass optimization. (WIP)

### Stage 2
- performance tests. (WIP)<br/>
- logical resources.<br/>
- advanced gpu memory managment.<br/>
- multithreading.<br/>
- samples.<br/>

### Stage 3
- multi-gpu.<br/>
- WebGL 2.0 (or WebGL-next) support.<br/>
