// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "compiler/SpvCompiler.h"

using namespace FG;


class FSBarycentricApp final : public IWindowEventListener, public VulkanDeviceFn
{
private:
	VulkanDeviceExt			vulkan;
	VulkanSwapchainPtr		swapchain;
	WindowPtr				window;
	SpvCompiler				spvCompiler;
	
	VkCommandPool			cmdPool			= VK_NULL_HANDLE;
	VkQueue					cmdQueue		= VK_NULL_HANDLE;
	VkCommandBuffer			cmdBuffers[2]	= {};
	VkFence					fences[2]		= {};
	VkSemaphore				semaphores[2]	= {};

	VkRenderPass			renderPass		= VK_NULL_HANDLE;
	VkFramebuffer			framebuffers[8]	= {};

	VkPipeline				pipeline		= VK_NULL_HANDLE;
	VkPipelineLayout		pplnLayout		= VK_NULL_HANDLE;
	VkShaderModule			vertShader		= VK_NULL_HANDLE;
	VkShaderModule			fragShader		= VK_NULL_HANDLE;

	bool					looping			= true;


public:
	FSBarycentricApp ()
	{
		VulkanDeviceFn_Init( vulkan );
	}
	
	void OnKey (StringView key, EKeyAction action) override;
	void OnResize (const uint2 &size) override;
	
	void OnRefrash () override {}
	void OnDestroy () override {}
	void OnUpdate () override {}

	bool Initialize ();
	void Destroy ();
	bool Run ();

	bool CreateCommandBuffers ();
	bool CreateSyncObjects ();
	bool CreateRenderPass ();
	bool CreateFramebuffers ();
	void DestroyFramebuffers ();
	bool CreatePipeline ();

	bool IsFragmentShaderBarycentricSupported () const	{ return vulkan.HasDeviceExtension( VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME ); }
};



/*
=================================================
	OnKey
=================================================
*/
void FSBarycentricApp::OnKey (StringView key, EKeyAction action)
{
	if ( action != EKeyAction::Down )
		return;

	if ( key == "escape" )
		looping = false;
}

/*
=================================================
	OnResize
=================================================
*/
void FSBarycentricApp::OnResize (const uint2 &size)
{
	VK_CALL( vkDeviceWaitIdle( vulkan.GetVkDevice() ));

	VK_CALL( vkResetCommandPool( vulkan.GetVkDevice(), cmdPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ));
	DestroyFramebuffers();

	CHECK( swapchain->Recreate( size ));

	CHECK( CreateFramebuffers() );
}

/*
=================================================
	Initialize
=================================================
*/
bool FSBarycentricApp::Initialize ()
{
# if defined(FG_ENABLE_GLFW)
	window.reset( new WindowGLFW() );

# elif defined(FG_ENABLE_SDL2)
	window.reset( new WindowSDL2() );

# elif defined(FG_ENABLE_SFML)
	window.reset( new WindowSFML() );

# else
#	error unknown window library!
# endif
	

	// create window and vulkan device
	{
		const char	title[] = "Fragment shader barycentric sample";

		CHECK_ERR( window->Create( { 800, 600 }, title ));
		window->AddListener( this );

		CHECK_ERR( vulkan.Create( window->GetVulkanSurface(),
								  title, "Engine",
								  VK_API_VERSION_1_1,
								  "",
								  {{ VK_QUEUE_PRESENT_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0.0f }},
								  VulkanDevice::GetRecomendedInstanceLayers(),
								  { VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME },
								  { VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME }
			));
		
		vulkan.CreateDebugCallback( VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT );

		CHECK_ERR( IsFragmentShaderBarycentricSupported() );
	}


	// initialize swapchain
	{
		VkFormat		color_fmt	= VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR	color_space	= VK_COLOR_SPACE_MAX_ENUM_KHR;

		swapchain.reset( new VulkanSwapchain{ vulkan } );

		CHECK_ERR( swapchain->ChooseColorFormat( INOUT color_fmt, INOUT color_space ));

		CHECK_ERR( swapchain->Create( window->GetSize(), color_fmt, color_space ));
	}


	// initialize vulkan objects
	cmdQueue = vulkan.GetVkQuues().front().id;

	CHECK_ERR( CreateCommandBuffers() );
	CHECK_ERR( CreateSyncObjects() );
	CHECK_ERR( CreateRenderPass() );
	CHECK_ERR( CreateFramebuffers() );
	CHECK_ERR( CreatePipeline() );
	return true;
}

