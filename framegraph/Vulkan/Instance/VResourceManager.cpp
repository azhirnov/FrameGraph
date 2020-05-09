// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VResourceManager.h"
#include "VDevice.h"
#include "VEnumCast.h"
#include "stl/Algorithms/StringUtils.h"
#include "Shared/PipelineResourcesHelper.h"

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
		
		ShaderModuleVk_t const&	GetData () const override		{ return *Cast<ShaderModuleVk_t>( &_module ); }

		StringView				GetEntry () const override		{ return _entry; }
		
		StringView				GetDebugName () const override	{ return ""; }

		size_t					GetHashOfData () const override	{ ASSERT(false);  return 0; }

		bool					ParseDebugOutput (EShaderDebugMode, ArrayView<uint8_t>, OUT Array<String> &) const override { return false; }
	};
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VResourceManager::VResourceManager (const VDevice &dev) :
		_device{ dev },
		_memoryMngr{ dev },
		_descMngr{ dev },
		_submissionCounter{ 0 }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VResourceManager::~VResourceManager ()
	{
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool  VResourceManager::Initialize ()
	{
		CHECK_ERR( _memoryMngr.Initialize() );
		CHECK_ERR( _descMngr.Initialize() );

		_CreateEmptyDescriptorSetLayout();
		_CheckHostVisibleMemory();

		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void  VResourceManager::Deinitialize ()
	{
		_DestroyStagingBuffers();
		_DestroyShaderDebuggerResources();

		_DestroyResourceCache( INOUT _samplerCache );
		_DestroyResourceCache( INOUT _pplnLayoutCache );
		_DestroyResourceCache( INOUT _dsLayoutCache );
		_DestroyResourceCache( INOUT _renderPassCache );
		_DestroyResourceCache( INOUT _framebufferCache );
		_DestroyResourceCache( INOUT _pplnResourcesCache );
		
		// release shader cache
		{
			EXLOCK( _shaderCacheGuard );

			for (auto& sh : _shaderCache)
			{
				ASSERT( sh.use_count() == 1 );

				Cast<VShaderModule>( sh )->Destroy( _device );
			}
			_shaderCache.clear();
		}

		// release pipeline compilers
		{
			EXLOCK( _compilersGuard );
			_compilers.clear();
		}
		
		_descMngr.Deinitialize();
		_memoryMngr.Deinitialize();
	}
	
/*
=================================================
	AddCompiler
=================================================
*/
	void  VResourceManager::AddCompiler (const PipelineCompiler &comp)
	{
		EXLOCK( _compilersGuard );
		_compilers.insert( comp );
	}
	
/*
=================================================
	OnSubmit
=================================================
*/
	void  VResourceManager::OnSubmit ()
	{
		_submissionCounter.fetch_add( 1, memory_order_relaxed );
	}
	
/*
=================================================
	_DestroyResourceCache
=================================================
*/
	template <typename DataT, size_t CS, size_t MC>
	inline void  VResourceManager::_DestroyResourceCache (INOUT CachedPoolTmpl<DataT,CS,MC> &res)
	{
		for (size_t i = 0, count = res.size(); i < count; ++i)
		{
			Index_t	id	 = Index_t(i);
			auto&	data = res[id];
				
			if ( data.IsCreated() )
			{
				res.RemoveFromCache( id );
				data.Destroy( *this );
				res.Unassign( id );
			}
		}
	}

/*
=================================================
	_CreateEmptyDescriptorSetLayout
=================================================
*/
	bool  VResourceManager::_CreateEmptyDescriptorSetLayout ()
	{
		auto&		pool	= _GetResourcePool( RawDescriptorSetLayoutID{} );
		Index_t		index	= UMax;
		CHECK_ERR( pool.Assign( OUT index ));

		auto&										res			= pool[ index ];
		PipelineDescription::UniformMapPtr			uniforms	= MakeShared<PipelineDescription::UniformMap_t>();
		VDescriptorSetLayout::DescriptorBinding_t	binding;

		res.Data().~VDescriptorSetLayout();
		new (&res.Data()) VDescriptorSetLayout{ uniforms, OUT binding };
		
		CHECK_ERR( res.Create( _device, binding ));
		CHECK_ERR( pool.AddToCache( index ).second );
		res.AddRef();

		_emptyDSLayout = RawDescriptorSetLayoutID{ index, res.GetInstanceID() };
		return true;
	}
	
/*
=================================================
	Replace
----
	destroy previous resource instance and construct new instance
=================================================
*/
	template <typename ResType, typename ...Args>
	inline void  Replace (INOUT ResourceBase<ResType> &target, Args&& ...args)
	{
		target.Data().~ResType();
		new (&target.Data()) ResType{ std::forward<Args &&>(args)... };
	}
	
/*
=================================================
	GetDebugShaderStorageSize
=================================================
*/
	ND_ BytesU  VResourceManager::GetDebugShaderStorageSize (EShaderStages stages, EShaderDebugMode mode)
	{
		if ( mode == EShaderDebugMode::Timemap )
		{
			return SizeOf<uint> * 4;	// tile size, width, (padding)
		}
		else
		if ( mode == EShaderDebugMode::Trace or mode == EShaderDebugMode::Profiling )
		{
			if ( EnumEq( EShaderStages::AllGraphics, stages ))
				return SizeOf<uint> * 4;	// fragcoord, (padding), position
		
			if ( stages == EShaderStages::Compute )
				return SizeOf<uint> * 4;	// global invocation, position
		
			if ( EnumEq( EShaderStages::AllRayTracing, stages ))
				return SizeOf<uint> * 4;	// launch, position
		}

		RETURN_ERR( "unsupported shader type" );
	}

/*
=================================================
	GetDescriptorSetLayout
=================================================
*/
	RawDescriptorSetLayoutID  VResourceManager::GetDescriptorSetLayout (EShaderDebugMode debugMode, EShaderStages debuggableShaders)
	{
		const uint	key  = (uint(debuggableShaders) & 0xFFFFFF) | (uint(debugMode) << 24);
		auto		iter = _shaderDbg.dsLayoutsCache.find( key );

		if ( iter != _shaderDbg.dsLayoutsCache.end() )
			return iter->second;

		PipelineDescription::UniformMap_t	uniforms;
		PipelineDescription::Uniform		sb_uniform;
		PipelineDescription::StorageBuffer	sb_desc;

		sb_desc.state				= EResourceState_FromShaders( debuggableShaders ) | EResourceState::ShaderReadWrite | EResourceState::_BufferDynamicOffset;
		sb_desc.arrayStride			= SizeOf<uint>;
		sb_desc.staticSize			= GetDebugShaderStorageSize( debuggableShaders, debugMode );
		sb_desc.dynamicOffsetIndex	= 0;

		sb_uniform.index			= BindingIndex{ UMax, 0 };
		sb_uniform.stageFlags		= debuggableShaders;
		sb_uniform.data				= sb_desc;
		sb_uniform.arraySize		= 1;

		uniforms.insert({ UniformID{"dbg_ShaderTrace"}, sb_uniform });

		auto	layout = CreateDescriptorSetLayout( MakeShared<const PipelineDescription::UniformMap_t>( std::move(uniforms) ));
		CHECK_ERR( layout );

		_shaderDbg.dsLayoutsCache.insert({ key, layout });
		return layout;
	}

/*
=================================================
	CreateDebugPipelineLayout
=================================================
*/
	RawPipelineLayoutID  VResourceManager::CreateDebugPipelineLayout (RawPipelineLayoutID baseLayout, EShaderDebugMode debugMode,
																	  EShaderStages debuggableShaders, const DescriptorSetID &dsID)
	{
		VPipelineLayout const*	origin = GetResource( baseLayout );
		CHECK_ERR( origin );
		
		PipelineDescription::PipelineLayout		desc;
		DSLayouts_t								ds_layouts;
		auto&									origin_sets = origin->GetDescriptorSets();
		auto&									ds_pool		= _GetResourcePool( RawDescriptorSetLayoutID{} );

		// copy descriptor set layouts
		for (auto& src : origin_sets)
		{
			auto&	ds_layout	= ds_pool[ src.second.layoutId.Index() ];
			ds_layout.AddRef();

			PipelineDescription::DescriptorSet	dst;
			dst.id				= src.first;
			dst.bindingIndex	= src.second.index;
			dst.uniforms		= ds_layout.Data().GetUniforms();

			ASSERT( src.second.index != FG_DebugDescriptorSet );

			desc.descriptorSets.push_back( std::move(dst) );
			ds_layouts.push_back({ src.second.layoutId, &ds_layout });
		}

		// append descriptor set layout for shader trace
		{
			auto	ds_layout_id = GetDescriptorSetLayout( debugMode, debuggableShaders );

			auto&	ds_layout	= ds_pool[ ds_layout_id.Index() ];
			ds_layout.AddRef();

			PipelineDescription::DescriptorSet	dst;
			dst.id				= dsID;
			dst.bindingIndex	= FG_DebugDescriptorSet;
			dst.uniforms		= ds_layout.Data().GetUniforms();
			
			desc.descriptorSets.push_back( std::move(dst) );
			ds_layouts.push_back({ ds_layout_id, &ds_layout });
		}

		// copy push constant ranges
		desc.pushConstants = origin->GetPushConstants();


		RawPipelineLayoutID						new_layout;
		ResourceBase<VPipelineLayout> const*	layout_ptr = null;
		CHECK_ERR( _CreatePipelineLayout( OUT new_layout, OUT layout_ptr, desc, ds_layouts ));

		return new_layout;
	}

/*
=================================================
	CreateDescriptorSetLayout
=================================================
*/
	RawDescriptorSetLayoutID  VResourceManager::CreateDescriptorSetLayout (const PipelineDescription::UniformMapPtr &uniforms)
	{
		ResourceBase<VDescriptorSetLayout>*	layout_ptr = null;
		RawDescriptorSetLayoutID			result;

		CHECK_ERR( _CreateDescriptorSetLayout( OUT result, OUT layout_ptr, uniforms ));

		return result;
	}
	
/*
=================================================
	_CreatePipelineLayout
=================================================
*/
	bool  VResourceManager::_CreatePipelineLayout (OUT RawPipelineLayoutID &id, OUT ResourceBase<VPipelineLayout> const* &layoutPtr,
												   PipelineDescription::PipelineLayout &&desc)
	{
		// init pipeline layout create info
		DSLayouts_t  ds_layouts;
		for (auto& ds : desc.descriptorSets)
		{
			RawDescriptorSetLayoutID			ds_id;
			ResourceBase<VDescriptorSetLayout>*	ds_layout = null;
			CHECK_ERR( _CreateDescriptorSetLayout( OUT ds_id, OUT ds_layout, ds.uniforms ));

			ds_layouts.push_back({ ds_id, ds_layout });
		}
		return _CreatePipelineLayout( OUT id, OUT layoutPtr, desc, ds_layouts );
	}
	
	bool  VResourceManager::_CreatePipelineLayout (OUT RawPipelineLayoutID &id, OUT ResourceBase<VPipelineLayout> const* &layoutPtr,
												   const PipelineDescription::PipelineLayout &desc, const DSLayouts_t &dsLayouts)
	{
		CHECK_ERR( _Assign( OUT id ));
		
		auto*	empty_layout = GetResource( _GetEmptyDescriptorSetLayout() );
		CHECK_ERR( empty_layout );

		auto&	pool	= _GetResourcePool( id );
		auto&	layout	= pool[ id.Index() ];
		Replace( layout, desc, dsLayouts );

		// search in cache
		Index_t	temp_id		= pool.Find( &layout );
		bool	is_created	= false;

		if ( temp_id == UMax )
		{
			// create new
			if ( not layout.Create( _device, empty_layout->Handle() ))
			{
				_Unassign( id );
				RETURN_ERR( "failed when creating pipeline layout" );
			}

			layout.AddRef();
			is_created = true;
			
			// try to add to cache
			temp_id = pool.AddToCache( id.Index() ).first;
		}

		if ( temp_id == id.Index() )
		{
			layoutPtr = &layout;
			return true;
		}
		
		// use already cached resource
		layoutPtr = &pool[ temp_id ];
		layoutPtr->AddRef();
		
		for (auto& ds : dsLayouts) {
			ReleaseResource( ds.first );
		}
		
		if ( is_created )
			layout.Destroy( *this );

		_Unassign( id );
		
		id = RawPipelineLayoutID( temp_id, layoutPtr->GetInstanceID() );
		return true;
	}

/*
=================================================
	_CreateDescriptorSetLayout
=================================================
*/
	bool  VResourceManager::_CreateDescriptorSetLayout (OUT RawDescriptorSetLayoutID &id, OUT ResourceBase<VDescriptorSetLayout>* &layoutPtr,
														const PipelineDescription::UniformMapPtr &uniforms)
	{
		CHECK_ERR( _Assign( OUT id ));
		
		// init descriptor set layout create info
		VDescriptorSetLayout::DescriptorBinding_t	binding;	// TODO: use custom allocator

		auto&	pool		= _GetResourcePool( id );
		auto&	ds_layout	= pool[ id.Index() ];
		Replace( ds_layout, uniforms, OUT binding );
		
		// search in cache
		Index_t	temp_id		= pool.Find( &ds_layout );
		bool	is_created	= false;

		if ( temp_id == UMax )
		{
			// create new
			if ( not ds_layout.Create( _device, binding ))
			{
				_Unassign( id );
				RETURN_ERR( "failed when creating descriptor set layout" );
			}

			ds_layout.AddRef();
			is_created = true;
			
			// try to add to cache
			temp_id = pool.AddToCache( id.Index() ).first;
		}

		if ( temp_id == id.Index() )
		{
			layoutPtr = &ds_layout;
			return true;
		}
		
		// use already cached resource
		layoutPtr = &pool[ temp_id ];
		layoutPtr->AddRef();
		
		if ( is_created )
			ds_layout.Destroy( *this );

		_Unassign( id );
		
		id = RawDescriptorSetLayoutID( temp_id, layoutPtr->GetInstanceID() );
		return true;
	}
	
	
/*
=================================================
	GetBuiltinFormats
=================================================
*/
	ND_ static FixedArray<EShaderLangFormat,16>  GetBuiltinFormats (const VDevice &dev)
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
	_CompileShaders
=================================================
*/
	template <typename DescT>
	bool  VResourceManager::_CompileShaders (INOUT DescT &desc, const VDevice &dev)
	{
		const EShaderLangFormat		req_format = dev.GetVkVersion() | EShaderLangFormat::ShaderModule;

		// try to use external compilers
		{
			SHAREDLOCK( _compilersGuard );

			for (auto& comp : _compilers)
			{
				if ( comp->IsSupported( desc, req_format ) )
				{
					return comp->Compile( INOUT desc, req_format );
				}
			}
		}

		// check is shaders supported by default compiler
		const auto	formats		 = GetBuiltinFormats( dev );
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

		if ( not is_supported )
			RETURN_ERR( "unsuported shader format!" );

		return true;
	}
	
/*
=================================================
	_CompileShader
=================================================
*/
	bool  VResourceManager::_CompileShader (INOUT ComputePipelineDesc &desc, const VDevice &dev)
	{
		const EShaderLangFormat		req_format = dev.GetVkVersion() | EShaderLangFormat::ShaderModule;
		
		// try to use external compilers
		{
			SHAREDLOCK( _compilersGuard );

			for (auto& comp : _compilers)
			{
				if ( comp->IsSupported( desc, req_format ) )
				{
					return comp->Compile( INOUT desc, req_format );
				}
			}
		}

		// check is shaders supported by default compiler
		const auto	formats = GetBuiltinFormats( dev );

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

		RETURN_ERR( "unsuported shader format!" );
	}

/*
=================================================
	_CompileShader
=================================================
*/
	bool  VResourceManager::_CompileSPIRVShader (const VDevice &dev, const PipelineDescription::ShaderDataUnion_t &shaderData, OUT VkShaderPtr &module)
	{
		const auto*	shader_data = UnionGetIf< PipelineDescription::SharedShaderPtr<Array<uint>> >( &shaderData );

		if ( not (shader_data and *shader_data) )
			RETURN_ERR( "invalid shader data format!" );
				
		VkShaderModuleCreateInfo	shader_info = {};
		shader_info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_info.codeSize	= size_t(ArraySizeOf( (*shader_data)->GetData() ));
		shader_info.pCode		= (*shader_data)->GetData().data();

		VkShaderModule		shader_id;
		VK_CHECK( dev.vkCreateShaderModule( dev.GetVkDevice(), &shader_info, null, OUT &shader_id ));

		dev.SetObjectName( BitCast<uint64_t>(shader_id), (*shader_data)->GetDebugName(), VK_OBJECT_TYPE_SHADER_MODULE );

		module = MakeShared<VShaderModule>( shader_id, (*shader_data)->GetEntry() );

		EXLOCK( _shaderCacheGuard );
		_shaderCache.push_back( module );

		return true;
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	RawMPipelineID  VResourceManager::CreatePipeline (INOUT MeshPipelineDesc &desc, StringView dbgName)
	{
		CHECK_ERR( _device.IsMeshShaderEnabled() );

		if ( not _CompileShaders( INOUT desc, _device ))
			return Default;

		RawPipelineLayoutID						layout_id;
		ResourceBase<VPipelineLayout> const*	layout	= null;
		CHECK_ERR( _CreatePipelineLayout( OUT layout_id, OUT layout, std::move(desc._pipelineLayout) ));
		
		RawMPipelineID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( desc, layout_id, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating mesh pipeline" );
		}

		layout->AddRef();
		data.AddRef();

		return id;
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	RawGPipelineID  VResourceManager::CreatePipeline (INOUT GraphicsPipelineDesc &desc, StringView dbgName)
	{
		if ( not _CompileShaders( INOUT desc, _device ))
			return Default;
		
		RawPipelineLayoutID						layout_id;
		ResourceBase<VPipelineLayout> const*	layout	= null;
		CHECK_ERR( _CreatePipelineLayout( OUT layout_id, OUT layout, std::move(desc._pipelineLayout) ));

		RawGPipelineID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( desc, layout_id, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating graphics pipeline" );
		}
		
		layout->AddRef();
		data.AddRef();

		return id;
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	RawCPipelineID  VResourceManager::CreatePipeline (INOUT ComputePipelineDesc &desc, StringView dbgName)
	{
		if ( not _CompileShader( INOUT desc, _device ))
			return Default;
		
		RawPipelineLayoutID						layout_id;
		ResourceBase<VPipelineLayout> const*	layout	= null;
		CHECK_ERR( _CreatePipelineLayout( OUT layout_id, OUT layout, std::move(desc._pipelineLayout) ));

		RawCPipelineID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( desc, layout_id, dbgName ) )
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating compute pipeline" );
		}

		layout->AddRef();
		data.AddRef();

		return id;
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	RawRTPipelineID  VResourceManager::CreatePipeline (INOUT RayTracingPipelineDesc &desc)
	{
		CHECK_ERR( _device.IsRayTracingEnabled() );

		if ( not _CompileShaders( INOUT desc, _device ))
			return Default;
		
		RawPipelineLayoutID						layout_id;
		ResourceBase<VPipelineLayout> const*	layout	= null;
		CHECK_ERR( _CreatePipelineLayout( OUT layout_id, OUT layout, std::move(desc._pipelineLayout) ));

		RawRTPipelineID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( desc, layout_id ) )
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating ray tracing pipeline" );
		}
		
		layout->AddRef();
		data.AddRef();

		return id;
	}
	
/*
=================================================
	_CreateMemory
=================================================
*/
	bool  VResourceManager::_CreateMemory (OUT RawMemoryID &id, OUT ResourceBase<VMemoryObj>* &memPtr, const MemoryDesc &desc, StringView dbgName)
	{
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( desc, dbgName ) )
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating memory object" );
		}
		
		memPtr = &data;
		return true;
	}

