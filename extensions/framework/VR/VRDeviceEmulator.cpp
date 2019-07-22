// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framework/VR/VRDeviceEmulator.h"

namespace FGC
{

/*
=================================================
	constructor
=================================================
*/
	VRDeviceEmulator::VRDeviceEmulator (WindowPtr wnd) :
		_vkInstance{VK_NULL_HANDLE},
		_vkPhysicalDevice{VK_NULL_HANDLE},
		_vkLogicalDevice{VK_NULL_HANDLE},
		_lastSignal{VK_NULL_HANDLE},
		_output{std::move(wnd)},
		_isCreated{false}
	{
		VulkanDeviceFn_Init( &_deviceFnTable );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VRDeviceEmulator::~VRDeviceEmulator ()
	{
	}
		
/*
=================================================
	Create
=================================================
*/
	bool VRDeviceEmulator::Create ()
	{
		CHECK_ERR( _output );
		CHECK_ERR( not _isCreated );

		_isCreated = true;
		return true;
	}
	
/*
=================================================
	SetVKDevice
=================================================
*/
	bool VRDeviceEmulator::SetVKDevice (VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
	{
		CHECK_ERR( _output and _isCreated );

		_vkInstance			= instance;
		_vkPhysicalDevice	= physicalDevice;
		_vkLogicalDevice	= logicalDevice;
		
		CHECK_ERR( VulkanLoader::Initialize() );
		VulkanLoader::LoadInstance( _vkInstance );
		VulkanLoader::LoadDevice( _vkLogicalDevice, OUT _deviceFnTable );

		auto	surface		= _output->GetVulkanSurface();
		auto	vk_surface	= surface->Create( _vkInstance );
		CHECK_ERR( vk_surface );
		
		// check if physical device supports present on this surface
		{
			uint	count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &count, null );
			CHECK_ERR( count > 0 );

			bool	is_supported = false;
			for (uint i = 0; i < count; ++i)
			{
				VkBool32	supported = 0;
				VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _vkPhysicalDevice, i, vk_surface, OUT &supported ));
				is_supported |= !!supported;
			}
			CHECK_ERR( is_supported );
		}

		_swapchain.reset(new VulkanSwapchain{ physicalDevice, logicalDevice, vk_surface, VulkanDeviceFn{&_deviceFnTable} });
		
		VkFormat		color_fmt	= VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR	color_space	= VK_COLOR_SPACE_MAX_ENUM_KHR;
		CHECK_ERR( _swapchain->ChooseColorFormat( INOUT color_fmt, INOUT color_space ));

