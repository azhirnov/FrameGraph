// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VResourceManagerThread.h"
#include "VBarrierManager.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VResourceManagerThread::VResourceManagerThread (Allocator_t& alloc, Statistics_t& stat, VResourceManager &rm) :
		_allocator{ alloc },
		_mainRM{ rm },
		_resourceStat{ stat }
	{
		_ResetLocalRemaping();
	}

/*
=================================================
	destructor
=================================================
*/
	VResourceManagerThread::~VResourceManagerThread ()
	{
	}

/*
=================================================
	OnBeginFrame
----
	called when all render threads synchronized
	with main thread
=================================================
*/
	void VResourceManagerThread::OnBeginFrame ()
	{
		SCOPELOCK( _rcCheck );

		_pipelineCache.OnBegin( _allocator );

		_samplerMap.Create( _allocator );
		_pplnLayoutMap.Create( _allocator );
		_dsLayoutMap.Create( _allocator );
		_renderPassMap.Create( _allocator );
		_framebufferMap.Create( _allocator );
		_pplnResourcesMap.Create( _allocator );

		_unassignIDs.Create( _allocator )->reserve( 256 );
	}
	
/*
=================================================
	OnEndFrame
----
	called when all render threads synchronized with main thread
=================================================
*/
	void VResourceManagerThread::OnEndFrame ()
	{
		SCOPELOCK( _rcCheck );

		for (auto& task : _syncTasks) {
			task();
		}
		_syncTasks.clear();

		// merge samplers
		for (auto& sampler : *_samplerMap)
		{
			if ( not _mainRM._AddToResourceCache( sampler.second ) )
				ReleaseResource( sampler.second, false );
		}

		// merge descriptor set layouts
		for (auto& ds : *_dsLayoutMap)
		{
			if ( not _mainRM._AddToResourceCache( ds.second ) )
				ReleaseResource( ds.second, false );
		}

		// merge pipeline layouts
		for (auto& layout : *_pplnLayoutMap)
		{
			if ( not _mainRM._AddToResourceCache( layout.second ) )
				ReleaseResource( layout.second, false );
		}

		// merge render passes
		for (auto& rp : *_renderPassMap)
		{
			if ( not _mainRM._AddToResourceCache( rp.second ) )
				ReleaseResource( rp.second, false );
		}

		// merge framebuffers
		for (auto& fb : *_framebufferMap)
		{
			if ( not _mainRM._AddToResourceCache( fb.second ) )
				ReleaseResource( fb.second, false, true );
		}

		// merge pipeline resources
		for (auto& res : *_pplnResourcesMap)
		{
			if ( not _mainRM._AddToResourceCache( res.second ) )
				ReleaseResource( res.second, false, true );
		}

		_pipelineCache.MergePipelines( _mainRM._GetReadyToDeleteQueue() );

		// unassign IDs
		for (auto& vid : *_unassignIDs) {
			Visit( vid.first, [this, force = vid.second] (auto id) { ReleaseResource( id, false, force ); });
		}
		
		_samplerMap.Destroy();
		_pplnLayoutMap.Destroy();
		_dsLayoutMap.Destroy();
		_renderPassMap.Destroy();
		_framebufferMap.Destroy();
		_pplnResourcesMap.Destroy();
		_unassignIDs.Destroy();
	}
	
/*
=================================================
	AfterFrameCompilation
=================================================
*/
	void VResourceManagerThread::AfterFrameCompilation ()
	{
		// process tasks
		for (uint i = 0; i < 10 and _mainRM._ProcessValidationTask( *this ); ++i)
		{}

		// destroy logical render passes
		for (uint i = 0; i < _logicalRenderPassCount; ++i)
		{
			auto&	rp = _logicalRenderPasses[ Index_t(i) ];

			if ( not rp.IsDestroyed() )
			{
				rp.Destroy( OUT _mainRM._GetReadyToDeleteQueue(), OUT _GetResourceIDs() );
				_logicalRenderPasses.Unassign( Index_t(i) );
			}
		}

		ASSERT( _localImages.empty() );
		ASSERT( _localBuffers.empty() );
		ASSERT( _logicalRenderPasses.empty() );
		ASSERT( _localRTGeometries.empty() );
		ASSERT( _localRTScenes.empty() );
		
		_logicalRenderPassCount	= 0;
	}

