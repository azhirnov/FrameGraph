## Vulkan Loader
Vulkan function loader. Adds `[[nodiscard]]` attribute for returned values.

## Framework
Wrapper for vkInstance, vkPhysicalDevice, vkDevice and VkSwapchainKHR.
Wrapper for some window libraries: [glfw](https://github.com/glfw/glfw), [SDL2](https://www.libsdl.org), [SFML](https://github.com/SFML/SFML).

## GraphViz
Helper class that simplifies [graphviz](https://www.graphviz.org/) usage.
[Graphviz](https://www.graphviz.org/) executable must be installed.

## UI
FrameGraph renderer for [imgui](https://github.com/ocornut/imgui).

## Scene
Work in progress.

## GLSL Trace
Standalone library (only glslang is required), that adds shader trace recording into glslang AST.

## Pipeline Compiler
Intergrated into FrameGraph and add ability to compile glsl shaders and generates shader reflection.<br/>
Can produce debuggable shaders using GLSLTrace library.

## Offline Pipeline Compiler
Compile glsl shader using PipelineCompiler library and serialize it to the C++ code.<br/>
Required for Android because shader compilation is not supported for it.

## Video
Video recorder implementation.
