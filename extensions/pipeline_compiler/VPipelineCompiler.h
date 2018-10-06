// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "EShaderCompilationFlags.h"

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
			ND_ size_t  operator () (const BinaryShaderData &value) const noexcept {
				return value->GetHashOfData();
			}
		};

		using ShaderCache_t		= HashMap< BinaryShaderData, VkShaderPtr >;
		using ShaderDataMap_t	= PipelineDescription::ShaderDataMap_t;


	// variables
	private:
		UniquePtr< class SpirvCompiler >	_spirvCompiler;

		VkPhysicalDevice_t					_physicalDevice;
		VkDevice_t							_logicalDevice;

		ShaderCache_t						_shaderCache;

		EShaderCompilationFlags				_compilerFlags			= Default;

		void *								_fpCreateShaderModule	= null;
		void *								_fpDestroyShaderModule	= null;


	// methods
	public:
		VPipelineCompiler ();
		VPipelineCompiler (VkPhysicalDevice_t physicalDevice, VkDevice_t device);
		~VPipelineCompiler ();

		bool SetCompilationFlags (EShaderCompilationFlags flags);

		void ReleaseUnusedShaders ();
		void ReleaseShaderCache ();

		bool IsSupported (const MeshProcessingPipelineDesc &ppln, EShaderLangFormat dstFormat) const override;
		bool IsSupported (const RayTracingPipelineDesc &ppln, EShaderLangFormat dstFormat) const override;
		bool IsSupported (const GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat) const override;
		bool IsSupported (const ComputePipelineDesc &ppln, EShaderLangFormat dstFormat) const override;
		
		bool Compile (INOUT MeshProcessingPipelineDesc &ppln, EShaderLangFormat dstFormat) override;
		bool Compile (INOUT RayTracingPipelineDesc &ppln, EShaderLangFormat dstFormat) override;
		bool Compile (INOUT GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat) override;
		bool Compile (INOUT ComputePipelineDesc &ppln, EShaderLangFormat dstFormat) override;


	private:
		bool _MergePipelineResources (const PipelineDescription::PipelineLayout &srcLayout,
									  INOUT PipelineDescription::PipelineLayout &dstLayout) const;

		bool _MergeUniforms (const PipelineDescription::UniformMap_t &srcUniforms,
							 INOUT PipelineDescription::UniformMap_t &dstUniforms) const;

		bool _CreateVulkanShader (INOUT PipelineDescription::Shader &shader);

		bool _IsSupported (const ShaderDataMap_t &data) const;
	};


}	// FG