/*
=================================================
	OnDiscardMemory
----
	called when pool allocator recycle all allocated memory,
	so we need to free all cached memory
=================================================
*/
	void VResourceManagerThread::OnDiscardMemory ()
	{
		SCOPELOCK( _rcCheck );
		
		ASSERT( not _samplerMap.IsCreated() );
		ASSERT( not _pplnLayoutMap.IsCreated() );
		ASSERT( not _dsLayoutMap.IsCreated() );
		ASSERT( not _renderPassMap.IsCreated() );
		ASSERT( not _framebufferMap.IsCreated() );
		ASSERT( not _pplnResourcesMap.IsCreated() );
		ASSERT( not _unassignIDs.IsCreated() );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool  VResourceManagerThread::Initialize ()
	{
		SCOPELOCK( _rcCheck );

		CHECK_ERR( _pipelineCache.Initialize( GetDevice() ));
		CHECK_ERR( _descriptorMngr.Initialize( GetDevice() ));
		return true;
	}

/*
=================================================
	_FreeIndexCache
=================================================
*/
	template <typename ID, typename ...IDs>
	inline void VResourceManagerThread::_FreeIndexCache (const Union<ID, IDs...> &)
	{
		auto&	cache = _GetIndexCache<ID>();

		_GetResourcePool(ID()).Unassign( cache.size(), INOUT cache );
		ASSERT( cache.empty() );

		if constexpr ( CountOf<IDs...>() > 0 )
			_FreeIndexCache( Union<IDs...>() );
	}

/*
=================================================
	Deinitialize
=================================================
*/
	void  VResourceManagerThread::Deinitialize ()
	{
		SCOPELOCK( _rcCheck );

		_pipelineCache.Deinitialize( GetDevice() );
		_descriptorMngr.Deinitialize( GetDevice() );

		_FreeIndexCache( UntypedResourceID_t() );

		DEBUG_ONLY(
		for (auto& cache : _indexCache) {
			ASSERT( cache.empty() );
		})

		_mainRM.OnEndFrame();
	}

/*
=================================================
	Replace
----
	destroy previous resource instance and construct new instance
=================================================
*/
	template <typename ResType, typename ...Args>
	inline void Replace (INOUT ResourceBase<ResType> &target, Args&& ...args)
	{
		target.Data().~ResType();
		new (&target.Data()) ResType{ std::forward<Args &&>(args)... };
	}
	
/*
=================================================
	ExtendPipelineLayout
=================================================
*/
	RawPipelineLayoutID  VResourceManagerThread::ExtendPipelineLayout (RawPipelineLayoutID baseLayout, RawDescriptorSetLayoutID additionalDSLayout,
																	   uint dsLayoutIndex, const DescriptorSetID &dsID, bool isAsync)
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

			PipelineDescription::DescriptorSet	dst;
			dst.id				= src.first;
			dst.bindingIndex	= src.second.index;
			dst.uniforms		= ds_layout.Data().GetUniforms();

			ASSERT( src.second.index != dsLayoutIndex );

			desc.descriptorSets.push_back( std::move(dst) );
			ds_layouts.push_back({ src.second.layoutId, &ds_layout });
		}

		// append additional descriptor set layout
		{
			auto&	ds_layout	= ds_pool[ additionalDSLayout.Index() ];
			
			PipelineDescription::DescriptorSet	dst;
			dst.id				= dsID;
			dst.bindingIndex	= dsLayoutIndex;
			dst.uniforms		= ds_layout.Data().GetUniforms();
			
			desc.descriptorSets.push_back( std::move(dst) );
			ds_layouts.push_back({ additionalDSLayout, &ds_layout });
		}

		// copy push constant ranges
		desc.pushConstants = origin->GetPushConstants();


		RawPipelineLayoutID						new_layout;
		ResourceBase<VPipelineLayout> const*	layout_ptr = null;
		CHECK_ERR( _CreatePipelineLayout( OUT new_layout, OUT layout_ptr, desc, ds_layouts, isAsync ));

		return new_layout;
	}
	
