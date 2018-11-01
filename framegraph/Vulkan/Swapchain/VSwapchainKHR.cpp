// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VSwapchainKHR.h"
#include "VFrameGraphThread.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VSwapchainKHR::VSwapchainKHR (const VDevice &dev, const VulkanSwapchainInfo &ci) :
		_swapchain{ dev.GetVkPhysicalDevice(), dev.GetVkDevice(), BitCast<VkSurfaceKHR>(ci.surface), dev }
	{
	}

/*
=================================================
	destructor
=================================================
*/
	VSwapchainKHR::~VSwapchainKHR ()
	{
		ASSERT( not _imageAvailable );
		ASSERT( not _renderFinished );
	}
	
/*
=================================================
	Acquire
=================================================
*/
	bool VSwapchainKHR::Acquire (VFrameGraphThread &fg)
	{
		fg.WaitSemaphore( _imageAvailable, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
		fg.SignalSemaphore( _renderFinished );

		_swapchain.AcquireNextImage( _imageAvailable );
		return true;
	}
	
/*
=================================================
	Present
=================================================
*/
	bool VSwapchainKHR::Present (VFrameGraphThread &)
	{
		_swapchain.Present( _presentQueue, {_renderFinished} );
		return true;
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VSwapchainKHR::Initialize (VkQueue queue)
	{
		CHECK_ERR( queue );
		
		// create semaphores
		{
			auto&		fn	= _swapchain;
			VkDevice	dev	= _swapchain.GetVkDevice();

			VkSemaphoreCreateInfo	info = {};
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VK_CHECK( fn.vkCreateSemaphore( dev, &info, null, OUT &_imageAvailable ));
			VK_CHECK( fn.vkCreateSemaphore( dev, &info, null, OUT &_renderFinished ));
		}

		// create swapchain
		{
			VkFormat		color_fmt	= VK_FORMAT_UNDEFINED;
			VkColorSpaceKHR	color_space	= VK_COLOR_SPACE_MAX_ENUM_KHR;

			CHECK_ERR( _swapchain.ChooseColorFormat( INOUT color_fmt, INOUT color_space ));
			CHECK_ERR( _swapchain.Create( uint2(128,128), color_fmt, color_space ));

			_presentQueue = queue;
		}

		_imageIDs.resize( _swapchain.GetSwapchainLength() );
		return true;
	}

/*
=================================================
	Deinitialize
=================================================
*/
	void VSwapchainKHR::Deinitialize ()
	{
		auto&		fn	= _swapchain;
		VkDevice	dev	= _swapchain.GetVkDevice();

		fn.vkDestroySemaphore( dev, _imageAvailable, null );
		fn.vkDestroySemaphore( dev, _renderFinished, null );

		_imageAvailable	= VK_NULL_HANDLE;
		_renderFinished	= VK_NULL_HANDLE;
		_presentQueue	= VK_NULL_HANDLE;
		_swapchain.Destroy();

		// TODO: destroy _imageIDs
		_imageIDs.clear();
	}

/*
=================================================
	IsCompatibleWithQueue
=================================================
*/
	bool VSwapchainKHR::IsCompatibleWithQueue (uint familyIndex) const
	{
		VkBool32	supports_present = 0;
		VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _swapchain.GetVkPhysicalDevice(), familyIndex, _swapchain.GetVkSurface(), OUT &supports_present ));
		return !!supports_present;
	}
	
/*
=================================================
	GetImage
=================================================
*/
	RawImageID  VSwapchainKHR::GetImage (ESwapchainImage type)
	{
		CHECK_ERR( type == ESwapchainImage::Primary );
		CHECK_ERR( _swapchain.GetCurretImageIndex() < _swapchain.GetSwapchainLength() );	// image must be acquired

		return _imageIDs[ _swapchain.GetCurretImageIndex() ].Get();
	}


}	// FG


// TODO: ...
#define FG_NO_VULKANDEVICE
#define VulkanSwapchain  KHRSwapchainImpl
#include "extensions/framework/Vulkan/VulkanSwapchain.cpp"
#undef  VulkanSwapchain
