// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Command Queue
	//

	class VCommandQueue
	{
	// types
	private:
		using CommandBuffers_t	= Array< VkCommandBuffer >;

		struct PerFrame
		{
			CommandBuffers_t	commands;
			VkFence				fence		= VK_NULL_HANDLE;
			VkSemaphore			semaphore	= VK_NULL_HANDLE;
		};
		using PerFrameArray_t	= FixedArray< PerFrame, FG_MaxSwapchainLength >;

		static constexpr uint	MaxItems = 32;

		using WaitSemaphores_t	= FixedArray< VkSemaphore, MaxItems >;
		using WaitDstStages_t	= FixedArray< VkPipelineStageFlags, MaxItems >;


	// variables
	private:
		PerFrameArray_t		_perFrame;
		uint				_currFrame;

		VDevice const&		_device;

		VkQueue				_queueId;
		VkQueueFlags		_queueFlags;
		uint				_familyIndex;
		float				_priority;
		bool				_supportsPresent;

		VkCommandPool		_cmdPoolId;

		WaitSemaphores_t	_waitSemaphores;
		WaitDstStages_t		_waitDstStageMasks;

		bool				_useFence;

		StaticString<64>	_debugName;


	// methods
	public:
		VCommandQueue (const VDevice &dev, VkQueue id, VkQueueFlags queueFlags, uint familyIndex,
					   float priority, bool supportsPresent, StringView debugName);
		~VCommandQueue ();

		bool Initialize (uint swapchainLength, bool useFence);
		void Deinitialize ();

		bool OnBeginFrame (uint frameIdx);

		bool SubmitFrame ();
		void WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage);

		//void ResetCommands ();

		ND_ VkCommandBuffer	CreateCommandBuffer ();

		ND_ VkQueueFlags	GetQueueFlags ()		const	{ return _queueFlags; }
		ND_ uint			GetFamilyIndex ()		const	{ return _familyIndex; }
		ND_ float			GetPriority ()			const	{ return _priority; }
		ND_ bool			IsSupportsPresent ()	const	{ return _supportsPresent; }
		ND_ StringView		GetDebugName ()			const	{ return _debugName; }

	private:
		bool _CreatePool ();
		void _DestroyPool ();
	};


}	// FG
