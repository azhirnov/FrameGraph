## Vulkan Loader
Vulkan function loader. Adds `[[nodiscard]]` attribute for returned values.

## Framework
Wrapper for vkInstance, vkPhysicalDevice, vkDevice and VkSwapchainKHR.
Wrapper for some window libraries: [glfw](https://github.com/glfw/glfw), [SDL2](https://www.libsdl.org).

## GraphViz
Helper class that simplifies [graphviz](https://www.graphviz.org/) usage.
[Graphviz](https://www.graphviz.org/) executable must be installed.

## UI
FrameGraph renderer for [imgui](https://github.com/ocornut/imgui).

## Scene
Work in progress.

## Pipeline Compiler
Intergrated into FrameGraph and add ability to compile glsl shaders and generates shader reflection.<br/>
Can produce debuggable shaders using GLSLTrace library.

## Pipeline Reflection
Generates pipeline reflection form SPIRV binary using [SPIRV-Reflect](https://github.com/chaoticbob/SPIRV-Reflect).

## Video
Video recorder implementation.
