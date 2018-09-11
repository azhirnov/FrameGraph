# FrameGraph
Work in progress.

## Building
Generate project with CMake and build.
C++17 standard support required.

CMake version 3.11 and greater will download all dependencies on configuration time. If not, download dependencies into 'external' directory or specify FG_EXTERNAL_PATH in cmake.

Dependencies:
[Vulkan-headers] (https://github.com/KhronosGroup/Vulkan-Headers) or [Vulkan SDK] (https://www.lunarg.com/vulkan-sdk/) - required.
[glfw] (https://github.com/glfw/glfw) or [SDL2] (https://www.libsdl.org) - required for framework.
[glslang] (https://github.com/KhronosGroup/glslang) - will be required later.
[VulkanMemoryAllocator] (https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - will be required later.
[lodepng] (https://github.com/lvandeve/lodepng) - optional.

## Roadmap
Stage 1
	- glsl compilation to spirv + reflection.
	- auto pipeline barrier placement.
	- render pass optimization.
	- unit tests (stl, resource barriers, glsl compiler, resource cache, ...)
	- graph node tests (buffer copy, image copy, draw triangle, dispatch, ...)
	- implementation tests:
		* correct and optimal barrier placement.
		* correct render pass optimization.

Stage 2
	- performance tests.
	- logical resources.
	- advanced gpu memory managment.
	- multithreading.
	- samples.

Stage 3
	- multi-gpu.
	- WebGL 2.0 (or WebGL-next) support.