// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifndef FG_OPTIMIZE_IDS
# if 1 //def FG_DEBUG
#	define FG_OPTIMIZE_IDS		false
# else
#	define FG_OPTIMIZE_IDS		true
# endif
#endif


namespace FG
{

# ifdef PLATFORM_ANDROID

	// buffer
	static constexpr unsigned	FG_MaxVertexBuffers			= 8;
	static constexpr unsigned	FG_MaxVertexAttribs			= 16;

	// render pass
	static constexpr unsigned	FG_MaxColorBuffers			= 4;
	static constexpr unsigned	FG_MaxViewports				= 1;
	
	// pipeline
	static constexpr bool		FG_EnableShaderDebugging	= false;
	static constexpr unsigned	FG_MaxDescriptorSets		= 4 - unsigned(FG_EnableShaderDebugging);
	static constexpr unsigned	FG_MaxBufferDynamicOffsets	= 12;	// 8 UBO + 4 SSBO
	
	// memory manager
	static constexpr unsigned	FG_VkHostWritePageSizeMb	= 64;	// Mb
	static constexpr unsigned	FG_VkDevicePageSizeMb		= 64;	// Mb
	
	// resource manager
	static constexpr unsigned	FG_MaxResources				= 8u << 10;

# else

	// buffer
	static constexpr unsigned	FG_MaxVertexBuffers			= 8;
	static constexpr unsigned	FG_MaxVertexAttribs			= 16;
	
	// render pass
	static constexpr unsigned	FG_MaxColorBuffers			= 8;
	static constexpr unsigned	FG_MaxViewports				= 16;

	// pipeline
	static constexpr bool		FG_EnableShaderDebugging	= true;
	static constexpr unsigned	FG_MaxDescriptorSets		= 8 - unsigned(FG_EnableShaderDebugging);
	static constexpr unsigned	FG_MaxBufferDynamicOffsets	= 16;
	static constexpr unsigned	FG_MaxElementsInUnsizedDesc	= 64;	// if used extension GL_EXT_nonuniform_qualifier

	// memory manager
	static constexpr unsigned	FG_VkHostWritePageSizeMb	= 256;	// Mb
	static constexpr unsigned	FG_VkDevicePageSizeMb		= 256;	// Mb
	
	// resource manager
	static constexpr unsigned	FG_MaxResources				= 32u << 10;

# endif


	// render pass
	static constexpr unsigned	FG_MaxRenderPassSubpasses	= 8;

	// pipeline
	static constexpr unsigned	FG_MaxPushConstants			= 8;
	static constexpr unsigned	FG_MaxPushConstantsSize		= 128;	// bytes
	static constexpr unsigned	FG_MaxSpecConstants			= 8;

	// queue
	static constexpr unsigned	FG_MaxQueueFamilies			= 32;

	// task
	static constexpr unsigned	FG_MaxTaskDependencies		= 8;
	static constexpr unsigned	FG_MaxCopyRegions			= 8;
	static constexpr unsigned	FG_MaxClearRanges			= 8;
	static constexpr unsigned	FG_MaxBlitRegions			= 8;
	static constexpr unsigned	FG_MaxResolveRegions		= 8;

	// frame graph
	static constexpr unsigned	FG_MaxRingBufferSize			= 4;
	static constexpr unsigned	FG_MaxCommandBatchCount			= 8;
	static constexpr unsigned	FG_MaxCommandBatchDependencies	= 4;


}	// FG


// check definitions
#if defined (COMPILER_MSVC) or defined (COMPILER_CLANG)
#  if FG_OPTIMIZE_IDS
#	pragma detect_mismatch( "FG_OPTIMIZE_IDS", "1" )
#  else
#	pragma detect_mismatch( "FG_OPTIMIZE_IDS", "0" )
#  endif
#endif	// COMPILER_MSVC or COMPILER_CLANG
