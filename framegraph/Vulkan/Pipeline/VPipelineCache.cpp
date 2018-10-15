// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VPipelineCache.h"
#include "VLogicalRenderPass.h"
#include "VDevice.h"
#include "VEnumCast.h"
#include "Public/IPipelineCompiler.h"

namespace std
{
	using namespace FG;

	template <>
	struct hash< VkVertexInputAttributeDescription >
	{
		ND_ size_t  operator () (const VkVertexInputAttributeDescription &value) const noexcept
		{
			return size_t(HashOf( value.location ) + HashOf( value.binding ) +
						  HashOf( value.format )   + HashOf( value.offset ));
		}
	};


	template <>
	struct hash< VkVertexInputBindingDescription >
	{
		ND_ size_t  operator () (const VkVertexInputBindingDescription &value) const noexcept
		{
			return size_t(HashOf( value.binding ) + HashOf( value.stride ) + HashOf( value.inputRate ));
		}
	};

}	// std
//-----------------------------------------------------------------------------


namespace FG
{

	//
	// Vulkan Shader Module
	//
	class VShaderModule final : public PipelineDescription::IShaderData< VkShaderModule_t >
	{
	// variables
	private:
		VkShaderModule		_module		= VK_NULL_HANDLE;
		StaticString<64>	_entry;


	// methods
	public:
		VShaderModule (VkShaderModule module, StringView entry) :
			_module{ module }, _entry{ entry } {}

		~VShaderModule () {
			CHECK( _module == VK_NULL_HANDLE );
		}

		void Destroy (const VDevice &dev)
		{
			if ( _module )
			{
				dev.vkDestroyShaderModule( dev.GetVkDevice(), _module, null );
				_module = VK_NULL_HANDLE;
			}
		}
		
		VkShaderModule_t const&		GetData () const override		{ return BitCast<VkShaderModule_t>( _module ); }

		StringView					GetEntry () const override		{ return _entry; }