/*
=================================================
	CreateImage
=================================================
*/
	RawImageID  VResourceManager::CreateImage (const ImageDesc &desc, const MemoryDesc &mem, EQueueFamilyMask queueFamilyMask,
											   EResourceState defaultState, StringView dbgName)
	{
		RawMemoryID					mem_id;
		ResourceBase<VMemoryObj>*	mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, mem, dbgName ));

		RawImageID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( *this, desc, mem_id, mem_obj->Data(), queueFamilyMask, defaultState, dbgName ))
		{
			ReleaseResource( mem_id );
			_Unassign( id );
			RETURN_ERR( "failed when creating image" );
		}
		
		mem_obj->AddRef();
		data.AddRef();
		return id;
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	RawBufferID  VResourceManager::CreateBuffer (const BufferDesc &desc, const MemoryDesc &mem, EQueueFamilyMask queueFamilyMask, StringView dbgName)
	{
		RawMemoryID					mem_id;
		ResourceBase<VMemoryObj>*	mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, mem, dbgName ));

		RawBufferID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *this, desc, mem_id, mem_obj->Data(), queueFamilyMask, dbgName ))
		{
			ReleaseResource( mem_id );
			_Unassign( id );
			RETURN_ERR( "failed when creating buffer" );
		}
		
		mem_obj->AddRef();
		data.AddRef();
		return id;
	}
	
