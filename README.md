[![Build Status](https://api.travis-ci.com/azhirnov/FrameGraph.svg?branch=dev)](https://travis-ci.com/azhirnov/FrameGraph)
[![Discord Chat](https://img.shields.io/discord/651853834246815744.svg)](https://discord.gg/cMW955R)

# FrameGraph

FrameGraph simplifies prototyping on Vulkan and can be used as a base layer for the graphics engine.
FrameGraph designed for maximum performance but not at the expense of usability. API developed as simple as possible, it hides all synchronizations, memory allocations and all this boilerplate code that is needed to get Vulkan work. Builtin validations together with Vulkan validation layers allow you to quickly find and fix errors.

## Features
* multithreaded command buffer building and submission.
* simple API, hides memory allocation, host<->device transfers, synchronizations.
* shader debugging and profiling.
* supports RTX extensions.
* supports async compute and async transfer queues.
* all render tasks are stateless.

Used vulkan features and extensions:
* push constants
* dynamic buffer offset
* specialization constants
* VK_EXT_debug_utils
* VK_KHR_dedicated_allocation
* VK_EXT_descriptor_indexing
* VK_NV_mesh_shader
* VK_NV_shading_rate_image
* VK_NV_ray_tracing
* VK_KHR_shader_clock

## Samples
[FrameGraph-Samples](https://github.com/azhirnov/FrameGraph-Samples)

## Documentation
* [Introduction](docs/Introduction.md)
* [Multithreading](docs/Multithreading.md)
* [Performance](docs/Performance.md)
* [Ray tracing](docs/RayTracing.md)
* [Porting from OpenGL](docs/Porting-from-OpenGL.md)
* [Porting from Vulkan](docs/Porting-from-Vulkan.md)
* [Extensions overview](extensions/Readme.md)
* [Shader debugging and profiling](docs/Shader-debugging.md)
* [Graph visualization](docs/Graph-visualization.md)


## Suported Platforms
* Windows (with MSVC 2017, 2019)
* Linux (with GCC 8.2)


## Building
Generate project with CMake and build.<br/>
Required C++17 standard support and CMake 3.11 and greater.<br/>

Dependencies:<br/>
[Vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers) or [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) - required.<br/>
[VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - required.<br/>
[glfw](https://github.com/glfw/glfw) or [SDL2](https://www.libsdl.org) - required for framework and some tests.<br/>
[glslang](https://github.com/KhronosGroup/glslang) - required for glsl compiler.<br/>
[SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools), [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers) and Python 3.7 - (optional) for spirv optimization.<br/>
[GLSL-Trace](https://github.com/azhirnov/glsl_trace) - (optional) for shader debugging.<br/>
[lodepng](https://github.com/lvandeve/lodepng) - (optional) for screenshots.<br/>
[graphviz](https://www.graphviz.org/) - (optional) for graph visualization.<br/>
[Assimp](https://github.com/assimp/assimp) - (optional) for Scene extension.<br/>
[DevIL](http://openil.sourceforge.net/) - (optional) for Scene extension.<br/>
[FreeImage](http://freeimage.sourceforge.net/) - (optional) for Scene extension.<br/>
[imgui](https://github.com/ocornut/imgui) - (optional) for UI extension.<br/>
[OpenVR](https://github.com/ValveSoftware/openvr) - (optional) for VR support.<br/>
[GLM](https://glm.g-truc.net/0.9.9/index.html) - (optional) for Scene extension.<br/>
[ffmpeg](https://www.ffmpeg.org/) - (optional) for video recording.<br/>


## References
1. [FrameGraph in Frostbite](https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in).<br/>
2. [Handles vs pointers](https://floooh.github.io/2018/06/17/handles-vs-pointers.html)<br/>
3. [Porting engine to vulkan](https://gpuopen.com/presentation-porting-engine-to-vulkan-dx12/).<br/>
4. [FrameGraph from Ubisoft](https://developer.download.nvidia.com/assets/gameworks/downloads/regular/GDC17/DX12CaseStudies_GDC2017_FINAL.pdf)<br/>
5. [RenderGraph from EA](https://www.khronos.org/assets/uploads/developers/library/2019-reboot-develop-blue/SEED-EA_Rapid-Innovation-Using-Modern-Graphics_Apr19.pdf)<br/>
