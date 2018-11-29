// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Public/IPipelineCompiler.h"
#include "VPipelineCache.h"
#include "VDevice.h"
#include "VEnumCast.h"
#include "VRenderPass.h"
#include "VResourceManagerThread.h"

namespace FG
{

	//
	// Vulkan Shader Module
	//
	class VShaderModule final : public PipelineDescription::IShaderData< ShaderModuleVk_t >
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
		
		ShaderModuleVk_t const&		GetData () const override		{ return BitCast<ShaderModuleVk_t>( _module ); }

		StringView					GetEntry () const override		{ return _entry; }

		size_t						GetHashOfData () const override	{ ASSERT(false);  return 0; }
	};
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VPipelineCache::VPipelineCache () :
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
		CHECK( _shaderCache.empty() );
		CHECK( _descriptorPools.empty() );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VPipelineCache::Initialize (const VDevice &dev)
	{
		CHECK_ERR( _CreatePipelineCache( dev ));
		CHECK_ERR( _CreateDescriptorPool( dev ));
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VPipelineCache::Deinitialize (const VDevice &dev)
	{
		for (auto& sh : _shaderCache)
		{
			ASSERT( sh.use_count() == 1 );

			Cast<VShaderModule>( sh )->Destroy( dev );
		}
		_shaderCache.clear();

		_compilers.clear();

		if ( _pipelinesCache )
		{
			dev.vkDestroyPipelineCache( dev.GetVkDevice(), _pipelinesCache, null );
			_pipelinesCache = VK_NULL_HANDLE;
		}

		for (auto& pool : _descriptorPools) {
			dev.vkDestroyDescriptorPool( dev.GetVkDevice(), pool, null );
		}
		_descriptorPools.clear();
	}

/*
=================================================
	OnBegin
=================================================
*/
	void VPipelineCache::OnBegin (LinearAllocator<> &alloc)
	{
		_graphicsPipelines.Create( alloc );
		_computePipelines.Create( alloc );
		_meshPipelines.Create( alloc );
		_rayTracingPipelines.Create( alloc );
	}
	
/*
=================================================
	MergeCache
=================================================
*/
	bool VPipelineCache::MergeCache (VPipelineCache &)
	{
		// TODO
		return false;
	}
	
/*
=================================================
	_MergePipelines
=================================================
*/
	template <typename T>
	void VPipelineCache::_MergePipelines (INOUT Storage<PipelineInstanceCacheTempl<T>> &map, OUT AppendableVkResources_t readyToDelete) const
	{
		for (auto& cached : *map)
		{
			auto*	ppln	 = const_cast<T *>( cached.first.first );
			auto	inserted = ppln->_instances.insert({ cached.first.second, cached.second });

			// delete if not inserted
			if ( not inserted.second )
				readyToDelete.emplace_back( VK_OBJECT_TYPE_PIPELINE, uint64_t(cached.second) );
		}
		map.Destroy();
	}
	
/*
=================================================
	MergePipelines
=================================================
*/
	void VPipelineCache::MergePipelines (OUT AppendableVkResources_t readyToDelete)
	{
		_MergePipelines( INOUT _graphicsPipelines, OUT readyToDelete );
		_MergePipelines( INOUT _computePipelines, OUT readyToDelete );
		_MergePipelines( INOUT _meshPipelines, OUT readyToDelete );
		_MergePipelines( INOUT _rayTracingPipelines, OUT readyToDelete );
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
	_GetBuiltinFormats
=================================================
*/
	FixedArray<EShaderLangFormat,16>  VPipelineCache::_GetBuiltinFormats (const VDevice &dev) const
	{
		const EShaderLangFormat				ver		= dev.GetVkVersion();
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
	CreateFramentOutput
=================================================
*/
	VPipelineCache::FragmentOutputPtr  VPipelineCache::CreateFramentOutput (ArrayView<GraphicsPipelineDesc::FragmentOutput> values)
	{
		VGraphicsPipeline::FragmentOutputInstance	inst{ values };

		return _fragmentOutputCache.insert( std::move(inst) ).first.operator-> ();
	}

/*
=================================================
	_CompileShaders
=================================================
*/
	template <typename DescT>
	bool  VPipelineCache::_CompileShaders (INOUT DescT &desc, const VDevice &dev)
	{
		// check is shaders supported by default compiler
		const auto	formats		 = _GetBuiltinFormats( dev );
		bool		is_supported = true;

		for (auto& sh : desc._shaders)
		{
			if ( sh.second.data.empty() )
				continue;

			bool	found = false;

			for (auto& fmt : formats)
			{
				auto	iter = sh.second.data.find( fmt );

				if ( iter == sh.second.data.end() )
					continue;

				if ( EnumEq( fmt, EShaderLangFormat::ShaderModule ) )
				{
					auto	shader_data = iter->second;
				
					sh.second.data.clear();
					sh.second.data.insert({ fmt, shader_data });

					found = true;
					break;
				}
			
				if ( EnumEq( fmt, EShaderLangFormat::SPIRV ) )
				{
					VkShaderPtr		mod;
					CHECK_ERR( _CompileSPIRVShader( dev, iter->second, OUT mod ));

					sh.second.data.clear();
					sh.second.data.insert({ fmt, mod });

					found = true;
					break;
				}
			}
			is_supported &= found;
		}

		if ( is_supported )
			return true;


		// try to compile
		for (auto& comp : _compilers)
		{
			if ( comp->IsSupported( desc, formats.front() ) )
			{
				CHECK_ERR( comp->Compile( INOUT desc, formats.front() ));
				return true;
			}
		}

		RETURN_ERR( "unsuported shader format!" );
	}
	
/*
=================================================
	CompileShaders
=================================================
*/
	bool VPipelineCache::CompileShaders (INOUT GraphicsPipelineDesc &desc, const VDevice &dev)
	{
		return _CompileShaders( INOUT desc, dev );
	}
	
	bool VPipelineCache::CompileShaders (INOUT MeshPipelineDesc &desc, const VDevice &dev)
	{
		return _CompileShaders( INOUT desc, dev );
	}

	bool VPipelineCache::CompileShaders (INOUT RayTracingPipelineDesc &desc, const VDevice &dev)
	{
		return _CompileShaders( INOUT desc, dev );
	}
	
/*
=================================================
	CompileShader
=================================================
*/
	bool VPipelineCache::CompileShader (INOUT ComputePipelineDesc &desc, const VDevice &dev)
	{
		// check is shaders supported by default compiler
		const auto	formats = _GetBuiltinFormats( dev );

		for (auto& fmt : formats)
		{
			auto	iter = desc._shader.data.find( fmt );

			if ( iter == desc._shader.data.end() )
				continue;

			if ( EnumEq( fmt, EShaderLangFormat::ShaderModule ) )
			{
				auto	shader_data = iter->second;
				
				desc._shader.data.clear();
				desc._shader.data.insert({ fmt, shader_data });
				return true;
			}
			
			if ( EnumEq( fmt, EShaderLangFormat::SPIRV ) )
			{
				VkShaderPtr		mod;
				CHECK_ERR( _CompileSPIRVShader( dev, iter->second, OUT mod ));

				desc._shader.data.clear();
				desc._shader.data.insert({ fmt, mod });
				return true;
			}
		}


		// try to compile
		for (auto& comp : _compilers)
		{
			if ( comp->IsSupported( desc, formats.front() ) )
			{
				CHECK_ERR( comp->Compile( INOUT desc, formats.front() ));
				return true;
			}
		}

		RETURN_ERR( "unsuported shader format!" );
	}

/*
=================================================
	_CompileShader
=================================================
*/
	bool  VPipelineCache::_CompileSPIRVShader (const VDevice &dev, const PipelineDescription::ShaderDataUnion_t &shaderData, OUT VkShaderPtr &module)
	{
		const auto*	shader_data = std::get_if< PipelineDescription::SharedShaderPtr<Array<uint>> >( &shaderData );

		if ( not (shader_data and *shader_data) )
			RETURN_ERR( "invalid shader data format!" );
				
		VkShaderModuleCreateInfo	shader_info = {};
		shader_info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_info.codeSize	= size_t(ArraySizeOf( (*shader_data)->GetData() ));
		shader_info.pCode		= (*shader_data)->GetData().data();

		VkShaderModule		shader_id;
		VK_CHECK( dev.vkCreateShaderModule( dev.GetVkDevice(), &shader_info, null, OUT &shader_id ));

		module = MakeShared<VShaderModule>( shader_id, (*shader_data)->GetEntry() );

		_shaderCache.push_back( module );
		return true;
	}
	
/*
=================================================
	_CreatePipelineCache
=================================================
*/
	bool  VPipelineCache::_CreatePipelineCache (const VDevice &dev)
	{
		CHECK_ERR( not _pipelinesCache );

		VkPipelineCacheCreateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		info.pNext				= null;
		info.flags				= 0;
		info.initialDataSize	= 0;
		info.pInitialData		= null;

		VK_CHECK( dev.vkCreatePipelineCache( dev.GetVkDevice(), &info, null, OUT &_pipelinesCache ));
		return true;
	}
	
/*
=================================================
	AllocDescriptorSet
=================================================
*/
	bool  VPipelineCache::AllocDescriptorSet (const VDevice &dev, VkDescriptorSetLayout layout, OUT VkDescriptorSet &ds)
	{
		VkDescriptorSetAllocateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorSetCount	= 1;
		info.pSetLayouts		= &layout;

		for (auto& pool : _descriptorPools)
		{
			info.descriptorPool = pool;
			
			if ( dev.vkAllocateDescriptorSets( dev.GetVkDevice(), &info, OUT &ds ) == VK_SUCCESS )
				return true;
		}

		CHECK_ERR( _CreateDescriptorPool( dev ));
		
		info.descriptorPool = _descriptorPools.back();
		VK_CHECK( dev.vkAllocateDescriptorSets( dev.GetVkDevice(), &info, OUT &ds ));

		return true;
	}

/*
=================================================
	_CreateDescriptorPool
=================================================
*/
	bool  VPipelineCache::_CreateDescriptorPool (const VDevice &dev)
	{
		FixedArray< VkDescriptorPoolSize, 32 >	pool_sizes;

		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER,						MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		MaxDescriptorPoolSize * 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,				MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,				MaxDescriptorPoolSize });

		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,				MaxDescriptorPoolSize * 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,				MaxDescriptorPoolSize * 2 });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,		MaxDescriptorPoolSize });
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,		MaxDescriptorPoolSize });
		
		pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV,	MaxDescriptorPoolSize });

		
		VkDescriptorPoolCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount	= uint(pool_sizes.size());
		info.pPoolSizes		= pool_sizes.data();
		info.maxSets		= MaxDescriptorSets;

		VkDescriptorPool			ds_pool;
		VK_CHECK( dev.vkCreateDescriptorPool( dev.GetVkDevice(), &info, null, OUT &ds_pool ));

		_descriptorPools.push_back( ds_pool );
		return true;
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
	OverrideColorStates
