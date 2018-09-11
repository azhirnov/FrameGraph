// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "framegraph/Public/LowLevel/Pipeline.h"

namespace FG
{

	//
	// Shader Data implementation
	//
	template <typename T>
	class ShaderDataImpl final : public PipelineDescription::IShaderData<T>
	{
	// variables
	private:
		T			_data;
		String		_entry;


	// methods
	public:
		ShaderDataImpl (T &&data, StringView entry) :
			_data{ std::move(data) },
			_entry{ entry }
		{}
		
		T const&	GetData () const override		{ return _data; }

		StringView	GetEntry () const override		{ return _entry; }

		size_t		GetHashOfData () const override	{ ASSERT(false);  return 0; }
	};
	
/*
=================================================
	AddShaderData
=================================================
*/
	void PipelineDescription::Shader::AddShaderData (EShaderLangFormat fmt, StringView entry, String &&src)
	{
		data.insert({ fmt, MakeShared<ShaderDataImpl<String>>( std::move(src), entry ) });
	}
	
/*
=================================================
	AddShaderData
=================================================
*/
	void PipelineDescription::Shader::AddShaderData (EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin)
	{
		data.insert({ fmt, MakeShared<ShaderDataImpl<Array<uint8_t>>>( std::move(bin), entry ) });
	}
	
/*
=================================================
	AddShaderData
=================================================
*/
	void PipelineDescription::Shader::AddShaderData (EShaderLangFormat fmt, StringView entry, Array<uint> &&bin)
	{
		data.insert({ fmt, MakeShared<ShaderDataImpl<Array<uint>>>( std::move(bin), entry ) });
	}
//-----------------------------------------------------------------------------



/*
=================================================
	_AddDescriptorSet
=================================================
*/
	void PipelineDescription::_AddDescriptorSet (const DescriptorSetID				&id,
												 uint								index,
												 ArrayView< _TextureUniform >		textures,
												 ArrayView< _SamplerUniform >		samplers,
												 ArrayView< _SubpassInputUniform >	subpassInputs,
												 ArrayView< _ImageUniform >			images,
												 ArrayView< _UBufferUniform >		uniformBuffers,
												 ArrayView< _StorageBufferUniform >	storageBuffers)
	{
		DEBUG_ONLY(
		for (auto& item : _pipelineLayout.descriptorSets)
		{
			ASSERT( item.id != id );
			ASSERT( item.bindingIndex != index );
		})

		DescriptorSet	ds;
		ds.id			= id;
		ds.bindingIndex	= index;

		for (auto& tex : textures) {
			ds.uniforms.insert({ tex.id, Uniform_t{ tex.data } });
		}

		for (auto& samp : samplers) {
			ds.uniforms.insert({ samp.id, Uniform_t{ samp.data } });
		}

		for (auto& spi : subpassInputs) {
			ds.uniforms.insert({ spi.id, Uniform_t{ spi.data } });
		}

		for (auto& img : images) {
			ds.uniforms.insert({ img.id, Uniform_t{ img.data } });
		}

		for (auto& ub : uniformBuffers) {
			ds.uniforms.insert({ ub.id, Uniform_t{ ub.data } });
		}

		for (auto& sb : storageBuffers) {
			ds.uniforms.insert({ sb.id, Uniform_t{ sb.data } });
		}

		_pipelineLayout.descriptorSets.push_back( std::move(ds) );
	}
		
/*
=================================================
	_SetPushConstants
=================================================
*/
	void PipelineDescription::_SetPushConstants (std::initializer_list< PushConstant > values)
	{
		_pipelineLayout.pushConstants.assign( values.begin(), values.end() );
	}

/*
=================================================
	CopySpecConstants
=================================================
*/
	static void CopySpecConstants (std::initializer_list< PipelineDescription::SpecConstant > src, OUT PipelineDescription::SpecConstants_t &dst)
	{
		for (auto& val : src) {
			dst.insert({ val.id, val.index });
		}

		CHECK( src.size() == dst.size() );
	}
//-----------------------------------------------------------------------------
	
	

/*
=================================================
	FragmentOutput::operator ==
=================================================
*/
	bool GraphicsPipelineDesc::FragmentOutput::operator == (const FragmentOutput &rhs) const
	{
		return	id		== rhs.id		and
				index	== rhs.index	and
				type	== rhs.type;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	GraphicsPipelineDesc::GraphicsPipelineDesc ()
	{
		for (size_t i = 0; i < _shaders.size(); ++i)
		{
			_shaders[i].shaderType = EShader(i + uint(EShader::_GraphicsBegin));
		}
	}

/*
=================================================
	AddShader
=================================================
*/
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src)
	{
		ASSERT( shaderType >= EShader::_GraphicsBegin and shaderType <= EShader::_GraphicsEnd );

		Shader&	shader = _shaders[ uint(shaderType) - uint(EShader::_GraphicsBegin) ];
		ASSERT( shader.data.count( fmt ) == 0 );

		shader.shaderType = shaderType;
		shader.AddShaderData( fmt, entry, std::move(src) );
		return *this;
	}
	
/*
=================================================
	AddShader
=================================================
*/
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin)
	{
		ASSERT( shaderType >= EShader::_GraphicsBegin and shaderType <= EShader::_GraphicsEnd );

		Shader&	shader = _shaders[ uint(shaderType) - uint(EShader::_GraphicsBegin) ];
		ASSERT( shader.data.count( fmt ) == 0 );
		
		shader.shaderType = shaderType;
		shader.AddShaderData( fmt, entry, std::move(bin) );
		return *this;
	}
	
