// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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

		bool		ParseDebugOutput (EShaderDebugMode, ArrayView<uint8_t>, OUT Array<String> &) const override { return false; }
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
	
	void PipelineDescription::Shader::AddShaderData (EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin)
	{
		data.insert({ fmt, MakeShared<ShaderDataImpl<Array<uint8_t>>>( std::move(bin), entry ) });
	}
	
	void PipelineDescription::Shader::AddShaderData (EShaderLangFormat fmt, StringView entry, Array<uint> &&bin)
	{
		data.insert({ fmt, MakeShared<ShaderDataImpl<Array<uint>>>( std::move(bin), entry ) });
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	_TextureUniform
=================================================
*/
	PipelineDescription::_TextureUniform::_TextureUniform (const UniformID &id, EImage textureType, const BindingIndex &index, EShaderStages stageFlags) :
		id{id}, data{ EResourceState::ShaderSample | EResourceState_FromShaders( stageFlags ), textureType }, index{index}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_SamplerUniform
=================================================
*/
	PipelineDescription::_SamplerUniform::_SamplerUniform (const UniformID &id, const BindingIndex &index, EShaderStages stageFlags) :
		id{id}, data{}, index{index}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_SubpassInputUniform
=================================================
*/
	PipelineDescription::_SubpassInputUniform::_SubpassInputUniform (const UniformID &id, uint attachmentIndex, bool isMultisample, const BindingIndex &index, EShaderStages stageFlags) :
		id{id}, data{ EResourceState::InputAttachment | EResourceState_FromShaders( stageFlags ), attachmentIndex, isMultisample },
		index{index}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_ImageUniform
=================================================
*/
	PipelineDescription::_ImageUniform::_ImageUniform (const UniformID &id, EImage imageType, EPixelFormat format, EShaderAccess access, const BindingIndex &index, EShaderStages stageFlags) :
		id{id}, data{ EResourceState_FromShaders( stageFlags ) | EResourceState_FromShaderAccess( access ), imageType, format },
		index{index}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_UBufferUniform
=================================================
*/
	PipelineDescription::_UBufferUniform::_UBufferUniform (const UniformID &id, BytesU size, const BindingIndex &index, EShaderStages stageFlags, uint dynamicOffsetIndex) :
		id{id},
		data{ EResourceState::UniformRead | EResourceState_FromShaders( stageFlags )
			  | (dynamicOffsetIndex != STATIC_OFFSET ? EResourceState::_BufferDynamicOffset : EResourceState::Unknown),
			  dynamicOffsetIndex, size },
		index{index}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_StorageBufferUniform
=================================================
*/
	PipelineDescription::_StorageBufferUniform::_StorageBufferUniform (const UniformID &id, BytesU staticSize, BytesU arrayStride, EShaderAccess access,
																	   const BindingIndex &index, EShaderStages stageFlags, uint dynamicOffsetIndex) :
		id{id},
		data{ EResourceState_FromShaders( stageFlags ) | EResourceState_FromShaderAccess( access )
			  | (dynamicOffsetIndex != STATIC_OFFSET ? EResourceState::_BufferDynamicOffset : EResourceState::Unknown),
			  dynamicOffsetIndex, staticSize, arrayStride },
		index{index}, stageFlags{stageFlags}
	{
		ASSERT( id.IsDefined() );
	}
	
/*
=================================================
	_RayTracingSceneUniform
=================================================
*/
	PipelineDescription::_RayTracingSceneUniform::_RayTracingSceneUniform (const UniformID &id, const BindingIndex &index, EShaderStages stageFlags) :
		id{id}, data{ EResourceState::_RayTracingShader | EResourceState::_Access_ShaderStorage }, index{index}, stageFlags{stageFlags}
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
			uniforms.insert({ tex.id, {Texture{ tex.data }, tex.index, tex.stageFlags} });
		}

		for (auto& samp : samplers) {
			uniforms.insert({ samp.id, {Sampler{ samp.data }, samp.index, samp.stageFlags} });
		}

		for (auto& spi : subpassInputs) {
			uniforms.insert({ spi.id, {SubpassInput{ spi.data }, spi.index, spi.stageFlags} });
		}

		for (auto& img : images) {
			uniforms.insert({ img.id, {Image{ img.data }, img.index, img.stageFlags} });
		}

		for (auto& ub : uniformBuffers) {
			uniforms.insert({ ub.id, {UniformBuffer{ ub.data }, ub.index, ub.stageFlags} });
		}

		for (auto& sb : storageBuffers) {
			uniforms.insert({ sb.id, {StorageBuffer{ sb.data }, sb.index, sb.stageFlags} });
		}

		for (auto& rts : rtScenes) {
			uniforms.insert({ rts.id, {RayTracingScene{ rts.data.state }, rts.index, rts.stageFlags} });
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
			_pipelineLayout.pushConstants.insert({ pc.id, pc.data });
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
	AddShaderSource
=================================================
*/
	static void AddShaderSource (INOUT FixedMap<EShader, PipelineDescription::Shader, 8> &shadersMap, EShader shaderType,
								 EShaderLangFormat fmt, StringView entry, String &&src)
	{
		auto&	shader = shadersMap.insert({ shaderType, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );

		shader.AddShaderData( fmt, entry, std::move(src) );
	}
	
/*
=================================================
	AddShaderBinary8
=================================================
*/
	static void AddShaderBinary8 (INOUT FixedMap<EShader, PipelineDescription::Shader, 8> &shadersMap, EShader shaderType,
								  EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin)
	{
		auto&	shader = shadersMap.insert({ shaderType, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );

		shader.AddShaderData( fmt, entry, std::move(bin) );
	}
	
/*
=================================================
	AddShaderBinary32
=================================================
*/
	static void AddShaderBinary32 (INOUT FixedMap<EShader, PipelineDescription::Shader, 8> &shadersMap, EShader shaderType,
								   EShaderLangFormat fmt, StringView entry, Array<uint> &&bin)
	{
		auto&	shader = shadersMap.insert({ shaderType, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );

		shader.AddShaderData( fmt, entry, std::move(bin) );
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

		shader.data.insert({ fmt, module });
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
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src)
	{
		ASSERT( IsGraphicsShader( shaderType ));
		AddShaderSource( INOUT _shaders, shaderType, fmt, entry, std::move(src) );
		return *this;
	}
	
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin)
	{
		ASSERT( IsGraphicsShader( shaderType ));
		AddShaderBinary8( INOUT _shaders, shaderType, fmt, entry, std::move(bin) );
		return *this;
	}
	
	GraphicsPipelineDesc&  GraphicsPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin)
	{
		ASSERT( IsGraphicsShader( shaderType ));
		AddShaderBinary32( INOUT _shaders, shaderType, fmt, entry, std::move(bin) );
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
	MeshPipelineDesc&  MeshPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src)
	{
		ASSERT( IsMeshProcessingShader( shaderType ));
		AddShaderSource( INOUT _shaders, shaderType, fmt, entry, std::move(src) );
		return *this;
	}
	
	MeshPipelineDesc&  MeshPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin)
	{
		ASSERT( IsMeshProcessingShader( shaderType ));
		AddShaderBinary8( INOUT _shaders, shaderType, fmt, entry, std::move(bin) );
		return *this;
	}
	
	MeshPipelineDesc&  MeshPipelineDesc::AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin)
	{
		ASSERT( IsMeshProcessingShader( shaderType ));
		AddShaderBinary32( INOUT _shaders, shaderType, fmt, entry, std::move(bin) );
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
	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src)
	{
		ASSERT( IsRayTracingShader( shaderType ));

		auto&	shader = _shaders.insert({ id, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );
		
		shader.shaderType = shaderType;
		shader.AddShaderData( fmt, entry, std::move(src) );
		return *this;
	}
	
	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin)
	{
		ASSERT( IsRayTracingShader( shaderType ));

		auto&	shader = _shaders.insert({ id, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );
		
		shader.shaderType = shaderType;
		shader.AddShaderData( fmt, entry, std::move(bin) );
		return *this;
	}
	
	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin)
	{
		ASSERT( IsRayTracingShader( shaderType ));

		auto&	shader = _shaders.insert({ id, {} }).first->second;
		ASSERT( shader.data.count( fmt ) == 0 );
		
		shader.shaderType = shaderType;
		shader.AddShaderData( fmt, entry, std::move(bin) );
		return *this;
	}
	
	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, const VkShaderPtr &module)
	{
		ASSERT( IsRayTracingShader( shaderType ));

		auto&	shader = _shaders.insert({ id, {} }).first->second;
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
	RayTracingPipelineDesc&  RayTracingPipelineDesc::SetSpecConstants (const RTShaderID &id, ArrayView< SpecConstant > values)
	{
		auto&	shader = _shaders.insert({ id, {} }).first->second;
		ASSERT( shader.data.size() > 0 );
		ASSERT( shader.specConstants.empty() );

		CopySpecConstants( values, OUT shader.specConstants );
		return *this;
	}
	
/*
=================================================
	add shader groups
=================================================
*/
	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddRayGenShader (const RTShaderGroupID &name, const RTShaderID &id)
	{
		ASSERT( name.IsDefined() );
		ASSERT( id.IsDefined() );
		ASSERT( _groups.count( name ) == 0 );

		_groups.insert_or_assign( name, GeneralGroup{id} );
		return *this;
	}

	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddRayMissShader (const RTShaderGroupID &name, const RTShaderID &id)
	{
		ASSERT( name.IsDefined() );
		ASSERT( id.IsDefined() );
		ASSERT( _groups.count( name ) == 0 );

		_groups.insert_or_assign( name, GeneralGroup{id} );
		return *this;
	}

	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddCallableShader (const RTShaderGroupID &name, const RTShaderID &id)
	{
		ASSERT( name.IsDefined() );
		ASSERT( id.IsDefined() );
		ASSERT( _groups.count( name ) == 0 );

		_groups.insert_or_assign( name, GeneralGroup{id} );
		return *this;
	}

	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddTriangleHitShaders (const RTShaderGroupID &name, const RTShaderID &closestHit, const RTShaderID &anyHit)
	{
		ASSERT( name.IsDefined() );
		ASSERT( closestHit.IsDefined() or anyHit.IsDefined() );
		ASSERT( _groups.count( name ) == 0 );

		_groups.insert_or_assign( name, TriangleHitGroup{closestHit, anyHit} );
		return *this;
	}

	RayTracingPipelineDesc&  RayTracingPipelineDesc::AddProceduralHitShaders (const RTShaderGroupID &name, const RTShaderID &closestHit, const RTShaderID &anyHit, const RTShaderID &inersection)
	{
		ASSERT( name.IsDefined() );
		ASSERT( inersection.IsDefined() and (closestHit.IsDefined() or anyHit.IsDefined()) );
		ASSERT( _groups.count( name ) == 0 );

		_groups.insert_or_assign( name, ProceduralHitGroup{closestHit, anyHit, inersection} );
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
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, StringView entry, String &&src)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		_shader.AddShaderData( fmt, entry, std::move(src) );
		return *this;
	}
	
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		_shader.AddShaderData( fmt, entry, std::move(bin) );
		return *this;
	}
	
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, StringView entry, Array<uint> &&bin)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		_shader.AddShaderData( fmt, entry, std::move(bin) );
		return *this;
	}
	
	ComputePipelineDesc&  ComputePipelineDesc::AddShader (EShaderLangFormat fmt, const VkShaderPtr &module)
	{
		ASSERT( _shader.data.count( fmt ) == 0 );
		_shader.data.insert({ fmt, module });
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
