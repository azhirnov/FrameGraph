// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	docs:
	https://github.com/KhronosGroup/GLSL/blob/master/extensions/nv/GLSL_NV_mesh_shader.txt
	https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#drawing-mesh-shading
	https://devblogs.nvidia.com/using-turing-mesh-shaders-nvidia-asteroids-demo/
	https://devblogs.nvidia.com/introduction-turing-mesh-shaders/
*/

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "compiler/SpvCompiler.h"

namespace {

class MeshShaderApp final : public IWindowEventListener, public VulkanDeviceFn
{
private:
	struct UBuffer
	{
		float4		points[3];
		float4		colors[3];
	};


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

	VkPipeline				meshPipeline	= VK_NULL_HANDLE;
	VkPipelineLayout		pplnLayout		= VK_NULL_HANDLE;
	VkShaderModule			meshShader		= VK_NULL_HANDLE;
	VkShaderModule			fragShader		= VK_NULL_HANDLE;

	VkDescriptorSetLayout	dsLayout		= VK_NULL_HANDLE;
	VkDescriptorPool		descriptorPool	= VK_NULL_HANDLE;
	VkDescriptorSet			descriptorSet	= VK_NULL_HANDLE;

	VkBuffer				uniformBuf		= VK_NULL_HANDLE;
	VkDeviceMemory			sharedMemory	= VK_NULL_HANDLE;

	bool					looping			= true;


public:
	MeshShaderApp ()
	{
		VulkanDeviceFn_Init( vulkan );
	}
	
	void OnKey (StringView key, EKeyAction action) override;
	void OnResize (const uint2 &size) override;
	
	void OnRefresh () override {}
	void OnDestroy () override {}
	void OnUpdate () override {}
	void OnMouseMove (const float2 &) override {}

	bool Initialize ();
	void Destroy ();
	bool Run ();

	bool CreateCommandBuffers ();
	bool CreateSyncObjects ();
	bool CreateRenderPass ();
	bool CreateFramebuffers ();
	void DestroyFramebuffers ();
	bool CreateResources ();
	bool CreateDescriptorSet ();
	bool CreateMeshPipeline ();

	ND_ bool IsMeshShaderSupported () const		{ return vulkan.GetDeviceMeshShaderFeatures().meshShader; }
};



/*
=================================================
	OnKey
=================================================
*/
void MeshShaderApp::OnKey (StringView key, EKeyAction action)
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
void MeshShaderApp::OnResize (const uint2 &size)
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
bool MeshShaderApp::Initialize ()
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
		CHECK_ERR( window->Create( { 800, 600 }, "Mesh Shader sample" ));
		window->AddListener( this );

		CHECK_ERR( vulkan.Create( window->GetVulkanSurface(),
								  "Mesh Shader sample", "Engine",
								  VK_API_VERSION_1_1,
								  "",
								  {{ VK_QUEUE_PRESENT_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0.0f }},
								  VulkanDevice::GetRecomendedInstanceLayers(),
								  VulkanDevice::GetRecomendedInstanceExtensions(),
								  { VK_NV_MESH_SHADER_EXTENSION_NAME }
			));
		
		vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );
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
	CHECK_ERR( CreateResources() );
	CHECK_ERR( CreateDescriptorSet() );
	CHECK_ERR( CreateMeshPipeline() );
	return true;
}