/*
=================================================
	Destroy
=================================================
*/
void FSBarycentricApp::Destroy ()
{
	VkDevice	dev = vulkan.GetVkDevice();

	VK_CALL( vkDeviceWaitIdle( dev ));

	vkDestroySemaphore( dev, semaphores[0], null );
	vkDestroySemaphore( dev, semaphores[1], null );
	vkDestroyFence( dev, fences[0], null );
	vkDestroyFence( dev, fences[1], null );
	vkDestroyCommandPool( dev, cmdPool, null );
	vkDestroyRenderPass( dev, renderPass, null );
	vkDestroyPipeline( dev, pipeline, null );
	vkDestroyShaderModule( dev, vertShader, null );
	vkDestroyShaderModule( dev, fragShader, null );
	vkDestroyPipelineLayout( dev, pplnLayout, null );

	cmdPool			= VK_NULL_HANDLE;
	cmdQueue		= VK_NULL_HANDLE;
	pipeline		= VK_NULL_HANDLE;
	vertShader		= VK_NULL_HANDLE;
	fragShader		= VK_NULL_HANDLE;
	pplnLayout		= VK_NULL_HANDLE;

	DestroyFramebuffers();
	swapchain->Destroy();
	swapchain.reset();

	vulkan.Destroy();

	window->Destroy();
	window.reset();
}

/*
=================================================
	Run
=================================================
*/
bool FSBarycentricApp::Run ()
{
	for (uint frameId = 0; looping; frameId = ((frameId + 1) & 1))
	{
		if ( not window->Update() )
			break;

		// wait and acquire next image
		{
			VK_CHECK( vkWaitForFences( vulkan.GetVkDevice(), 1, &fences[frameId], true, ~0ull ));
			VK_CHECK( vkResetFences( vulkan.GetVkDevice(), 1, &fences[frameId] ));

			VK_CALL( swapchain->AcquireNextImage( semaphores[0] ));
		}

		// build command buffer
		{
			VkCommandBufferBeginInfo	begin_info = {};
			begin_info.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.pNext			= null;
			begin_info.flags			= 0;
			begin_info.pInheritanceInfo	= null;
			VK_CALL( vkBeginCommandBuffer( cmdBuffers[frameId], &begin_info ));
			
			// begin render pass
			{
				VkClearValue			clear_value = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
				VkRenderPassBeginInfo	begin		= {};
				begin.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				begin.framebuffer		= framebuffers[ swapchain->GetCurretImageIndex() ];
				begin.renderPass		= renderPass;
				begin.renderArea		= { {0,0}, {swapchain->GetSurfaceSize().x, swapchain->GetSurfaceSize().y} };
				begin.clearValueCount	= 1;
				begin.pClearValues		= &clear_value;

				vkCmdBeginRenderPass( cmdBuffers[frameId], &begin, VK_SUBPASS_CONTENTS_INLINE );
			}
			
			vkCmdBindPipeline( cmdBuffers[frameId], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );

			// set dynamic states
			{
				VkViewport	viewport = {};
				viewport.x			= 0.0f;
				viewport.y			= 0.0f;
				viewport.width		= float(swapchain->GetSurfaceSize().x);
				viewport.height		= float(swapchain->GetSurfaceSize().y);
				viewport.minDepth	= 0.0f;
				viewport.maxDepth	= 1.0f;

				vkCmdSetViewport( cmdBuffers[frameId], 0, 1, &viewport );

				VkRect2D	scissor_rect = { {0,0}, {swapchain->GetSurfaceSize().x, swapchain->GetSurfaceSize().y} };

				vkCmdSetScissor( cmdBuffers[frameId], 0, 1, &scissor_rect );
			}
			
			vkCmdDraw( cmdBuffers[frameId], 4, 1, 0, 0 );
			
			vkCmdEndRenderPass( cmdBuffers[frameId] );

			VK_CALL( vkEndCommandBuffer( cmdBuffers[frameId] ));
		}


		// submit commands
		{
			VkSemaphore				signal_semaphores[] = { semaphores[1] };
			VkSemaphore				wait_semaphores[]	= { semaphores[0] };
			VkPipelineStageFlags	wait_dst_mask[]		= { VK_PIPELINE_STAGE_TRANSFER_BIT };
			STATIC_ASSERT( CountOf(wait_semaphores) == CountOf(wait_dst_mask) );

			VkSubmitInfo				submit_info = {};
			submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.commandBufferCount		= 1;
			submit_info.pCommandBuffers			= &cmdBuffers[frameId];
			submit_info.waitSemaphoreCount		= uint(CountOf(wait_semaphores));
			submit_info.pWaitSemaphores			= wait_semaphores;
			submit_info.pWaitDstStageMask		= wait_dst_mask;
			submit_info.signalSemaphoreCount	= uint(CountOf(signal_semaphores));
			submit_info.pSignalSemaphores		= signal_semaphores;

			VK_CHECK( vkQueueSubmit( cmdQueue, 1, &submit_info, fences[frameId] ));
		}

		// present
		VkResult	err = swapchain->Present( cmdQueue, {semaphores[1]} );
		switch ( err ) {
			case VK_SUCCESS :
				break;

			case VK_SUBOPTIMAL_KHR :
			case VK_ERROR_SURFACE_LOST_KHR :
			case VK_ERROR_OUT_OF_DATE_KHR :
				OnResize( swapchain->GetSurfaceSize() );
				break;

			default :
				RETURN_ERR( "Present failed" );
		}
	}
	return true;
}