/*
=================================================
	CreateImage
=================================================
*/
	RawImageID  VResourceManager::CreateImage (const VulkanImageDesc &desc, IFrameGraph::OnExternalImageReleased_t &&onRelease, StringView dbgName)
	{
		RawImageID		id;
		CHECK_ERR( _Assign( OUT id ));
		
		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( _device, desc, dbgName, std::move(onRelease) ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating external image" );
		}
		
		data.AddRef();
		return id;
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	RawBufferID  VResourceManager::CreateBuffer (const VulkanBufferDesc &desc, IFrameGraph::OnExternalBufferReleased_t &&onRelease, StringView dbgName)
	{
		RawBufferID		id;
		CHECK_ERR( _Assign( OUT id ));
		
		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( _device, desc, dbgName, std::move(onRelease) ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating external buffer" );
		}
		
		data.AddRef();
		return id;
	}

/*
=================================================
	_CreateCachedResource
=================================================
*/
	template <typename ID, typename FnInitialize, typename FnCreate>
	inline ID  VResourceManager::_CreateCachedResource (StringView errorStr, FnInitialize&& fnInit, FnCreate&& fnCreate)
	{
		ID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	pool	= _GetResourcePool( id );
		auto&	data	= pool[ id.Index() ];
		fnInit( INOUT data );
		
		// search in cache
		Index_t	temp_id		= pool.Find( &data );
		bool	is_created	= false;

		if ( temp_id == UMax )
		{
			// create new
			if ( not fnCreate( data ) )
			{
				_Unassign( id );
				RETURN_ERR( errorStr );
			}

			data.AddRef();
			is_created = true;
			
			// try to add to cache
			temp_id = pool.AddToCache( id.Index() ).first;
		}

		if ( temp_id == id.Index() )
			return id;

		// use already cached resource
		auto&	temp = pool[ temp_id ];
		temp.AddRef();

		if ( is_created )
			data.Destroy( *this );
		
		_Unassign( id );

		return ID( temp_id, temp.GetInstanceID() );
	}

/*
=================================================
	Create***
=================================================
*/
	RawSamplerID  VResourceManager::CreateSampler (const SamplerDesc &desc, StringView dbgName)
	{
		return _CreateCachedResource<RawSamplerID>( "failed when creating sampler",
										[&] (auto& data) { return Replace( data, _device, desc ); },
										[&] (auto& data) { return data.Create( _device, dbgName ); });
	}
	
	RawRenderPassID  VResourceManager::CreateRenderPass (ArrayView<VLogicalRenderPass*> logicalPasses, StringView dbgName)
	{
		return _CreateCachedResource<RawRenderPassID>( "failed when creating render pass",
										[&] (auto& data) { return Replace( data, logicalPasses ); },
										[&] (auto& data) { return data.Create( _device, dbgName ); });
	}
	
	RawFramebufferID  VResourceManager::CreateFramebuffer (ArrayView<Pair<RawImageID, ImageViewDesc>> attachments,
																 RawRenderPassID rp, uint2 dim, uint layers, StringView dbgName)
	{
		return _CreateCachedResource<RawFramebufferID>( "failed when creating framebuffer",
										[&] (auto& data) {
											return Replace( data, attachments, rp, dim, layers );
										},
										[&] (auto& data) {
											if ( data.Create( *this, dbgName )) {
												_validation.createdFramebuffers.fetch_add( 1, memory_order_relaxed );
												return true;
											}
											return false;
										});
	}
	
/*
=================================================
	CreateDescriptorSet
=================================================
*/
	VPipelineResources const*  VResourceManager::CreateDescriptorSet (const PipelineResources &desc, VCmdBatch::ResourceMap_t &resourceMap)
	{
		using Resource_t = VCmdBatch::Resource;

		RawPipelineResourcesID	id = PipelineResourcesHelper::GetCached( desc );
		
		// use cached resources
		if ( id )
		{
			auto&	res = _GetResourcePool( id )[ id.Index() ];

			if ( res.GetInstanceID() == id.InstanceID() )
			{
				if ( resourceMap.insert({ Resource_t{ id }, 1 }).second )
					res.AddRef();
				
				ASSERT( res.Data().IsAllResourcesAlive( *this ));
				return &res.Data();
			}
		}
	
		CHECK_ERR( desc.IsInitialized() );
	
		auto&	layout = _GetResourcePool( desc.GetLayout() )[ desc.GetLayout().Index() ];
		CHECK_ERR( layout.IsCreated() and desc.GetLayout().InstanceID() == layout.GetInstanceID() );

		id = _CreateCachedResource<RawPipelineResourcesID>( "failed when creating descriptor set",
								   [&] (auto& data) { return Replace( data, desc ); },
								   [&] (auto& data) {
										if (data.Create( *this )) {
											layout.AddRef();
											_validation.createdPplnResources.fetch_add( 1, memory_order_relaxed );
											return true;
										}
										return false;
									});

		if ( id )
		{
			PipelineResourcesHelper::SetCache( desc, id );

			auto&	res = _GetResourcePool( id )[ id.Index() ];
			
			if ( resourceMap.insert({ Resource_t{ id }, 1 }).second )
				res.AddRef();

			ASSERT( res.Data().IsAllResourcesAlive( *this ));
			return &res.Data();
		}
		return null;
	}
	
/*
=================================================
	CacheDescriptorSet
=================================================
*/
	bool  VResourceManager::CacheDescriptorSet (INOUT PipelineResources &desc)
	{
		RawPipelineResourcesID	id = PipelineResourcesHelper::GetCached( desc );
		
		// use cached resources
		if ( id )
		{
			auto&	res = _GetResourcePool( id )[ id.Index() ];

			if ( res.GetInstanceID() == id.InstanceID() )
				return true;
		}
	
		CHECK_ERR( desc.IsInitialized() );
		CHECK_ERR( desc.GetDynamicOffsets().empty() );	// not supported
	
		auto&	layout = _GetResourcePool( desc.GetLayout() )[ desc.GetLayout().Index() ];
		CHECK_ERR( layout.IsCreated() and desc.GetLayout().InstanceID() == layout.GetInstanceID() );

		id = _CreateCachedResource<RawPipelineResourcesID>( "failed when creating descriptor set",
								   [&] (auto& data) { return Replace( data, INOUT desc ); },
								   [&] (auto& data) { if (data.Create( *this )) { layout.AddRef(); return true; }  return false; });

		PipelineResourcesHelper::SetCache( desc, id );
		return true;
	}
	
/*
=================================================
	ReleaseResource
=================================================
*/
	void  VResourceManager::ReleaseResource (INOUT PipelineResources &desc)
	{
		RawPipelineResourcesID	id = PipelineResourcesHelper::GetCached( desc );

		if ( id )
		{
			PipelineResourcesHelper::SetCache( desc, RawPipelineResourcesID{} );
			ReleaseResource( id );
		}
	}

/*
=================================================
	CreateRayTracingGeometry
=================================================
*/
	RawRTGeometryID  VResourceManager::CreateRayTracingGeometry (const RayTracingGeometryDesc &desc, const MemoryDesc &mem, StringView dbgName)
	{
		CHECK_ERR( _device.IsRayTracingEnabled() );

		RawMemoryID					mem_id;
		ResourceBase<VMemoryObj>*	mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, mem, dbgName ));

		RawRTGeometryID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( *this, desc, mem_id, mem_obj->Data(), dbgName ))
		{
			ReleaseResource( mem_id );
			_Unassign( id );
			RETURN_ERR( "failed when creating raytracing geometry" );
		}
		
		mem_obj->AddRef();
		data.AddRef();

		DEBUG_ONLY({
			for (auto& item : desc.triangles) {
				_hashCollisionCheck.Add( item.geometryId );
			}
			for (auto& item : desc.aabbs) {
				_hashCollisionCheck.Add( item.geometryId );
			}
		})
		return id;
	}
	