		size_t						GetHashOfData () const override	{ ASSERT(false);  return 0; }
	};
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VPipelineCache::VPipelineCache (const VDevice &dev) :
		_device{ dev },
		_pipelinesCache{ VK_NULL_HANDLE }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VPipelineCache::~VPipelineCache ()
	{
		CHECK( _pipelinesCache == VK_NULL_HANDLE );
	}
		
/*
=================================================
	Initialize
=================================================
*/
	bool VPipelineCache::Initialize ()
	{
		CHECK_ERR( not _pipelinesCache );

		VkPipelineCacheCreateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		info.pNext				= null;
		info.flags				= 0;
		info.initialDataSize	= 0;
		info.pInitialData		= null;

		VK_CHECK( _device.vkCreatePipelineCache( _device.GetVkDevice(), &info, null, OUT &_pipelinesCache ) );
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VPipelineCache::Deinitialize ()
	{
		_Destroy();

		_compilers.clear();
	}
	
/*
=================================================
	_GetBuiltinFormats
=================================================
*/
	FixedArray<EShaderLangFormat,16>  VPipelineCache::_GetBuiltinFormats () const
	{
		const EShaderLangFormat				ver		= _device.GetVkVersion();
		FixedArray<EShaderLangFormat,16>	result;

		// at first request external managed shader modules
		result.push_back( ver | EShaderLangFormat::ShaderModule );

		if ( ver > EShaderLangFormat::Vulkan_110 )
			result.push_back( EShaderLangFormat::VkShader_110 );

		if ( ver > EShaderLangFormat::Vulkan_100 )
			result.push_back( EShaderLangFormat::VkShader_100 );
		

		// at second request shader in binary format
		result.push_back( ver | EShaderLangFormat::SPIRV );

		if ( ver > EShaderLangFormat::Vulkan_110 )
			result.push_back( EShaderLangFormat::SPIRV_110 );

		if ( ver > EShaderLangFormat::Vulkan_100 )
			result.push_back( EShaderLangFormat::SPIRV_100 );

		return result;
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	VMeshPipelinePtr  VPipelineCache::CreatePipeline (const VFrameGraphWeak &fg, MeshPipelineDesc &&desc)
	{
		ASSERT(false);
		return null;
	}

/*
=================================================
	CreatePipeline
=================================================
*/
	VRayTracingPipelinePtr  VPipelineCache::CreatePipeline (const VFrameGraphWeak &fg, RayTracingPipelineDesc &&desc)
	{
		ASSERT(false);
		return null;
	}

/*
=================================================
	CreatePipeline
=================================================
*/
	VGraphicsPipelinePtr  VPipelineCache::CreatePipeline (const VFrameGraphWeak &fg, GraphicsPipelineDesc &&desc)
	{
		// check is shaders supported by default compiler
		const auto	formats		 = _GetBuiltinFormats();
		bool		is_supported = true;

		for (auto& sh : desc._shaders)
		{
			if ( sh.second.data.empty() )
				continue;

			bool	found = false;

			for (auto& fmt : formats)
			{
				if ( sh.second.data.find( fmt ) != sh.second.data.end() )
				{
					found = true;
					break;
				}
			}
			is_supported &= found;
		}

		if ( is_supported )
			return _CreatePipeline( fg, std::move(desc) );


		// try to compile
		for (auto& comp : _compilers)
		{
			if ( comp->IsSupported( desc, formats.front() ) )
			{
				CHECK_ERR( comp->Compile( INOUT desc, formats.front() ) );

				return _CreatePipeline( fg, std::move(desc) );
			}
		}

		RETURN_ERR( "unsuported shader format!" );
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	VComputePipelinePtr  VPipelineCache::CreatePipeline (const VFrameGraphWeak &fg, ComputePipelineDesc &&desc)
	{
		// check is shaders supported by default compiler
		const auto	formats		 = _GetBuiltinFormats();
		bool		is_supported = false;

		for (auto& fmt : formats)
		{
			if ( desc._shader.data.find( fmt ) !=  desc._shader.data.end() )
			{
				is_supported = true;
				break;
			}
		}

		if ( is_supported )
			return _CreatePipeline( fg, std::move(desc) );


		// try to compile
		for (auto& comp : _compilers)
		{
			if ( comp->IsSupported( desc, formats.front() ) )
			{
				CHECK_ERR( comp->Compile( INOUT desc, formats.front() ) );

				return _CreatePipeline( fg, std::move(desc) );
			}
		}

		RETURN_ERR( "unsuported shader format!" );
	}

/*
=================================================
	_CreatePipeline
=================================================
*/
	VGraphicsPipelinePtr  VPipelineCache::_CreatePipeline (const VFrameGraphWeak &fg, GraphicsPipelineDesc &&desc)
	{
		VGraphicsPipelinePtr	ppln	= MakeShared< VGraphicsPipeline >( fg );

		ppln->_layout					= _CreatePipelineLayout( std::move(desc._pipelineLayout) );
		ppln->_supportedTopology		= desc._supportedTopology;
		ppln->_fragmentOutput			= _CreateFramentOutput( desc._fragmentOutput );
		ppln->_vertexAttribs			= desc._vertexAttribs;
		ppln->_patchControlPoints		= desc._patchControlPoints;

		//_ValidateFragmentOutput( ppln->_renderState.color, ppln->_fragmentOutput );	// TODO

		for (auto& sh : desc._shaders)
		{
			ASSERT( not sh.second.data.empty() );

			VkShaderPtr		module;
			CHECK_ERR( _CompileShader( sh.second.data, OUT module ) );

			ppln->_shaders.push_back({ VEnumCast( sh.first ), module });
		}

		return ppln;
	}
	
/*
=================================================
	_CreatePipeline
=================================================
*/
	VComputePipelinePtr  VPipelineCache::_CreatePipeline (const VFrameGraphWeak &fg, ComputePipelineDesc &&desc)
	{
		VComputePipelinePtr		ppln	= MakeShared< VComputePipeline >( fg );
		
		ppln->_layout					= _CreatePipelineLayout( std::move(desc._pipelineLayout) );
		ppln->_defaultLocalGroupSize	= desc._defaultLocalGroupSize;
		ppln->_localSizeSpec			= desc._localSizeSpec;

		CHECK_ERR( _CompileShader( desc._shader.data, OUT ppln->_shader ) );
		return ppln;
	}
	
/*
=================================================
	_CreateFramentOutput
=================================================
*/
	VPipelineCache::FragmentOutputPtr  VPipelineCache::_CreateFramentOutput (ArrayView<GraphicsPipelineDesc::FragmentOutput> values)
	{
		VGraphicsPipeline::FragmentOutputInstance	inst{ values };

		return _fragmentOutputCache.insert( std::move(inst) ).first.operator-> ();
	}

/*
=================================================
	_ClearTemp
=================================================
*/
	void VPipelineCache::_ClearTemp ()
	{
		_tempStages.clear();
		_tempSpecialization.clear();
		_tempDynamicStates.clear();
		_tempViewports.clear();
		_tempScissors.clear();
		_tempVertexAttribs.clear();
		_tempVertexBinding.clear();
		_tempAttachments.clear();
		_tempSpecEntries.clear();
		_tempSpecData.clear();
	}

/*
=================================================
	CreatePipelineInstance
=================================================
*/
	VkPipeline  VPipelineCache::CreatePipelineInstance (const VGraphicsPipelinePtr	&ppln,
														const VRenderPassPtr		&renderPass,
														const uint					 subpassIndex,
														const RenderState			&renderState,
														const VertexInputState		&vertexInput,
														EPipelineDynamicState		 dynamicState,
														const VkPipelineCreateFlags	 pipelineFlags,
														uint						 viewportCount)
	{
		CHECK_ERR( ppln and renderPass and viewportCount > 0 );

		// not supported yet
		ASSERT( EnumEq( dynamicState, EPipelineDynamicState::Viewport ) );
		ASSERT( EnumEq( dynamicState, EPipelineDynamicState::Scissor ) );

		VGraphicsPipeline::PipelineInstance		inst;
		inst.dynamicState	= dynamicState;
		inst.renderPass		= renderPass;
		inst.subpassIndex	= subpassIndex;
		inst.renderState	= renderState;
		inst.vertexInput	= vertexInput;
		inst.flags			= pipelineFlags;
		inst.viewportCount	= viewportCount;

		inst.vertexInput.ApplyAttribs( ppln->GetVertexAttribs() );
		_ValidateRenderState( INOUT inst.renderState, INOUT inst.dynamicState );

		inst._hash			= HashOf( inst.renderPass )		+ HashOf( inst.subpassIndex )	+
							  HashOf( inst.renderState )	+ HashOf( inst.vertexInput )	+
							  HashOf( inst.dynamicState )	+ HashOf( inst.viewportCount )	+
							  HashOf( inst.flags );


		// find existing instance
		{
			auto	iter = ppln->_instances.find( inst );

			if ( iter != ppln->_instances.end() )
				return iter->second;
		}


		// create new instance
		_ClearTemp();

		VkGraphicsPipelineCreateInfo			pipeline_info		= {};
		VkPipelineInputAssemblyStateCreateInfo	input_assembly_info	= {};
		VkPipelineColorBlendStateCreateInfo		blend_info			= {};
		VkPipelineDepthStencilStateCreateInfo	depth_stencil_info	= {};
		VkPipelineMultisampleStateCreateInfo	multisample_info	= {};
		VkPipelineRasterizationStateCreateInfo	rasterization_info	= {};
		VkPipelineTessellationStateCreateInfo	tessellation_info	= {};
		VkPipelineDynamicStateCreateInfo		dynamic_state_info	= {};
		VkPipelineVertexInputStateCreateInfo	vertex_input_info	= {};
		VkPipelineViewportStateCreateInfo		viewport_info		= {};

		_SetShaderStages( OUT _tempStages, INOUT _tempSpecialization, INOUT _tempSpecEntries, ppln->_shaders );
		_SetDynamicState( OUT dynamic_state_info, OUT _tempDynamicStates, inst.dynamicState );
		_SetColorBlendState( OUT blend_info, OUT _tempAttachments, inst.renderState.color, inst.renderPass, inst.subpassIndex );
		_SetMultisampleState( OUT multisample_info, inst.renderState.multisample );
		_SetTessellationState( OUT tessellation_info, ppln->_patchControlPoints );
		_SetDepthStencilState( OUT depth_stencil_info, inst.renderState.depth, inst.renderState.stencil );
		_SetRasterizationState( OUT rasterization_info, inst.renderState.rasterization );
		_SetupPipelineInputAssemblyState( OUT input_assembly_info, inst.renderState.inputAssembly );
		_SetVertexInputState( OUT vertex_input_info, OUT _tempVertexAttribs, OUT _tempVertexBinding, inst.vertexInput );
		_SetViewportState( OUT viewport_info, OUT _tempViewports, OUT _tempScissors,
						   uint2(1024, 1024), inst.viewportCount, inst.dynamicState );

		pipeline_info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.pNext					= null;
		pipeline_info.flags					= pipelineFlags;
		pipeline_info.pInputAssemblyState	= &input_assembly_info;
		pipeline_info.pRasterizationState	= &rasterization_info;
		pipeline_info.pColorBlendState		= &blend_info;
		pipeline_info.pDepthStencilState	= &depth_stencil_info;
		pipeline_info.pMultisampleState		= &multisample_info;
		pipeline_info.pTessellationState	= (ppln->_patchControlPoints > 0 ? &tessellation_info : null);
		pipeline_info.pVertexInputState		= &vertex_input_info;
		pipeline_info.pDynamicState			= (_tempDynamicStates.empty() ? null : &dynamic_state_info);
		pipeline_info.basePipelineIndex		= -1;
		pipeline_info.basePipelineHandle	= VK_NULL_HANDLE;
		pipeline_info.layout				= ppln->_layout->GetLayoutID();
		pipeline_info.stageCount			= uint(_tempStages.size());
		pipeline_info.pStages				= _tempStages.data();
		pipeline_info.renderPass			= renderPass->GetRenderPassID();
		pipeline_info.subpass				= subpassIndex;
		
		if ( not rasterization_info.rasterizerDiscardEnable )
		{
			pipeline_info.pViewportState		= &viewport_info;
		}else{
			pipeline_info.pViewportState		= null;
			pipeline_info.pMultisampleState		= null;
			pipeline_info.pDepthStencilState	= null;
			pipeline_info.pColorBlendState		= null;
		}

		VkPipeline	ppln_id = {};
		VK_CHECK( _device.vkCreateGraphicsPipelines( _device.GetVkDevice(), _pipelinesCache, 1, &pipeline_info, null, OUT &ppln_id ) );

		ppln->_instances.insert({ inst, ppln_id });
		return ppln_id;
	}
	
/*
=================================================
	CreatePipelineInstance
=================================================
*/
	VkPipeline  VPipelineCache::CreatePipelineInstance (const VComputePipelinePtr	&ppln,
														const Optional<uint3>		&localGroupSize,
														VkPipelineCreateFlags		 pipelineFlags)
	{
		VComputePipeline::PipelineInstance		inst;
		inst.flags			= pipelineFlags;
		inst.localGroupSize	= localGroupSize.value_or( ppln->GetDefaultLocalSize() );
		inst.localGroupSize = { ppln->_localSizeSpec.x != ComputePipelineDesc::UNDEFINED_SPECIALIZATION ? inst.localGroupSize.x : ppln->GetDefaultLocalSize().x,
								ppln->_localSizeSpec.y != ComputePipelineDesc::UNDEFINED_SPECIALIZATION ? inst.localGroupSize.y : ppln->GetDefaultLocalSize().y,
								ppln->_localSizeSpec.z != ComputePipelineDesc::UNDEFINED_SPECIALIZATION ? inst.localGroupSize.z : ppln->GetDefaultLocalSize().z };
		inst._hash			= (HashOf( inst.localGroupSize ) + HashOf( inst.flags ));


		// find existing instance
		{
			auto	iter = ppln->_instances.find( inst );

			if ( iter != ppln->_instances.end() )
				return iter->second;
		}


		// create new instance
		_ClearTemp();
		CHECK_ERR( ppln->_shader );

		VkSpecializationInfo			spec = {};
		VkComputePipelineCreateInfo		pipeline_info = {};

		pipeline_info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_info.layout		= ppln->_layout->GetLayoutID();
		pipeline_info.flags			= inst.flags;
		pipeline_info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipeline_info.stage.module	= BitCast<VkShaderModule>( ppln->_shader->GetData() );
		pipeline_info.stage.pName	= ppln->_shader->GetEntry().data();
		pipeline_info.stage.flags	= 0;
		pipeline_info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;

		_AddLocalGroupSizeSpecialization( OUT _tempSpecEntries, OUT _tempSpecData, ppln->_localSizeSpec, inst.localGroupSize );

		if ( not _tempSpecEntries.empty() )
		{
			spec.mapEntryCount	= uint(_tempSpecEntries.size());
			spec.pMapEntries	= _tempSpecEntries.data();
			spec.dataSize		= size_t(ArraySizeOf( _tempSpecData ));
			spec.pData			= _tempSpecData.data();

			pipeline_info.stage.pSpecializationInfo	= &spec;
		}

		VkPipeline	ppln_id = {};
		VK_CHECK( _device.vkCreateComputePipelines( _device.GetVkDevice(), _pipelinesCache, 1, &pipeline_info, null, OUT &ppln_id ) );
		
		ppln->_instances.insert({ inst, ppln_id });
		return ppln_id;
	}
	
/*
=================================================
	CreatePipelineInstance
=================================================
*/
	VkPipeline  VPipelineCache::CreatePipelineInstance (const VMeshPipelinePtr		&ppln,
														VkPipelineCreateFlags		 pipelineFlags)
	{
		ASSERT(false);
		return null;
	}
	
/*
=================================================
	CreatePipelineInstance
=================================================
*/
	VkPipeline  VPipelineCache::CreatePipelineInstance (const VRayTracingPipelinePtr	&ppln,
														VkPipelineCreateFlags			 pipelineFlags)
	{
		ASSERT(false);
		return null;
	}

/*
=================================================
	AddCompiler
=================================================
*/
	void VPipelineCache::AddCompiler (const IPipelineCompilerPtr &comp)
	{
		_compilers.insert( comp );
	}
	
/*
=================================================
	InitPipelineResources
=================================================
*/
	bool VPipelineCache::InitPipelineResources (const VPipelineResourcesPtr &pr)
	{
		if ( pr->_IsCreated() )
			return true;

		IDescriptorPoolPtr	pool;
		VkDescriptorSet		set;

		CHECK_ERR( pr->_layout->_CreateDescriptorSet( _device, OUT pool, OUT set ) );

		pr->_Create( pool, set );
		return true;
	}
	
/*
=================================================
	UpdatePipelineResources
=================================================
*/
	bool VPipelineCache::UpdatePipelineResources (const VPipelineResourcesPtr &pr, const VPipelineResources::UpdateDescriptors &upd)
	{
		ASSERT( pr and pr->_IsCreated() );
		ASSERT( not upd.descriptors.empty() );

		_device.vkUpdateDescriptorSets( _device.GetVkDevice(),
										uint(upd.descriptors.size()),
										upd.descriptors.data(),
										0, null );
		return true;
	}

/*
=================================================
	_Destroy
=================================================
*/
	void VPipelineCache::_Destroy ()
	{
		for (auto& ds : _descriptorSetCache)
		{
			ds.ptr->_Destroy( _device );
		}
		_descriptorSetCache.clear();


		for (auto& pl : _pipelineLayoutCache)
		{
			pl.ptr->_Destroy( _device );
		}
		_pipelineLayoutCache.clear();
		

		for (auto& sh : _shaderCache)
		{
			ASSERT( sh.use_count() == 1 );

			Cast<VShaderModule>( sh )->Destroy( _device );
		}
		_shaderCache.clear();


		if ( _pipelinesCache )
		{
			_device.vkDestroyPipelineCache( _device.GetVkDevice(), _pipelinesCache, null );
			_pipelinesCache = VK_NULL_HANDLE;
		}
	}

/*
=================================================
	_CreateDescriptorSetLayout
=================================================
*/
	VDescriptorSetLayoutPtr  VPipelineCache::_CreateDescriptorSetLayout (DescriptorSet_t &&ds)
	{
		VDescriptorSetLayoutPtr		key = MakeShared<VDescriptorSetLayout>( std::move(ds.uniforms) );

		auto	iter = _descriptorSetCache.find( DescriptorSetItem{key} );

		if ( iter != _descriptorSetCache.end() )
			return iter->ptr;

		return _descriptorSetCache.insert(
					DescriptorSetItem{MakeShared<VDescriptorSetLayout>( _device, std::move(*key) )}).first->ptr;
	}
	
/*
=================================================
	_CreatePipelineLayout
=================================================
*/
	VPipelineLayoutPtr  VPipelineCache::_CreatePipelineLayout (PipelineLayout_t &&ppln)
	{
		FixedArray< VDescriptorSetLayoutPtr, FG_MaxDescriptorSets >		ds_layouts;

		for (auto& ds : ppln.descriptorSets) {
			ds_layouts.push_back( _CreateDescriptorSetLayout( std::move(ds) ) );
		}

		VPipelineLayoutPtr	key = MakeShared<VPipelineLayout>( ppln, ds_layouts );
		
		auto	iter = _pipelineLayoutCache.find( PipelineLayoutItem{key} );

		if ( iter != _pipelineLayoutCache.end() )
			return iter->ptr;

		return _pipelineLayoutCache.insert(
					PipelineLayoutItem{MakeShared<VPipelineLayout>( _device, std::move(*key) )}).first->ptr;
	}
	
/*
=================================================
	_CompileShader
=================================================
*/
	bool VPipelineCache::_CompileShader (const PipelineDescription::ShaderDataMap_t &shaders,
										 OUT VkShaderPtr &module)
	{
		CHECK_ERR( not shaders.empty() );

		const EShaderLangFormat		spv_fmt[] = {
			_device.GetVkVersion() | EShaderLangFormat::ShaderModule,
			_device.GetVkVersion() | EShaderLangFormat::SPIRV,
			EShaderLangFormat::VkShader_110,
			EShaderLangFormat::VkShader_100,
			EShaderLangFormat::SPIRV_110,
			EShaderLangFormat::SPIRV_100
		};

		for (auto& lang : spv_fmt)
		{
			auto	iter = shaders.find( lang );

			if ( iter == shaders.end() )
				continue;

			// get already created shader module
			if ( EnumEq( iter->first, EShaderLangFormat::ShaderModule ) )
			{
				const auto*	shader_module = std::get_if<VkShaderPtr>( &iter->second );
				CHECK_ERR( shader_module and *shader_module, "invalid shader data format!" );

				module = *shader_module;
				return true;
			}

			// get SPIRV shader binary
			if ( EnumEq( iter->first, EShaderLangFormat::SPIRV ) )
			{
				const auto*	shader_data = std::get_if< PipelineDescription::SharedShaderPtr<Array<uint>> >( &iter->second );
				CHECK_ERR( shader_data and *shader_data, "invalid shader data format!" );
				
				VkShaderModuleCreateInfo	shader_info = {};
				shader_info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				shader_info.codeSize	= size_t(ArraySizeOf( (*shader_data)->GetData() ));
				shader_info.pCode		= (*shader_data)->GetData().data();

				VkShaderModule		shader_id;
				VK_CHECK( _device.vkCreateShaderModule( _device.GetVkDevice(), &shader_info, null, OUT &shader_id ) );

				module = MakeShared<VShaderModule>( shader_id, (*shader_data)->GetEntry() );

				_shaderCache.push_back( module );
				return true;
			}
		}

		RETURN_ERR( "SPIRV shader binary not found" );
	}
	
/*
=================================================
	_SetShaderStages
=================================================
*/
	void VPipelineCache::_SetShaderStages (OUT ShaderStages_t &stages,
										   INOUT Specializations_t &/*specialization*/,
										   INOUT SpecializationEntries_t &/*specEntries*/,
										   ArrayView< ShaderModule_t > shaders) const
	{
		for (auto& sh : shaders)
		{
			ASSERT( sh.module );

			VkPipelineShaderStageCreateInfo	info = {};
			info.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.pNext	= null;
			info.flags	= 0;
			info.module	= BitCast<VkShaderModule>( sh.module->GetData() );
			info.pName	= sh.module->GetEntry().data();
			info.stage	= sh.stage;
			info.pSpecializationInfo = null;	// TODO

			stages.push_back( info );
		}
	}

/*
=================================================
	_SetDynamicState
=================================================
*/
	void VPipelineCache::_SetDynamicState (OUT VkPipelineDynamicStateCreateInfo &outState,
										   OUT DynamicStates_t &states,
										   EPipelineDynamicState inState) const
	{
		outState.sType	= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		outState.pNext	= null;
		outState.flags	= 0;

		for (EPipelineDynamicState t = EPipelineDynamicState(1 << 0);
			 t < EPipelineDynamicState::_Last;
			 t = EPipelineDynamicState(uint(t) << 1)) 
		{
			if ( not EnumEq( inState, t ) )
				continue;

			states.push_back( VEnumCast( t ));
		}

		outState.dynamicStateCount	= uint(states.size());
		outState.pDynamicStates		= states.data();
	}
	
/*
=================================================
	_SetMultisampleState
=================================================
*/
	void VPipelineCache::_SetMultisampleState (OUT VkPipelineMultisampleStateCreateInfo &outState,
											   const RenderState::MultisampleState &inState) const
	{
		outState.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		outState.pNext					= null;
		outState.flags					= 0;
		outState.rasterizationSamples	= VEnumCast( inState.samples );
		outState.sampleShadingEnable	= inState.sampleShading;
		outState.minSampleShading		= inState.minSampleShading;
		outState.pSampleMask			= inState.samples.IsEnabled() ? inState.sampleMask.data() : null;
		outState.alphaToCoverageEnable	= inState.alphaToCoverage;
		outState.alphaToOneEnable		= inState.alphaToOne;
	}
	
/*
=================================================
	_SetTessellationState
=================================================
*/
	void VPipelineCache::_SetTessellationState (OUT VkPipelineTessellationStateCreateInfo &outState,
												uint patchSize) const
	{
		outState.sType				= VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		outState.pNext				= null;
		outState.flags				= 0;
		outState.patchControlPoints	= patchSize;
	}
	
/*
=================================================
	SetStencilOpState
=================================================
*/
	static void SetStencilOpState (OUT VkStencilOpState &outState, const RenderState::StencilFaceState &inState)
	{
		outState.failOp			= VEnumCast( inState.failOp );
		outState.passOp			= VEnumCast( inState.passOp );
		outState.depthFailOp	= VEnumCast( inState.depthFailOp );
		outState.compareOp		= VEnumCast( inState.compareOp );
		outState.compareMask	= inState.compareMask;
		outState.writeMask		= inState.writeMask;
		outState.reference		= inState.reference;
	}

/*
=================================================
	_SetDepthStencilState
=================================================
*/
	void VPipelineCache::_SetDepthStencilState (OUT VkPipelineDepthStencilStateCreateInfo &outState,
												const RenderState::DepthBufferState &depth,
												const RenderState::StencilBufferState &stencil) const
	{
		outState.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		outState.pNext					= null;
		outState.flags					= 0;

		// depth
		outState.depthTestEnable		= depth.test;
		outState.depthWriteEnable		= depth.write;
		outState.depthCompareOp			= VEnumCast( depth.compareOp );
		outState.depthBoundsTestEnable	= depth.boundsEnabled;
		outState.minDepthBounds			= depth.bounds.x;
		outState.maxDepthBounds			= depth.bounds.y;
		
		// stencil
		outState.stencilTestEnable		= stencil.enabled;
		SetStencilOpState( OUT outState.front, stencil.front );
		SetStencilOpState( OUT outState.back,  stencil.back  );
	}
	
/*
=================================================
	_SetRasterizationState
=================================================
*/
	void VPipelineCache::_SetRasterizationState (OUT VkPipelineRasterizationStateCreateInfo &outState,
												 const RenderState::RasterizationState &inState) const
	{
		outState.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		outState.pNext						= null;
		outState.flags						= 0;
		outState.polygonMode				= VEnumCast( inState.polygonMode );
		outState.lineWidth					= inState.lineWidth;
		outState.depthBiasConstantFactor	= inState.depthBiasConstFactor;
		outState.depthBiasClamp				= inState.depthBiasClamp;
		outState.depthBiasSlopeFactor		= inState.depthBiasSlopeFactor;
		outState.depthBiasEnable			= inState.depthBias;
		outState.depthClampEnable			= inState.depthClamp;
		outState.rasterizerDiscardEnable	= inState.rasterizerDiscard;
		outState.frontFace					= inState.frontFaceCCW ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
		outState.cullMode					= VEnumCast( inState.cullMode );
	}

/*
=================================================
	_SetupPipelineInputAssemblyState
=================================================
*/
	void VPipelineCache::_SetupPipelineInputAssemblyState (OUT VkPipelineInputAssemblyStateCreateInfo &outState,
														   const RenderState::InputAssemblyState &inState) const
	{
		outState.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		outState.pNext					= null;
		outState.flags					= 0;
		outState.topology				= VEnumCast( inState.topology );
		outState.primitiveRestartEnable	= inState.primitiveRestart;
	}

/*
=================================================
	_SetVertexInputState
=================================================
*/
	void VPipelineCache::_SetVertexInputState (OUT VkPipelineVertexInputStateCreateInfo &outState,
											   OUT VertexInputAttributes_t &vertexAttribs,
											   OUT VertexInputBindings_t &vertexBinding,
											   const VertexInputState &inState) const
	{
		outState.sType	= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		outState.pNext	= null;
		outState.flags	= 0;

		for (const auto& src : inState.Vertices())
		{
			VkVertexInputAttributeDescription	dst = {};

			ASSERT( src.second.index != ~0U );

			dst.binding		= src.second.bindingIndex;
			dst.format		= VEnumCast( src.second.type );
			dst.location	= src.second.index;
			dst.offset		= uint(src.second.offset);

			vertexAttribs.push_back( std::move(dst) );
		}

		for (const auto& src : inState.BufferBindings())
		{
			VkVertexInputBindingDescription	dst = {};

			dst.binding		= src.second.index;
			dst.inputRate	= VEnumCast( src.second.rate );
			dst.stride		= uint(src.second.stride);

			vertexBinding.push_back( dst );
		}

		// if pipeline has attributes then buffer binding must be defined too
		CHECK( vertexAttribs.empty() == vertexBinding.empty() );

		outState.pVertexAttributeDescriptions		= vertexAttribs.data();
		outState.vertexAttributeDescriptionCount	= uint(vertexAttribs.size());

		outState.pVertexBindingDescriptions			= vertexBinding.data();
		outState.vertexBindingDescriptionCount		= uint(vertexBinding.size());
	}

/*
=================================================
	_SetVertexInputState
=================================================
*/
	void VPipelineCache::_SetViewportState (OUT VkPipelineViewportStateCreateInfo &outState,
											OUT Viewports_t &tmpViewports,
											OUT Scissors_t &tmpScissors,
											const uint2 &viewportSize,
											const uint viewportCount,
											EPipelineDynamicState dynamicStates) const
	{
		tmpViewports.resize( viewportCount );
		tmpScissors.resize( viewportCount );

		for (uint i = 0; i < viewportCount; ++i)
		{
			tmpViewports[i] = VkViewport{ 0, 0, float(viewportSize.x), float(viewportSize.y), 0.0f, 1.0f };
			tmpScissors[i]	= VkRect2D{ VkOffset2D{ 0, 0 }, VkExtent2D{ viewportSize.x, viewportSize.y } };
		}

		outState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		outState.pNext			= null;
		outState.flags			= 0;
		outState.pViewports		= EnumEq( dynamicStates, EPipelineDynamicState::Viewport ) ? null : tmpViewports.data();
		outState.viewportCount	= uint(tmpViewports.size());
		outState.pScissors		= EnumEq( dynamicStates, EPipelineDynamicState::Scissor ) ? null : tmpScissors.data();
		outState.scissorCount	= uint(tmpScissors.size());
	}
	
/*
=================================================
	SetColorBlendAttachmentState
=================================================
*/
	static void SetColorBlendAttachmentState (OUT VkPipelineColorBlendAttachmentState &outState,
											  const RenderState::ColorBuffer &inState,
											  const bool logicOpEnabled)
	{
		outState.blendEnable			= logicOpEnabled ? false : inState.blend;
		outState.srcColorBlendFactor	= VEnumCast( inState.srcBlendFactor.color );
		outState.srcAlphaBlendFactor	= VEnumCast( inState.srcBlendFactor.alpha );
		outState.dstColorBlendFactor	= VEnumCast( inState.dstBlendFactor.color );
		outState.dstAlphaBlendFactor	= VEnumCast( inState.dstBlendFactor.alpha );
		outState.colorBlendOp			= VEnumCast( inState.blendOp.color );
		outState.alphaBlendOp			= VEnumCast( inState.blendOp.alpha );
		outState.colorWriteMask			= (inState.colorMask.x ? VK_COLOR_COMPONENT_R_BIT : 0) |
										  (inState.colorMask.y ? VK_COLOR_COMPONENT_G_BIT : 0) |
										  (inState.colorMask.z ? VK_COLOR_COMPONENT_B_BIT : 0) |
										  (inState.colorMask.w ? VK_COLOR_COMPONENT_A_BIT : 0);
	}

/*
=================================================
	_SetColorBlendState
=================================================
*/
	void VPipelineCache::_SetColorBlendState (OUT VkPipelineColorBlendStateCreateInfo &outState,
											  OUT ColorAttachments_t &attachments,
											  const RenderState::ColorBuffersState &inState,
											  const VRenderPassPtr &renderPass,
											  const uint subpassIndex) const
	{
		ASSERT( subpassIndex < renderPass->GetCreateInfo().subpassCount );

		const bool					logic_op_enabled	= ( inState.logicOp != ELogicOp::None );
		const VkSubpassDescription&	subpass				= renderPass->GetCreateInfo().pSubpasses[ subpassIndex ];

		for (size_t i = 0; i < subpass.colorAttachmentCount; ++i)
		{
			VkPipelineColorBlendAttachmentState		color_state = {};
			color_state.colorWriteMask	= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
										  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			attachments.push_back( color_state );
		}

		for (const auto& cb : inState.buffers)
		{
			uint	index;
			CHECK( renderPass->GetColorAttachmentIndex( cb.first, OUT index ) );

			SetColorBlendAttachmentState( attachments[index], cb.second, logic_op_enabled );
		}

		outState.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		outState.pNext				= null;
		outState.flags				= 0;
		outState.attachmentCount	= uint(attachments.size());
		outState.pAttachments		= attachments.empty() ? null : attachments.data();
		outState.logicOpEnable		= logic_op_enabled;
		outState.logicOp			= logic_op_enabled ? VEnumCast( inState.logicOp ) : VK_LOGIC_OP_CLEAR;

		outState.blendConstants[0] = inState.blendColor.r;
		outState.blendConstants[1] = inState.blendColor.g;
		outState.blendConstants[2] = inState.blendColor.b;
		outState.blendConstants[3] = inState.blendColor.a;
	}
	
/*
=================================================
	_ValidateRenderState
=================================================
*/
	void VPipelineCache::_ValidateRenderState (INOUT RenderState &renderState, INOUT EPipelineDynamicState &dynamicStates) const
	{
		if ( renderState.rasterization.rasterizerDiscard )
		{
			renderState.color	= Default;
			renderState.depth	= Default;
			renderState.stencil	= Default;
			dynamicStates		= dynamicStates & ~(EPipelineDynamicState::RasterizerMask);
		}

		// reset to default state if dynamic state enabled.
		// it is needed for correct hash calculation.
		for (EPipelineDynamicState t = EPipelineDynamicState(1 << 0);
			 t < EPipelineDynamicState::_Last;
			 t = EPipelineDynamicState(uint(t) << 1)) 
		{
			if ( not EnumEq( dynamicStates, t ) )
				continue;

			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case EPipelineDynamicState::Viewport :
					break;
				case EPipelineDynamicState::Scissor :
					break;

				case EPipelineDynamicState::LineWidth :
					renderState.rasterization.lineWidth = 1.0f;
					break;

				case EPipelineDynamicState::DepthBias :
					ASSERT( renderState.rasterization.depthBias );
					renderState.rasterization.depthBiasConstFactor	= 0.0f;
					renderState.rasterization.depthBiasClamp		= 0.0f;
					renderState.rasterization.depthBiasSlopeFactor	= 0.0f;
					break;

				case EPipelineDynamicState::BlendConstants :
					renderState.color.blendColor = RGBA32f{ 1.0f };
					break;

				case EPipelineDynamicState::DepthBounds :
					ASSERT( renderState.depth.boundsEnabled );
					renderState.depth.bounds = { 0.0f, 1.0f };
					break;

				case EPipelineDynamicState::StencilCompareMask :
					ASSERT( renderState.stencil.enabled ); 
					renderState.stencil.front.compareMask = ~0u;
					renderState.stencil.back.compareMask  = ~0u;
					break;

				case EPipelineDynamicState::StencilWriteMask :
					ASSERT( renderState.stencil.enabled ); 
					renderState.stencil.front.writeMask = ~0u;
					renderState.stencil.back.writeMask  = ~0u;
					break;

				case EPipelineDynamicState::StencilReference :
					ASSERT( renderState.stencil.enabled ); 
					renderState.stencil.front.reference = 0;
					renderState.stencil.back.reference  = 0;
					break;

				case EPipelineDynamicState::None :
				case EPipelineDynamicState::_Last :
				case EPipelineDynamicState::All :
					break;	// to shutup warnings

				default :
					ASSERT(false);	// not supported
					break;
			}
			DISABLE_ENUM_CHECKS();
		}

		// validate color buffer states
		{
			const bool	dual_src_blend	= _device.GetDeviceFeatures().dualSrcBlend;

			const auto	IsDualSrcBlendFactor = [] (EBlendFactor value) {
				switch ( value ) {
					case EBlendFactor::Src1Color :
					case EBlendFactor::OneMinusSrc1Color :
					case EBlendFactor::Src1Alpha :
					case EBlendFactor::OneMinusSrc1Alpha :
						return true;
				}
				return false;
			};

			for (auto& cb : renderState.color.buffers)
			{
				if ( not cb.second.blend )
				{	
					cb.second.srcBlendFactor = { EBlendFactor::One,		EBlendFactor::One };
					cb.second.dstBlendFactor = { EBlendFactor::Zero,	EBlendFactor::Zero };
					cb.second.blendOp		 = { EBlendOp::Add,			EBlendOp::Add };
				}
				else
				{
					if ( not dual_src_blend )
					{
						ASSERT( not IsDualSrcBlendFactor( cb.second.srcBlendFactor.color ) );
						ASSERT( not IsDualSrcBlendFactor( cb.second.srcBlendFactor.alpha ) );
						ASSERT( not IsDualSrcBlendFactor( cb.second.dstBlendFactor.color ) );
						ASSERT( not IsDualSrcBlendFactor( cb.second.dstBlendFactor.alpha ) );
					}
				}
			}
		}

		// validate depth states
		{
			if ( not renderState.depth.test )
				renderState.depth.compareOp = ECompareOp::LEqual;

			//if ( not renderState.depth.write )

			if ( not renderState.depth.boundsEnabled )
				renderState.depth.bounds = { 0.0f, 1.0f };
		}

		// validate stencil states
		{
			if ( not renderState.stencil.enabled )
			{
				renderState.stencil = Default;
			}
		}
	}
	
/*
=================================================
	_AddLocalGroupSizeSpecialization
=================================================
*/
	void VPipelineCache::_AddLocalGroupSizeSpecialization (INOUT SpecializationEntries_t &outEntries,
														   INOUT SpecializationData_t &outEntryData,
														   const uint3 &localSizeSpec,
														   const uint3 &localGroupSize) const
	{
		const bool3	has_spec = (localSizeSpec != uint3(ComputePipelineDesc::UNDEFINED_SPECIALIZATION));
		//ASSERT(Any( has_spec ));

		if ( has_spec.x )
		{
			VkSpecializationMapEntry	entry;
			entry.constantID	= localSizeSpec.x;
			entry.offset		= uint(ArraySizeOf(outEntryData));
			entry.size			= sizeof(uint);
			outEntries.push_back( entry );
			outEntryData.push_back( BitCast<uint>( localGroupSize.x ));
		}
		
		if ( has_spec.y )
		{
			VkSpecializationMapEntry	entry;
			entry.constantID	= localSizeSpec.y;
			entry.offset		= uint(ArraySizeOf(outEntryData));
			entry.size			= sizeof(uint);
			outEntries.push_back( entry );
			outEntryData.push_back( BitCast<uint>( localGroupSize.y ));
		}
		
		if ( has_spec.z )
		{
			VkSpecializationMapEntry	entry;
			entry.constantID	= localSizeSpec.z;
			entry.offset		= uint(ArraySizeOf(outEntryData));
			entry.size			= sizeof(uint);
			outEntries.push_back( entry );
			outEntryData.push_back( BitCast<uint>( localGroupSize.z ));
		}
	}


}	// FG