/*
=================================================
	AddShader
=================================================
*/
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin)
	{
		ASSERT( shaderType >= EShader::_GraphicsBegin and shaderType <= EShader::_GraphicsEnd );

		Shader&	shader = _shaders[ uint(shaderType) - uint(EShader::_GraphicsBegin) ];
		ASSERT( shader.data.count( fmt ) == 0 );
		
		shader.shaderType = shaderType;
		shader.AddShaderData( fmt, entry, std::move(bin) );
		return *this;
	}
	
/*
=================================================
	AddShader
=================================================
*/
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, const VkShaderPtr &module)
	{
		ASSERT( shaderType >= EShader::_GraphicsBegin and shaderType <= EShader::_GraphicsEnd );

		Shader&	shader = _shaders[ uint(shaderType) - uint(EShader::_GraphicsBegin) ];
		ASSERT( shader.data.count( fmt ) == 0 );
		
		shader.shaderType = shaderType;
		shader.data.insert({ fmt, module });
		return *this;
	}

/*
=================================================
	SetSpecConstants
=================================================
*/
	GraphicsPipelineDesc&  GraphicsPipelineDesc::SetSpecConstants (EShader shaderType, std::initializer_list< SpecConstant > values)
	{
		ASSERT( shaderType >= EShader::_GraphicsBegin and shaderType <= EShader::_GraphicsEnd );
		
		Shader&	shader = _shaders[ uint(shaderType) - uint(EShader::_GraphicsBegin) ];
		ASSERT( shader.specConstants.empty() );

		CopySpecConstants( values, OUT shader.specConstants );
		return *this;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	AddShader
=================================================
*/
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, StringView entry, String &&src)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		
		_shader.shaderType = EShader::Compute;
		_shader.AddShaderData( fmt, entry, std::move(src) );
		return *this;
	}
	
/*
=================================================
	AddShader
=================================================
*/
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		
		_shader.shaderType = EShader::Compute;
		_shader.AddShaderData( fmt, entry, std::move(bin) );
		return *this;
	}
	
/*
=================================================
	AddShader
=================================================
*/
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, StringView entry, Array<uint> &&bin)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		
		_shader.shaderType = EShader::Compute;
		_shader.AddShaderData( fmt, entry, std::move(bin) );
		return *this;
	}
	
/*
=================================================
	AddShader
=================================================
*/
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, const VkShaderPtr &module)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		
		_shader.shaderType = EShader::Compute;
		_shader.data.insert({ fmt, module });
		return *this;
	}

/*
=================================================
	SetSpecConstants
=================================================
*/
	ComputePipelineDesc&  ComputePipelineDesc::SetSpecConstants (std::initializer_list< SpecConstant > values)
	{
		ASSERT( _shader.specConstants.empty() );
		
		CopySpecConstants( values, OUT _shader.specConstants );
		return *this;
	}
//-----------------------------------------------------------------------------


}	// FG