		CHECK_ERR( _swapchain->Create( _output->GetSize(), color_fmt, color_space, 2,
										VK_PRESENT_MODE_FIFO_KHR,
										VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
										VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
										VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT ));

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VRDeviceEmulator::Destroy ()
	{
		if ( _vkLogicalDevice ) {
			VK_CALL( vkDeviceWaitIdle( _vkLogicalDevice ));
		}

		for (auto& q : _queues)
		{
			vkDestroyCommandPool( _vkLogicalDevice, q.cmdPool, null );

			for (auto& fence : q.fences) {
				vkDestroyFence( _vkLogicalDevice, fence, null );
			}
			for (auto& sem : q.waitSemaphores) {
				vkDestroySemaphore( _vkLogicalDevice, sem, null );
			}
			for (auto& sem : q.signalSemaphores) {
				vkDestroySemaphore( _vkLogicalDevice, sem, null );
			}
		}
		_queues.clear();

		if ( _swapchain )
		{
			VkSurfaceKHR	surf = _swapchain->GetVkSurface();

			_swapchain->Destroy();
			_swapchain.reset();

			if ( surf )
				vkDestroySurfaceKHR( _vkInstance, surf, null );
		}

		_vkInstance			= VK_NULL_HANDLE;
		_vkPhysicalDevice	= VK_NULL_HANDLE;
		_vkLogicalDevice	= VK_NULL_HANDLE;
		
		VulkanLoader::ResetDevice( OUT _deviceFnTable );
		VulkanLoader::Unload();

		if ( _output )
		{
			_output->Destroy();
			_output.reset();
		}

		_isCreated = false;
	}
	
/*
=================================================
	AddListener
=================================================
*/
	void VRDeviceEmulator::AddListener (IVRDeviceEventListener *listener)
	{
		ASSERT( listener );
		_listeners.insert( listener );
	}
	
/*
=================================================
	RemoveListener
=================================================
*/
	void VRDeviceEmulator::RemoveListener (IVRDeviceEventListener *listener)
	{
		ASSERT( listener );
		_listeners.erase( listener );
	}
	
/*
=================================================
	Update
=================================================
*/
	bool VRDeviceEmulator::Update ()
	{
		if ( not _output or not _swapchain )
			return false;

		if ( not _output->Update() )
			return false;

		uint2	surf_size = _output->GetSize();

		if ( Any(surf_size != _swapchain->GetSurfaceSize()) )
		{
			CHECK_ERR( _swapchain->Recreate( surf_size ));
		}

		// controllers emulation
		return true;
	}
	
/*
=================================================
	GetCamera
=================================================
*/
	void  VRDeviceEmulator::GetCamera (OUT VRCamera &camera) const
	{
		camera.clipPlanes	= _clipPlanes;
		camera.left.proj	= _projection;
		camera.left.view	= Mat4_t::Identity();
		camera.right.proj	= _projection;
		camera.right.view	= Mat4_t::Identity();
		camera.pose			= Mat3_t::Identity();
		camera.position		= float3(0.0f);
	}

/*
=================================================
	SetupCamera
=================================================
*/
	void VRDeviceEmulator::SetupCamera (const float2 &clipPlanes)
	{
		if ( not Any( Equals( _clipPlanes, clipPlanes )) )
		{
			_clipPlanes = clipPlanes;
			
			const float	fov_y			= 1.0f;
			const float	aspect			= 1.0f;
			const float	tan_half_fovy	= tan( fov_y * 0.5f );
			_projection = Mat4_t{};

			_projection[0][0] = 1.0f / (aspect * tan_half_fovy);
			_projection[1][1] = 1.0f / tan_half_fovy;
			_projection[2][2] = _clipPlanes[1] / (_clipPlanes[1] - _clipPlanes[0]);
			_projection[2][3] = 1.0f;
			_projection[3][2] = -(_clipPlanes[1] * _clipPlanes[0]) / (_clipPlanes[1] - _clipPlanes[0]);
		}
	}
	
/*
=================================================
	Submit
----
	Image must be in 'VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL' layout and
	must be created with usage flags 'VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT'
=================================================
*/
	bool VRDeviceEmulator::Submit (const VRImage &img, Eye eye)
	{
		CHECK_ERR( img.queueFamilyIndex < _queues.capacity() );
		CHECK_ERR( _swapchain and _vkLogicalDevice );

		VkBool32	supports_present = false;
		VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _vkPhysicalDevice, img.queueFamilyIndex, _swapchain->GetVkSurface(), OUT &supports_present ));

		_queues.resize( Max(_queues.size(), img.queueFamilyIndex+1 ));

		auto&	q = _queues[img.queueFamilyIndex];

		// create command pool
		if ( not q.cmdPool )
		{
			VkCommandPoolCreateInfo	info = {};
			info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			info.queueFamilyIndex	= img.queueFamilyIndex;
			VK_CHECK( vkCreateCommandPool( _vkLogicalDevice, &info, null, OUT &q.cmdPool ));
		}

		// create command buffer
		if ( not q.cmdBuffers[q.frame] )
		{
			VkCommandBufferAllocateInfo	info = {};
			info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.commandBufferCount	= 1;
			info.commandPool		= q.cmdPool;
			info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			VK_CHECK( vkAllocateCommandBuffers( _vkLogicalDevice, &info, OUT &q.cmdBuffers[q.frame] ));
		}

		// create or reset fence
		if ( not q.fences[q.frame] )
		{
			VkFenceCreateInfo	info = {};
			info.sType		= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			info.flags		= 0;
			VK_CHECK( vkCreateFence( _vkLogicalDevice, &info, null, OUT &q.fences[q.frame] ));
		}
		else
		{
			VK_CHECK( vkWaitForFences( _vkLogicalDevice, 1, &q.fences[q.frame], true, ~0ull ));
			VK_CHECK( vkResetFences( _vkLogicalDevice, 1, &q.fences[q.frame] ));
		}

		// create semaphore
		if ( not q.waitSemaphores[q.frame] )
		{
			VkSemaphoreCreateInfo	info = {};
			info.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			info.flags		= 0;
			VK_CHECK( vkCreateSemaphore( _vkLogicalDevice, &info, null, OUT &q.waitSemaphores[q.frame] ));
		}
		if ( not q.signalSemaphores[q.frame] )
		{
			VkSemaphoreCreateInfo	info = {};
			info.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			info.flags		= 0;
			VK_CHECK( vkCreateSemaphore( _vkLogicalDevice, &info, null, OUT &q.signalSemaphores[q.frame] ));
		}

		// begin
		{
			VkCommandBufferBeginInfo	info = {};
			info.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags		= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			VK_CHECK( vkBeginCommandBuffer( q.cmdBuffers[q.frame], &info ));
		}

		// acquire image
		bool	just_acquired = false;
		if ( not _swapchain->IsImageAcquired() )
		{
			VK_CHECK( _swapchain->AcquireNextImage( q.waitSemaphores[q.frame] ));
			just_acquired = true;

			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcAccessMask		= 0;
			barrier.dstAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.image				= _swapchain->GetCurrentImage();
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			vkCmdPipelineBarrier( q.cmdBuffers[q.frame], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}

		// blit
		{
			const uint2		surf_size = _swapchain->GetSurfaceSize();
			VkImageBlit		region	= {};
			region.srcOffsets[0]	= { int(img.dimension.x * img.bounds.left + 0.5f), int(img.dimension.y * img.bounds.top - 0.5f), 0 };
			region.srcOffsets[1]	= { int(img.dimension.x * img.bounds.right + 0.5f), int(img.dimension.y * img.bounds.bottom + 0.5f), 1 };
			region.srcSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			region.dstOffsets[0]	= eye == Eye::Left ? VkOffset3D{ 0, 0, 0 } : VkOffset3D{ int(surf_size.x/2), 0, 0 };
			region.dstOffsets[1]	= eye == Eye::Left ? VkOffset3D{ int(surf_size.x/2), int(surf_size.y), 1 } : VkOffset3D{ int(surf_size.x), int(surf_size.y), 1 };
			region.dstSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			vkCmdBlitImage( q.cmdBuffers[q.frame], img.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _swapchain->GetCurrentImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR );
		}

		// barrier
		if ( not just_acquired )
		{
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask		= 0;
			barrier.oldLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout			= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.image				= _swapchain->GetCurrentImage();
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			vkCmdPipelineBarrier( q.cmdBuffers[q.frame], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}

		VK_CHECK( vkEndCommandBuffer( q.cmdBuffers[q.frame] ));

		// submit
		{
			VkPipelineStageFlags	stage	= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			VkSubmitInfo			info	= {};
			info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			info.commandBufferCount	= 1;
			info.pCommandBuffers	= &q.cmdBuffers[q.frame];

			if ( just_acquired )
			{
				info.pWaitSemaphores		= &q.waitSemaphores[q.frame];
				info.pWaitDstStageMask		= &stage;
				info.waitSemaphoreCount		= 1;
				info.pSignalSemaphores		= &q.signalSemaphores[q.frame];
				info.signalSemaphoreCount	= 1;
			}
			else
			{
				info.pWaitSemaphores		= &_lastSignal;
				info.pWaitDstStageMask		= &stage;
				info.waitSemaphoreCount		= 1;
				info.pSignalSemaphores		= &q.signalSemaphores[q.frame];
				info.signalSemaphoreCount	= 1;
			}

			VK_CHECK( vkQueueSubmit( img.currQueue, 1, &info, q.fences[q.frame] ));

			_lastSignal = q.signalSemaphores[q.frame];
		}
		
		if ( not just_acquired )
		{
			VK_CHECK( _swapchain->Present( img.currQueue, {_lastSignal} ));
			_lastSignal = VK_NULL_HANDLE;

			VK_CHECK( vkQueueWaitIdle( img.currQueue ));
		}

		if ( ++q.frame >= PerQueue::MaxFrames )
		{
			q.frame = 0;
		}
		return true;
	}
	
/*
=================================================
	GetRequiredInstanceExtensions
=================================================
*/
	Array<String>  VRDeviceEmulator::GetRequiredInstanceExtensions () const
	{
		CHECK_ERR( _isCreated );

		ArrayView<const char*>	ext		= _output->GetVulkanSurface()->GetRequiredExtensions();
		Array<String>			result;	result.reserve( ext.size() );

		for (auto& e : ext) {
			result.emplace_back( e );
		}
		return result;
	}
	
/*
=================================================
	GetRequiredDeviceExtensions
=================================================
*/
	Array<String>  VRDeviceEmulator::GetRequiredDeviceExtensions (VkInstance) const
	{
		CHECK_ERR( _isCreated );
		return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	}
	
/*
=================================================
	GetRenderTargetDimension
=================================================
*/
	uint2  VRDeviceEmulator::GetRenderTargetDimension () const
	{
		CHECK_ERR( _isCreated );
		return uint2{1024};
	}
		
/*
=================================================
	IsEnabled
=================================================
*/
	bool  VRDeviceEmulator::IsEnabled () const
	{
		return _isCreated;
	}

}	// FGC
