// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/BindingIndex.h"
#include "framegraph/Public/LowLevel/PipelineResources.h"
#include "framegraph/Public/LowLevel/RenderState.h"
#include "framegraph/Public/LowLevel/ShaderEnums.h"
#include "framegraph/Public/LowLevel/VertexInputState.h"
#include "framegraph/Public/LowLevel/VulkanTypes.h"

namespace FG
{

	//
	// Pipeline Description
	//

	struct PipelineDescription
	{
	// types
	public:
		struct Texture
		{
			EImage				textureType;
			BindingIndex		index;
			EShaderStages		stageFlags;
		};

		struct Sampler
		{
			BindingIndex		index;
			EShaderStages		stageFlags;
		};

		struct SubpassInput
		{
			uint				attachmentIndex;
			bool				isMultisample;
			BindingIndex		index;
			EShaderStages		stageFlags;
		};

		struct Image
		{
			EImage				imageType;
			EPixelFormat		format;
			EShaderAccess		access;
			BindingIndex		index;
			EShaderStages		stageFlags;
		};

		struct UniformBuffer
		{
			BytesU				size;
			BindingIndex		index;
			EShaderStages		stageFlags;
		};

		struct StorageBuffer
		{
			BytesU				staticSize;
			BytesU				arrayStride;
			EShaderAccess		access;
			BindingIndex		index;
			EShaderStages		stageFlags;
		};

		struct AccelerationStructure
		{
		};


	// uniforms
		struct _TextureUniform
		{
			UniformID		id;
			Texture			data;

			_TextureUniform (const UniformID &id, EImage textureType, const BindingIndex &index, EShaderStages stageFlags) :
				id{id}, data{textureType, index, stageFlags} {}
		};

		struct _SamplerUniform
		{
			UniformID		id;
			Sampler			data;

			_SamplerUniform (const UniformID &id, const BindingIndex &index, EShaderStages stageFlags) :
				id{id}, data{index, stageFlags} {}
		};

		struct _SubpassInputUniform
		{
			UniformID		id;
			SubpassInput	data;

			_SubpassInputUniform (const UniformID &id, uint attachmentIndex, bool isMultisample, const BindingIndex &index, EShaderStages stageFlags) :
				id{id}, data{attachmentIndex, isMultisample, index, stageFlags} {}
		};

		struct _ImageUniform
		{
			UniformID		id;
			Image			data;

			_ImageUniform (const UniformID &id, EImage imageType, EPixelFormat format, EShaderAccess access, const BindingIndex &index, EShaderStages stageFlags) :
				id{id}, data{imageType, format, access, index, stageFlags} {}
		};

		struct _UBufferUniform
		{
			UniformID		id;
			UniformBuffer	data;

			_UBufferUniform (const UniformID &id, BytesU size, const BindingIndex &index, EShaderStages stageFlags) :
				id{id}, data{size, index, stageFlags} {}
		};

		struct _StorageBufferUniform
		{
			UniformID		id;
			StorageBuffer	data;

			_StorageBufferUniform (const UniformID &id, BytesU staticSize, BytesU arrayStride, EShaderAccess access, const BindingIndex &index, EShaderStages stageFlags) :
				id{id}, data{staticSize, arrayStride, access, index, stageFlags} {}
		};
		
		struct _AccelerationStructureUniform
		{
			UniformID		id;
			AccelerationStructure	data;
		};

		using Uniform_t		= Union< std::monostate, Texture, Sampler, SubpassInput, Image, UniformBuffer, StorageBuffer, AccelerationStructure >;
		using UniformMap_t	= HashMap< UniformID, Uniform_t >;

		struct DescriptorSet
		{
			DescriptorSetID		id;
			uint				bindingIndex	= ~0u;
			UniformMap_t		uniforms;
		};

		struct PushConstant
		{
			BytesU				offset;
			// TODO
		};

		struct SpecConstant
		{
			SpecializationID	id;
			int					index;
		};

		using DescriptorSets_t	= FixedArray< DescriptorSet, FG_MaxDescriptorSets >;
		using PushConstants_t	= FixedArray< PushConstant, FG_MaxPushConstants >;

		struct PipelineLayout
		{
			DescriptorSets_t	descriptorSets;
			PushConstants_t		pushConstants;
		};

		template <typename T>
		class IShaderData
		{
		public:
			ND_ virtual T const&	GetData () const = 0;
			ND_ virtual StringView	GetEntry () const = 0;
			ND_ virtual size_t		GetHashOfData () const = 0;
		};

		template <typename T>
		using SharedShaderPtr	= SharedPtr< IShaderData<T> >;
		using VkShaderPtr		= SharedShaderPtr<VkShaderModule_t>;

