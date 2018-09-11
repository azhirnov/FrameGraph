# FrameGraph
Work in progress.

## Building
Generate project with CMake and build.
C++17 standard support required.

CMake version 3.11 and greater will download all dependencies on configuration time. If not, download dependencies into 'external' directory or specify FG_EXTERNAL_PATH in cmake.

Dependencies:<br/>
[Vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers) or [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) - required.<br/>
[glfw](https://github.com/glfw/glfw) or [SDL2](https://www.libsdl.org) - required for framework.<br/>
[glslang](https://github.com/KhronosGroup/glslang) - will be required later.<br/>
[VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - will be required later.<br/>
[lodepng](https://github.com/lvandeve/lodepng) - optional.<br/>

## Roadmap
### Stage 1
- glsl compilation to spirv + reflection.<br/>
- auto pipeline barrier placement.<br/>
- render pass optimization.<br/>
- unit tests (stl, resource barriers, glsl compiler, resource cache, ...)<br/>
- graph node tests (buffer copy, image copy, draw triangle, dispatch, ...)<br/>
- implementation tests:<br/>
    correct and optimal barrier placement.<br/>
    correct render pass optimization.

### Stage 2
- performance tests.<br/>
- logical resources.<br/>
- advanced gpu memory managment.<br/>
- multithreading.<br/>
- samples.<br/>

### Stage 3
- multi-gpu.<br/>
- WebGL 2.0 (or WebGL-next) support.<br/>