=================================================
*/
	void OverrideColorStates (INOUT RenderState::ColorBuffersState &currColorStates, const _fg_hidden_::ColorBuffers_t &newStates)
	{
		for (auto& cb : newStates)
		{
			auto	iter = currColorStates.buffers.find( cb.first );
			ASSERT( iter != currColorStates.buffers.end() );

			if ( iter != currColorStates.buffers.end() )
			{
				iter->second = cb.second;
			}
		}
	}

/*
=================================================
	CreatePipelineInstance
=================================================
*/
	VkPipeline  VPipelineCache::CreatePipelineInstance (VResourceManagerThread		&resMngr,
														const VLogicalRenderPass	&logicalRP,
														const VBaseDrawVerticesTask	&drawTask)
	{
		CHECK_ERR( drawTask.pipeline and logicalRP.GetRenderPassID() );

		// not supported yet
		ASSERT( EnumEq( drawTask.dynamicStates, EPipelineDynamicState::Viewport ));
		//ASSERT( EnumEq( drawTask.dynamicStates, EPipelineDynamicState::Scissor ));

		VDevice const&				dev			= resMngr.GetDevice();
		VGraphicsPipeline const*	gppln		= resMngr.GetResource( drawTask.pipeline );
		VPipelineLayout const*		layout		= resMngr.GetResource( gppln->GetLayoutID() );
		VRenderPass const*			render_pass	= resMngr.GetResource( logicalRP.GetRenderPassID() );
		
		// check topology
		CHECK_ERR(	uint(drawTask.topology) < gppln->_supportedTopology.size() and
					gppln->_supportedTopology[uint(drawTask.topology)] );

		VGraphicsPipeline::PipelineInstance		inst;
		inst.dynamicState				= drawTask.dynamicStates | EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;
		inst.renderPassId				= logicalRP.GetRenderPassID();
		inst.subpassIndex				= logicalRP.GetSubpassIndex();
		inst.vertexInput				= drawTask.vertexInput;
		inst.flags						= 0;	//pipelineFlags;	// TODO
		inst.viewportCount				= uint(logicalRP.GetViewports().size());
		inst.renderState.color			= logicalRP.GetColorState();
		inst.renderState.depth			= logicalRP.GetDepthState();
		inst.renderState.stencil		= logicalRP.GetStencilState();
		inst.renderState.rasterization	= logicalRP.GetRasterizationState();
		inst.renderState.multisample	= logicalRP.GetMultisampleState();

		inst.renderState.inputAssembly.topology			= drawTask.topology;
		inst.renderState.inputAssembly.primitiveRestart = drawTask.primitiveRestart;

		inst.vertexInput.ApplyAttribs( gppln->GetVertexAttribs() );
		OverrideColorStates( INOUT inst.renderState.color, drawTask.colorBuffers );

		_ValidateRenderState( dev, INOUT inst.renderState, INOUT inst.dynamicState );

		inst._hash	= HashOf( inst.renderPassId )	+ HashOf( inst.subpassIndex )	+
					  HashOf( inst.renderState )	+ HashOf( inst.vertexInput )	+
					  HashOf( inst.dynamicState )	+ HashOf( inst.viewportCount )	+
					  HashOf( inst.flags );


		// find existing instance
		{
			auto iter1 = gppln->_instances.find( inst );
			if ( iter1 != gppln->_instances.end() )
				return iter1->second;
			
			auto iter2 = _graphicsPipelines->find({ gppln, inst });
			if ( iter2 != _graphicsPipelines->end() )
				return iter2->second;
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

		_SetShaderStages( OUT _tempStages, INOUT _tempSpecialization, INOUT _tempSpecEntries, gppln->_shaders );
		_SetDynamicState( OUT dynamic_state_info, OUT _tempDynamicStates, inst.dynamicState );
		_SetColorBlendState( OUT blend_info, OUT _tempAttachments, inst.renderState.color, *render_pass, inst.subpassIndex );
		_SetMultisampleState( OUT multisample_info, inst.renderState.multisample );
		_SetTessellationState( OUT tessellation_info, gppln->_patchControlPoints );
		_SetDepthStencilState( OUT depth_stencil_info, inst.renderState.depth, inst.renderState.stencil );
		_SetRasterizationState( OUT rasterization_info, inst.renderState.rasterization );
		_SetupPipelineInputAssemblyState( OUT input_assembly_info, inst.renderState.inputAssembly );
		_SetVertexInputState( OUT vertex_input_info, OUT _tempVertexAttribs, OUT _tempVertexBinding, inst.vertexInput );
		_SetViewportState( OUT viewport_info, OUT _tempViewports, OUT _tempScissors,
						   uint2(1024, 1024), inst.viewportCount, inst.dynamicState );

		pipeline_info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.pNext					= null;
		pipeline_info.flags					= inst.flags;
		pipeline_info.pInputAssemblyState	= &input_assembly_info;
		pipeline_info.pRasterizationState	= &rasterization_info;
		pipeline_info.pColorBlendState		= &blend_info;
		pipeline_info.pDepthStencilState	= &depth_stencil_info;
		pipeline_info.pMultisampleState		= &multisample_info;
		pipeline_info.pTessellationState	= (gppln->_patchControlPoints > 0 ? &tessellation_info : null);
		pipeline_info.pVertexInputState		= &vertex_input_info;
		pipeline_info.pDynamicState			= (_tempDynamicStates.empty() ? null : &dynamic_state_info);
		pipeline_info.basePipelineIndex		= -1;
		pipeline_info.basePipelineHandle	= VK_NULL_HANDLE;
		pipeline_info.layout				= layout->Handle();
		pipeline_info.stageCount			= uint(_tempStages.size());
		pipeline_info.pStages				= _tempStages.data();
		pipeline_info.renderPass			= render_pass->Handle();
		pipeline_info.subpass				= inst.subpassIndex;
		
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
		VK_CHECK( dev.vkCreateGraphicsPipelines( dev.GetVkDevice(), _pipelinesCache, 1, &pipeline_info, null, OUT &ppln_id ));

		CHECK( _graphicsPipelines->insert_or_assign( {gppln, std::move(inst)}, ppln_id ).second );
		return ppln_id;
	}
/*
=================================================
	CreatePipelineInstance
=================================================
*/
	VkPipeline  VPipelineCache::CreatePipelineInstance (VResourceManagerThread		&resMngr,
														const VLogicalRenderPass	&logicalRP,
														const VBaseDrawMeshes		&drawTask)
	{
		CHECK_ERR( resMngr.GetDevice().IsMeshShaderEnabled() );
		CHECK_ERR( drawTask.pipeline and logicalRP.GetRenderPassID() );

		// not supported yet
		ASSERT( EnumEq( drawTask.dynamicStates, EPipelineDynamicState::Viewport ));
		ASSERT( EnumEq( drawTask.dynamicStates, EPipelineDynamicState::Scissor ));

		VDevice const&				dev			= resMngr.GetDevice();
		VMeshPipeline const*		mppln		= resMngr.GetResource( drawTask.pipeline );
		VPipelineLayout const*		layout		= resMngr.GetResource( mppln->GetLayoutID() );
		VRenderPass const*			render_pass	= resMngr.GetResource( logicalRP.GetRenderPassID() );
		
		// check topology
		//CHECK_ERR(	uint(renderState.inputAssembly.topology) < mppln->_supportedTopology.size() and
		//			mppln->_supportedTopology[uint(renderState.inputAssembly.topology)] );

		VMeshPipeline::PipelineInstance		inst;
		inst.dynamicState				= drawTask.dynamicStates | EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;
		inst.renderPassId				= logicalRP.GetRenderPassID();
		inst.subpassIndex				= logicalRP.GetSubpassIndex();
		inst.flags						= 0;	//pipelineFlags;	// TODO
		inst.viewportCount				= uint(logicalRP.GetViewports().size());
		inst.renderState.color			= logicalRP.GetColorState();
		inst.renderState.depth			= logicalRP.GetDepthState();
		inst.renderState.stencil		= logicalRP.GetStencilState();
		inst.renderState.rasterization	= logicalRP.GetRasterizationState();
		inst.renderState.multisample	= logicalRP.GetMultisampleState();
		inst.renderState.inputAssembly.topology	= EPrimitive::TriangleList;		// TODO

		OverrideColorStates( INOUT inst.renderState.color, drawTask.colorBuffers );
		_ValidateRenderState( dev, INOUT inst.renderState, INOUT inst.dynamicState );

		inst._hash	= HashOf( inst.renderPassId )	+ HashOf( inst.subpassIndex )	+
					  HashOf( inst.renderState )	+ HashOf( inst.dynamicState )	+
					  HashOf( inst.viewportCount )	+ HashOf( inst.flags );


		// find existing instance
		{
			auto iter1 = mppln->_instances.find( inst );
			if ( iter1 != mppln->_instances.end() )
				return iter1->second;
			
			auto iter2 = _meshPipelines->find({ mppln, inst });
			if ( iter2 != _meshPipelines->end() )
				return iter2->second;
		}


		// create new instance
		_ClearTemp();

		VkGraphicsPipelineCreateInfo			pipeline_info		= {};
		VkPipelineInputAssemblyStateCreateInfo	input_assembly_info	= {};
		VkPipelineColorBlendStateCreateInfo		blend_info			= {};
		VkPipelineDepthStencilStateCreateInfo	depth_stencil_info	= {};
		VkPipelineMultisampleStateCreateInfo	multisample_info	= {};
		VkPipelineRasterizationStateCreateInfo	rasterization_info	= {};
		VkPipelineDynamicStateCreateInfo		dynamic_state_info	= {};
		VkPipelineVertexInputStateCreateInfo	vertex_input_info	= {};
		VkPipelineViewportStateCreateInfo		viewport_info		= {};

		_SetShaderStages( OUT _tempStages, INOUT _tempSpecialization, INOUT _tempSpecEntries, mppln->_shaders );
		_SetDynamicState( OUT dynamic_state_info, OUT _tempDynamicStates, inst.dynamicState );
		_SetColorBlendState( OUT blend_info, OUT _tempAttachments, inst.renderState.color, *render_pass, inst.subpassIndex );
		_SetMultisampleState( OUT multisample_info, inst.renderState.multisample );
		_SetDepthStencilState( OUT depth_stencil_info, inst.renderState.depth, inst.renderState.stencil );
		_SetRasterizationState( OUT rasterization_info, inst.renderState.rasterization );
		_SetupPipelineInputAssemblyState( OUT input_assembly_info, inst.renderState.inputAssembly );
		_SetViewportState( OUT viewport_info, OUT _tempViewports, OUT _tempScissors,
						   uint2(1024, 1024), inst.viewportCount, inst.dynamicState );

		vertex_input_info.sType	= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		pipeline_info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.pNext					= null;
		pipeline_info.flags					= inst.flags;
		pipeline_info.pInputAssemblyState	= &input_assembly_info;
		pipeline_info.pRasterizationState	= &rasterization_info;
		pipeline_info.pColorBlendState		= &blend_info;
		pipeline_info.pDepthStencilState	= &depth_stencil_info;
		pipeline_info.pMultisampleState		= &multisample_info;
		pipeline_info.pVertexInputState		= &vertex_input_info;
		pipeline_info.pDynamicState			= (_tempDynamicStates.empty() ? null : &dynamic_state_info);
		pipeline_info.basePipelineIndex		= -1;
		pipeline_info.basePipelineHandle	= VK_NULL_HANDLE;
		pipeline_info.layout				= layout->Handle();
		pipeline_info.stageCount			= uint(_tempStages.size());
		pipeline_info.pStages				= _tempStages.data();
		pipeline_info.renderPass			= render_pass->Handle();
		pipeline_info.subpass				= inst.subpassIndex;
		
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
		VK_CHECK( dev.vkCreateGraphicsPipelines( dev.GetVkDevice(), _pipelinesCache, 1, &pipeline_info, null, OUT &ppln_id ));

		CHECK( _meshPipelines->insert_or_assign( {mppln, std::move(inst)}, ppln_id ).second );
		return ppln_id;
	}

/*
=================================================
	CreatePipelineInstance
=================================================
*/
	VkPipeline  VPipelineCache::CreatePipelineInstance (VResourceManagerThread	&resMngr,
														RawCPipelineID			 pplnId,
														const Optional<uint3>	&localGroupSize,
														VkPipelineCreateFlags	 pipelineFlags)
	{
		CHECK_ERR( pplnId );

		VDevice const &			dev		= resMngr.GetDevice();
		VComputePipeline const*	cppln	= resMngr.GetResource( pplnId );
		VPipelineLayout const*	layout	= resMngr.GetResource( cppln->GetLayoutID() );

		VComputePipeline::PipelineInstance		inst;
		inst.flags			= pipelineFlags;
		inst.localGroupSize	= localGroupSize.value_or( cppln->_defaultLocalGroupSize );
		inst.localGroupSize = { cppln->_localSizeSpec.x != ComputePipelineDesc::UNDEFINED_SPECIALIZATION ? inst.localGroupSize.x : cppln->_defaultLocalGroupSize.x,
								cppln->_localSizeSpec.y != ComputePipelineDesc::UNDEFINED_SPECIALIZATION ? inst.localGroupSize.y : cppln->_defaultLocalGroupSize.y,
								cppln->_localSizeSpec.z != ComputePipelineDesc::UNDEFINED_SPECIALIZATION ? inst.localGroupSize.z : cppln->_defaultLocalGroupSize.z };
		inst._hash			= (HashOf( inst.localGroupSize ) + HashOf( inst.flags ));


		// find existing instance
		{
			auto iter1 = cppln->_instances.find( inst );
			if ( iter1 != cppln->_instances.end() )
				return iter1->second;

			auto iter2 = _computePipelines->find({ cppln, inst });
			if ( iter2 != _computePipelines->end() )
				return iter2->second;
		}


		// create new instance
		_ClearTemp();
		CHECK_ERR( cppln->_shader );

		VkSpecializationInfo			spec = {};
		VkComputePipelineCreateInfo		pipeline_info = {};

		pipeline_info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_info.layout		= layout->Handle();
		pipeline_info.flags			= inst.flags;
		pipeline_info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipeline_info.stage.module	= BitCast<VkShaderModule>( cppln->_shader->GetData() );
		pipeline_info.stage.pName	= cppln->_shader->GetEntry().data();
		pipeline_info.stage.flags	= 0;
		pipeline_info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;

		_AddLocalGroupSizeSpecialization( OUT _tempSpecEntries, OUT _tempSpecData, cppln->_localSizeSpec, inst.localGroupSize );

		if ( not _tempSpecEntries.empty() )
		{
			spec.mapEntryCount	= uint(_tempSpecEntries.size());
			spec.pMapEntries	= _tempSpecEntries.data();
			spec.dataSize		= size_t(ArraySizeOf( _tempSpecData ));
			spec.pData			= _tempSpecData.data();

			pipeline_info.stage.pSpecializationInfo	= &spec;
		}

		VkPipeline	ppln_id = {};
		VK_CHECK( dev.vkCreateComputePipelines( dev.GetVkDevice(), _pipelinesCache, 1, &pipeline_info, null, OUT &ppln_id ));

		CHECK( _computePipelines->insert_or_assign( {cppln, std::move(inst)}, ppln_id ).second );
		return ppln_id;
	}

/*
=================================================
	_SetShaderStages
=================================================
*/
	void VPipelineCache::_SetShaderStages (OUT ShaderStages_t &stages,
										   INOUT Specializations_t &,
										   INOUT SpecializationEntries_t &,
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

		for (auto& src : inState.Vertices())
		{
			VkVertexInputAttributeDescription	dst = {};

			ASSERT( src.second.index != ~0U );

			dst.binding		= src.second.bindingIndex;
			dst.format		= VEnumCast( src.second.type );
			dst.location	= src.second.index;
			dst.offset		= uint(src.second.offset);

			vertexAttribs.push_back( std::move(dst) );
		}

		for (auto& src : inState.BufferBindings())
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
											  const VRenderPass &renderPass,
											  const uint subpassIndex) const
	{
		ASSERT( subpassIndex < renderPass.GetCreateInfo().subpassCount );

		const bool					logic_op_enabled	= ( inState.logicOp != ELogicOp::None );
		const VkSubpassDescription&	subpass				= renderPass.GetCreateInfo().pSubpasses[ subpassIndex ];

		for (size_t i = 0; i < subpass.colorAttachmentCount; ++i)
		{
			VkPipelineColorBlendAttachmentState		color_state = {};
			color_state.colorWriteMask	= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
										  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			attachments.push_back( color_state );
		}

		for (auto& cb : inState.buffers)
		{
			ASSERT( cb.first.IsDefined() );

			uint	index;
			CHECK( renderPass.GetColorAttachmentIndex( cb.first, OUT index ) );

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
	void VPipelineCache::_ValidateRenderState (const VDevice &dev, INOUT RenderState &renderState, INOUT EPipelineDynamicState &dynamicStates) const
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
			const bool	dual_src_blend	= dev.GetDeviceFeatures().dualSrcBlend;

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