		using ShaderDataUnion_t	= Union< std::monostate, SharedShaderPtr<String>, SharedShaderPtr<Array<uint8_t>>, SharedShaderPtr<Array<uint>>, VkShaderPtr >;
		using ShaderDataMap_t	= HashMap< EShaderLangFormat, ShaderDataUnion_t >;
		using SpecConstants_t	= FixedMap< SpecializationID, uint, FG_MaxSpecConstants >;

		struct Shader
		{
		// variables
			ShaderDataMap_t		data;
			SpecConstants_t		specConstants;

		// methods
			Shader () {}

			void AddShaderData (EShaderLangFormat fmt, StringView entry, String &&src);
			void AddShaderData (EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin);
			void AddShaderData (EShaderLangFormat fmt, StringView entry, Array<uint> &&bin);
		};


	// variables
	public:
		PipelineLayout		_pipelineLayout;


	// methods
	protected:
		PipelineDescription () {}

		void _AddDescriptorSet (const DescriptorSetID						&id,
								uint										index,
								ArrayView< _TextureUniform >				textures,
								ArrayView< _SamplerUniform >				samplers,
								ArrayView< _SubpassInputUniform >			subpassInputs,
								ArrayView< _ImageUniform >					images,
								ArrayView< _UBufferUniform >				uniformBuffers,
								ArrayView< _StorageBufferUniform >			storageBuffers,
								ArrayView< _AccelerationStructureUniform >	accelerationStructures);

		void _SetPushConstants (ArrayView< PushConstant > values);

	protected:
		ND_ PipelineLayout const&	GetPipelineLayout () const		{ return _pipelineLayout; }
	};



	//
	// Graphics Pipeline Description
	//

	struct GraphicsPipelineDesc final : PipelineDescription
	{
	// types
		struct FragmentOutput
		{
		// variables
			RenderTargetID	id;
			uint			index	= ~0u;
			EFragOutput		type	= Default;

		// methods
			FragmentOutput () {}
			FragmentOutput (const RenderTargetID &id, uint index, EFragOutput type) : id{id}, index{index}, type{type} {}

			ND_ bool operator == (const FragmentOutput &rhs) const;
		};

		using Self				= GraphicsPipelineDesc;
		using TopologyBits_t	= BitSet< uint(EPrimitive::_Count) >;
		using Shaders_t			= FixedMap< EShader, Shader, 8 >;
		using VertexAttrib		= VertexInputState::VertexAttrib;
		using VertexAttribs_t	= FixedArray< VertexAttrib, FG_MaxAttribs >;
		using FragmentOutputs_t	= FixedArray< FragmentOutput, FG_MaxColorBuffers >;
		

	// variables
		Shaders_t					_shaders;
		TopologyBits_t				_supportedTopology;
		FragmentOutputs_t			_fragmentOutput;
		VertexAttribs_t				_vertexAttribs;
		uint						_patchControlPoints;
		bool						_earlyFragmentTests	= true;


	// methods
		GraphicsPipelineDesc ();

		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, const VkShaderPtr &module);
		
		Self&  AddTopology (EPrimitive primitive)
		{
			_supportedTopology[uint(primitive)] = true;
			return *this;
		}
		
		Self&  SetFragmentOutputs (ArrayView<FragmentOutput> outputs)
		{
			_fragmentOutput = outputs;
			return *this;
		}

		Self&  SetVertexAttribs (ArrayView<VertexAttrib> attribs)
		{
			_vertexAttribs = attribs;
			return *this;
		}
		
		Self&  SetEarlyFragmentTests (bool value)
		{
			_earlyFragmentTests = value;
			return *this;
		}
		
		Self&  AddDescriptorSet (const DescriptorSetID				&id,
								 const uint							index,
								 ArrayView< _TextureUniform >		textures,
								 ArrayView< _SamplerUniform >		samplers,
								 ArrayView< _SubpassInputUniform >	subpassInputs,
								 ArrayView< _ImageUniform >			images,
								 ArrayView< _UBufferUniform >		uniformBuffers,
								 ArrayView< _StorageBufferUniform >	storageBuffers)
		{
			_AddDescriptorSet( id, index, textures, samplers, subpassInputs, images, uniformBuffers, storageBuffers, Default );
			return *this;
		}

		Self&  SetPushConstants (ArrayView< PushConstant > values)
		{
			_SetPushConstants( values );
			return *this;
		}
		