/*
=================================================
	CreateCommandBuffers
=================================================
*/
bool FSBarycentricApp::CreateCommandBuffers ()
{
	VkCommandPoolCreateInfo		pool_info = {};
	pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex	= vulkan.GetVkQuues().front().familyIndex;
	pool_info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK( vkCreateCommandPool( vulkan.GetVkDevice(), &pool_info, null, OUT &cmdPool ));

	VkCommandBufferAllocateInfo	info = {};
	info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext				= null;
	info.commandPool		= cmdPool;
	info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount	= 2;
	VK_CHECK( vkAllocateCommandBuffers( vulkan.GetVkDevice(), &info, OUT cmdBuffers ));

	return true;
}

/*
=================================================
	CreateSyncObjects
=================================================
*/
bool FSBarycentricApp::CreateSyncObjects ()
{
	VkFenceCreateInfo	fence_info	= {};
	fence_info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags	= VK_FENCE_CREATE_SIGNALED_BIT;
	VK_CHECK( vkCreateFence( vulkan.GetVkDevice(), &fence_info, null, OUT &fences[0] ));
	VK_CHECK( vkCreateFence( vulkan.GetVkDevice(), &fence_info, null, OUT &fences[1] ));
			
	VkSemaphoreCreateInfo	sem_info = {};
	sem_info.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	sem_info.flags		= 0;
	VK_CALL( vkCreateSemaphore( vulkan.GetVkDevice(), &sem_info, null, OUT &semaphores[0] ) );
	VK_CALL( vkCreateSemaphore( vulkan.GetVkDevice(), &sem_info, null, OUT &semaphores[1] ) );

	return true;
}

/*
=================================================
	CreateRenderPass
=================================================
*/
bool FSBarycentricApp::CreateRenderPass ()
{
	// setup attachment
	VkAttachmentDescription		attachments[1] = {};

	attachments[0].format			= swapchain->GetColorFormat();
	attachments[0].samples			= VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


	// setup subpasses
	VkSubpassDescription	subpasses[1]		= {};
	VkAttachmentReference	attachment_ref[1]	= {};

	attachment_ref[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	subpasses[0].pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].colorAttachmentCount	= 1;
	subpasses[0].pColorAttachments		= &attachment_ref[0];


	// setup dependencies
	VkSubpassDependency		dependencies[2] = {};

	dependencies[0].srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass		= 0;
	dependencies[0].srcStageMask	= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass		= 0;
	dependencies[1].dstSubpass		= VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask	= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;


	// setup create info
	VkRenderPassCreateInfo	info = {};
	info.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.flags				= 0;
	info.attachmentCount	= uint(CountOf( attachments ));
	info.pAttachments		= attachments;
	info.subpassCount		= uint(CountOf( subpasses ));
	info.pSubpasses			= subpasses;
	info.dependencyCount	= uint(CountOf( dependencies ));
	info.pDependencies		= dependencies;

	VK_CHECK( vkCreateRenderPass( vulkan.GetVkDevice(), &info, null, OUT &renderPass ));
	return true;
}

/*
=================================================
	CreateFramebuffers
=================================================
*/
bool FSBarycentricApp::CreateFramebuffers ()
{
	VkFramebufferCreateInfo	fb_info = {};
	fb_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.renderPass		= renderPass;
	fb_info.attachmentCount	= 1;
	fb_info.width			= swapchain->GetSurfaceSize().x;
	fb_info.height			= swapchain->GetSurfaceSize().y;
	fb_info.layers			= 1;

	for (uint i = 0; i < swapchain->GetSwapchainLength(); ++i)
	{
		VkImageView	view = swapchain->GetImageView( i );
		fb_info.pAttachments = &view;

		VK_CALL( vkCreateFramebuffer( vulkan.GetVkDevice(), &fb_info, null, OUT &framebuffers[i] ));
	}
	return true;
}