/*
=================================================
	CreateRayTracingScene
=================================================
*/
	RawRTSceneID  VResourceManager::CreateRayTracingScene (const RayTracingSceneDesc &desc, const MemoryDesc &mem, StringView dbgName)
	{
		CHECK_ERR( _device.IsRayTracingEnabled() );

		RawMemoryID					mem_id;
		ResourceBase<VMemoryObj>*	mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, mem, dbgName ));

		RawRTSceneID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( *this, desc, mem_id, mem_obj->Data(), dbgName ))
		{
			ReleaseResource( mem_id );
			_Unassign( id );
			RETURN_ERR( "failed when creating raytracing scene" );
		}
		
		mem_obj->AddRef();
		data.AddRef();
		return id;
	}
	
/*
=================================================
	CreateRayTracingShaderTable
=================================================
*/
	RawRTShaderTableID  VResourceManager::CreateRayTracingShaderTable (StringView dbgName)
	{
		RawRTShaderTableID	id;
		CHECK_ERR( _Assign( OUT id ));
		
		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( dbgName ) )
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating raytracing shader binding table" );
		}

		data.AddRef();
		return id;
	}
	
/*
=================================================
	CreateSwapchain
=================================================
*/
	RawSwapchainID  VResourceManager::CreateSwapchain (const VulkanSwapchainCreateInfo &desc, RawSwapchainID oldSwapchain,
													   VFrameGraph &fg, StringView dbgName)
	{
		if ( auto* swapchain = GetResource( oldSwapchain, false, true ) )
		{
			if ( not const_cast<VSwapchain*>(swapchain)->Create( fg, desc, dbgName ))
				RETURN_ERR( "failed when recreating swapchain" );

			return oldSwapchain;
		}

		RawSwapchainID	id;
		CHECK_ERR( _Assign( OUT id ));
		
		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( fg, desc, dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating swapchain" );
		}

		data.AddRef();
		return id;
	}
	