		Self&  SetSpecConstants (EShader shaderType, ArrayView< SpecConstant > values);
	};



	//
	// Mesh Processing Pipeline Description
	//

	struct MeshPipelineDesc final : PipelineDescription
	{
	// types
		using Self				= MeshPipelineDesc;
		using TopologyBits_t	= GraphicsPipelineDesc::TopologyBits_t;
		using Shaders_t			= FixedMap< EShader, Shader, 8 >;
		using FragmentOutput	= GraphicsPipelineDesc::FragmentOutput;
		using FragmentOutputs_t	= FixedArray< FragmentOutput, FG_MaxColorBuffers >;


	// variables
		Shaders_t			_shaders;
		TopologyBits_t		_supportedTopology;
		FragmentOutputs_t	_fragmentOutput;
		bool				_earlyFragmentTests	= true;


	// methods
		MeshPipelineDesc ();

		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, const VkShaderPtr &module);
		
		Self&  AddTopology (EPrimitive primitive)
		{
			_supportedTopology[uint(primitive)] = true;
			return *this;
		}

		Self&  AddDescriptorSet (const DescriptorSetID				&id,
								 const uint							index,
								 ArrayView< _TextureUniform >		textures,
								 ArrayView< _SamplerUniform >		samplers,
								 ArrayView< _SubpassInputUniform >	subpassInputs,
								 ArrayView< _ImageUniform >			images,
								 ArrayView< _UBufferUniform >		uniformBuffers,
								 ArrayView< _StorageBufferUniform >	storageBuffers)
		{
			_AddDescriptorSet( id, index, textures, samplers, subpassInputs, images, uniformBuffers, storageBuffers, Default );
			return *this;
		}

		Self&  SetPushConstants (ArrayView< PushConstant > values)
		{
			_SetPushConstants( values );
			return *this;
		}

		Self&  SetFragmentOutputs (ArrayView<FragmentOutput> outputs)
		{
			_fragmentOutput = outputs;
			return *this;
		}
		
		Self&  SetEarlyFragmentTests (bool value)
		{
			_earlyFragmentTests = value;
			return *this;
		}
		
		Self&  SetSpecConstants (EShader shaderType, ArrayView< SpecConstant > values);
	};



	//
	// Ray Tracing Pipeline Desription
	//

	struct RayTracingPipelineDesc final : PipelineDescription
	{
	// types
		using Self				= RayTracingPipelineDesc;
		using Shaders_t			= FixedMap< EShader, Shader, 8 >;


	// variables
		Shaders_t			_shaders;


	// methods
		RayTracingPipelineDesc ();

		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, const VkShaderPtr &module);
		
		Self&  AddDescriptorSet (const DescriptorSetID						&id,
								 const uint									index,
								 ArrayView< _TextureUniform >				textures,
								 ArrayView< _SamplerUniform >				samplers,
								 ArrayView< _SubpassInputUniform >			subpassInputs,
								 ArrayView< _ImageUniform >					images,
								 ArrayView< _UBufferUniform >				uniformBuffers,
								 ArrayView< _StorageBufferUniform >			storageBuffers,
								 ArrayView< _AccelerationStructureUniform >	accelerationStructures)
		{
			_AddDescriptorSet( id, index, textures, samplers, subpassInputs, images, uniformBuffers, storageBuffers, accelerationStructures );
			return *this;
		}

		Self&  SetSpecConstants (EShader shaderType, ArrayView< SpecConstant > values);
	};



	//
	// Compute Pipeline Description
	//

	struct ComputePipelineDesc final : PipelineDescription
	{
	// types
		using Self = ComputePipelineDesc;
		
		static constexpr uint	UNDEFINED_SPECIALIZATION = ~0u;

	// variables
		Shader		_shader;
		uint3		_defaultLocalGroupSize;
		uint3		_localSizeSpec;


	// methods
		ComputePipelineDesc ();
		
		Self&  AddShader (EShaderLangFormat fmt, StringView entry, String &&src);
		Self&  AddShader (EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin);
		Self&  AddShader (EShaderLangFormat fmt, StringView entry, Array<uint> &&bin);
		Self&  AddShader (EShaderLangFormat fmt, const VkShaderPtr &module);

		Self&  SetLocalGroupSize (uint x, uint y, uint z)
		{
			_defaultLocalGroupSize = { x, y, z };
			return *this;
		}

		Self&  SetLocalGroupSpecialization (uint x = UNDEFINED_SPECIALIZATION,
											uint y = UNDEFINED_SPECIALIZATION,
											uint z = UNDEFINED_SPECIALIZATION)
		{
			_localSizeSpec = { x, y, z };
			return *this;
		}
		
		Self&  AddDescriptorSet (const DescriptorSetID				&id,
								 const uint							index,
								 ArrayView< _TextureUniform >		textures,
								 ArrayView< _SamplerUniform >		samplers,
								 ArrayView< _SubpassInputUniform >	subpassInputs,
								 ArrayView< _ImageUniform >			images,
								 ArrayView< _UBufferUniform >		uniformBuffers,
								 ArrayView< _StorageBufferUniform >	storageBuffers)
		{
			_AddDescriptorSet( id, index, textures, samplers, subpassInputs, images, uniformBuffers, storageBuffers, Default );
			return *this;
		}
		
		Self&  SetPushConstants (ArrayView< PushConstant > values)
		{
			_SetPushConstants( values );
			return *this;
		}
		
		Self&  SetSpecConstants (ArrayView< SpecConstant > values);
	};


}	// FG