/*
=================================================
	CreateDescriptorSetLayout
=================================================
*/
	RawDescriptorSetLayoutID  VResourceManagerThread::CreateDescriptorSetLayout (const PipelineDescription::UniformMapPtr &uniforms, bool isAsync)
	{
		ResourceBase<VDescriptorSetLayout> const*	layout_ptr = null;
		RawDescriptorSetLayoutID					result;

		CHECK_ERR( _CreateDescriptorSetLayout( OUT result, OUT layout_ptr, uniforms, isAsync ));

		layout_ptr->AddRef();
		return result;
	}
	
/*
=================================================
	_CreatePipelineLayout
=================================================
*/
	bool  VResourceManagerThread::_CreatePipelineLayout (OUT RawPipelineLayoutID &id, OUT ResourceBase<VPipelineLayout> const* &layoutPtr,
														 PipelineDescription::PipelineLayout &&desc, bool isAsync)
	{
		// init pipeline layout create info
		DSLayouts_t  ds_layouts;
		for (auto& ds : desc.descriptorSets)
		{
			RawDescriptorSetLayoutID					ds_id;
			ResourceBase<VDescriptorSetLayout> const*	ds_layout = null;
			CHECK_ERR( _CreateDescriptorSetLayout( OUT ds_id, OUT ds_layout, ds.uniforms, isAsync ));

			ds_layouts.push_back({ ds_id, ds_layout });
		}
		return _CreatePipelineLayout( OUT id, OUT layoutPtr, desc, ds_layouts, isAsync );
	}
	
	bool  VResourceManagerThread::_CreatePipelineLayout (OUT RawPipelineLayoutID &id, OUT ResourceBase<VPipelineLayout> const* &layoutPtr,
														 const PipelineDescription::PipelineLayout &desc, const DSLayouts_t &dsLayouts, bool isAsync)
	{
		CHECK_ERR( _Assign( OUT id ));
		
		auto*	empty_layout = GetResource( _mainRM._emptyDSLayout );
		CHECK_ERR( empty_layout );

		auto&	pool	= _GetResourcePool( id );
		auto&	layout	= pool[ id.Index() ];
		Replace( layout, desc, dsLayouts );


		if ( not isAsync )
		{
			auto[index, inserted] = pool.AddToCache( id.Index() );
		
			// if not unique
			if ( not inserted )
			{
				_Unassign( id );
				layoutPtr	= &pool[ index ];
				id			= RawPipelineLayoutID( index, layoutPtr->GetInstanceID() );
				layoutPtr->AddRef();
				return true;
			}
		}
		else
		{
			// search in main cache
			const Index_t  temp_id = pool.Find( &layout );

			if ( temp_id < pool.size() )
			{
				_Unassign( id );
				layoutPtr	= &pool[ temp_id ];
				id			= RawPipelineLayoutID( temp_id, layoutPtr->GetInstanceID() );
				return true;
			}
		
			// search in local cache
			auto	iter = _pplnLayoutMap->find( &layout );
			if ( iter != _pplnLayoutMap->end() )
			{
				_Unassign( id );
				id			= iter->second;
				layoutPtr	= iter->first;
				return true;
			}
		
			auto[index, inserted] = _pplnLayoutMap->insert_or_assign( &layout, id );
			ASSERT( inserted );
		}


		// create new pipeline layout
		if ( not layout.Create( GetDevice(), empty_layout->Handle() ))
		{
			if ( not isAsync )
				pool.RemoveFromCache( id.Index() );

			_Unassign( id );
			RETURN_ERR( "failed when creating pipeline layout" );
		}

		for (auto& ds : dsLayouts) {
			ds.second->AddRef();
		}

		layoutPtr = &layout;
		return true;
	}