/*
=================================================
	CheckTask
=================================================
*/
	void  VResourceManager::CheckTask (const BuildRayTracingScene &task)
	{
		DEBUG_ONLY({
			for (auto& inst : task.instances) {
				_hashCollisionCheck.Add( inst.instanceId );
			}
		})
	}
	
/*
=================================================
	RunValidation
=================================================
*/
	void  VResourceManager::RunValidation (uint maxIter)
	{
		static constexpr uint	scale = MaxCached / 16;

		const auto	UpdateCounter = [] (INOUT Atomic<uint> &counter, uint maxValue) -> uint
		{
			if ( not maxValue )
				return 0;

			uint	expected = 0;
			uint	count	 = 0;
			for (; not counter.compare_exchange_weak( INOUT expected, expected - count, memory_order_relaxed );) {
				count = Min( maxValue, expected * scale );
			}
			return count;
		};

		const auto	UpdateLastIndex = [] (INOUT Atomic<uint> &lastIndex, uint count, uint size)
		{
			uint	new_value = count;
			uint	expected  = 0;
			for (; not lastIndex.compare_exchange_weak( INOUT expected, new_value, memory_order_relaxed );) {
				new_value = expected + count;
				new_value = new_value >= size ? new_value - size : new_value;
				ASSERT( new_value < size );
			}
			return expected;
		};

		const auto	ValidateResources = [this, &UpdateCounter, &UpdateLastIndex, maxIter] (INOUT Atomic<uint> &counter, INOUT Atomic<uint> &lastIndex, auto& pool)
		{
			const uint	max_iter = UpdateCounter( INOUT counter, maxIter );
			if ( max_iter )
			{
				const uint	max_count = uint(pool.size());
				const uint	last_idx  = UpdateLastIndex( INOUT lastIndex, max_iter, max_count );

				for (uint i = 0; i < max_iter; ++i)
				{
					uint	j		= last_idx + i;	j = (j >= max_count ? j - max_count : j);
					Index_t	index	= Index_t(j);

					auto&	res = pool [index];
					if ( res.IsCreated() and not res.Data().IsAllResourcesAlive( *this ) )
					{
						pool.RemoveFromCache( index );
						res.Destroy( *this );
						pool.Unassign( index );
					}
				}
			}
		};

		ValidateResources( _validation.createdPplnResources, _validation.lastCheckedPipelineResource, _pplnResourcesCache );
		ValidateResources( _validation.createdFramebuffers, _validation.lastCheckedFramebuffer, _framebufferCache );
	}
	