/*
=================================================
	DestroyFramebuffers
=================================================
*/
void FSBarycentricApp::DestroyFramebuffers ()
{
	for (uint i = 0; i < swapchain->GetSwapchainLength(); ++i)
	{
		vkDestroyFramebuffer( vulkan.GetVkDevice(), framebuffers[i], null );
	}
}

/*
=================================================
	CreatePipeline
=================================================
*/
bool FSBarycentricApp::CreatePipeline ()
{
	// create vertex shader
	{
		static const char	vert_shader_source[] = R"#(
#version 450 core
#extension GL_ARB_separate_shader_objects : enable

const vec2	g_Positions[4] = {
	vec2( -1.0,  1.0 ),
	vec2( -1.0, -1.0 ),
	vec2(  1.0,  1.0 ),
	vec2(  1.0, -1.0 )
};

void main()
{
	gl_Position = vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT vertShader, vulkan, vert_shader_source, "main", EShLangVertex ));
	}

	// create fragment shader
	// see https://github.com/KhronosGroup/GLSL/blob/master/extensions/nv/GLSL_NV_fragment_shader_barycentric.txt
	{
		static const char	frag_shader_source[] = R"#(
#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_NV_fragment_shader_barycentric : require

// TODO: use pervertexNV

//in vec3 gl_BaryCoordNV;
//in vec3 gl_BaryCoordNoPerspNV;

layout(location = 0) out vec4  out_Color;

void main ()
{
	out_Color = vec4( gl_BaryCoordNV, 1.0 );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT fragShader, vulkan, frag_shader_source, "main", EShLangFragment ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 0;
		info.pSetLayouts			= null;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &pplnLayout ));
	}

	VkPipelineShaderStageCreateInfo			stages[2] = {};
	stages[0].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage		= VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module	= vertShader;
	stages[0].pName		= "main";
	stages[1].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage		= VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module	= fragShader;
	stages[1].pName		= "main";

	VkPipelineVertexInputStateCreateInfo	vertex_input = {};
	vertex_input.sType		= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	VkPipelineInputAssemblyStateCreateInfo	input_assembly = {};
	input_assembly.sType	= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	VkPipelineViewportStateCreateInfo		viewport = {};
	viewport.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport.viewportCount	= 1;
	viewport.scissorCount	= 1;

	VkPipelineRasterizationStateCreateInfo	rasterization = {};
	rasterization.sType			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode	= VK_POLYGON_MODE_FILL;
	rasterization.cullMode		= VK_CULL_MODE_NONE;
	rasterization.frontFace		= VK_FRONT_FACE_COUNTER_CLOCKWISE;

	VkPipelineMultisampleStateCreateInfo	multisample = {};
	multisample.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo	depth_stencil = {};
	depth_stencil.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable		= VK_FALSE;
	depth_stencil.depthWriteEnable		= VK_FALSE;
	depth_stencil.depthCompareOp		= VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil.depthBoundsTestEnable	= VK_FALSE;
	depth_stencil.stencilTestEnable		= VK_FALSE;

	VkPipelineColorBlendAttachmentState		blend_attachment = {};
	blend_attachment.blendEnable		= VK_FALSE;
	blend_attachment.colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo		blend_state = {};
	blend_state.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_state.logicOpEnable		= VK_FALSE;
	blend_state.attachmentCount		= 1;
	blend_state.pAttachments		= &blend_attachment;

	VkDynamicState							dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo		dynamic_state = {};
	dynamic_state.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount	= uint(CountOf( dynamic_states ));
	dynamic_state.pDynamicStates	= dynamic_states;

	// create pipeline
	VkGraphicsPipelineCreateInfo	info = {};
	info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount				= uint(CountOf( stages ));
	info.pStages				= stages;
	info.pViewportState			= &viewport;
	info.pVertexInputState		= &vertex_input;
	info.pInputAssemblyState	= &input_assembly;
	info.pRasterizationState	= &rasterization;
	info.pMultisampleState		= &multisample;
	info.pDepthStencilState		= &depth_stencil;
	info.pColorBlendState		= &blend_state;
	info.pDynamicState			= &dynamic_state;
	info.layout					= pplnLayout;
	info.renderPass				= renderPass;
	info.subpass				= 0;

	VK_CHECK( vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &pipeline ));
	return true;
}

/*
=================================================
	FragShaderBarycentric_Sample1
=================================================
*/
extern void FragShaderBarycentric_Sample1 ()
{
	FSBarycentricApp	app;
	
	if ( app.Initialize() )
	{
		app.Run();
		app.Destroy();
	}
}