/*
=================================================
	_CreateDescriptorSetLayout
=================================================
*/
	bool  VResourceManagerThread::_CreateDescriptorSetLayout (OUT RawDescriptorSetLayoutID &id, OUT ResourceBase<VDescriptorSetLayout> const* &layoutPtr,
															  const PipelineDescription::UniformMapPtr &uniforms, bool isAsync)
	{
		CHECK_ERR( _Assign( OUT id ));
		
		// init descriptor set layout create info
		VDescriptorSetLayout::DescriptorBinding_t	binding;		// TODO: use custom allocator

		auto&	pool		= _GetResourcePool( id );
		auto&	ds_layout	= pool[ id.Index() ];
		Replace( ds_layout, uniforms, OUT binding );

		if ( not isAsync )
		{
			auto[index, inserted] = pool.AddToCache( id.Index() );

			// if not unique
			if ( not inserted )
			{
				_Unassign( id );
				layoutPtr	= &pool[ index ];
				id			= RawDescriptorSetLayoutID( index, layoutPtr->GetInstanceID() );
				layoutPtr->AddRef();
				return true;
			}
		}
		else
		{
			// search in main cache
			const Index_t  temp_id = pool.Find( &ds_layout );

			if ( temp_id < pool.size() )
			{
				_Unassign( id );
				layoutPtr	= &pool[ temp_id ];
				id			= RawDescriptorSetLayoutID( temp_id, layoutPtr->GetInstanceID() );
				return true;
			}
		
			// search in local cache
			auto	iter = _dsLayoutMap->find( &ds_layout );
			if ( iter != _dsLayoutMap->end() )
			{
				_Unassign( id );
				id			= iter->second;
				layoutPtr	= iter->first;
				return true;
			}
		
			auto[index, inserted] = _dsLayoutMap->insert_or_assign( &ds_layout, id );
			ASSERT( inserted );
		}

		// create new descriptor set layout
		if ( not ds_layout.Create( GetDevice(), binding ))
		{
			if ( not isAsync )
				pool.RemoveFromCache( id.Index() );

			_Unassign( id );
			RETURN_ERR( "failed when creating descriptor set layout" );
		}

		layoutPtr = &ds_layout;
		return true;
	}

/*
=================================================
	CreatePipeline
=================================================
*/
	RawMPipelineID  VResourceManagerThread::CreatePipeline (INOUT MeshPipelineDesc &desc, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		
		if ( not _pipelineCache.CompileShaders( INOUT desc, GetDevice() ))
			return Default;

		RawPipelineLayoutID						layout_id;
		ResourceBase<VPipelineLayout> const*	layout	= null;
		CHECK_ERR( _CreatePipelineLayout( OUT layout_id, OUT layout, std::move(desc._pipelineLayout), isAsync ));
		
		RawMPipelineID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( desc, layout_id, _pipelineCache.CreateFramentOutput(desc._fragmentOutput), dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating mesh pipeline" );
		}

		layout->AddRef();
		return id;
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	RawGPipelineID  VResourceManagerThread::CreatePipeline (INOUT GraphicsPipelineDesc &desc, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		
		if ( not _pipelineCache.CompileShaders( INOUT desc, GetDevice() ))
			return Default;
		
		RawPipelineLayoutID						layout_id;
		ResourceBase<VPipelineLayout> const*	layout	= null;
		CHECK_ERR( _CreatePipelineLayout( OUT layout_id, OUT layout, std::move(desc._pipelineLayout), isAsync ));

		RawGPipelineID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( desc, layout_id, _pipelineCache.CreateFramentOutput(desc._fragmentOutput), dbgName ))
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating graphics pipeline" );
		}
		
		layout->AddRef();
		return id;
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	RawCPipelineID  VResourceManagerThread::CreatePipeline (INOUT ComputePipelineDesc &desc, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		
		if ( not _pipelineCache.CompileShader( INOUT desc, GetDevice() ))
			return Default;
		
		RawPipelineLayoutID						layout_id;
		ResourceBase<VPipelineLayout> const*	layout	= null;
		CHECK_ERR( _CreatePipelineLayout( OUT layout_id, OUT layout, std::move(desc._pipelineLayout), isAsync ));

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
		return id;
	}
	