/*
=================================================
	_CheckHostVisibleMemory
=================================================
*/
	bool  VResourceManager::_CheckHostVisibleMemory ()
	{
		auto&	dev		= _device;
		auto&	props	= dev.GetDeviceMemoryProperties();
		
		VkMemoryRequirements	transfer_mem_req = {};
		VkMemoryRequirements	uniform_mem_req  = {};
		{
			VkBuffer			id;
			VkBufferCreateInfo	info = {};
			info.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			info.pNext	= null;
			info.flags	= 0;
			info.usage	= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			info.size	= 64 << 10;
			info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
			info.pQueueFamilyIndices	= null;
			info.queueFamilyIndexCount	= 0;

			VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &info, null, OUT &id ));
			dev.vkGetBufferMemoryRequirements( dev.GetVkDevice(), id, OUT &transfer_mem_req );
			dev.vkDestroyBuffer( dev.GetVkDevice(), id, null );
			
			info.size  = 256;
			info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

			VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &info, null, OUT &id ));
			dev.vkGetBufferMemoryRequirements( dev.GetVkDevice(), id, OUT &uniform_mem_req );
			dev.vkDestroyBuffer( dev.GetVkDevice(), id, null );
		}

		BitSet<VK_MAX_MEMORY_HEAPS>		cached_heaps;
		BitSet<VK_MAX_MEMORY_HEAPS>		cocherent_heaps;
		BitSet<VK_MAX_MEMORY_HEAPS>		uniform_heaps;

		for (uint i = 0; i < props.memoryTypeCount; ++i)
		{
			auto&	mt = props.memoryTypes[i];

			if ( EnumEq( transfer_mem_req.memoryTypeBits, 1u << i ))
			{
				if ( EnumEq( mt.propertyFlags, VK_MEMORY_PROPERTY_HOST_CACHED_BIT ))
					cached_heaps[ mt.heapIndex ] = true;
			
				if ( EnumEq( mt.propertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ))
					cocherent_heaps[ mt.heapIndex ] = true;
			}

			if ( EnumEq( uniform_mem_req.memoryTypeBits, 1u << i ) and
				 EnumEq( mt.propertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ))
			{
				uniform_heaps[ mt.heapIndex ] = true;
			}
		}

		BytesU	uniform_heap_size;
		BytesU	transfer_heap_size;

		for (uint i = 0; i < props.memoryHeapCount; ++i)
		{
			if ( uniform_heaps[i] )
				uniform_heap_size += props.memoryHeaps[i].size;

			if ( cached_heaps[i] or cocherent_heaps[i] )
				transfer_heap_size += props.memoryHeaps[i].size;
		}

		BytesU	un_size = uniform_heap_size  / StagingBufferfPool_t::Capacity();
		BytesU	tr_size = transfer_heap_size / StagingBufferfPool_t::Capacity();

		if ( un_size > 128_Mb )		un_size = 32_Mb;	else
		if ( un_size > 64_Mb )		un_size = 16_Mb;	else
									un_size = 8_Mb;

		if ( tr_size > 512_Mb )		tr_size = 256_Mb;	else
		if ( tr_size > 128_Mb )		tr_size = 128_Mb;	else
									tr_size = 64_Mb;

		_staging.writeBufPageSize   = _staging.readBufPageSize = tr_size;
		_staging.uniformBufPageSize = un_size;

		return true;
	}

