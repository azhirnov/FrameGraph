// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VSwapchain.h"

// TODO: ...
#define VulkanSwapchain  KHRSwapchainImpl
#include "extensions/framework/Vulkan/VulkanSwapchain.h"
#undef  VulkanSwapchain

namespace FG
{

	//
	// Vulkan Default Swapchain (KHR)
	//

	class VSwapchainKHR : public VSwapchain
	{
	// types
	private:
		using SwapchainImages_t		= FixedArray< ImageID, FG_MaxSwapchainLength >;


	// variables
	protected:
		KHRSwapchainImpl	_swapchain;
		VkSemaphore			_imageAvailable		= VK_NULL_HANDLE;
		VkSemaphore			_renderFinished		= VK_NULL_HANDLE;
		VkQueue				_presentQueue		= VK_NULL_HANDLE;
		SwapchainImages_t	_imageIDs;


	// methods
	public:
		VSwapchainKHR (const VDevice &dev, const VulkanSwapchainInfo &ci);
		~VSwapchainKHR () override;
		
		bool Acquire (VFrameGraphThread &) override;
		bool Present (VFrameGraphThread &) override;
		
		bool Initialize (VkQueue queue) override;
		void Deinitialize () override;

		bool IsCompatibleWithQueue (uint familyIndex) const override;
		
		RawImageID  GetImage (ESwapchainImage type) override;
	};


}	// FG
