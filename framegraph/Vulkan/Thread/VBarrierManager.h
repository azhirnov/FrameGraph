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
		using MemoryBarriers_t			= Array< VkMemoryBarrier >;		// TODO: custom allocator
		using ImageMemoryBarriers_t		= Array< VkImageMemoryBarrier >;
		using BufferMemoryBarriers_t	= Array< VkBufferMemoryBarrier >;


	// variables
	private:
		ImageMemoryBarriers_t		_imageBarriers;
		BufferMemoryBarriers_t		_bufferBarriers;
		MemoryBarriers_t			_memoryBarriers;

		VkPipelineStageFlags		_srcStageMask		= 0;
		VkPipelineStageFlags		_dstStageMask		= 0;
		VkDependencyFlags			_dependencyFlags	= 0;


	// methods
	public:
		explicit VBarrierManager ()
		{
			_memoryBarriers.reserve( 8 );
			_imageBarriers.reserve( 32 );
			_bufferBarriers.reserve( 64 );
		}


		void Commit (const VDevice &dev, VkCommandBuffer cmd)
		{
			if ( _memoryBarriers.size() or _bufferBarriers.size() or _imageBarriers.size() )
			{
				dev.vkCmdPipelineBarrier( cmd, _srcStageMask, _dstStageMask, _dependencyFlags,
										  uint(_memoryBarriers.size()), _memoryBarriers.data(),
										  uint(_bufferBarriers.size()), _bufferBarriers.data(),
										  uint(_imageBarriers.size()), _imageBarriers.data() );
			}
			Clear();
		}


		void Clear ()
		{
			_imageBarriers.clear();
			_bufferBarriers.clear();
			_memoryBarriers.clear();
			_srcStageMask = _dstStageMask = 0;
			_dependencyFlags = 0;
		}


		ND_ bool  Empty () const
		{
			return	_imageBarriers.empty()	and
					_bufferBarriers.empty()	and
					_memoryBarriers.empty();
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


		void AddMemoryBarrier (VkPipelineStageFlags		srcStageMask,
							   VkPipelineStageFlags		dstStageMask,
							   VkDependencyFlags		dependencyFlags,
							   const VkMemoryBarrier	&barrier)
		{
			_srcStageMask		|= srcStageMask;
			_dstStageMask		|= dstStageMask;
			_dependencyFlags	|= dependencyFlags;

			_memoryBarriers.push_back( barrier );
		}
	};

}	// FG
