// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	enum class EShader : uint
	{
		Vertex,
		TessControl,
		TessEvaluation,
		Geometry,
		Fragment,
		Compute,

		MeshTask,
		Mesh,
		
		RayGen,
		RayAnyHit,
		RayClosestHit,
		RayMiss,
		RayIntersection,
		RayCallable,

		_Count,
		Unknown		= ~0u,
	};

	
	enum class EShaderStages : uint
	{
		Vertex			= 1 << uint(EShader::Vertex),
		TessControl		= 1 << uint(EShader::TessControl),
		TessEvaluation	= 1 << uint(EShader::TessEvaluation),
		Geometry		= 1 << uint(EShader::Geometry),
		Fragment		= 1 << uint(EShader::Fragment),
		Compute			= 1 << uint(EShader::Compute),
		MeshTask		= 1 << uint(EShader::MeshTask),
		Mesh			= 1 << uint(EShader::Mesh),
		RayGen			= 1 << uint(EShader::RayGen),
		RayAnyHit		= 1 << uint(EShader::RayAnyHit),
		RayClosestHit	= 1 << uint(EShader::RayClosestHit),
		RayMiss			= 1 << uint(EShader::RayMiss),
		RayIntersection	= 1 << uint(EShader::RayIntersection),
		RayCallable		= 1 << uint(EShader::RayCallable),
		_Last,

		All				= ((_Last-1) << 1) - 1,
		AllGraphics		= Vertex | TessControl | TessEvaluation | Geometry | Fragment,
		Unknown			= 0,
	};
	FG_BIT_OPERATORS( EShaderStages );
	

	enum class EShaderAccess : uint
	{
		ReadOnly,
		WriteOnly,
		WriteDiscard,	// (optimization) same as WriteOnly, but previous data will be discarded
		ReadWrite,
	};

	
	enum class EShaderLangFormat : uint
	{
		// api
		_ApiOffset			= 18,
		_ApiMask			= 0xF << _ApiOffset,
		OpenGL				= 1 << _ApiOffset,
		OpenGLES			= 2 << _ApiOffset,
		DirectX				= 3 << _ApiOffset,
		OpenCL				= 4 << _ApiOffset,
		Vulkan				= 5 << _ApiOffset,
		Metal				= 6 << _ApiOffset,
		CUDA				= 7 << _ApiOffset,
		Software			= 8 << _ApiOffset,	// c++ shader for software renderer

		// version
		_VersionOffset		= 8,
		_VersionMask		= 0x3FF << _VersionOffset,
		OpenGL_330			= (330 << _VersionOffset) | OpenGL,
		OpenGL_420			= (420 << _VersionOffset) | OpenGL,
		OpenGL_430			= (430 << _VersionOffset) | OpenGL,
		OpenGL_440			= (440 << _VersionOffset) | OpenGL,
		OpenGL_450			= (450 << _VersionOffset) | OpenGL,
		OpenGL_460			= (460 << _VersionOffset) | OpenGL,
		OpenGLES_200		= (200 << _VersionOffset) | OpenGLES,
		OpenGLES_300		= (300 << _VersionOffset) | OpenGLES,
		OpenGLES_310		= (310 << _VersionOffset) | OpenGLES,
		OpenGLES_320		= (320 << _VersionOffset) | OpenGLES,
		DirectX_10			= (100 << _VersionOffset) | DirectX,
		DirectX_11			= (110 << _VersionOffset) | DirectX,
		DirectX_12			= (120 << _VersionOffset) | DirectX,
		OpenCL_120			= (120 << _VersionOffset) | OpenCL,
		OpenCL_200			= (200 << _VersionOffset) | OpenCL,
		OpenCL_210			= (210 << _VersionOffset) | OpenCL,
		Vulkan_100			= (100 << _VersionOffset) | Vulkan,
		Vulkan_110			= (110 << _VersionOffset) | Vulkan,
		Software_100		= (100 << _VersionOffset) | Software,

		// storage
		_StorageOffset		= 4,
		_StorageMask		= 0xF << _StorageOffset,
		Source				= 1 << _StorageOffset,
		Binary				= 2 << _StorageOffset,					// compiled program (HLSL bytecode, SPIRV)
		Executable			= 3 << _StorageOffset,					// compiled program (exe, dll, so, ...)

		// format
		_FormatOffset		= 0,
		_FormatMask			= 0xF << _FormatOffset,
		HighLevel			= (1 << _FormatOffset) | Source,		// GLSL, HLSL, CL
		SPIRV				= (2 << _FormatOffset) | Binary,
		GL_Binary			= (3 << _FormatOffset) | Binary,		// vendor specific
		DXBC				= (4 << _FormatOffset) | Binary,		// HLSL bytecode
		DXIL				= (5 << _FormatOffset) | Binary,		// HLSL IL
		Assembler			= (6 << _FormatOffset) | Source,
		Invocable			= (7 << _FormatOffset) | Executable,	// function pointer
		ShaderModule		= (8 << _FormatOffset) | Executable,	// vkShaderModule, GLuint, ...

		// debug/profiling mode
		_ModeOffset			= 22,
		_ModeMask			= 0xF << _ModeOffset,
		WithDebugTrace		= 1 << _ModeOffset,		// writes trace only for selected shader invocation (may be very slow)
		WithDebugAsserts	= 2 << _ModeOffset,		// writes only shader unique invocation id in which assert was trigged
		WithDebugView		= 3 << _ModeOffset,		// if assertion failed then writes specified value in separate image
		WithProfiling		= 4 << _ModeOffset,		// 

		// independent flags
		_FlagsOffset		= 26,
		_FlagsMask			= 0xF << _FlagsOffset,
		HasInputAttachment	= 1 << _FlagsOffset,	// if shader contains input attachment then may be generated 2 shaders:
													// 1. keep input attachments and add flag 'HasInputAttachment'.
													// 2. replaces attachments by sampler2D and texelFetch function.

		Unknown				= 0,

		// combined masks
		_StorageFormatMask		= _StorageMask | _FormatMask,
		_ApiStorageFormatMask	= _ApiMask | _StorageMask | _FormatMask,
		_ApiVersionMask			= _ApiMask | _VersionMask,

		// default
		GLSL_440		= OpenGL_440 | HighLevel,
		GLSL_450		= OpenGL_450 | HighLevel,
		GLSL_460		= OpenGL_460 | HighLevel,
		VKSL_100		= Vulkan_100 | HighLevel,
		VKSL_110		= Vulkan_110 | HighLevel,
		SPIRV_100		= Vulkan_100 | SPIRV,
		SPIRV_110		= Vulkan_110 | SPIRV,
		VkShader_100	= Vulkan_100 | ShaderModule,
		VkShader_110	= Vulkan_110 | ShaderModule,
	};
	FG_BIT_OPERATORS( EShaderLangFormat );


	enum class EShaderDebugMode : uint
	{
		None,
		Trace,
		Asserts,
		View,
		Unknown		= None,
	};


}	// FG
