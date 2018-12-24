// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "EShaderCompilationFlags.h"
#include "framegraph/Public/ResourceEnums.h"
#include "framegraph/Public/VertexEnums.h"
#include "glslang/glslang/Include/ResourceLimits.h"

class TIntermNode;

namespace glslang
{
	class TType;
	class TIntermediate;
}

namespace FG
{

	//
	// SPIRV Compiler
	//

	class SpirvCompiler final
	{
	// types
	public:
		struct ShaderReflection
		{
		// types
			using TopologyBits_t		= GraphicsPipelineDesc::TopologyBits_t;
			using Shader				= PipelineDescription::Shader;
			using PipelineLayout		= PipelineDescription::PipelineLayout;
			using VertexAttribs_t		= GraphicsPipelineDesc::VertexAttribs_t;
			using FragmentOutputs_t		= GraphicsPipelineDesc::FragmentOutputs_t;
			using SpecConstants_t		= PipelineDescription::SpecConstants_t;


		// variables
			Shader				shader;
			PipelineLayout		layout;
			SpecConstants_t		specConstants;

			struct {
				TopologyBits_t		supportedTopology;
				VertexAttribs_t		vertexAttribs;

			}				vertex;

			struct {
				uint				patchControlPoints;

			}				tessellation;

			struct {
				FragmentOutputs_t	fragmentOutput;
				bool				earlyFragmentTests	= true;

			}				fragment;

			struct {
				uint3				localGroupSize;
				uint3				localGroupSpecialization;

			}				compute;

			struct {
				uint3				taskGroupSize;
				uint3				taskGroupSpecialization;

				uint3				meshGroupSize;
				uint3				meshGroupSpecialization;

				EPrimitive			topology		= Default;
				uint				maxPrimitives	= 0;
				uint				maxIndices		= 0;
				uint				maxVertices		= 0;

			}				mesh;
		};

	private:
		struct GLSLangResult;


	// variables
	private:
		EShaderCompilationFlags		_compilerFlags	= Default;

		glslang::TIntermediate *	_intermediate	= null;
		EShaderStages				_currentStage	= Default;
		bool						_targetVulkan	= true;
		uint						_spirvTraget	= 0;		// spv_target_env

		TBuiltInResource			_builtinResource;


	// methods
	public:
		SpirvCompiler ();
		~SpirvCompiler ();
		
		bool SetCompilationFlags (EShaderCompilationFlags flags);

		bool SetDefaultResourceLimits ();
		bool SetCurrentResourceLimits (PhysicalDeviceVk_t physicalDevice);

		bool Compile (EShader shaderType, EShaderLangFormat srcShaderFmt, EShaderLangFormat dstShaderFmt,
					  StringView entry, StringView source,
					  OUT PipelineDescription::Shader &outShader, OUT ShaderReflection &outReflection, OUT String &log);
		
		bool Compile (EShader shaderType, EShaderLangFormat srcShaderFmt, EShaderLangFormat dstShaderFmt,
					  StringView entry, ArrayView<uint> binary,
					  OUT PipelineDescription::Shader &outShader, OUT ShaderReflection &outReflection, OUT String &log);
		

	private:
		bool _ParseGLSL (EShader shaderType, EShaderLangFormat srcShaderFmt, EShaderLangFormat dstShaderFmt,
						 StringView entry, ArrayView<const char *> source, OUT GLSLangResult &glslangData, INOUT String &log);

		bool _CompileSPIRV (const GLSLangResult &glslangData, OUT Array<uint> &spirv, INOUT String &log) const;
		bool _OptimizeSPIRV (INOUT Array<uint> &spirv, INOUT String &log) const;

		bool _BuildReflection (const GLSLangResult &glslangData, OUT ShaderReflection &reflection);

		bool _OnCompilationFailed (ArrayView<const char *> source, INOUT String &log) const;

		static void _GenerateResources (OUT TBuiltInResource& res);


	// GLSL deserializer
	private:
		bool _ProcessExternalObjects (TIntermNode* root, TIntermNode* node, INOUT ShaderReflection &result) const;

		bool _DeserializeExternalObjects (TIntermNode* node, INOUT ShaderReflection &result) const;

		bool _ProcessShaderInfo (INOUT ShaderReflection &result) const;
		
		bool _CalculateStructSize (const glslang::TType &bufferType, OUT BytesU &staticSize, OUT BytesU &arrayStride, OUT BytesU &offset) const;
		
		void _MergeWithGeometryInputPrimitive (INOUT GraphicsPipelineDesc::TopologyBits_t &topologyBits, /*TLayoutGeometry*/uint type) const;

		ND_ BindingIndex	_ToBindingIndex (uint index) const;
		ND_ EImage			_ExtractImageType (const glslang::TType &type) const;
		ND_ EVertexType		_ExtractVertexType (const glslang::TType &type) const;
		ND_ EPixelFormat	_ExtractImageFormat (/*TLayoutFormat*/uint format) const;
		ND_ EFragOutput		_ExtractFragmentOutputType (const glslang::TType &type) const;
	};


}	// FG