/*
=================================================
	CreatePipeline
=================================================
*/
	RawRTPipelineID  VResourceManagerThread::CreatePipeline (INOUT RayTracingPipelineDesc &desc, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		
		if ( not _pipelineCache.CompileShaders( INOUT desc, GetDevice() ))
			return Default;
		
		RawPipelineLayoutID						layout_id;
		ResourceBase<VPipelineLayout> const*	layout	= null;
		CHECK_ERR( _CreatePipelineLayout( OUT layout_id, OUT layout, std::move(desc._pipelineLayout), isAsync ));

		RawRTPipelineID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( *this, desc, layout_id, dbgName ) )
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating ray tracing pipeline" );
		}
		
		layout->AddRef();
		return id;
	}
	
/*
=================================================
	_CreateMemory
=================================================
*/
	bool  VResourceManagerThread::_CreateMemory (OUT RawMemoryID &id, OUT ResourceBase<VMemoryObj>* &memPtr, const MemoryDesc &desc, VMemoryManager &alloc, StringView dbgName)
	{
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( desc, alloc, dbgName ) )
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
	RawImageID  VResourceManagerThread::CreateImage (const ImageDesc &desc, const MemoryDesc &mem, VMemoryManager &alloc,
													 EQueueFamilyMask queueFamilyMask, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );

		RawMemoryID					mem_id;
		ResourceBase<VMemoryObj>*	mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, mem, alloc, dbgName ));

		RawImageID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( GetDevice(), desc, mem_id, mem_obj->Data(), queueFamilyMask, dbgName ))
		{
			ReleaseResource( mem_id, isAsync );
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
	RawBufferID  VResourceManagerThread::CreateBuffer (const BufferDesc &desc, const MemoryDesc &mem, VMemoryManager &alloc,
													   EQueueFamilyMask queueFamilyMask, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		
		RawMemoryID					mem_id;
		ResourceBase<VMemoryObj>*	mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, mem, alloc, dbgName ));

		RawBufferID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), desc, mem_id, mem_obj->Data(), queueFamilyMask, dbgName ))
		{
			ReleaseResource( mem_id, isAsync );
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
	RawImageID  VResourceManagerThread::CreateImage (const VulkanImageDesc &desc, FrameGraphThread::OnExternalImageReleased_t &&onRelease, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		
		RawImageID		id;
		CHECK_ERR( _Assign( OUT id ));
		
		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), desc, dbgName, std::move(onRelease) ))
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
	RawBufferID  VResourceManagerThread::CreateBuffer (const VulkanBufferDesc &desc, FrameGraphThread::OnExternalBufferReleased_t &&onRelease, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		
		RawBufferID		id;
		CHECK_ERR( _Assign( OUT id ));
		
		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), desc, dbgName, std::move(onRelease) ))
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
	template <typename DataT, typename ID, typename FnInitialize, typename FnCreate>
	inline ID  VResourceManagerThread::_CreateCachedResource (InPlace<CachedResourceMap<ID, DataT>> &localCache, bool isAsync,
															  StringView errorStr, FnInitialize&& fnInit, FnCreate&& fnCreate)
	{
		SCOPELOCK( _rcCheck );
		
		ID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	pool	= _GetResourcePool( id );
		auto&	data	= pool[ id.Index() ];
		fnInit( INOUT data );
		
		if ( not isAsync )
		{
			auto[index, inserted] = pool.AddToCache( id.Index() );
		
			// if not unique
			if ( not inserted )
			{
				auto&	temp = pool[ index ];
				temp.AddRef();
				_Unassign( id );
				return ID( index, temp.GetInstanceID() );
			}
		}
		else
		{
			// search in main cache
			const Index_t  temp_id = pool.Find( &data );

			if ( temp_id < pool.size() )
			{
				auto&	temp = pool[ temp_id ];
				temp.AddRef();
				_Unassign( id );
				return ID( temp_id, temp.GetInstanceID() );
			}

			// search in local cache
			auto	iter = localCache->find( &data );
			if ( iter != localCache->end() )
			{
				iter->first->AddRef();
				_Unassign( id );
				return iter->second;
			}

			auto[index, inserted] = localCache->insert_or_assign( &data, id );
			ASSERT( inserted );
		}

		// create new
		if ( not fnCreate( data ) )
		{
			_Unassign( id );
			RETURN_ERR( errorStr );
		}

		data.AddRef();
		return id;
	}

