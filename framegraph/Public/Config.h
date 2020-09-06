// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifndef FG_OPTIMIZE_IDS
# ifdef FG_DEBUG
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
	static constexpr unsigned	FG_MaxDescriptorSets		= 4;
	static constexpr unsigned	FG_MaxBufferDynamicOffsets	= 12;	// 8 UBO + 4 SSBO
	static constexpr unsigned	FG_MaxElementsInUnsizedDesc	= 1;	// if used extension GL_EXT_nonuniform_qualifier
	
	// memory
	static constexpr unsigned	FG_VkDevicePageSizeMb		= 64;

# else

	// buffer
	static constexpr unsigned	FG_MaxVertexBuffers			= 8;
	static constexpr unsigned	FG_MaxVertexAttribs			= 16;
	
	// render pass
	static constexpr unsigned	FG_MaxColorBuffers			= 8;
	static constexpr unsigned	FG_MaxViewports				= 16;

	// pipeline
	static constexpr bool		FG_EnableShaderDebugging	= true;
	static constexpr unsigned	FG_MaxDescriptorSets		= 8;
	static constexpr unsigned	FG_MaxBufferDynamicOffsets	= 16;
	static constexpr unsigned	FG_MaxElementsInUnsizedDesc	= 64;	// if used extension GL_EXT_nonuniform_qualifier

	// memory
	static constexpr unsigned	FG_VkDevicePageSizeMb		= 256;

# endif


	// render pass
	static constexpr unsigned	FG_MaxRenderPassSubpasses	= 8;

	// pipeline
	static constexpr unsigned	FG_MaxPushConstants			= 8;
	static constexpr unsigned	FG_MaxPushConstantsSize		= 128;	// bytes
	static constexpr unsigned	FG_MaxSpecConstants			= 8;
	static constexpr unsigned	FG_DebugDescriptorSet		= FG_MaxDescriptorSets-1;

	// queue
	static constexpr unsigned	FG_MaxQueueFamilies			= 32;

	// task
	static constexpr unsigned	FG_MaxTaskDependencies		= 8;
	static constexpr unsigned	FG_MaxCopyRegions			= 8;
	static constexpr unsigned	FG_MaxClearRanges			= 8;
	static constexpr unsigned	FG_MaxBlitRegions			= 8;
	static constexpr unsigned	FG_MaxResolveRegions		= 8;
	static constexpr unsigned	FG_MaxDrawCommands			= 4;


}	// FG


// check definitions
#ifdef FG_CPP_DETECT_MISMATCH
#  if FG_OPTIMIZE_IDS
#	pragma detect_mismatch( "FG_OPTIMIZE_IDS", "1" )
#  else
#	pragma detect_mismatch( "FG_OPTIMIZE_IDS", "0" )
#  endif
#endif	// FG_CPP_DETECT_MISMATCH
