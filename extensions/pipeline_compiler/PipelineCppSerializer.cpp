// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "PipelineCppSerializer.h"
#include "stl/include/StringUtils.h"

namespace FG
{
	
/*
=================================================
	IdxToString
=================================================
*/
	static String  IdxToString (uint idx)
	{
		if ( idx == ~0u )
			return "~0u"s;

		return ToString( idx ) << 'u';
	};

/*
=================================================
	Serialize
=================================================
*/
	bool PipelineCppSerializer::Serialize (const GraphicsPipelineDesc &ppln, StringView name, OUT String &src) const
	{
		src.clear();

		src = "PipelinePtr  Create_"s << name << " (const VFrameGraphPtr &fg)\n"
			<< "{\n"
			<< "	GraphicsPipelineDesc  desc;\n\n";

		for (size_t i = 0; i < ppln._supportedTopology.size(); ++i)
		{
			if ( ppln._supportedTopology.test( i ) )
				src << "\tdesc.AddTopology( " << _Topology_ToString( EPrimitive(i) ) << " );\n";
		}

		if ( not ppln._fragmentOutput.empty() )
		{
			src << "\tdesc.SetFragmentOutputs({\n";

			for (auto& frag : ppln._fragmentOutput)
			{
				if ( &frag != ppln._fragmentOutput.begin() )
					src << ",\n";

				src << "\t\t\t{ " << _RenderTargetID_ToString( frag.id ) << ", "
					<< ToString( frag.index ) << ", "
					<< _FragOutput_ToString( frag.type ) << " }";
			}
			src << " });\n\n";
		}

		if ( not ppln._vertexAttribs.empty() )
		{
			src << "\tdesc.SetVertexAttribs({\n";

			for (auto& attr : ppln._vertexAttribs)
			{
				if ( &attr != ppln._vertexAttribs.begin() )
					src << ",\n";

				src << "\t\t\t{ " << _VertexID_ToString( attr.id ) << ", "
					<< ToString( attr.index ) << ", "
					<< _VertexType_ToString( attr.type ) << " }";
			}
			src << " });\n\n";
		}
		
		src << "\tdesc.SetEarlyFragmentTests( " << ToString( ppln._earlyFragmentTests ) << " );\n\n";

		src << _SerializePipelineLayout( ppln._pipelineLayout );

		for (auto& sh : ppln._shaders)
		{
			for (auto& data : sh.data)
			{
				src << "\tdesc.AddShader( " << _ShaderType_ToString( sh.shaderType ) << ", "
					<< _ShaderLangFormat_ToString( data.first ) << ", "
					<< _ShaderToString( data.second ) << " );\n\n";
			}

			if ( not sh.specConstants.empty() )
			{
				src << "\tdesc.SetSpecConstants( "
					<< _ShaderType_ToString( sh.shaderType ) << ", {\n";

				for (auto& spec : sh.specConstants)
				{
					if ( &spec != sh.specConstants.begin() )
						src << ",\n";

					src << _SpecConstant_ToString( spec );
				}
				src << " });\n\n";
			}
		}

		src << "\treturn fg->CreatePipeline( std::move(desc) );\n"
			<< "}\n";

		return true;
	}
	
/*
=================================================
	Serialize
=================================================
*/
	bool PipelineCppSerializer::Serialize (const ComputePipelineDesc &ppln, StringView name, OUT String &src) const
	{
		src.clear();

		src << "PipelinePtr  Create_" << name << " (const VFrameGraphPtr &fg)\n"
			<< "{\n"
			<< "	ComputePipelineDesc  desc;\n\n";

		src << "\tdesc.SetLocalGroupSize( "
			<< ToString( ppln._defaultLocalGroupSize.x ) << ", "
			<< ToString( ppln._defaultLocalGroupSize.y ) << ", "
			<< ToString( ppln._defaultLocalGroupSize.z ) << " );\n\n";

		if ( ppln._localSizeSpec.x != ComputePipelineDesc::UNDEFINED_SPECIALIZATION or
			 ppln._localSizeSpec.y != ComputePipelineDesc::UNDEFINED_SPECIALIZATION or
			 ppln._localSizeSpec.z != ComputePipelineDesc::UNDEFINED_SPECIALIZATION )
		{
			src << "\tdesc.SetLocalGroupSpecialization( "
				<< IdxToString( ppln._localSizeSpec.x ) << ", "
				<< IdxToString( ppln._localSizeSpec.y ) << ", "
				<< IdxToString( ppln._localSizeSpec.z ) << " );\n\n";
		}
		
		src << _SerializePipelineLayout( ppln._pipelineLayout );

		for (auto& data : ppln._shader.data)
		{
			src << "\tdesc.AddShader( "
				<< _ShaderLangFormat_ToString( data.first ) << ", "
				<< _ShaderToString( data.second ) << " );\n\n";
		}
		
		if ( not ppln._shader.specConstants.empty() )
		{
			src << "\tdesc.SetSpecConstants({\n";

			for (auto& spec : ppln._shader.specConstants)
			{
				if ( &spec != ppln._shader.specConstants.begin() )
					src << ",\n";

				src << _SpecConstant_ToString( spec );
			}
			src << " });\n\n";
		}

		src << "\treturn fg->CreatePipeline( std::move(desc) );\n"
			<< "}\n";

		return true;
	}
	
/*
=================================================
	_ShaderToString
----
	returns: "entry, {data}"
=================================================
*/
	String  PipelineCppSerializer::_ShaderToString (const PipelineDescription::ShaderDataUnion_t &shaderData) const
	{
		static const char	hex_sequence[] = "0123456789ABCDEF";

		String	src;

		std::visit(overloaded{
			[&src] (const PipelineDescription::SharedShaderPtr<Array<uint8_t>> &data)
			{
				src << '"' << data->GetEntry() << "\", Array<uint8_t>{";
				
				for (size_t i = 0; i < data->GetData().size(); ++i)
				{
					const uint8_t c = data->GetData()[i];

					src << (i ? ", " : "") << (i%36 == 0 ? "\n" : "")
						<< "0x" << hex_sequence[(c >> 4) & 0xF] << hex_sequence[c & 0xF];
				}
				src << " }";
			},

			[&src] (const PipelineDescription::SharedShaderPtr<Array<uint32_t>> &data)
			{
				src << '"' << data->GetEntry() << "\", Array<uint32_t>{";
				
				for (size_t i = 0; i < data->GetData().size(); ++i)
				{
					const uint32_t c = data->GetData()[i];

					src << (i ? ", " : "") << (i%12 == 0 ? "\n" : "") << "0x"
						<< hex_sequence[(c >> 28) & 0xF] << hex_sequence[(c >> 24) & 0xF]
						<< hex_sequence[(c >> 20) & 0xF] << hex_sequence[(c >> 16) & 0xF]
						<< hex_sequence[(c >> 12) & 0xF] << hex_sequence[(c >>  8) & 0xF]
						<< hex_sequence[(c >>  4) & 0xF] << hex_sequence[(c >>  0) & 0xF];
				}
				src << " }";
			},

			[] (const auto& ) { ASSERT(false); }

		}, shaderData );

		return src;
	}
	
/*
=================================================
	_SerializePipelineLayout
=================================================
*/
	String  PipelineCppSerializer::_SerializePipelineLayout (const PipelineDescription::PipelineLayout &layout) const
	{
		String	src;

		for (auto& ds : layout.descriptorSets)
		{
			src << _SerializeDescriptorSet( ds );
		}

		// TODO: push constants

		return src;
	}
	
/*
=================================================
	_SerializeDescriptorSet
=================================================
*/
	String  PipelineCppSerializer::_SerializeDescriptorSet (const PipelineDescription::DescriptorSet &ds) const
	{
		String	textures;
		String	samplers;
		String	subpasses;
		String	images;
		String	ubuffers;
		String	sbuffers;

		for (auto& un : ds.uniforms)
		{
			std::visit( overloaded{
				[this, &textures, id = &un.first] (const PipelineDescription::Texture &tex)
				{
					if ( not textures.empty() )
						textures << ",\n\t\t\t  ";

					textures << "{ " << _UniformID_ToString( *id ) << ", "
							<< _ImageType_ToString( tex.textureType ) << ", "
							<< _BindingIndex_ToString( tex.index ) << ", "
							<< _ShaderStages_ToString( tex.stageFlags ) << " }";
				},

				[this, &samplers, id = &un.first] (const PipelineDescription::Sampler &samp)
				{
					if ( not samplers.empty() )
						samplers << ",\n\t\t\t  ";

					samplers << "{ " << _UniformID_ToString( *id ) << ", "
							<< _BindingIndex_ToString( samp.index ) << ", "
							<< _ShaderStages_ToString( samp.stageFlags ) << " }";
				},

				[this, &subpasses, id = &un.first] (const PipelineDescription::SubpassInput &spi)
				{
					if ( not subpasses.empty() )
						subpasses << ",\n\t\t\t  ";

					subpasses << "{ " << _UniformID_ToString( *id ) << ", "
							<< ToString( spi.attachmentIndex ) << ", "
							<< ToString( spi.isMultisample ) << ", "
							<< _BindingIndex_ToString( spi.index ) << ", "
							<< _ShaderStages_ToString( spi.stageFlags ) << " }";
				},

				[this, &images, id = &un.first] (const PipelineDescription::Image &img)
				{
					if ( not images.empty() )
						images << ",\n\t\t\t  ";

					images << "{ " << _UniformID_ToString( *id ) << ", "
							<< _ImageType_ToString( img.imageType ) << ", "
							<< _PixelFormat_ToString( img.format ) << ", "
							<< _ShaderAccess_ToString( img.access ) << ", "
							<< _BindingIndex_ToString( img.index ) << ", "
							<< _ShaderStages_ToString( img.stageFlags ) << " }";
				},

				[this, &ubuffers, id = &un.first] (const PipelineDescription::UniformBuffer &ubuf)
				{
					if ( not ubuffers.empty() )
						ubuffers << ",\n\t\t\t  ";

					ubuffers << "{ " << _UniformID_ToString( *id ) << ", "
							<< ToString( uint64_t(ubuf.size) ) << "_b, "
							<< _BindingIndex_ToString( ubuf.index ) << ", "
							<< _ShaderStages_ToString( ubuf.stageFlags ) << " }";
				},

				[this, &sbuffers, id = &un.first] (const PipelineDescription::StorageBuffer &sbuf)
				{
					if ( not sbuffers.empty() )
						sbuffers << ",\n\t\t\t  ";

					sbuffers << "{ " << _UniformID_ToString( *id ) << ", "
							<< ToString( uint64_t(sbuf.staticSize) ) << "_b, "
							<< ToString( uint64_t(sbuf.arrayStride) ) << "_b, "
							<< _ShaderAccess_ToString( sbuf.access ) << ", "
							<< _BindingIndex_ToString( sbuf.index ) << ", "
							<< _ShaderStages_ToString( sbuf.stageFlags ) << " }";
				},

				[] (const auto &) { ASSERT(false); }

			}, un.second );
		}

		String	src;
		src << "\tdesc.AddDescriptorSet(\n"
			<< "\t\t\t" << _DescriptorSetID_ToString( ds.id ) << ",\n"
			<< "\t\t\t" << ToString( ds.bindingIndex ) << ",\n"
			<< "\t\t\t{" << textures << "},\n"
			<< "\t\t\t{" << samplers << "},\n"
			<< "\t\t\t{" << subpasses << "},\n"
			<< "\t\t\t{" << images << "},\n"
			<< "\t\t\t{" << ubuffers << "},\n"
			<< "\t\t\t{" << sbuffers << "} );\n\n";

		return src;
	}
	
/*
=================================================
	_ShaderType_ToString
=================================================
*/
	String  PipelineCppSerializer::_ShaderType_ToString (EShader value) const
	{
		switch ( value )
		{
			case EShader::Vertex :			return "EShader::Vertex";
			case EShader::TessControl :		return "EShader::TessControl";
			case EShader::TessEvaluation :	return "EShader::TessEvaluation";
			case EShader::Geometry :		return "EShader::Geometry";
			case EShader::Fragment :		return "EShader::Fragment";
			case EShader::Compute :			return "EShader::Compute";
		}
		RETURN_ERR( "unsupported shader type!" );
	}
	
/*
=================================================
	_ShaderLangFormat_ToString
=================================================
*/
	String  PipelineCppSerializer::_ShaderLangFormat_ToString (EShaderLangFormat value) const
	{
		String	src;

		// search default values
		switch ( value )
		{
			case EShaderLangFormat::GLSL_450 :		return "EShaderLangFormat::GLSL_450";
			case EShaderLangFormat::GLSL_460 :		return "EShaderLangFormat::GLSL_460";
			case EShaderLangFormat::VKSL_100 :		return "EShaderLangFormat::VKSL_100";
			case EShaderLangFormat::VKSL_110 :		return "EShaderLangFormat::VKSL_110";
			case EShaderLangFormat::SPIRV_100 :		return "EShaderLangFormat::SPIRV_100";
			case EShaderLangFormat::SPIRV_110 :		return "EShaderLangFormat::SPIRV_110";
		}

		switch ( value & EShaderLangFormat::_ApiVersionMask )
		{
			case EShaderLangFormat::OpenGL_330 :	src << "EShaderLangFormat::OpenGL_330";		break;
			case EShaderLangFormat::OpenGL_420 :	src << "EShaderLangFormat::OpenGL_420";		break;
			case EShaderLangFormat::OpenGL_450 :	src << "EShaderLangFormat::OpenGL_450";		break;
			case EShaderLangFormat::OpenGL_460 :	src << "EShaderLangFormat::OpenGL_460";		break;
			case EShaderLangFormat::OpenGLES_200 :	src << "EShaderLangFormat::OpenGLES_200";	break;
			case EShaderLangFormat::OpenGLES_300 :	src << "EShaderLangFormat::OpenGLES_300";	break;
			case EShaderLangFormat::OpenGLES_310 :	src << "EShaderLangFormat::OpenGLES_310";	break;
			case EShaderLangFormat::OpenGLES_320 :	src << "EShaderLangFormat::OpenGLES_320";	break;
			case EShaderLangFormat::DirectX_10 :	src << "EShaderLangFormat::DirectX_10";		break;
			case EShaderLangFormat::DirectX_11 :	src << "EShaderLangFormat::DirectX_11";		break;
			case EShaderLangFormat::DirectX_12 :	src << "EShaderLangFormat::DirectX_12";		break;
			case EShaderLangFormat::OpenCL_120 :	src << "EShaderLangFormat::OpenCL_120";		break;
			case EShaderLangFormat::OpenCL_200 :	src << "EShaderLangFormat::OpenCL_200";		break;
			case EShaderLangFormat::OpenCL_210 :	src << "EShaderLangFormat::OpenCL_210";		break;
			case EShaderLangFormat::Vulkan_100 :	src << "EShaderLangFormat::Vulkan_100";		break;
			case EShaderLangFormat::Vulkan_110 :	src << "EShaderLangFormat::Vulkan_110";		break;
			case EShaderLangFormat::Software_100 :	src << "EShaderLangFormat::Software_100";	break;
			default :								RETURN_ERR( "unknown shader language api & version" );
		}

		src << " | ";

		switch ( value & EShaderLangFormat::_StorageFormatMask )
		{
			case EShaderLangFormat::HighLevel :		src << "EShaderLangFormat::HighLevel";		break;
			case EShaderLangFormat::SPIRV :			src << "EShaderLangFormat::SPIRV";			break;
			case EShaderLangFormat::GL_Binary :		src << "EShaderLangFormat::GL_Binary";		break;
			case EShaderLangFormat::DXBC :			src << "EShaderLangFormat::DXBC";			break;
			case EShaderLangFormat::DXIL :			src << "EShaderLangFormat::DXIL";			break;
			case EShaderLangFormat::Assembler :		src << "EShaderLangFormat::Assembler";		break;
			case EShaderLangFormat::Invocable :		src << "EShaderLangFormat::Invocable";		break;
			case EShaderLangFormat::ShaderModule :	src << "EShaderLangFormat::ShaderModule";	break;
			default :								RETURN_ERR( "unknown shader language storage format" );
		}

		return src;
	}
	
/*
=================================================
	_Topology_ToString
=================================================
*/
	String  PipelineCppSerializer::_Topology_ToString (EPrimitive value) const
	{
		switch ( value )
		{
			case EPrimitive::Point :					return "EPrimitive::Point";
		
			case EPrimitive::LineList :					return "EPrimitive::LineList";
			case EPrimitive::LineStrip :				return "EPrimitive::LineStrip";
			case EPrimitive::LineListAdjacency :		return "EPrimitive::LineListAdjacency";
			case EPrimitive::LineStripAdjacency :		return "EPrimitive::LineStripAdjacency";

			case EPrimitive::TriangleList :				return "EPrimitive::TriangleList";
			case EPrimitive::TriangleStrip :			return "EPrimitive::TriangleStrip";
			case EPrimitive::TriangleFan :				return "EPrimitive::TriangleFan";
			case EPrimitive::TriangleListAdjacency :	return "EPrimitive::TriangleListAdjacency";
			case EPrimitive::TriangleStripAdjacency :	return "EPrimitive::TriangleStripAdjacency";

			case EPrimitive::Patch :					return "EPrimitive::Patch";
		}
		RETURN_ERR( "unsupported primitive topology!" );
	}
	
/*
=================================================
	_FragOutput_ToString
=================================================
*/
	String  PipelineCppSerializer::_FragOutput_ToString (EFragOutput value) const
	{
		switch ( value )
		{
			case EFragOutput::Int4 :	return "EFragOutput::Int4";
			case EFragOutput::UInt4 :	return "EFragOutput::UInt4";
			case EFragOutput::Float4 :	return "EFragOutput::Float4";
		}
		RETURN_ERR( "unsupported fragment output type!" );
	}
	
/*
=================================================
	_VertexType_ToString
=================================================
*/
	String  PipelineCppSerializer::_VertexType_ToString (EVertexType value) const
	{
		String	src;

		switch ( value & EVertexType::_TypeMask )
		{
			case EVertexType::_Byte :	src << "EVertexType::Byte";		break;
			case EVertexType::_UByte :	src << "EVertexType::UByte";	break;
			case EVertexType::_Short :	src << "EVertexType::Short";	break;
			case EVertexType::_UShort :	src << "EVertexType::UShort";	break;
			case EVertexType::_Int :	src << "EVertexType::Int";		break;
			case EVertexType::_UInt :	src << "EVertexType::UInt";		break;
			case EVertexType::_Long :	src << "EVertexType::Long";		break;
			case EVertexType::_ULong :	src << "EVertexType::ULong";	break;
			case EVertexType::_Half :	src << "EVertexType::Half";		break;
			case EVertexType::_Float :	src << "EVertexType::Float";	break;
			case EVertexType::_Double :	src << "EVertexType::Double";	break;
			default :					RETURN_ERR( "unknown vertex type!" );
		}

		switch ( value & EVertexType::_VecMask )
		{
			case EVertexType::_Vec1 :	break;
			case EVertexType::_Vec2 :	src << "2";	break;
			case EVertexType::_Vec3 :	src << "3";	break;
			case EVertexType::_Vec4 :	src << "4";	break;
			default :					RETURN_ERR( "unsupported vector size!" );
		}

		if ( EnumEq( value, EVertexType::NormalizedFlag ) )
			src << "_Norm";

		return src;
	}
	
/*
=================================================
	_ImageType_ToString
=================================================
*/
	String  PipelineCppSerializer::_ImageType_ToString (EImage value) const
	{
		switch ( value )
		{
			case EImage::Tex1D :			return "EImage::Tex1D";
			case EImage::Tex1DArray :		return "EImage::Tex1DArray";
			case EImage::Tex2D :			return "EImage::Tex2D";
			case EImage::Tex2DArray :		return "EImage::Tex2DArray";
			case EImage::Tex2DMS :			return "EImage::Tex2DMS";
			case EImage::Tex2DMSArray :		return "EImage::Tex2DMSArray";
			case EImage::TexCube :			return "EImage::TexCube";
			case EImage::TexCubeArray :		return "EImage::TexCubeArray";
			case EImage::Tex3D :			return "EImage::Tex3D";
			case EImage::Buffer :			return "EImage::Buffer";
		}
		RETURN_ERR( "unknown image type!" );
	}
	
/*
=================================================
	_ShaderStages_ToString
=================================================
*/
	String  PipelineCppSerializer::_ShaderStages_ToString (EShaderStages values) const
	{
		String	src;

		for (EShaderStages t = EShaderStages(1 << 0); t < EShaderStages::_Last; t = EShaderStages(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;

			if ( not src.empty() )
				src << " | ";

			switch ( t )
			{
				case EShaderStages::Vertex :			src << "EShaderStages::Vertex";				break;
				case EShaderStages::TessControl :		src << "EShaderStages::TessControl";		break;
				case EShaderStages::TessEvaluation :	src << "EShaderStages::TessEvaluation";		break;
				case EShaderStages::Geometry :			src << "EShaderStages::Geometry";			break;
				case EShaderStages::Fragment :			src << "EShaderStages::Fragment";			break;
				case EShaderStages::Compute :			src << "EShaderStages::Compute";			break;
				default :								RETURN_ERR( "unknown shader stage!" );
			}
		}
		return src;
	}
	
/*
=================================================
	_PixelFormat_ToString
=================================================
*/
	String  PipelineCppSerializer::_PixelFormat_ToString (EPixelFormat value) const
	{
		switch ( value )
		{
			case EPixelFormat::RGBA32F :		return "EPixelFormat::RGBA32F";
			case EPixelFormat::RGBA16F :		return "EPixelFormat::RGBA16F";
			case EPixelFormat::R32F :			return "EPixelFormat::R32F";
			case EPixelFormat::RGBA8_UNorm :	return "EPixelFormat::RGBA8_UNorm";
			case EPixelFormat::RGBA8_SNorm :	return "EPixelFormat::RGBA8_SNorm";
			case EPixelFormat::RG32F :			return "EPixelFormat::RG32F";
			case EPixelFormat::RG16F :			return "EPixelFormat::RG16F";
			case EPixelFormat::RGB_11_11_10F :	return "EPixelFormat::RGB_11_11_10F";
			case EPixelFormat::R16F :			return "EPixelFormat::R16F";
			case EPixelFormat::RGBA16_UNorm :	return "EPixelFormat::RGBA16_UNorm";
			case EPixelFormat::RGB10_A2_UNorm :	return "EPixelFormat::RGB10_A2_UNorm";
			case EPixelFormat::RG16_UNorm :		return "EPixelFormat::RG16_UNorm";
			case EPixelFormat::RG8_UNorm :		return "EPixelFormat::RG8_UNorm";
			case EPixelFormat::R16_UNorm :		return "EPixelFormat::R16_UNorm";
			case EPixelFormat::R8_UNorm :		return "EPixelFormat::R8_UNorm";
			case EPixelFormat::RGBA16_SNorm :	return "EPixelFormat::RGBA16_SNorm";
			case EPixelFormat::RG16_SNorm :		return "EPixelFormat::RG16_SNorm";
			case EPixelFormat::RG8_SNorm :		return "EPixelFormat::RG8_SNorm";
			case EPixelFormat::R16_SNorm :		return "EPixelFormat::R16_SNorm";
			case EPixelFormat::R8_SNorm :		return "EPixelFormat::R8_SNorm";
			case EPixelFormat::RGBA32I :		return "EPixelFormat::RGBA32I";
			case EPixelFormat::RGBA16I :		return "EPixelFormat::RGBA16I";
			case EPixelFormat::RGBA8I :			return "EPixelFormat::RGBA8I";
			case EPixelFormat::R32I :			return "EPixelFormat::R32I";
			case EPixelFormat::RG32I :			return "EPixelFormat::RG32I";
			case EPixelFormat::RG16I :			return "EPixelFormat::RG16I";
			case EPixelFormat::RG8I :			return "EPixelFormat::RG8I";
			case EPixelFormat::R16I :			return "EPixelFormat::R16I";
			case EPixelFormat::R8I :			return "EPixelFormat::R8I";
			case EPixelFormat::RGBA32U :		return "EPixelFormat::RGBA32U";
			case EPixelFormat::RGBA16U :		return "EPixelFormat::RGBA16U";
			case EPixelFormat::RGBA8U :			return "EPixelFormat::RGBA8U";
			case EPixelFormat::R32U :			return "EPixelFormat::R32U";
			case EPixelFormat::RG32U :			return "EPixelFormat::RG32U";
			case EPixelFormat::RG16U :			return "EPixelFormat::RG16U";
			case EPixelFormat::RGB10_A2U :		return "EPixelFormat::RGB10_A2U";
			case EPixelFormat::RG8U :			return "EPixelFormat::RG8U";
			case EPixelFormat::R16U :			return "EPixelFormat::R16U";
			case EPixelFormat::R8U :			return "EPixelFormat::R8U";
		}
		RETURN_ERR( "unsupported pixel format!" );
	}
	
/*
=================================================
	_ShaderAccess_ToString
=================================================
*/
	String  PipelineCppSerializer::_ShaderAccess_ToString (EShaderAccess value) const
	{
		switch ( value )
		{
			case EShaderAccess::ReadOnly :		return "EShaderAccess::ReadOnly";
			case EShaderAccess::WriteOnly :		return "EShaderAccess::WriteOnly";
			case EShaderAccess::WriteDiscard :	return "EShaderAccess::WriteDiscard";
			case EShaderAccess::ReadWrite :		return "EShaderAccess::ReadWrite";
		}
		RETURN_ERR( "unknown shader access type!" );
	}
	
/*
=================================================
	_BindingIndex_ToString
=================================================
*/
	String  PipelineCppSerializer::_BindingIndex_ToString (const BindingIndex &index) const
	{
		return "BindingIndex{"s << IdxToString( index.GLBinding() ) << ", " << IdxToString( index.VKBinding() ) << "}";
	}
	
/*
=================================================
	_SpecConstant_ToString
=================================================
*/
	String  PipelineCppSerializer::_SpecConstant_ToString (const PipelineDescription::SpecConstants_t::pair_t &value) const
	{
		return "\t\t\t{"s << _SpecializationID_ToString( value.first ) << ", " << IdxToString( value.second ) << "}";
	}

/*
=================================================
	_VertexID_ToString
=================================================
*/
	String  PipelineCppSerializer::_VertexID_ToString (const VertexID &id) const
	{
		return "VertexID{\""s << id.GetName() << "\"}";
	}
	
/*
=================================================
	_UniformID_ToString
=================================================
*/
	String  PipelineCppSerializer::_UniformID_ToString (const UniformID &id) const
	{
		return "UniformID{\""s << id.GetName() << "\"}";
	}
	
/*
=================================================
	_RenderTargetID_ToString
=================================================
*/
	String  PipelineCppSerializer::_RenderTargetID_ToString (const RenderTargetID &id) const
	{
		return "RenderTargetID{\""s << id.GetName() << "\"}";
	}
	
/*
=================================================
	_DescriptorSetID_ToString
=================================================
*/
	String  PipelineCppSerializer::_DescriptorSetID_ToString (const DescriptorSetID &id) const
	{
		return "DescriptorSetID{\""s << id.GetName() << "\"}";
	}
	
/*
=================================================
	_SpecializationID_ToString
=================================================
*/
	String  PipelineCppSerializer::_SpecializationID_ToString (const SpecializationID &id) const
	{
		return "SpecializationID{\""s << id.GetName() << "\"}";
	}


}	// FG
