Warning: this is just an example, something may change, so see [tests](../tests/framegraph) or [samples](../samples/framegraph) for currently working code.

## Initialization
The FrameGraph doesn't support device initialization and window creation, so you must create it before creating FrameGraph instance. You can use [framework](../extensions/framework) for this.

```cpp
#include "framework/Window/WindowGLFW.h"
#include "framework/Vulkan/VulkanDeviceExt.h"

UniquePtr<IWindow> window { new WindowGLFW() };
VulkanDeviceExt    vulkan;

// create window
window->Create( uint2{800,600}, "Sample" );

// create vulkan instance and logical device
vulkan.Create( window->GetVulkanSurface(), "ApplicationTitle", "Engine", VK_API_VERSION_1_1 );

// create listener for debug messages
vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );

// setup device description
VulkanDeviceInfo  vulkan_info;
vulkan_info.instance = vulkan.GetVkInstance();
vulkan_info.physicalDevice = vulkan.GetVkPhysicalDevice();
vulkan_info.device = vulkan.GetVkDevice();

VulkanDeviceInfo::QueueInfo queue;
queue.id = vulkan.GetVkQueues()[0].id;
queue.familyFlags = vulkan.GetVkQueues()[0].flags;
queue.familyIndex = vulkan.GetVkQueues()[0].familyIndex;
vulkan_info.push_back( queue );

// create framegraph instance
FrameGraph frameGraoh = IFrameGraph::CreateFrameGraph( vulkan_info );

// setup swapchain description
VulkanSwapchainCreateInfo swapchain;
swapchain.surface = vulkan.GetVkSurface();
swapchain.surfaceSize = window->GetSize();

// create swapchain
FrameGraphThread::SwapchainCreateInfo swapchain_info = swapchain;
SwapchainID swapchain = frameGraph->CreateSwapchain( swapchain_info );
```

## Shader compilation
The FrameGraph doesn't support shader compilation, but you can use [PipelineCompiler](../extensions/pipeline_compiler) extension.

Add shader compiler to the framegraph:
```cpp
#include "pipeline_compiler/VPipelineCompiler.h"

// create pipeline compiler
IPipelineCompilerPtr compiler = MakeShared<VPipelineCompiler>( vulkan.GetVkPhysicalDevice(), vulkan.GetVkDevice() );

// setup
compiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations );

// add to framegraph
frameGraoh->AddPipelineCompiler( compiler );
```

Now create graphics pipeline:
```cpp
GraphicsPipelineDesc  desc;

// add vertex shader
// VKSL_100 means that it is GLSL with vulkan semantics and should be compiled for Vulkan 1.0.
// SPIRV_110 means that it is SPIRV binary compiled for Vulkan 1.1.
// VkShader_100 used for already created shader module.
// GLSL_450 means that shader compatible with OpenGL 4.5, but also can be used in Vulkan.
// Warning: any SPIRV and VKSL shaders have higher priority than any GLSL shader if used Vulkan backend (currently it is only one backend).
desc.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
out vec3  v_Color;

const vec2  g_Positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

const vec3  g_Colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
    v_Color = g_Colors[gl_VertexIndex];
}
)#" );

// add fragment shader
desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
in  vec3  v_Color;
out vec4  out_Color;

void main() {
    out_Color = vec4(v_Color, 1.0);
}
)#" );

// create pipeline from description.
GPipelineID  pipeline = frameGraph->CreatePipeline( desc );
```

## Command buffer
```cpp
// TBD
CommandBufferDesc cmd_desc{ EQueueType::Graphics };

// TBD
CommandBuffer cmdBuffer = frameGraph->Begin( cmd_desc );

...

// TBD
frameGraoh->Execute( cmdBuffer );

// TBD
frameGraph->Flush();
```

## Drawing
```cpp
// create render target
ImageDesc  image_desc{ EImage::Tex2D,
                       uint3{800, 600, 1},
                       EPixelFormat::RGBA8_UNorm,
                       EImageUsage::ColorAttachment };
ImageID  image = frameGraph->CreateImage( image_desc );

// skiped some code, this wiil be descripbed below
...

// set render area size
RenderPassDesc  rp_desc{ uint2{800, 600} };

// add render target.
// render target ID must be same as in output parameter in fragment shader.
// before rendering image will be cleared with 0.0f value.
// after rendering result will be stored to the image.
rp_desc.AddTarget( RenderTargetID{"out_Color"}, image, RGBA32f{0.0f}, EAttachmentStoreOp::Store );

// setup viewport
rp_desc.AddViewport( float2{800.0f, 600.0f} );

// setup render states
rp_desc.SetDepthTestEnabled(false);

// create render pass
LogicalPassID  render_pass = cmdBuffer->CreateRenderPass( rp_desc );

// create draw task
DrawVertices  draw_triangle;

// draw 3 vertices
draw_triangle.Draw( 3 );

// draw as triangle list
draw_triangle.SetTopology( EPrimitive::TriangleList );

// use pipeline from previous chapter
draw_triangle.SetPipeline( pipeline );

// add draw task to the render pass
cmdBuffer->AddTask( render_pass, draw_triangles );

// add render pass to the frame graph.
// after that you can not add draw tasks into the render pass.
Task submit = cmdBuffer->AddTask( SubmitRenderPass{ render_pass });

// present to swapchain.
// this task must be executed after drawing.
Task present = cmdBuffer->AddTask( Present{ image }.DependsOn( submit ));

// skiped some code, this wiil be descripbed below
...
```

## Deinitialization
```cpp
frameGraoh->Deinitialize();
frameGraoh = nullptr;
```