/*
=================================================
	Create***
=================================================
*/
	RawSamplerID  VResourceManagerThread::CreateSampler (const SamplerDesc &desc, StringView dbgName, bool isAsync)
	{
		return _CreateCachedResource( _samplerMap, isAsync, "failed when creating sampler",
									  [&] (auto& data) { return Replace( data, GetDevice(), desc ); },
									  [&] (auto& data) { return data.Create( GetDevice(), dbgName ); });
	}
	
	RawRenderPassID  VResourceManagerThread::CreateRenderPass (ArrayView<VLogicalRenderPass*> logicalPasses, ArrayView<GraphicsPipelineDesc::FragmentOutput> fragOutput,
															   StringView dbgName, bool isAsync)
	{
		return _CreateCachedResource( _renderPassMap, isAsync, "failed when creating render pass",
									  [&] (auto& data) { return Replace( data, logicalPasses, fragOutput ); },
									  [&] (auto& data) { return data.Create( GetDevice(), dbgName ); });
	}
	
	RawFramebufferID  VResourceManagerThread::CreateFramebuffer (ArrayView<Pair<RawImageID, ImageViewDesc>> attachments, RawRenderPassID rp, uint2 dim,
																 uint layers, StringView dbgName, bool isAsync)
	{
		return _CreateCachedResource( _framebufferMap, isAsync, "failed when creating framebuffer",
									  [&] (auto& data) { return Replace( data, attachments, rp, dim, layers ); },
									  [&] (auto& data) { return data.Create( *this, dbgName ); });
	}
	
/*
=================================================
	CreateDescriptorSet
=================================================
*/
	VPipelineResources const*  VResourceManagerThread::CreateDescriptorSet (const PipelineResources &desc, bool isAsync)
	{
		RawPipelineResourcesID	id = PipelineResourcesInitializer::GetCached( desc );
		
		// use cached resources
		if ( id )
		{
			auto&	res = _GetResourcePool( id )[ id.Index() ];

			if ( res.GetInstanceID() == id.InstanceID() )
			{
				res.AddRef();
				return &res.Data();
			}
		}
		
		auto&	layout = _GetResourcePool( desc.GetLayout() )[ desc.GetLayout().Index() ];
		CHECK_ERR( layout.IsCreated() and desc.GetLayout().InstanceID() == layout.GetInstanceID() );

		RawPipelineResourcesID	result =
			_CreateCachedResource( _pplnResourcesMap, isAsync, "failed when creating descriptor set",
								   [&] (auto& data) { return Replace( data, desc ); },
								   [&] (auto& data) { if (data.Create( *this )) { layout.AddRef(); return true; }  return false; });

		PipelineResourcesInitializer::SetCache( desc, result );

		return result ? &_GetResourcePool( result )[ result.Index() ].Data() : nullptr;
	}
	
