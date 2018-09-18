// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "EShaderCompilationFlags.h"
#include "framegraph/Public/LowLevel/ResourceEnums.h"
#include "framegraph/Public/LowLevel/VertexEnums.h"

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
			Shader			shader;
			PipelineLayout	layout;
			SpecConstants_t	specConstants;

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
		};

	private:
		struct GLSLangResult;


	// variables
	private:
		EShaderCompilationFlags		_compilerFlags	= Default;

		glslang::TIntermediate *	_intermediate	= null;
		EShaderStages				_currentStage;
		bool						_targetVulkan	= true;


	// methods
	public:
		SpirvCompiler ();
		~SpirvCompiler ();
		
		bool SetCompilationFlags (EShaderCompilationFlags flags);

		bool Compile (EShader shaderType, EShaderLangFormat srcShaderFmt, EShaderLangFormat dstShaderFmt,
					  StringView entry, StringView source,
					  OUT PipelineDescription::Shader &outShader, OUT ShaderReflection &outReflection, OUT String &log);
		
		bool Compile (EShader shaderType, EShaderLangFormat srcShaderFmt, EShaderLangFormat dstShaderFmt,
					  StringView entry, ArrayView<uint> binary,
					  OUT PipelineDescription::Shader &outShader, OUT ShaderReflection &outReflection, OUT String &log);
		

	private:
		bool _ParseGLSL (EShader shaderType, EShaderLangFormat srcShaderFmt, EShaderLangFormat dstShaderFmt,
						 StringView entry, StringView source, OUT GLSLangResult &glslangData, INOUT String &log) const;

		bool _CompileSPIRV (const GLSLangResult &glslangData, OUT Array<uint> &spirv, INOUT String &log) const;

		bool _BuildReflection (const GLSLangResult &glslangData, OUT ShaderReflection &reflection);

		bool _OnCompilationFailed (ArrayView<StringView> source, INOUT String &log) const;
		

	// GLSL deserializer
	private:
		bool _ProcessExternalObjects (TIntermNode* root, TIntermNode* node, INOUT ShaderReflection &result) const;

		bool _DeserializeExternalObjects (TIntermNode* node, INOUT ShaderReflection &result) const;

		bool _ProcessShaderInfo (INOUT ShaderReflection &result) const;
		
        bool _CalculateStructSize (const glslang::TType &bufferType, OUT BytesU &staticSize, OUT BytesU &arrayStride) const;
		
        void _MergeWithGeometryInputPrimitive (INOUT GraphicsPipelineDesc::TopologyBits_t &topologyBits, /*TLayoutGeometry*/uint type) const;

		ND_ BindingIndex	_ToBindingIndex (uint index) const;
		ND_ EImage			_ExtractImageType (const glslang::TType &type) const;
		ND_ EVertexType		_ExtractVertexType (const glslang::TType &type) const;
        ND_ EPixelFormat	_ExtractImageFormat (/*TLayoutFormat*/uint format) const;
		ND_ EFragOutput		_ExtractFragmentOutputType (const glslang::TType &type) const;
	};


}	// FG
