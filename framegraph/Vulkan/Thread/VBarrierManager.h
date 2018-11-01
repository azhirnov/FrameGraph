// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VDevice.h"

namespace FG
{

	//
	// Vulkan Barrier Manager
	//

	class VBarrierManager final
	{
	// types
	private:
		using ImageMemoryBarriers_t		= Array< VkImageMemoryBarrier >;
		using BufferMemoryBarriers_t	= Array< VkBufferMemoryBarrier >;


	// variables
	private:
		ImageMemoryBarriers_t		_imageBarriers;
		BufferMemoryBarriers_t		_bufferBarriers;
		VkPipelineStageFlags		_srcStageMask		= 0;
		VkPipelineStageFlags		_dstStageMask		= 0;
		VkDependencyFlags			_dependencyFlags	= 0;


	// methods
	public:
		explicit VBarrierManager ()
		{
			_imageBarriers.reserve( 32 );
			_bufferBarriers.reserve( 64 );
		}


		void Commit (const VDevice &dev, VkCommandBuffer cmd)
		{
			if ( (not _bufferBarriers.empty()) or (not _imageBarriers.empty()) )
			{
				dev.vkCmdPipelineBarrier( cmd, _srcStageMask, _dstStageMask, _dependencyFlags,
										  0, null,
										  uint(_bufferBarriers.size()), _bufferBarriers.data(),
										  uint(_imageBarriers.size()), _imageBarriers.data() );
			}

			Clear();
		}


		void Clear ()
		{
			_imageBarriers.clear();
			_bufferBarriers.clear();
			_srcStageMask = _dstStageMask = 0;
			_dependencyFlags = 0;
		}


		void AddBufferBarrier (VkPipelineStageFlags			srcStageMask,
							   VkPipelineStageFlags			dstStageMask,
							   VkDependencyFlags			dependencyFlags,
							   const VkBufferMemoryBarrier	&barrier)
		{
			_srcStageMask		|= srcStageMask;
			_dstStageMask		|= dstStageMask;
			_dependencyFlags	|= dependencyFlags;

			_bufferBarriers.push_back( barrier );
		}
		

		void AddImageBarrier (VkPipelineStageFlags			srcStageMask,
							  VkPipelineStageFlags			dstStageMask,
							  VkDependencyFlags				dependencyFlags,
							  const VkImageMemoryBarrier	&barrier)
		{
			_srcStageMask		|= srcStageMask;
			_dstStageMask		|= dstStageMask;
			_dependencyFlags	|= dependencyFlags;

			_imageBarriers.push_back( barrier );
		}
	};

}	// FG