/*
=================================================
	CreateRayTracingGeometry
=================================================
*/
	RawRTGeometryID  VResourceManagerThread::CreateRayTracingGeometry (const RayTracingGeometryDesc &desc, const MemoryDesc &mem, VMemoryManager &alloc,
																	   StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		
		RawMemoryID					mem_id;
		ResourceBase<VMemoryObj>*	mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, mem, alloc, dbgName ));

		RawRTGeometryID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( GetDevice(), desc, mem_id, mem_obj->Data(), dbgName ))
		{
			ReleaseResource( mem_id, isAsync );
			_Unassign( id );
			RETURN_ERR( "failed when creating raytracing geometry" );
		}
		
		mem_obj->AddRef();
		data.AddRef();
		return id;
	}
	
/*
=================================================
	CreateRayTracingScene
=================================================
*/
	RawRTSceneID  VResourceManagerThread::CreateRayTracingScene (const RayTracingSceneDesc &desc, const MemoryDesc &mem, VMemoryManager &alloc,
																 StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		
		RawMemoryID					mem_id;
		ResourceBase<VMemoryObj>*	mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, mem, alloc, dbgName ));

		RawRTSceneID	id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( GetDevice(), desc, mem_id, mem_obj->Data(), dbgName ))
		{
			ReleaseResource( mem_id, isAsync );
			_Unassign( id );
			RETURN_ERR( "failed when creating raytracing scene" );
		}
		
		mem_obj->AddRef();
		data.AddRef();
		return id;
	}

/*
=================================================
	CreateLogicalRenderPass
=================================================
*/
	LogicalPassID  VResourceManagerThread::CreateLogicalRenderPass (const RenderPassDesc &desc)
	{
		SCOPELOCK( _rcCheck );

		Index_t		index = 0;
		CHECK_ERR( _logicalRenderPasses.Assign( OUT index ));
		
		auto&		data = _logicalRenderPasses[ index ];
		Replace( data );

		if ( not data.Create( *this, desc ) )
		{
			_logicalRenderPasses.Unassign( index );
			RETURN_ERR( "failed when creating logical render pass" );
		}

		_logicalRenderPassCount = Max( index+1, _logicalRenderPassCount );

		return LogicalPassID( index, 0 );
	}
	
/*
=================================================
	FlushLocalResourceStates
=================================================
*/
	void  VResourceManagerThread::FlushLocalResourceStates (ExeOrderIndex index, ExeOrderIndex batchExeOrder,
															VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger)
	{
		// reset state & destroy local images
		for (uint i = 0; i < _localImagesCount; ++i)
		{
			auto&	image = _localImages[ Index_t(i) ];

			if ( not image.IsDestroyed() )
			{
				image.Data().ResetState( index, barrierMngr, debugger );
				image.Destroy( OUT _mainRM._GetReadyToDeleteQueue(), OUT _GetResourceIDs() );
				_localImages.Unassign( Index_t(i) );
			}
		}
		
		// reset state & destroy local buffers
		for (uint i = 0; i < _localBuffersCount; ++i)
		{
			auto&	buffer = _localBuffers[ Index_t(i) ];

			if ( not buffer.IsDestroyed() )
			{
				buffer.Data().ResetState( index, barrierMngr, debugger );
				buffer.Destroy( OUT _mainRM._GetReadyToDeleteQueue(), OUT _GetResourceIDs() );
				_localBuffers.Unassign( Index_t(i) );
			}
		}
	
		// reset state & destroy local ray tracing geometries
		for (uint i = 0; i < _localRTGeometryCount; ++i)
		{
			auto&	geometry = _localRTGeometries[ Index_t(i) ];

			if ( not geometry.IsDestroyed() )
			{
				geometry.Data().ResetState( index, barrierMngr, debugger );
				geometry.Destroy( OUT _mainRM._GetReadyToDeleteQueue(), OUT _GetResourceIDs() );
				_localRTGeometries.Unassign( Index_t(i) );
			}
		}

		// merge & destroy ray tracing scenes
		for (uint i = 0; i < _localRTSceneCount; ++i)
		{
			auto&	scene = _localRTScenes[ Index_t(i) ];

			if ( not scene.IsDestroyed() )
			{
				if ( scene.Data().HasUncommitedChanges() )
				{
					_syncTasks.emplace_back( [this, obj = scene.Data().ToGlobal()] () {
												obj->CommitChanges( this->GetFrameIndex() );
											});
				}
				scene.Data().ResetState( index, barrierMngr, debugger );
				scene.Destroy( OUT _mainRM._GetReadyToDeleteQueue(), OUT _GetResourceIDs(), batchExeOrder, GetFrameIndex() );
				_localRTScenes.Unassign( Index_t(i) );
			}
		}
		
		_ResetLocalRemaping();
	}
	
