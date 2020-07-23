// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/BindingIndex.h"
#include "framegraph/Public/RenderState.h"
#include "framegraph/Public/ShaderEnums.h"
#include "framegraph/Public/ResourceEnums.h"
#include "framegraph/Public/VertexInputState.h"
#include "framegraph/Public/VulkanTypes.h"
#include "framegraph/Public/EResourceState.h"

namespace FG
{

	//
	// Pipeline Description
	//

	struct PipelineDescription
	{
	// types
	public:
		static constexpr uint	STATIC_OFFSET = UMax;

		struct Texture
		{
			EResourceState		state				= Default;
			EImage				textureType			= Default;

			ND_ bool  operator == (const Texture &rhs) const;
		};

		struct Sampler
		{
			ND_ bool  operator == (const Sampler &rhs) const;
		};

		struct SubpassInput
		{
			EResourceState		state				= Default;
			uint				attachmentIndex		= UMax;
			bool				isMultisample		= false;
			
			ND_ bool  operator == (const SubpassInput &rhs) const;
		};

		struct Image
		{
			EResourceState		state				= Default;
			EImage				imageType			= Default;
			EPixelFormat		format				= Default;
			
			ND_ bool  operator == (const Image &rhs) const;
		};

		struct UniformBuffer
		{
			EResourceState		state				= Default;
			uint				dynamicOffsetIndex	= STATIC_OFFSET;
			BytesU				size;

			ND_ bool  operator == (const UniformBuffer &rhs) const;
		};

		struct StorageBuffer
		{
			EResourceState		state				= Default;
			uint				dynamicOffsetIndex	= STATIC_OFFSET;
			BytesU				staticSize;
			BytesU				arrayStride;

			ND_ bool  operator == (const StorageBuffer &rhs) const;
		};

		struct RayTracingScene
		{
			EResourceState		state				= Default;
			
			ND_ bool  operator == (const RayTracingScene &rhs) const;
		};


	// uniforms
		struct _TextureUniform
		{
			const UniformID			id;
			const Texture			data;
			const BindingIndex		index;
			const uint				arraySize;
			const EShaderStages		stageFlags;

			_TextureUniform (const UniformID &id, EImage textureType, const BindingIndex &index, uint arraySize, EShaderStages stageFlags);
		};

		struct _SamplerUniform
		{
			const UniformID			id;
			const Sampler			data;
			const BindingIndex		index;
			const uint				arraySize;
			const EShaderStages		stageFlags;

			_SamplerUniform (const UniformID &id, const BindingIndex &index, uint arraySize, EShaderStages stageFlags);
		};

		struct _SubpassInputUniform
		{
			const UniformID			id;
			const SubpassInput		data;
			const BindingIndex		index;
			const uint				arraySize;
			const EShaderStages		stageFlags;

			_SubpassInputUniform (const UniformID &id, uint attachmentIndex, bool isMultisample, const BindingIndex &index,
								  uint arraySize, EShaderStages stageFlags);
		};

		struct _ImageUniform
		{
			const UniformID			id;
			const Image				data;
			const BindingIndex		index;
			const uint				arraySize;
			const EShaderStages		stageFlags;

			_ImageUniform (const UniformID &id, EImage imageType, EPixelFormat format, EShaderAccess access,
						   const BindingIndex &index, uint arraySize, EShaderStages stageFlags);
		};

		struct _UBufferUniform
		{
			const UniformID			id;
			const UniformBuffer		data;
			const BindingIndex		index;
			const uint				arraySize;
			const EShaderStages		stageFlags;

			_UBufferUniform (const UniformID &id, BytesU size, const BindingIndex &index, uint arraySize,
							 EShaderStages stageFlags, uint dynamicOffsetIndex = STATIC_OFFSET);
		};

		struct _StorageBufferUniform
		{
			const UniformID			id;
			const StorageBuffer		data;
			const BindingIndex		index;
			const uint				arraySize;
			const EShaderStages		stageFlags;

			_StorageBufferUniform (const UniformID &id, BytesU staticSize, BytesU arrayStride, EShaderAccess access, const BindingIndex &index,
								   uint arraySize, EShaderStages stageFlags, uint dynamicOffsetIndex = STATIC_OFFSET);
		};
		
