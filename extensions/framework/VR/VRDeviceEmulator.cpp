// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framework/VR/VRDeviceEmulator.h"

namespace FGC
{
namespace {

	template <typename T>
	static constexpr T  Pi = T( 3.14159265358979323846 );

/*
=================================================
	Rotate*
=================================================
*/
	IVRDevice::Mat3_t  RotateX (float angle)
	{
		float	s = sin( angle );
		float	c = cos( angle );

		return IVRDevice::Mat3_t{
			1.0f,  0.0f,  0.0f,
			0.0f,   c,     s,
			0.0f,  -s,     c
		};
	}

	IVRDevice::Mat3_t  RotateY (float angle)
	{
		float	s = sin( angle );
		float	c = cos( angle );

		return IVRDevice::Mat3_t{
			 c,    0.0f,  -s,
			0.0f,  1.0f,  0.0f,
			 s,    0.0f,   c
		};
	}

	IVRDevice::Mat3_t  RotateZ (float angle)
	{
		float	s = sin( angle );
		float	c = cos( angle );

		return IVRDevice::Mat3_t{
			 c,    0.0f,   s,
			-s,     c,    0.0f,
			0.0f,  0.0f,  1.0f
		};
	}
}
//-----------------------------------------------------------------------------


/*
=================================================
	OnKey
=================================================
*/
	void  VRDeviceEmulator::WindowEventListener::OnKey (StringView key, EKeyAction action)
	{
		if ( action != EKeyAction::Up )
		{
			if ( key == "W" )			_controller.left.dpad.y += 1.0f;		else
			if ( key == "S" )			_controller.left.dpad.y -= 1.0f;
			if ( key == "D" )			_controller.left.dpad.x -= 1.0f;		else
			if ( key == "A" )			_controller.left.dpad.x += 1.0f;

			if ( key == "arrow up" )	_controller.right.dpad.y += 1.0f;		else
			if ( key == "arrow down" )	_controller.right.dpad.y -= 1.0f;
			if ( key == "arrow right" )	_controller.right.dpad.x -= 1.0f;		else
			if ( key == "arrow left" )	_controller.right.dpad.x += 1.0f;
		}

		if ( key == "left mb" )			_mousePressed = (action != EKeyAction::Up);
	}
	
/*
=================================================
	OnMouseMove
=================================================
*/
	void  VRDeviceEmulator::WindowEventListener::OnMouseMove (const float2 &pos)
	{
		if ( _mousePressed )
		{
			float2	delta  = pos - _lastMousePos;
			_cameraAngle   += float2{delta.x, -delta.y} * _mouseSens;
		}
		_lastMousePos = pos;
	}
			
/*
=================================================
	Update
=================================================
*/
	void  VRDeviceEmulator::WindowEventListener::Update (OUT Mat3_t &view, INOUT ControllerEmulator &cont)
	{
		_cameraAngle.x = Wrap( _cameraAngle.x, -Pi<float>, Pi<float> );
		_cameraAngle.y = Wrap( _cameraAngle.y, -Pi<float>, Pi<float> );

		view = RotateX( -_cameraAngle.y ) * RotateY( -_cameraAngle.x );
		
		_controller.left.dpad	= Normalize( _controller.left.dpad );
		_controller.right.dpad	= Normalize( _controller.right.dpad );
		
		cont.left.dpadChanged	= false;
		cont.right.dpadChanged	= false;

		if ( Length( _controller.left.dpad ) > 0.01f )
		{
			cont.left.dpadDelta		= _controller.left.dpad - cont.left.dpad;
			cont.left.dpad			= _controller.left.dpad;
			cont.left.dpadChanged	= true;
		}
		
		if ( Length( _controller.right.dpad ) > 0.01f )
		{
			cont.right.dpadDelta	= _controller.right.dpad - cont.right.dpad;
			cont.right.dpad			= _controller.right.dpad;
			cont.right.dpadChanged	= true;
		}

		_controller	= Default;
	}
			
/*
=================================================
	OnDestroy
=================================================
*/
	void  VRDeviceEmulator::WindowEventListener::OnDestroy ()
	{
		_isActive	= false;
		_isVisible	= false;
	}
			
/*
=================================================
	OnResize
=================================================
*/
	void  VRDeviceEmulator::WindowEventListener::OnResize (const uint2 &newSize)
	{
		if ( All( newSize > uint2(0) ))
			_isVisible = true;
		else
			_isVisible = false;
	}
//-----------------------------------------------------------------------------


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
		_output->AddListener( &_wndListener );

		_camera.pose		= Mat3_t::Identity();
		_camera.position	= float3(0.0f);