/*
=================================================
	Destroy
=================================================
*/
void MeshShaderApp::Destroy ()
{
	VkDevice	dev = vulkan.GetVkDevice();

	VK_CALL( vkDeviceWaitIdle( dev ));

	for (auto& sem : semaphores) {
		vkDestroySemaphore( dev, sem, null );
		sem = VK_NULL_HANDLE;
	}
	for (auto& fen : fences) {
		vkDestroyFence( dev, fen, null );
		fen = VK_NULL_HANDLE;
	}
	vkDestroyCommandPool( dev, cmdPool, null );
	vkDestroyRenderPass( dev, renderPass, null );
	vkDestroyPipeline( dev, meshPipeline, null );
	vkDestroyShaderModule( dev, meshShader, null );
	vkDestroyShaderModule( dev, fragShader, null );
	vkDestroyPipelineLayout( dev, pplnLayout, null );
	vkDestroyDescriptorSetLayout( dev, dsLayout, null );
	vkDestroyDescriptorPool( dev, descriptorPool, null );
	vkDestroyBuffer( dev, uniformBuf, null );
	vkFreeMemory( dev, sharedMemory, null );

	cmdPool			= VK_NULL_HANDLE;
	cmdQueue		= VK_NULL_HANDLE;
	meshPipeline	= VK_NULL_HANDLE;
	meshShader		= VK_NULL_HANDLE;
	fragShader		= VK_NULL_HANDLE;
	pplnLayout		= VK_NULL_HANDLE;
	dsLayout		= VK_NULL_HANDLE;
	descriptorPool	= VK_NULL_HANDLE;
	descriptorSet	= VK_NULL_HANDLE;
	uniformBuf		= VK_NULL_HANDLE;
	sharedMemory	= VK_NULL_HANDLE;

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
bool MeshShaderApp::Run ()
{
	for (uint frameId = 0; looping; frameId = ((frameId + 1) & 1))
	{
		if ( not window->Update() )
			break;

		// wait and acquire next image
		{
			VK_CHECK( vkWaitForFences( vulkan.GetVkDevice(), 1, &fences[frameId], true, UMax ));
			VK_CHECK( vkResetFences( vulkan.GetVkDevice(), 1, &fences[frameId] ));

			VK_CALL( swapchain->AcquireNextImage( semaphores[0] ));
		}

		// build command buffer
		{
			VkCommandBufferBeginInfo	begin_info = {};
			begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			VK_CALL( vkBeginCommandBuffer( cmdBuffers[frameId], &begin_info ));
			
			// begin render pass
			{
				VkClearValue			clear_value = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
				VkRenderPassBeginInfo	begin		= {};
				begin.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				begin.framebuffer		= framebuffers[ swapchain->GetCurretImageIndex() ];
				begin.renderPass		= renderPass;
				begin.renderArea		= { {0,0}, {swapchain->GetSurfaceSize().x, swapchain->GetSurfaceSize().y} };
				begin.clearValueCount	= 1;
				begin.pClearValues		= &clear_value;

				vkCmdBeginRenderPass( cmdBuffers[frameId], &begin, VK_SUBPASS_CONTENTS_INLINE );
			}
			
			vkCmdBindPipeline( cmdBuffers[frameId], VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline );
			vkCmdBindDescriptorSets( cmdBuffers[frameId], VK_PIPELINE_BIND_POINT_GRAPHICS, pplnLayout, 0, 1, &descriptorSet, 0, null );

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
			
			// draw with mesh shader
			if ( IsMeshShaderSupported() )
			{
				vkCmdDrawMeshTasksNV( cmdBuffers[frameId], 1, 0 );
			}
			else
			// draw with vertex shader
			{
				vkCmdDraw( cmdBuffers[frameId], 3, 1, 0, 0 );
			}

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
				CHECK_FATAL( !"Present failed" );
		}
	}
	return true;
}

/*
=================================================
	CreateCommandBuffers
=================================================
*/
bool MeshShaderApp::CreateCommandBuffers ()
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
	info.commandBufferCount	= uint(CountOf( cmdBuffers ));
	VK_CHECK( vkAllocateCommandBuffers( vulkan.GetVkDevice(), &info, OUT cmdBuffers ));

	return true;
}

/*
=================================================
	CreateSyncObjects
=================================================
*/
bool MeshShaderApp::CreateSyncObjects ()
{
	VkDevice	dev = vulkan.GetVkDevice();

	VkFenceCreateInfo	fence_info	= {};
	fence_info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags	= VK_FENCE_CREATE_SIGNALED_BIT;

	for (auto& fence : fences) {
		VK_CHECK( vkCreateFence( dev, &fence_info, null, OUT &fence ));
	}
			
	VkSemaphoreCreateInfo	sem_info = {};
	sem_info.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	sem_info.flags		= 0;

	for (auto& sem : semaphores) {
		VK_CALL( vkCreateSemaphore( dev, &sem_info, null, OUT &sem ) );
	}

	return true;
}