		struct _RayTracingSceneUniform
		{
			const UniformID			id;
			const RayTracingScene	data;
			const BindingIndex		index;
			const uint				arraySize;
			const EShaderStages		stageFlags;

			_RayTracingSceneUniform (const UniformID &id, const BindingIndex &index, uint arraySize, EShaderStages stageFlags);
		};

		struct PushConstant
		{
			EShaderStages		stageFlags;
			Bytes<uint16_t>		offset;
			Bytes<uint16_t>		size;

			PushConstant () {}
			PushConstant (EShaderStages stages, BytesU offset, BytesU size) : stageFlags{stages}, offset{offset}, size{size} {}
		};

		struct _PushConstant
		{
			PushConstantID		id;
			PushConstant		data;

			_PushConstant (const PushConstantID &id, EShaderStages stages, BytesU offset, BytesU size) : id{id}, data{stages, offset, size} {}
		};

		struct SpecConstant
		{
			SpecializationID	id;
			uint				index;
		};

		using UniformData_t		= Union< NullUnion, Texture, Sampler, SubpassInput, Image, UniformBuffer, StorageBuffer, RayTracingScene >;

		struct Uniform
		{
			UniformData_t		data;
			BindingIndex		index;
			uint				arraySize	= 1;
			EShaderStages		stageFlags	= Default;
			
			ND_ bool  operator == (const Uniform &rhs) const;
		};

		using UniformMap_t	= HashMap< UniformID, Uniform >;
		using UniformMapPtr	= SharedPtr< const UniformMap_t >;

		struct DescriptorSet
		{
			DescriptorSetID		id;
			uint				bindingIndex	= UMax;
			UniformMapPtr		uniforms;
		};

		using DescriptorSets_t	= FixedArray< DescriptorSet, FG_MaxDescriptorSets >;
		using PushConstants_t	= FixedMap< PushConstantID, PushConstant, FG_MaxPushConstants >;

		struct PipelineLayout
		{
			DescriptorSets_t	descriptorSets;
			PushConstants_t		pushConstants;

			PipelineLayout () {}
		};

		template <typename T>
		class IShaderData {
		public:
			ND_ virtual T const&	GetData () const = 0;
			ND_ virtual StringView	GetEntry () const = 0;
			ND_ virtual StringView	GetDebugName () const = 0;
			ND_ virtual size_t		GetHashOfData () const = 0;
				virtual bool		ParseDebugOutput (EShaderDebugMode mode, ArrayView<uint8_t> trace, OUT Array<String> &result) const = 0;
		};

		template <typename T>
		using SharedShaderPtr	= SharedPtr< IShaderData<T> >;
		using VkShaderPtr		= SharedShaderPtr< ShaderModuleVk_t >;

		using ShaderDataUnion_t	= Union< NullUnion, SharedShaderPtr<String>, SharedShaderPtr<Array<uint8_t>>, SharedShaderPtr<Array<uint>>, VkShaderPtr >;
		using ShaderDataMap_t	= HashMap< EShaderLangFormat, ShaderDataUnion_t >;
		using SpecConstants_t	= FixedMap< SpecializationID, uint, FG_MaxSpecConstants >;	// id, index

		struct Shader
		{
			ShaderDataMap_t		data;
			SpecConstants_t		specConstants;

			Shader () {}
			void AddShaderData (EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName = Default);
			void AddShaderData (EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName = Default);
			void AddShaderData (EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName = Default);
		};


	// variables
	public:
		PipelineLayout		_pipelineLayout;


	// methods
	protected:
		PipelineDescription () {}

		void _AddDescriptorSet (const DescriptorSetID					&id,
								uint									index,
								ArrayView< _TextureUniform >			textures,
								ArrayView< _SamplerUniform >			samplers,
								ArrayView< _SubpassInputUniform >		subpassInputs,
								ArrayView< _ImageUniform >				images,
								ArrayView< _UBufferUniform >			uniformBuffers,
								ArrayView< _StorageBufferUniform >		storageBuffers,
								ArrayView< _RayTracingSceneUniform >	rtScenes);