/*
=================================================
	_ToLocal
=================================================
*/
	template <typename ID, typename T, size_t CS, size_t MC>
	inline T const*  VResourceManagerThread::_ToLocal (ID id, INOUT PoolTmpl<T,CS,MC> &localPool, INOUT GlobalToLocal_t &globalToLocal,
													   INOUT uint &counter, bool incRef, StringView msg) const
	{
		SCOPELOCK( _rcCheck );

		if ( id.Index() >= globalToLocal.size() )
			return null;

		Index_t&	local = globalToLocal[ id.Index() ];

		if ( local != UMax )
		{
			if ( incRef )
				_mainRM._GetResourcePool( id )[ id.Index() ].AddRef();

			return &(localPool[ local ].Data());
		}

		auto*	res  = GetResource( id );
		if ( not res )
			return null;

		CHECK_ERR( localPool.Assign( OUT local ));

		auto&	data = localPool[ local ];
		Replace( data );
		
		if ( not data.Create( res ) )
		{
			localPool.Unassign( local );
			RETURN_ERR( msg );
		}

		if ( incRef )
			_mainRM._GetResourcePool( id )[ id.Index() ].AddRef();

		counter = Max( local+1, counter );
		return &(data.Data());
	}
	
/*
=================================================
	ToLocal
=================================================
*/
	VLocalBuffer const*  VResourceManagerThread::ToLocal (RawBufferID id, bool acquireRef)
	{
		return _ToLocal( id, _localBuffers, _bufferToLocal, _localBuffersCount, acquireRef, "failed when creating local buffer" );
	}

	VLocalImage const*  VResourceManagerThread::ToLocal (RawImageID id, bool acquireRef)
	{
		return _ToLocal( id, _localImages, _imageToLocal, _localImagesCount, acquireRef, "failed when creating local image" );
	}

	VLocalRTGeometry const*  VResourceManagerThread::ToLocal (RawRTGeometryID id, bool acquireRef)
	{
		return _ToLocal( id, _localRTGeometries, _rtGeometryToLocal, _localRTGeometryCount, acquireRef, "failed when creating local ray tracing geometry" );
	}

	VLocalRTScene const*  VResourceManagerThread::ToLocal (RawRTSceneID id, bool acquireRef)
	{
		return _ToLocal( id, _localRTScenes, _rtSceneToLocal, _localRTSceneCount, acquireRef, "failed when creating local ray tracing scene" );
	}
	
/*
=================================================
	_ResetLocalRemaping
=================================================
*/
	void VResourceManagerThread::_ResetLocalRemaping ()
	{
		memset( _imageToLocal.data(), ~0u, size_t(ArraySizeOf(_imageToLocal)) );
		memset( _bufferToLocal.data(), ~0u, size_t(ArraySizeOf(_bufferToLocal)) );
		memset( _rtSceneToLocal.data(), ~0u, size_t(ArraySizeOf(_rtSceneToLocal)) );
		memset( _rtGeometryToLocal.data(), ~0u, size_t(ArraySizeOf(_rtGeometryToLocal)) );

		_localImagesCount		= 0;
		_localBuffersCount		= 0;
		_localRTGeometryCount	= 0;
		_localRTSceneCount		= 0;
	}


}	// FG
