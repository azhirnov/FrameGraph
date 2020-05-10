// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "EShaderCompilationFlags.h"
#include "framegraph/Shared/HashCollisionCheck.h"
#include <mutex>

namespace FG
{

	//
	// Vulkan Pipeline Compiler
	//

	class VPipelineCompiler final : public IPipelineCompiler
	{
	// types
	private:
		using StringShaderData	= PipelineDescription::SharedShaderPtr< String >;
		using BinaryShaderData	= PipelineDescription::SharedShaderPtr< Array<uint> >;
		using VkShaderPtr		= PipelineDescription::VkShaderPtr;

		struct BinaryShaderDataHash {
			ND_ size_t  operator () (const BinaryShaderData &value) const {
				return value->GetHashOfData();
			}
		};

		using ShaderCache_t		= HashMap< BinaryShaderData, VkShaderPtr >;
		using ShaderDataMap_t	= PipelineDescription::ShaderDataMap_t;


	// variables
	private:
		Mutex								_lock;
		Array< String >						_directories;
		UniquePtr< class SpirvCompiler >	_spirvCompiler;
		ShaderCache_t						_shaderCache;
		EShaderCompilationFlags				_compilerFlags			= Default;

		DEBUG_ONLY(
			HashCollisionCheck<>			_hashCollisionCheck;	// for uniforms and descriptor sets
		)

		// immutable:
		InstanceVk_t						_vkInstance;
		PhysicalDeviceVk_t					_vkPhysicalDevice;
		DeviceVk_t							_vkLogicalDevice;
		void *								_fpCreateShaderModule			= null;
		void *								_fpDestroyShaderModule			= null;


	// methods
	public:
		VPipelineCompiler ();
		VPipelineCompiler (InstanceVk_t instance, PhysicalDeviceVk_t physicalDevice, DeviceVk_t device);
		~VPipelineCompiler ();

		bool SetCompilationFlags (EShaderCompilationFlags flags);
		void AddDirectory (StringView path);
		
		// set debug flags for all shaders
		void SetDebugFlags (EShaderLangFormat flags);

		ND_ EShaderCompilationFlags  GetCompilationFlags ()		{ EXLOCK( _lock );  return _compilerFlags; }

		void ReleaseUnusedShaders ();
		void ReleaseShaderCache ();

		bool IsSupported (const MeshPipelineDesc &ppln, EShaderLangFormat dstFormat) const override;
		bool IsSupported (const RayTracingPipelineDesc &ppln, EShaderLangFormat dstFormat) const override;
		bool IsSupported (const GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat) const override;
		bool IsSupported (const ComputePipelineDesc &ppln, EShaderLangFormat dstFormat) const override;
		
		bool Compile (INOUT MeshPipelineDesc &ppln, EShaderLangFormat dstFormat) override;
		bool Compile (INOUT RayTracingPipelineDesc &ppln, EShaderLangFormat dstFormat) override;
		bool Compile (INOUT GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat) override;
		bool Compile (INOUT ComputePipelineDesc &ppln, EShaderLangFormat dstFormat) override;


	private:
		bool _MergePipelineResources (const PipelineDescription::PipelineLayout &srcLayout,
									  INOUT PipelineDescription::PipelineLayout &dstLayout) const;

		bool _MergeUniforms (const PipelineDescription::UniformMap_t &srcUniforms,
							 INOUT PipelineDescription::UniformMap_t &dstUniforms) const;

		bool _CreateVulkanShader (INOUT PipelineDescription::Shader &shader);

		static bool _IsSupported (const ShaderDataMap_t &data);
		
		void _CheckHashCollision (const MeshPipelineDesc &);
		void _CheckHashCollision (const RayTracingPipelineDesc &);
		void _CheckHashCollision (const GraphicsPipelineDesc &);
		void _CheckHashCollision (const ComputePipelineDesc &);
		void _CheckHashCollision2 (const PipelineDescription &);

		bool _CheckDescriptorBindings (const PipelineDescription &) const;
	};


}	// FG
