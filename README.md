[![Build Status](https://api.travis-ci.com/azhirnov/FrameGraph.svg?branch=dev)](https://travis-ci.com/azhirnov/FrameGraph)

# FrameGraph

FrameGraph simplifies prototyping on Vulkan and can be used as a base layer for the graphics engine.
FrameGraph designed for maximum performance but not at the expense of usability. API developed as simple as possible, it hides all synchronizations, memory allocations and all this boilerplate code that is needed to get Vulkan work. Builtin validations together with Vulkan validation layers allow you to quickly find and fix errors.

## Features
* multithreaded command buffer building and submission.
* simple API, hides memory allocation, host<->device transfers, synchronizations.
* glsl debugging.
* supports RTX extensions.
* supports async compute and async transfer queues.

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
* [Shader debugging](docs/Shader-debugging.md)
* [Graph visualization](docs/Graph-visualization.md)
* [Roadmap](docs/Roadmap.md)


## Suported Platforms
* Windows (with MSVC 2017)
* Linux (with GCC 8.2)


## Building
Generate project with CMake and build.<br/>
Required C++17 standard support.
Warning: VC toolset v142 has bug with very long compilation of cmake external projects, so use v141 toolset.

CMake version 3.11 and greater will download all dependencies during configuration time.<br/>
If it didn't, manualy download dependencies into 'external' directory or in cmake specify `FG_EXTERNAL_***` pathes for each dependency.

Dependencies:<br/>
[Vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers) or [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) - required.<br/>
[VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - required.<br/>
[glfw](https://github.com/glfw/glfw) or [SDL2](https://www.libsdl.org) - required for framework and some tests.<br/>
[glslang](https://github.com/KhronosGroup/glslang) - required for glsl compiler.<br/>
[SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools), [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers) and Python - optional, for spirv optimization.<br/>
[lodepng](https://github.com/lvandeve/lodepng) - optional.<br/>
[graphviz](https://www.graphviz.org/) - (optional) for graph visualization.<br/>
[Assimp](https://github.com/assimp/assimp) - (optional) for Scene extension.<br/>
[DevIL](http://openil.sourceforge.net/) - (optional) for Scene extension.<br/>
[FreeImage](http://freeimage.sourceforge.net/) - (optional) for Scene extension.<br/>
[imgui](https://github.com/ocornut/imgui) - (optional) for UI extension.<br/>
[OpenVR](https://github.com/ValveSoftware/openvr) - (optional) for VR support.<br/>
[GLM](https://glm.g-truc.net/0.9.9/index.html) - (optional) for Scene extension.<br/>


## References
1. [FrameGraph in Frostbite](https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in).<br/>
2. [Handles vs pointers](https://floooh.github.io/2018/06/17/handles-vs-pointers.html)<br/>
3. [Porting engine to vulkan](https://gpuopen.com/presentation-porting-engine-to-vulkan-dx12/).<br/>
4. [FrameGraph from Ubisoft](https://developer.download.nvidia.com/assets/gameworks/downloads/regular/GDC17/DX12CaseStudies_GDC2017_FINAL.pdf)
