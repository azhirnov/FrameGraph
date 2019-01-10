## Vulkan Loader
Vulkan function loader. Adds `[[nodiscard]]` attribute for return values.

## Framework
Wrapper for vkInstance, vkPhysicalDevice, vkDevice and VkSwapchainKHR.
Wrapper for some GUI libraries: [glfw](https://github.com/glfw/glfw), [SDL2](https://www.libsdl.org), [SFML](https://github.com/SFML/SFML).

## GraphViz
Helper class that simplifies [graphviz](https://www.graphviz.org/) usage.

## UI
FrameGraph renderer for [imgui](https://github.com/ocornut/imgui).

## Scene
Work in progress.

## GLSL Trace
This library adds shader trace recording into glslang AST.

## Pipeline Compiler
Intergrated into FrameGraph and add ability to compile glsl shaders and generates shader reflection.
Can produce debuggable shaders using GLSLTrace library.

## Offline Pipeline Compiler
Compile glsl shader using PipelineCompiler library and serialize to the C++ code.