/*
=================================================
	CreateRenderPass
=================================================
*/
bool MeshShaderApp::CreateRenderPass ()
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
	dependencies[1].dstAccessMask	= 0;
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
bool MeshShaderApp::CreateFramebuffers ()
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
void MeshShaderApp::DestroyFramebuffers ()
{
	for (uint i = 0; i < swapchain->GetSwapchainLength(); ++i)
	{
		vkDestroyFramebuffer( vulkan.GetVkDevice(), framebuffers[i], null );
	}
}

/*
=================================================
	CreateResources
=================================================
*/
bool MeshShaderApp::CreateResources ()
{
	VkDeviceSize					total_size		= 0;
	uint							mem_type_bits	= 0;
	VkMemoryPropertyFlags			mem_property	= 0;
	Array<std::function<void ()>>	bind_mem;

	// create uniform buffer
	{
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.size			= sizeof(UBuffer);
		info.usage			= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &uniformBuf ));

		VkMemoryRequirements	mem_req;
		vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), uniformBuf, OUT &mem_req );

		VkDeviceSize	offset = AlignToLarger( total_size, mem_req.alignment );
		total_size		 = offset + mem_req.size;
		mem_type_bits	|= mem_req.memoryTypeBits;
		mem_property	|= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		bind_mem.push_back( [this, offset] () {
			VK_CALL( vkBindBufferMemory( vulkan.GetVkDevice(), uniformBuf, sharedMemory, offset ));
		});
	}

	// allocate memory
	{
		VkMemoryAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize		= total_size;

		CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_type_bits, mem_property, OUT info.memoryTypeIndex ));

		VK_CHECK( vkAllocateMemory( vulkan.GetVkDevice(), &info, null, OUT &sharedMemory ));
	}

	// bind resources
	for (auto& bind : bind_mem) {
		bind();
	}

	// update resources
	{
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( cmdBuffers[0], &begin_info ));

		// update uniform buffer
		{
			UBuffer		temp;
			temp.points[0] = {  0.0f, -0.5f,  0.0f, 1.0f };
			temp.points[1] = {  0.5f,  0.5f,  0.0f, 1.0f };
			temp.points[2] = { -0.5f,  0.5f,  0.0f, 1.0f };

			temp.colors[0] = {  1.0f,  0.0f,  0.0f, 1.0f };
			temp.colors[1] = {  0.0f,  1.0f,  0.0f, 1.0f };
			temp.colors[2] = {  0.0f,  0.0f,  1.0f, 1.0f };

			vkCmdUpdateBuffer( cmdBuffers[0], uniformBuf, 0, sizeof(temp), &temp );
		}

		VK_CALL( vkEndCommandBuffer( cmdBuffers[0] ));

		VkSubmitInfo		submit_info = {};
		submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount	= 1;
		submit_info.pCommandBuffers		= &cmdBuffers[0];

		VK_CHECK( vkQueueSubmit( cmdQueue, 1, &submit_info, VK_NULL_HANDLE ));
	}

	VK_CALL( vkQueueWaitIdle( cmdQueue ));
	return true;
}

/*
=================================================
	CreateDescriptorSet
=================================================
*/
bool MeshShaderApp::CreateDescriptorSet ()
{
	// create layout
	{
		VkDescriptorSetLayoutBinding		binding[1] = {};
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding[0].descriptorCount	= 1;
		binding[0].stageFlags		= IsMeshShaderSupported() ? VK_SHADER_STAGE_MESH_BIT_NV : VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= uint(CountOf( binding ));
		info.pBindings		= binding;

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &dsLayout ));
	}

	// create pool
	{
		const VkDescriptorPoolSize		sizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 }
		};

		VkDescriptorPoolCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.maxSets		= 100;
		info.poolSizeCount	= uint(CountOf( sizes ));
		info.pPoolSizes		= sizes;

		VK_CHECK( vkCreateDescriptorPool( vulkan.GetVkDevice(), &info, null, OUT &descriptorPool ));
	}

	// allocate descriptor set
	{
		VkDescriptorSetAllocateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool		= descriptorPool;
		info.descriptorSetCount	= 1;
		info.pSetLayouts		= &dsLayout;

		VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &info, OUT &descriptorSet ));
	}

	// update descriptor set
	{
		VkDescriptorBufferInfo	buffers[1] = {};
		buffers[0].buffer		= uniformBuf;
		buffers[0].offset		= 0;
		buffers[0].range		= VK_WHOLE_SIZE;

		VkWriteDescriptorSet	writes[1] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= descriptorSet;
		writes[0].dstBinding		= 0;
		writes[0].descriptorCount	= uint(CountOf( buffers ));
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].pBufferInfo		= buffers;

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf( writes )), writes, 0, null );
	}
	return true;
}

