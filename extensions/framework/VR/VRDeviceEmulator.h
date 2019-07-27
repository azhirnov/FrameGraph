// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/VR/IVRDevice.h"
#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanSwapchain.h"

namespace FGC
{

	//
	// VR Device Emulator
	//

	class VRDeviceEmulator final : public IVRDevice, public VulkanDeviceFn
	{
	// types
	private:
		using Listeners_t	= HashSet< IVRDeviceEventListener *>;

		struct PerQueue
		{
			static constexpr uint	MaxFrames = 8;
			using CmdBuffers_t		= StaticArray< VkCommandBuffer, MaxFrames >;
			using Fences_t			= StaticArray< VkFence, MaxFrames >;
			using Semaphores_t		= StaticArray< VkSemaphore, MaxFrames >;

			VkCommandPool	cmdPool		= VK_NULL_HANDLE;
			CmdBuffers_t	cmdBuffers;
			Fences_t		fences;
			Semaphores_t	waitSemaphores;
			Semaphores_t	signalSemaphores;
			uint			frame		= 0;
		};

		class WindowEventListener final : public IWindowEventListener
		{
		private:
			using TimePoint_t	= std::chrono::high_resolution_clock::time_point;

		private:
			float3			_positionDelta;
			float2			_cameraAngle;
			float2			_lastMousePos;
			TimePoint_t		_lastUpdateTime;
			bool			_mousePressed		= false;
			const float		_mouseSens			= 0.01f;
			bool			_isActive			= true;

		public:
			void OnResize (const uint2 &) override {}
			void OnRefresh () override {}
			void OnDestroy () override;
			void OnUpdate () override {}
			void OnKey (StringView key, EKeyAction action) override;
			void OnMouseMove (const float2 &pos) override;
			void Update (OUT Mat3_t &pose, INOUT float3 &pos);
			ND_ bool IsActive () const { return _isActive; }
		};

		using Queues_t = FixedArray< PerQueue, 16 >;


	// variables
	private:
		Listeners_t				_listeners;
		VulkanDeviceFnTable		_deviceFnTable;
		VRCamera				_camera;
		WindowEventListener		_wndListener;

		VkInstance				_vkInstance;
		VkPhysicalDevice		_vkPhysicalDevice;
		VkDevice				_vkLogicalDevice;
		Queues_t				_queues;
		VkSemaphore				_lastSignal;
		
		WindowPtr				_output;
		VulkanSwapchainPtr		_swapchain;
		
		BitSet<2>				_submitted;
		bool					_isCreated;


	// methods
	public:
		explicit VRDeviceEmulator (WindowPtr);
		~VRDeviceEmulator () override;
		
		bool Create () override;
		bool SetVKDevice (VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice logicalDevice) override;
		void Destroy () override;
		void AddListener (IVRDeviceEventListener *listener) override;
		void RemoveListener (IVRDeviceEventListener *listener) override;
		bool Update () override;
		void SetupCamera (const float2 &clipPlanes) override;
		bool Submit (const VRImage &, Eye) override;

		VRCamera const&	GetCamera () const override						{ return _camera; }
		EHmdStatus		GetHmdStatus () const override;
		Array<String>	GetRequiredInstanceExtensions () const override;
		Array<String>	GetRequiredDeviceExtensions (VkInstance) const override;
		uint2			GetRenderTargetDimension () const override;
	};

}	// FGC