		_camera.left.view	= Mat4_t{
			1.00000072f, -0.000183293581f, -0.000353380980f, -0.000000000f,
			0.000182049334f, 0.999995828f, -0.00308410777f, 0.000000000f,
			0.000353740237f, 0.00308382465f, 0.999995828f, -0.000000000f,
			0.0329737701f, -0.000433419773f, 0.000178515897f, 1.00000000f
		};
		_camera.right.view	= Mat4_t{
			1.00000072f, 0.000182215153f, 0.000351947267f, -0.000000000f,
			-0.000183455661f, 0.999995947f, 0.00308232009f, 0.000000000f,
			-0.000351546332f, -0.00308261835f, 0.999995947f, -0.000000000f,
			-0.0329739153f, 0.000422920042f, -0.000199772359f, 1.00000000f
		};
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
	bool  VRDeviceEmulator::Create ()
	{
		CHECK_ERR( _output );
		CHECK_ERR( not _isCreated );

		_isCreated	= true;
		_hmdStatus	= EHmdStatus::Mounted;

		for (auto& listener : _listeners) {
			listener->HmdStatusChanged( _hmdStatus );
		}

		_lastUpdateTime = TimePoint_t::clock::now();
		return true;
	}
	
/*
=================================================
	SetVKDevice
=================================================
*/
	bool  VRDeviceEmulator::SetVKDevice (VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
	{
		CHECK_ERR( _output and _isCreated );

		_vkInstance			= instance;
		_vkPhysicalDevice	= physicalDevice;
		_vkLogicalDevice	= logicalDevice;
		
		CHECK_ERR( VulkanLoader::Initialize() );
		CHECK_ERR( VulkanLoader::LoadInstance( _vkInstance ));
		CHECK_ERR( VulkanLoader::LoadDevice( _vkLogicalDevice, OUT _deviceFnTable ));

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
	void  VRDeviceEmulator::Destroy ()
	{
		_hmdStatus = EHmdStatus::PowerOff;

		for (auto& listener : _listeners) {
			listener->HmdStatusChanged( _hmdStatus );
		}
		_listeners.clear();

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
			_output->RemoveListener( &_wndListener );
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
	void  VRDeviceEmulator::AddListener (IVRDeviceEventListener *listener)
	{
		ASSERT( listener );
		_listeners.insert( listener );
	}
	
/*
=================================================
	RemoveListener
=================================================
*/
	void  VRDeviceEmulator::RemoveListener (IVRDeviceEventListener *listener)
	{
		ASSERT( listener );
		_listeners.erase( listener );
	}
	
/*
=================================================
	Update
=================================================
*/
	bool  VRDeviceEmulator::Update ()
	{
		// update hmd status
		{
			const EHmdStatus	new_stat =	not _wndListener.IsActive()	? EHmdStatus::PowerOff	:
											_wndListener.IsVisible()	? EHmdStatus::Mounted	: EHmdStatus::Active;

			if ( new_stat != _hmdStatus )
			{
				_hmdStatus = new_stat;

				for (auto& listener : _listeners) {
					listener->HmdStatusChanged( _hmdStatus );
				}
			}
		}

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
		_wndListener.Update( OUT _camera.pose, INOUT _controller );
		
		auto	time	= TimePoint_t::clock::now();
		auto	dt		= std::chrono::duration_cast<SecondsF>( time - _lastUpdateTime ).count();
		_lastUpdateTime = time;

		if ( _controller.left.dpadChanged )
		{
			for (auto& listener : _listeners) {
				listener->OnAxisStateChanged( ControllerID::LeftHand, "dpad", _controller.left.dpad, _controller.left.dpadDelta, dt );
			}
		}
		if ( _controller.right.dpadChanged )
		{
			for (auto& listener : _listeners) {
				listener->OnAxisStateChanged( ControllerID::RightHand, "dpad", _controller.right.dpad, _controller.right.dpadDelta, dt );
			}
		}

		return true;
	}

/*
=================================================
	SetupCamera
=================================================
*/
	void  VRDeviceEmulator::SetupCamera (const float2 &clipPlanes)
	{
		if ( not Any( Equals( _camera.clipPlanes, clipPlanes )) )
		{
			_camera.clipPlanes = clipPlanes;
			
			const float	fov_y			= 1.0f;
			const float	aspect			= 1.0f;
			const float	tan_half_fovy	= tan( fov_y * 0.5f );

			Mat4_t	proj;
			proj[0][0] = 1.0f / (aspect * tan_half_fovy);
			proj[1][1] = 1.0f / tan_half_fovy;
			proj[2][2] = clipPlanes[1] / (clipPlanes[1] - clipPlanes[0]);
			proj[2][3] = 1.0f;
			proj[3][2] = -(clipPlanes[1] * clipPlanes[0]) / (clipPlanes[1] - clipPlanes[0]);
			
			_camera.left.proj  = proj;
			_camera.right.proj = proj;
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
	bool  VRDeviceEmulator::Submit (const VRImage &img, Eye eye)
	{
		CHECK_ERR( uint(img.queueFamilyIndex) < _queues.capacity() );
		CHECK_ERR( _swapchain and _vkLogicalDevice );
		CHECK_ERR( _wndListener.IsActive() );

		VkBool32	supports_present = false;
		VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _vkPhysicalDevice, uint(img.queueFamilyIndex), _swapchain->GetVkSurface(), OUT &supports_present ));
		CHECK_ERR( supports_present );

		_queues.resize( Max(_queues.size(), uint(img.queueFamilyIndex) + 1 ));

		auto&	q = _queues[ uint(img.queueFamilyIndex) ];

		// create command pool
		if ( not q.cmdPool )
		{
			VkCommandPoolCreateInfo	info = {};
			info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			info.queueFamilyIndex	= uint(img.queueFamilyIndex);
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
			region.dstOffsets[0]	= ( eye == Eye::Left ?	VkOffset3D{ 0, int(surf_size.y), 0 }	: VkOffset3D{ int(surf_size.x/2), int(surf_size.y), 0 });
			region.dstOffsets[1]	= ( eye == Eye::Left ?	VkOffset3D{ int(surf_size.x/2), 0, 1 }	: VkOffset3D{ int(surf_size.x), 0, 1 });
			region.dstSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			vkCmdBlitImage( q.cmdBuffers[q.frame], img.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _swapchain->GetCurrentImage(),
						    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR );
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
		
		_submitted[uint(eye)] = true;
		
		if ( _submitted[uint(Eye::Left)] and _submitted[uint(Eye::Right)] )
		{
			_submitted[uint(Eye::Left)]  = false;
			_submitted[uint(Eye::Right)] = false;

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

}	// FGC