		void _SetPushConstants (ArrayView< _PushConstant > values);
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
			uint			index	= UMax;
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
		using VertexAttribs_t	= FixedArray< VertexAttrib, FG_MaxVertexAttribs >;
		using FragmentOutputs_t	= FixedArray< FragmentOutput, FG_MaxColorBuffers >;
		

	// variables
		Shaders_t					_shaders;
		TopologyBits_t				_supportedTopology;
		FragmentOutputs_t			_fragmentOutput;
		VertexAttribs_t				_vertexAttribs;
		uint						_patchControlPoints		= 0;
		bool						_earlyFragmentTests		= true;


	// methods
		GraphicsPipelineDesc ();

		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName = Default);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName = Default);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName = Default);
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

		Self&  SetPushConstants (ArrayView< _PushConstant > values)
		{
			_SetPushConstants( values );
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
		
		static constexpr uint	UNDEFINED_SPECIALIZATION = UMax;

	// variables
		Shader		_shader;
		uint3		_defaultLocalGroupSize;
		uint3		_localSizeSpec;


	// methods
		ComputePipelineDesc ();
		
		Self&  AddShader (EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName = Default);
		Self&  AddShader (EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName = Default);
		Self&  AddShader (EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName = Default);
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
		
		Self&  SetPushConstants (ArrayView< _PushConstant > values)
		{
			_SetPushConstants( values );
			return *this;
		}
		
		Self&  SetSpecConstants (ArrayView< SpecConstant > values);
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
		
		static constexpr uint	UNDEFINED_SPECIALIZATION = UMax;


	// variables
		Shaders_t			_shaders;
		EPrimitive			_topology			= Default;
		FragmentOutputs_t	_fragmentOutput;
		uint				_maxVertices		= 0;
		uint				_maxIndices			= 0;
		uint3				_defaultTaskGroupSize;
		uint3				_taskSizeSpec;
		uint3				_defaultMeshGroupSize;
		uint3				_meshSizeSpec;
		bool				_earlyFragmentTests	= true;


	// methods
		MeshPipelineDesc ();

		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName = Default);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName = Default);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName = Default);
		Self&  AddShader (EShader shaderType, EShaderLangFormat fmt, const VkShaderPtr &module);
		
		Self&  SetTopology (EPrimitive value)
		{
			_topology = value;
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

		Self&  SetPushConstants (ArrayView< _PushConstant > values)
		{
			_SetPushConstants( values );
			return *this;
		}

		Self&  SetFragmentOutputs (ArrayView< FragmentOutput > outputs)
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
		struct RTShader final : Shader
		{
			EShader		shaderType	= Default;
		};

		using Self				= RayTracingPipelineDesc;
		using Shaders_t			= HashMap< RTShaderID, RTShader >;


	// variables
		Shaders_t		_shaders;


	// methods
		RayTracingPipelineDesc ();

		Self&  AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, StringView entry, String &&src, StringView dbgName = Default);
		Self&  AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint8_t> &&bin, StringView dbgName = Default);
		Self&  AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, StringView entry, Array<uint> &&bin, StringView dbgName = Default);
		Self&  AddShader (const RTShaderID &id, EShader shaderType, EShaderLangFormat fmt, const VkShaderPtr &module);
		
		Self&  AddDescriptorSet (const DescriptorSetID					&id,
								 const uint								index,
								 ArrayView< _TextureUniform >			textures,
								 ArrayView< _SamplerUniform >			samplers,
								 ArrayView< _SubpassInputUniform >		subpassInputs,
								 ArrayView< _ImageUniform >				images,
								 ArrayView< _UBufferUniform >			uniformBuffers,
								 ArrayView< _StorageBufferUniform >		storageBuffers,
								 ArrayView< _RayTracingSceneUniform >	rtScenes)
		{
			_AddDescriptorSet( id, index, textures, samplers, subpassInputs, images, uniformBuffers, storageBuffers, rtScenes );
			return *this;
		}
		
		Self&  SetPushConstants (ArrayView< _PushConstant > values)
		{
			_SetPushConstants( values );
			return *this;
		}

		Self&  SetSpecConstants (const RTShaderID &id, ArrayView< SpecConstant > values);
	};


}	// FG