/*
=================================================
	CreateMeshPipeline
=================================================
*/
bool MeshShaderApp::CreateMeshPipeline ()
{
	// create mesh shader
	if ( IsMeshShaderSupported() )
	{
		static const char	mesh_shader_source[] = R"#(
#extension GL_NV_mesh_shader : require

layout(local_size_x=3) in;
layout(triangles) out;
layout(max_vertices=3, max_primitives=1) out;

//out uint gl_PrimitiveCountNV;
//out uint gl_PrimitiveIndicesNV[]; // [max_primitives * 3 for triangles]

out gl_MeshPerVertexNV {
	vec4	gl_Position;
} gl_MeshVerticesNV[]; // [max_vertices]

layout(location = 0) out MeshOutput {
	vec4	color;
} Output[]; // [max_vertices]

layout(binding = 0, std140) uniform UBuffer {
	vec4	points[3];
	vec4	colors[3];
} ub;

void main ()
{
	const uint I = gl_LocalInvocationID.x;

	gl_MeshVerticesNV[I].gl_Position	= ub.points[I];
	Output[I].color						= ub.colors[I];
	gl_PrimitiveIndicesNV[I]			= I;

	if ( I == 0 )
		gl_PrimitiveCountNV = 1;
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT meshShader, vulkan, {mesh_shader_source}, "main", EShLangMeshNV ));
	}
	else

	// create vertex shader
	{
		static const char	vert_shader_source[] = R"#(
layout(binding = 0, std140) uniform UBuffer {
	vec4	points[3];
	vec4	colors[3];
} ub;

layout(location = 0) out MeshOutput {
	vec4	color;
} Output;

void main()
{
	gl_Position  = ub.points[gl_VertexIndex];
	Output.color = ub.colors[gl_VertexIndex];
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT meshShader, vulkan, {vert_shader_source}, "main", EShLangVertex ));
	}

	// create fragment shader
	{
		static const char	frag_shader_source[] = R"#(
layout(location = 0) out vec4  out_Color;

layout(location = 0) in MeshOutput {
	vec4	color;
} Input;

void main ()
{
	out_Color = Input.color;
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT fragShader, vulkan, {frag_shader_source}, "main", EShLangFragment ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &pplnLayout ));
	}

	VkPipelineShaderStageCreateInfo			stages[2] = {};
	stages[0].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage		= IsMeshShaderSupported() ? VK_SHADER_STAGE_MESH_BIT_NV : VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module	= meshShader;
	stages[0].pName		= "main";
	stages[1].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage		= VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module	= fragShader;
	stages[1].pName		= "main";

	VkPipelineVertexInputStateCreateInfo	vertex_input = {};
	vertex_input.sType		= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	VkPipelineInputAssemblyStateCreateInfo	input_assembly = {};
	input_assembly.sType	= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
	info.pVertexInputState		= &vertex_input;		// ignored in mesh shader
	info.pInputAssemblyState	= &input_assembly;
	info.pRasterizationState	= &rasterization;
	info.pMultisampleState		= &multisample;
	info.pDepthStencilState		= &depth_stencil;
	info.pColorBlendState		= &blend_state;
	info.pDynamicState			= &dynamic_state;
	info.layout					= pplnLayout;
	info.renderPass				= renderPass;
	info.subpass				= 0;

	VK_CHECK( vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &meshPipeline ));
	return true;
}
}	// anonymous namespace

/*
=================================================
	MeshShader_Sample1
=================================================
*/
extern void MeshShader_Sample1 ()
{
	MeshShaderApp	app;
	
	if ( app.Initialize() )
	{
		app.Run();
		app.Destroy();
	}
}