/*
=================================================
	CreateStagingBuffer
=================================================
*/
	bool  VResourceManager::CreateStagingBuffer (EBufferUsage usage, OUT RawBufferID &outBufferId, OUT StagingBufferIdx &outIndex)
	{
		BufferDesc				desc;
		EMemoryType				mem_type;
		StagingBufferfPool_t*	pool		= null;
		uint					idx_mask	= 0;
		const char *			name		= "";
		desc.usage = usage;

		switch ( usage )
		{
			case EBufferUsage::TransferSrc :
				pool		= &_staging.write;
				desc.size	= _staging.writeBufPageSize;
				mem_type	= EMemoryType::HostWrite;
				idx_mask	= 1u << 30;
				name		= "HostWriteBuffer";
				break;

			case EBufferUsage::TransferDst :
				pool		= &_staging.read;
				desc.size	= _staging.readBufPageSize;
				mem_type	= EMemoryType::HostRead;
				idx_mask	= 2u << 30;
				name		= "HostReadBuffer";
				break;

			case EBufferUsage::Uniform :
				pool		= &_staging.uniform;
				desc.size	= _staging.uniformBufPageSize;
				mem_type	= EMemoryType::HostWrite;
				idx_mask	= 3u << 30;
				name		= "HostUniformBuffer";
				break;

			default :
				RETURN_ERR( "unsupported buffer usage" );
		}

		const auto	ctor = [this, name, mem_type, &desc] (BufferID *ptr, uint)
		{
			RawBufferID	id = CreateBuffer( desc, MemoryDesc{ mem_type }, EQueueFamilyMask::Unknown, name );
			CHECK( id );
			PlacementNew< BufferID >( ptr, id );
		};

		uint	index;
		CHECK_ERR( pool->Assign( OUT index, ctor ));
		
		outBufferId = (*pool)[ index ];

		outIndex = StagingBufferIdx(index | idx_mask);
		return true;
	}
	
/*
=================================================
	ReleaseStagingBuffer
=================================================
*/
	void  VResourceManager::ReleaseStagingBuffer (StagingBufferIdx index)
	{
		const uint	idx = uint(index) & ~(3u << 30);

		switch ( uint(index) >> 30 )
		{
			case 1 :
				_staging.write.Unassign( idx );
				break;

			case 2 :
				_staging.read.Unassign( idx );
				break;
				
			case 3 :
				_staging.uniform.Unassign( idx );
				break;

			default :
				CHECK( !"something goes wrong!" );
				break;
		}
	}
	
/*
=================================================
	_DestroyStagingBuffers
=================================================
*/
	void  VResourceManager::_DestroyStagingBuffers ()
	{
		const auto	dtor = [this] (BufferID &id)
		{
			ReleaseResource( id.Release() );
		};

		_staging.write.Release( dtor );
		_staging.read.Release( dtor );
	}
	
/*
=================================================
	_CreateFindMaxValuePipeline1
=================================================
*/
	bool  VResourceManager::_CreateFindMaxValuePipeline1 ()
	{
		if ( _shaderDbg.pplnFindMaxValue1 )
			return true;

		ComputePipelineDesc		desc;
		desc.AddShader( EShaderLangFormat::VKSL_110, "main", R"#(
layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, std430) readonly buffer un_Timemap
{
	uvec2	maxValue;
	uvec2	dimension;
	uvec2	pixels[];
};

layout(binding = 1, std430) buffer un_MaxValues
{
	uvec2	line[];
};

bool Greater (uvec2 lhs, uvec2 rhs)
{
	return lhs.x == rhs.x ? lhs.y > rhs.y : lhs.x > rhs.x;
}

void main ()
{
	uvec2	mv = uvec2(0);

	for (uint x = 0; x < dimension.x; ++x)
	{
		uint	i = x + dimension.x * gl_GlobalInvocationID.x;
		uvec2	v = pixels[i];

		if ( Greater( v, mv ))
			mv = v;
	}

	line[gl_GlobalInvocationID.x] = mv;
}
)#");
		_shaderDbg.pplnFindMaxValue1 = CPipelineID{ CreatePipeline( desc, Default )};
		CHECK_ERR( _shaderDbg.pplnFindMaxValue1 );

		return true;
	}
	
