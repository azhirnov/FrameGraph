// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/VFG.h"

namespace FG
{

	//
	// Pipeline C++ Serializer
	//

	class PipelineCppSerializer final
	{
	// methods
	public:
		bool Serialize (const GraphicsPipelineDesc &ppln, StringView name, OUT String &src) const;
		bool Serialize (const ComputePipelineDesc &ppln, StringView name, OUT String &src) const;

	private:
		String  _ShaderToString (const PipelineDescription::ShaderDataUnion_t &shaderData) const;

		String  _SerializePipelineLayout (const PipelineDescription::PipelineLayout &layout) const;
		String  _SerializeDescriptorSet (const PipelineDescription::DescriptorSet &ds) const;

		String  _ShaderType_ToString (EShader value) const;
		String  _ShaderLangFormat_ToString (EShaderLangFormat value) const;
		String  _Topology_ToString (EPrimitive value) const;
		String  _FragOutput_ToString (EFragOutput value) const;
		String  _VertexType_ToString (EVertexType value) const;
		String  _ImageType_ToString (EImage value) const;
		String  _ShaderStages_ToString (EShaderStages value) const;
		String  _PixelFormat_ToString (EPixelFormat value) const;
		String  _ShaderAccess_ToString (EShaderAccess value) const;
		String  _BindingIndex_ToString (const BindingIndex &index) const;
		String  _SpecConstant_ToString (const PipelineDescription::SpecConstants_t::pair_type &value) const;

		String  _VertexID_ToString (const VertexID &id) const;
		String  _UniformID_ToString (const UniformID &id) const;
		String  _RenderTargetID_ToString (const RenderTargetID &id) const;
		String  _DescriptorSetID_ToString (const DescriptorSetID &id) const;
		String  _SpecializationID_ToString (const SpecializationID &id) const;
	};


}	// FG
