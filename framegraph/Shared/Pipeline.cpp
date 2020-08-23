// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Public/Pipeline.h"
#include "Shared/EnumUtils.h"

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
		T					_data;
		StaticString<64>	_entry;
		StaticString<64>	_dbgName;


	// methods
	public:
		ShaderDataImpl (T &&data, StringView entry, StringView dbgName) :
			_data{ std::move(data) },
			_entry{ entry },
			_dbgName{ dbgName }
		{}
		
		T const&	GetData () const override		{ return _data; }

		StringView	GetEntry () const override		{ return _entry; }
		
		StringView	GetDebugName () const override	{ return _dbgName; }

		size_t		GetHashOfData () const override	{ ASSERT(false);  return 0; }

		bool		ParseDebugOutput (EShaderDebugMode, ArrayView<uint8_t>, OUT Array<String> &) const override { return false; }
	};
	
/*
=================================================
	AddShaderData
=================================================
*/
	void PipelineDescription::Shader::AddShaderData (EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName)
	{
		data.insert_or_assign( fmt, MakeShared<ShaderDataImpl<String>>( std::move(src), entry, dbgName ));
	}
	
	void PipelineDescription::Shader::AddShaderData (EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName)
	{
		data.insert_or_assign( fmt, MakeShared<ShaderDataImpl<Array<uint8_t>>>( std::move(bin), entry, dbgName ));
	}
	
	void PipelineDescription::Shader::AddShaderData (EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName)
	{
		data.insert_or_assign( fmt, MakeShared<ShaderDataImpl<Array<uint>>>( std::move(bin), entry, dbgName ));
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	operator ==
=================================================
*/
	bool  PipelineDescription::Texture::operator == (const PipelineDescription::Texture &rhs) const
	{
		return	this->textureType	== rhs.textureType	and
				this->state			== rhs.state;
	}

	bool  PipelineDescription::Sampler::operator == (const PipelineDescription::Sampler &) const
	{
		return true;
	}

	bool  PipelineDescription::SubpassInput::operator == (const PipelineDescription::SubpassInput &rhs) const
	{
		return	this->attachmentIndex	== rhs.attachmentIndex		and
				this->isMultisample		== rhs.isMultisample		and
				this->state				== rhs.state;
	}
	
	bool  PipelineDescription::Image::operator == (const PipelineDescription::Image &rhs) const
	{
		return	this->imageType		== rhs.imageType	and
				this->format		== rhs.format		and
				this->state			== rhs.state;
	}
	
	bool  PipelineDescription::UniformBuffer::operator == (const PipelineDescription::UniformBuffer &rhs) const
	{
		return	this->size				== rhs.size					and
				this->dynamicOffsetIndex== rhs.dynamicOffsetIndex	and
				this->state				== rhs.state;
	}
	
	bool  PipelineDescription::StorageBuffer::operator == (const PipelineDescription::StorageBuffer &rhs) const
	{
		return	this->staticSize		== rhs.staticSize			and
				this->arrayStride		== rhs.arrayStride			and
				this->dynamicOffsetIndex== rhs.dynamicOffsetIndex	and
				this->state				== rhs.state;
	}
	
	bool  PipelineDescription::RayTracingScene::operator == (const PipelineDescription::RayTracingScene &rhs) const
	{
		return	this->state	== rhs.state;
	}

	bool  PipelineDescription::Uniform::operator == (const PipelineDescription::Uniform &rhs) const
	{
		return	this->index.VKBinding()	== rhs.index.VKBinding()	and
				this->stageFlags		== rhs.stageFlags			and
				this->arraySize			== rhs.arraySize			and
				this->data				== rhs.data;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	_TextureUniform
=================================================
*/
	PipelineDescription::_TextureUniform::_TextureUniform (const UniformID &id, EImage textureType, const BindingIndex &index,
														   uint arraySize, EShaderStages stageFlags) :
		id{id}, data{ EResourceState::ShaderSample | EResourceState_FromShaders( stageFlags ), textureType },
		index{index}, arraySize{arraySize}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_SamplerUniform
=================================================
*/
	PipelineDescription::_SamplerUniform::_SamplerUniform (const UniformID &id, const BindingIndex &index, uint arraySize, EShaderStages stageFlags) :
		id{id}, data{}, index{index}, arraySize{arraySize}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_SubpassInputUniform
=================================================
*/
	PipelineDescription::_SubpassInputUniform::_SubpassInputUniform (const UniformID &id, uint attachmentIndex, bool isMultisample,
																	 const BindingIndex &index, uint arraySize, EShaderStages stageFlags) :
		id{id}, data{ EResourceState::InputAttachment | EResourceState_FromShaders( stageFlags ), attachmentIndex, isMultisample },
		index{index}, arraySize{arraySize}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_ImageUniform
=================================================
*/
	PipelineDescription::_ImageUniform::_ImageUniform (const UniformID &id, EImage imageType, EPixelFormat format, EShaderAccess access,
													   const BindingIndex &index, uint arraySize, EShaderStages stageFlags) :
		id{id}, data{ EResourceState_FromShaders( stageFlags ) | EResourceState_FromShaderAccess( access ), imageType, format },
		index{index}, arraySize{arraySize}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_UBufferUniform
=================================================
*/
	PipelineDescription::_UBufferUniform::_UBufferUniform (const UniformID &id, BytesU size, const BindingIndex &index, uint arraySize,
														   EShaderStages stageFlags, uint dynamicOffsetIndex) :
		id{id},
		data{ EResourceState::UniformRead | EResourceState_FromShaders( stageFlags )
			  | (dynamicOffsetIndex != STATIC_OFFSET ? EResourceState::_BufferDynamicOffset : EResourceState::Unknown),
			  dynamicOffsetIndex, size },
		index{index}, arraySize{arraySize}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_StorageBufferUniform
=================================================
*/
	PipelineDescription::_StorageBufferUniform::_StorageBufferUniform (const UniformID &id, BytesU staticSize, BytesU arrayStride, EShaderAccess access,
																	   const BindingIndex &index, uint arraySize, EShaderStages stageFlags, uint dynamicOffsetIndex) :
		id{id},
		data{ EResourceState_FromShaders( stageFlags ) | EResourceState_FromShaderAccess( access )
			  | (dynamicOffsetIndex != STATIC_OFFSET ? EResourceState::_BufferDynamicOffset : EResourceState::Unknown),
			  dynamicOffsetIndex, staticSize, arrayStride },
		index{index}, arraySize{arraySize}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_RayTracingSceneUniform
=================================================
*/
	PipelineDescription::_RayTracingSceneUniform::_RayTracingSceneUniform (const UniformID &id, const BindingIndex &index, uint arraySize, EShaderStages stageFlags) :
		id{id}, data{ EResourceState::_RayTracingShader | EResourceState::_Access_ShaderStorage }, index{index}, arraySize{arraySize}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	_AddDescriptorSet
=================================================
*/
	void PipelineDescription::_AddDescriptorSet (const DescriptorSetID					&id,
												 uint									index,
												 ArrayView< _TextureUniform >			textures,
												 ArrayView< _SamplerUniform >			samplers,
												 ArrayView< _SubpassInputUniform >		subpassInputs,
												 ArrayView< _ImageUniform >				images,
												 ArrayView< _UBufferUniform >			uniformBuffers,
												 ArrayView< _StorageBufferUniform >		storageBuffers,
												 ArrayView< _RayTracingSceneUniform>	rtScenes)
	{
		ASSERT( id.IsDefined() );
		ASSERT( index < FG_MaxDescriptorSets );

		DEBUG_ONLY(
		for (auto& item : _pipelineLayout.descriptorSets)
		{
			ASSERT( item.id != id );				// descriptor set ID must be unique
			ASSERT( item.bindingIndex != index );	// binding index must be unique
		})

		DescriptorSet	ds;
		UniformMap_t	uniforms;

		ds.id			= id;
		ds.bindingIndex	= index;
		uniforms.reserve( textures.size() + samplers.size() + subpassInputs.size() + images.size() +
						  uniformBuffers.size() + storageBuffers.size() + rtScenes.size() );

		for (auto& tex : textures) {
			uniforms.insert_or_assign( tex.id, Uniform{ Texture{ tex.data }, tex.index, tex.arraySize, tex.stageFlags });
		}

		for (auto& samp : samplers) {
			uniforms.insert_or_assign( samp.id, Uniform{ Sampler{ samp.data }, samp.index, samp.arraySize, samp.stageFlags });
		}

		for (auto& spi : subpassInputs) {
			uniforms.insert_or_assign( spi.id, Uniform{ SubpassInput{ spi.data }, spi.index, spi.arraySize, spi.stageFlags });
		}

		for (auto& img : images) {
			uniforms.insert_or_assign( img.id, Uniform{ Image{ img.data }, img.index, img.arraySize, img.stageFlags });
		}

		for (auto& ub : uniformBuffers) {
			uniforms.insert_or_assign( ub.id, Uniform{ UniformBuffer{ ub.data }, ub.index, ub.arraySize, ub.stageFlags });
		}

		for (auto& sb : storageBuffers) {
			uniforms.insert_or_assign( sb.id, Uniform{ StorageBuffer{ sb.data }, sb.index, sb.arraySize, sb.stageFlags });
		}

		for (auto& rts : rtScenes) {
			uniforms.insert_or_assign( rts.id, Uniform{ RayTracingScene{ rts.data.state }, rts.index, rts.arraySize, rts.stageFlags });
		}

		ds.uniforms = MakeShared<UniformMap_t>( std::move(uniforms) );
		_pipelineLayout.descriptorSets.push_back( std::move(ds) );
	}
		
/*
=================================================
	_SetPushConstants
=================================================
*/
	void PipelineDescription::_SetPushConstants (ArrayView< _PushConstant > values)
	{
		for (auto& pc : values)
		{
			_pipelineLayout.pushConstants.insert_or_assign( pc.id, PushConstant{pc.data} );
		}
	}

/*
=================================================
	CopySpecConstants
=================================================
*/
	static void CopySpecConstants (ArrayView<PipelineDescription::SpecConstant> src, OUT PipelineDescription::SpecConstants_t &dst)
	{
		for (auto& val : src) {
			dst.insert_or_assign( val.id, uint(val.index) );
		}

		CHECK( dst.size() >= src.size() );
	}
//-----------------------------------------------------------------------------
	
	

/*
=================================================
	FragmentOutput::operator ==
=================================================
*/
	bool GraphicsPipelineDesc::FragmentOutput::operator == (const FragmentOutput &rhs) const
	{
		return	index	== rhs.index	and
				type	== rhs.type;
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	AddShaderSource
=================================================
*/
	static void AddShaderSource (INOUT FixedMap<EShader, PipelineDescription::Shader, 8> &shadersMap, EShader shaderType,
								 EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName)
	{
		auto&	shader = shadersMap.insert({ shaderType, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );

		shader.AddShaderData( fmt, entry, std::move(src), dbgName );
	}
	
/*
=================================================
	AddShaderBinary8
=================================================
*/
	static void AddShaderBinary8 (INOUT FixedMap<EShader, PipelineDescription::Shader, 8> &shadersMap, EShader shaderType,
								  EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName)
	{
		auto&	shader = shadersMap.insert({ shaderType, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );

		shader.AddShaderData( fmt, entry, std::move(bin), dbgName );
	}
	
/*
=================================================
	AddShaderBinary32
=================================================
*/
	static void AddShaderBinary32 (INOUT FixedMap<EShader, PipelineDescription::Shader, 8> &shadersMap, EShader shaderType,
								   EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName)
	{
		auto&	shader = shadersMap.insert({ shaderType, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );

		shader.AddShaderData( fmt, entry, std::move(bin), dbgName );
	}
	
/*
=================================================
	AddShaderModule
=================================================
*/
	static void AddShaderModule (INOUT FixedMap<EShader, PipelineDescription::Shader, 8> &shadersMap, EShader shaderType,
								 EShaderLangFormat fmt, const PipelineDescription::VkShaderPtr &module)
	{
		auto&	shader = shadersMap.insert({ shaderType, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );

		shader.data.insert_or_assign( fmt, module );
	}

/*
=================================================
	SetSpecializationConstants
=================================================
*/
	static void SetSpecializationConstants (INOUT FixedMap<EShader, PipelineDescription::Shader, 8> &shadersMap, EShader shaderType,
											ArrayView<PipelineDescription::SpecConstant> values)
	{
		auto&	shader = shadersMap.insert({ shaderType, {} }).first->second;
		ASSERT( shader.data.size() > 0 );
		ASSERT( shader.specConstants.empty() );

		CopySpecConstants( values, OUT shader.specConstants );
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	constructor
=================================================
*/
	GraphicsPipelineDesc::GraphicsPipelineDesc ()
	{
	}
	
/*
=================================================
	IsGraphicsShader
=================================================
*/
	ND_ inline bool  IsGraphicsShader (EShader shaderType)
	{
		switch( shaderType ) { 
			case EShader::Vertex :
			case EShader::TessControl :
			case EShader::TessEvaluation :
			case EShader::Geometry :
			case EShader::Fragment :
				return true;
		}
		return false;
	}

/*
=================================================
	AddShader
=================================================
*/
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName)
	{
		ASSERT( IsGraphicsShader( shaderType ));
		AddShaderSource( INOUT _shaders, shaderType, fmt, entry, std::move(src), dbgName );
		return *this;
	}
	
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName)
	{
		ASSERT( IsGraphicsShader( shaderType ));
		AddShaderBinary8( INOUT _shaders, shaderType, fmt, entry, std::move(bin), dbgName );
		return *this;
	}
	
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName)
	{
		ASSERT( IsGraphicsShader( shaderType ));
		AddShaderBinary32( INOUT _shaders, shaderType, fmt, entry, std::move(bin), dbgName );
		return *this;
	}
	
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, const VkShaderPtr &module)
	{
		ASSERT( IsGraphicsShader( shaderType ));
		AddShaderModule( INOUT _shaders, shaderType, fmt, std::move(module) );
		return *this;
	}

/*
=================================================
	SetSpecConstants
=================================================
*/
	GraphicsPipelineDesc&  GraphicsPipelineDesc::SetSpecConstants (EShader shaderType, ArrayView< SpecConstant > values)
	{
		ASSERT( IsGraphicsShader( shaderType ));
		
		SetSpecializationConstants( INOUT _shaders, shaderType, values );
		return *this;
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	constructor
=================================================
*/
	MeshPipelineDesc::MeshPipelineDesc () :
		_taskSizeSpec{ UNDEFINED_SPECIALIZATION },
		_meshSizeSpec{ UNDEFINED_SPECIALIZATION }
	{
	}

/*
=================================================
	IsMeshProcessingShader
=================================================
*/
	ND_ inline bool  IsMeshProcessingShader (EShader shaderType)
	{
		switch ( shaderType ) {
			case EShader::MeshTask :
			case EShader::Mesh :
			case EShader::Fragment :
				return true;
		}
		return false;
	}

/*
=================================================
	AddShader
=================================================
*/
	MeshPipelineDesc&  MeshPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName)
	{
		ASSERT( IsMeshProcessingShader( shaderType ));
		AddShaderSource( INOUT _shaders, shaderType, fmt, entry, std::move(src), dbgName );
		return *this;
	}
	
	MeshPipelineDesc&  MeshPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName)
	{
		ASSERT( IsMeshProcessingShader( shaderType ));
		AddShaderBinary8( INOUT _shaders, shaderType, fmt, entry, std::move(bin), dbgName );
		return *this;
	}
	
	MeshPipelineDesc&  MeshPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName)
	{
		ASSERT( IsMeshProcessingShader( shaderType ));
		AddShaderBinary32( INOUT _shaders, shaderType, fmt, entry, std::move(bin), dbgName );
		return *this;
	}
	
	MeshPipelineDesc&  MeshPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, const VkShaderPtr &module)
	{
		ASSERT( IsMeshProcessingShader( shaderType ));
		AddShaderModule( INOUT _shaders, shaderType, fmt, std::move(module) );
		return *this;
	}

/*
=================================================
	SetSpecConstants
=================================================
*/
	MeshPipelineDesc&  MeshPipelineDesc::SetSpecConstants (EShader shaderType, ArrayView< SpecConstant > values)
	{
		ASSERT( IsMeshProcessingShader( shaderType ));
		
		SetSpecializationConstants( INOUT _shaders, shaderType, values );
		return *this;
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	constructor
=================================================
*/
	RayTracingPipelineDesc::RayTracingPipelineDesc ()
	{
	}

/*
=================================================
	IsRayTracingShader
=================================================
*/
	ND_ inline bool  IsRayTracingShader (EShader shaderType)
	{
		switch ( shaderType ) {
			case EShader::RayGen :
			case EShader::RayAnyHit :
			case EShader::RayClosestHit :
			case EShader::RayMiss :
			case EShader::RayIntersection :
			case EShader::RayCallable :
				return true;
		}
		return false;
	}

/*
=================================================
	AddShader
=================================================
*/
	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName)
	{
		ASSERT( IsRayTracingShader( shaderType ));

		auto&	shader = _shaders.insert({ id, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );
		
		shader.shaderType = shaderType;
		shader.AddShaderData( fmt, entry, std::move(src), dbgName );
		return *this;
	}
	
	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName)
	{
		ASSERT( IsRayTracingShader( shaderType ));

		auto&	shader = _shaders.insert({ id, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );
		
		shader.shaderType = shaderType;
		shader.AddShaderData( fmt, entry, std::move(bin), dbgName );
		return *this;
	}
	
	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName)
	{
		ASSERT( IsRayTracingShader( shaderType ));

		auto&	shader = _shaders.insert({ id, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );
		
		shader.shaderType = shaderType;
		shader.AddShaderData( fmt, entry, std::move(bin), dbgName );
		return *this;
	}
	
	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, const VkShaderPtr &module)
	{
		ASSERT( IsRayTracingShader( shaderType ));

		auto&	shader = _shaders.insert({ id, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );

		shader.shaderType = shaderType;
		shader.data.insert_or_assign( fmt, module );
		return *this;
	}

/*
=================================================
	SetSpecConstants
=================================================
*/
	RayTracingPipelineDesc&  RayTracingPipelineDesc::SetSpecConstants (const RTShaderID &id, ArrayView< SpecConstant > values)
	{
		auto&	shader = _shaders.insert({ id, {} }).first->second;
		ASSERT( shader.data.size() > 0 );
		ASSERT( shader.specConstants.empty() );

		CopySpecConstants( values, OUT shader.specConstants );
		return *this;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	ComputePipelineDesc::ComputePipelineDesc () :
		_localSizeSpec{UNDEFINED_SPECIALIZATION}
	{}

/*
=================================================
	AddShader
=================================================
*/
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		_shader.AddShaderData( fmt, entry, std::move(src), dbgName );
		return *this;
	}
	
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		_shader.AddShaderData( fmt, entry, std::move(bin), dbgName );
		return *this;
	}
	
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		_shader.AddShaderData( fmt, entry, std::move(bin), dbgName );
		return *this;
	}
	
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, const VkShaderPtr &module)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		_shader.data.insert_or_assign( fmt, module );
		return *this;
	}

/*
=================================================
	SetSpecConstants
=================================================
*/
	ComputePipelineDesc&  ComputePipelineDesc::SetSpecConstants (ArrayView< SpecConstant > values)
	{
		ASSERT( _shader.data.size() > 0 );
		ASSERT( _shader.specConstants.empty() );
		
		CopySpecConstants( values, OUT _shader.specConstants );
		return *this;
	}
//-----------------------------------------------------------------------------


}	// FG