/*
=================================================
	_CreateFindMaxValuePipeline2
=================================================
*/
	bool  VResourceManager::_CreateFindMaxValuePipeline2 ()
	{
		if ( _shaderDbg.pplnFindMaxValue2 )
			return true;

		ComputePipelineDesc		desc;
		desc.AddShader( EShaderLangFormat::VKSL_110, "main", R"#(
layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, std430) buffer un_Timemap
{
	coherent uvec2	maxValue;
	readonly uvec2	dimension;
	readonly uvec2	pixels[];
};

layout(binding = 1, std430) readonly buffer un_MaxValues
{
	uvec2	line[];
};

bool Greater (uvec2 lhs, uvec2 rhs)
{
	return lhs.x == rhs.x ? lhs.y > rhs.y : lhs.x > rhs.x;
}

void main ()
{
	uvec2	mv = uvec2(0);

	for (uint i = 0; i < dimension.y; ++i)
	{
		uvec2	v = line[i];

		if ( Greater( v, mv ))
			mv = v;
	}

	//if ( gl_LocalInvocationID.x == 0 )
	{
		maxValue = mv;
	}
}
)#");
		_shaderDbg.pplnFindMaxValue2 = CPipelineID{ CreatePipeline( desc, Default )};
		CHECK_ERR( _shaderDbg.pplnFindMaxValue2 );

		return true;
	}

/*
=================================================
	_CreateTimemapRemapPipeline
=================================================
*/
	bool  VResourceManager::_CreateTimemapRemapPipeline ()
	{
		if ( _shaderDbg.pplnRemap )
			return true;

		ComputePipelineDesc		desc;
		desc.AddShader( EShaderLangFormat::VKSL_110, "main", R"#(
layout (local_size_x_id = 0, local_size_y_id = 1, local_size_z = 1) in;

layout(binding = 0, std430) buffer un_Timemap
{
	readonly uvec2	maxValue;
	readonly ivec2	dimension;
	readonly uvec2	pixels[];
};

layout(binding = 1) writeonly uniform image2D  un_OutImage;

double ToDouble (uvec2 v)
{
	return double(v.x) + double(v.y) * double(0xFFFFFFFF);
}

vec3 HSVtoRGB (const vec3 hsv)
{
	// from http://chilliant.blogspot.ru/2014/04/rgbhsv-in-hlsl-5.html
	vec3 col = vec3( abs( hsv.x * 6.0 - 3.0 ) - 1.0,
					 2.0 - abs( hsv.x * 6.0 - 2.0 ),
					 2.0 - abs( hsv.x * 6.0 - 4.0 ) );
	return (( clamp( col, vec3(0.0), vec3(1.0) ) - 1.0 ) * hsv.y + 1.0 ) * hsv.z;
}

float Remap (const vec2 src, const vec2 dst, const float x)
{
	return (x - src.x) / (src.y - src.x) * (dst.y - dst.x) + dst.x;
}

float RemapClamped (const vec2 src, const vec2 dst, const float x)
{
	return clamp( Remap( src, dst, x ), dst.x, dst.y );
}

vec3 Rainbow (float factor)
{
	return HSVtoRGB( vec3(RemapClamped( vec2(0.0, 1.45), vec2(0.0, 1.0), 1.0 - factor ), 1.0, 1.0) );
}

void main ()
{
	double	mv			= ToDouble( maxValue );
	double	vsum		= 0.0;
	ivec2	coord		= ivec2(gl_GlobalInvocationID.xy);
	ivec2	size		= ivec2(gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);
	vec2	ncoord		= vec2(coord) / vec2(size);
	ivec2	px_coord	= ivec2( ncoord * vec2(dimension) );
	ivec2	px_size		= ivec2(1); // vec2(size) / vec2(dimension) + 0.5 );

	for (int y = 0; y < px_size.y; ++y)
	for (int x = 0; x < px_size.x; ++x)
	{
		double	v = ToDouble( pixels[ (px_coord.x + x) + (px_coord.y + y) * dimension.x ] );
		vsum += v;
	}

	vsum /= double(px_size.x * px_size.y);

	imageStore( un_OutImage, coord, vec4(Rainbow( float(vsum / mv) ), 1.0 ));
}
)#");
		_shaderDbg.pplnRemap = CPipelineID{ CreatePipeline( desc, Default )};
		CHECK_ERR( _shaderDbg.pplnRemap );

		return true;
	}
	
/*
=================================================
	_DestroyShaderDebuggerResources
=================================================
*/
	void  VResourceManager::_DestroyShaderDebuggerResources ()
	{
		if ( _shaderDbg.pplnFindMaxValue1 )
			ReleaseResource( _shaderDbg.pplnFindMaxValue1.Release() );

		if ( _shaderDbg.pplnFindMaxValue2 )
			ReleaseResource( _shaderDbg.pplnFindMaxValue2.Release() );

		if ( _shaderDbg.pplnRemap )
			ReleaseResource( _shaderDbg.pplnRemap.Release() );
	
		_shaderDbg.dsLayoutsCache.clear();
	}
	
/*
=================================================
	GetShaderTimemapPipelines
=================================================
*/
	Tuple<RawCPipelineID, RawCPipelineID, RawCPipelineID>  VResourceManager::GetShaderTimemapPipelines ()
	{
		CHECK_ERR( _CreateFindMaxValuePipeline1() );
		CHECK_ERR( _CreateFindMaxValuePipeline2() );
		CHECK_ERR( _CreateTimemapRemapPipeline() );

		return Tuple<RawCPipelineID, RawCPipelineID, RawCPipelineID>{
					_shaderDbg.pplnFindMaxValue1.Get(),
					_shaderDbg.pplnFindMaxValue2.Get(),
					_shaderDbg.pplnRemap.Get() };
	}

}	// FG
