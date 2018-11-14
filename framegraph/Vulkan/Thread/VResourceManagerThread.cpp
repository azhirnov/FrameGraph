// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VResourceManagerThread.h"
#include "VBarrierManager.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VResourceManagerThread::VResourceManagerThread (Allocator_t& alloc, VResourceManager &rm) :
		_allocator{ alloc },
		_mainRM{ rm }
	{
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

		_imageToLocal.Create( _allocator )->resize( _mainRM._imagePool.size() );
		_bufferToLocal.Create( _allocator )->resize( _mainRM._bufferPool.size() );
		
		_localImages.Create( _allocator );
		_localBuffers.Create( _allocator );
		_logicalRenderPasses.Create( _allocator );

		_samplerMap.Create( _allocator );
		_pplnLayoutMap.Create( _allocator );
		_dsLayoutMap.Create( _allocator );
		_renderPassMap.Create( _allocator );
		_framebufferMap.Create( _allocator );
		_pplnResourcesMap.Create( _allocator );

		_unassignIDs.Create( _allocator )->reserve( 256 );

		// init default ID
		/*{
			Index_t		index;
			CHECK( _localImages.Assign( OUT index ));
			CHECK( index == 0 );

			CHECK( _localBuffers.Assign( OUT index ));
			CHECK( index == 0 );
		}*/

		// TODO
	}
	
/*
=================================================
	OnEndFrame
----
	called when all render threads synchronized
	with main thread
=================================================
*/
	void VResourceManagerThread::OnEndFrame ()
	{
		SCOPELOCK( _rcCheck );

		// merge samplers
		for (auto& sampler : *_samplerMap)
		{
			auto	inserted = _mainRM._samplerCache.AddToCache( sampler.second.Index() );
			if ( not inserted.second )
				DestroyResource( sampler.second, false );
		}

		// merge descriptor set layouts
		for (auto& ds : *_dsLayoutMap)
		{
			auto	inserted = _mainRM._dsLayoutCache.AddToCache( ds.second.Index() );
			if ( not inserted.second )
				DestroyResource( ds.second, false );
		}

		// merge pipeline layouts
		for (auto& layout : *_pplnLayoutMap)
		{
			auto	inserted = _mainRM._pplnLayoutCache.AddToCache( layout.second.Index() );
			if ( not inserted.second )
				DestroyResource( layout.second, false );
		}

		// merge render passes
		for (auto& rp : *_renderPassMap)
		{
			auto	inserted = _mainRM._renderPassCache.AddToCache( rp.second.Index() );
			if ( not inserted.second )
				DestroyResource( rp.second, false );
		}

		// merge framebuffers
		for (auto& fb : *_framebufferMap)
		{
			auto	inserted = _mainRM._framebufferCache.AddToCache( fb.second.Index() );
			if ( not inserted.second )
				DestroyResource( fb.second, false );
		}

		// merge pipeline resources
		for (auto& res : *_pplnResourcesMap)
		{
			auto	inserted = _mainRM._pplnResourcesCache.AddToCache( res.second.Index() );
			if ( not inserted.second )
				DestroyResource( res.second, false );
		}

		// destroy local images
		for (size_t i = 0; i < _localImagesCount; ++i)
		{
			auto&	image = (*_localImages)[ Index_t(i) ];
			if ( not image.IsDestroyed() )
				image.Destroy( OUT _mainRM._GetReadyToDeleteQueue(), OUT *_unassignIDs );
		}
		
		// destroy local buffers
		for (size_t i = 0; i < _localBuffersCount; ++i)
		{
			auto&	buffer = (*_localBuffers)[ Index_t(i) ];
			if ( not buffer.IsDestroyed() )
				buffer.Destroy( OUT _mainRM._GetReadyToDeleteQueue(), OUT *_unassignIDs );
		}

		_pipelineCache.MergePipelines( _mainRM._GetReadyToDeleteQueue() );

		// unassign IDs
        for (auto& vid : *_unassignIDs) {
            std::visit( [this] (auto id) { DestroyResource( id, false ); }, vid );
		}
		
		_imageToLocal.Destroy();
		_bufferToLocal.Destroy();
		_localImages.Destroy();
		_localBuffers.Destroy();
		_logicalRenderPasses.Destroy();
		_samplerMap.Destroy();
		_pplnLayoutMap.Destroy();
		_dsLayoutMap.Destroy();
		_renderPassMap.Destroy();
		_framebufferMap.Destroy();
		_pplnResourcesMap.Destroy();
		_unassignIDs.Destroy();
		
		_localImagesCount	= 0;
		_localBuffersCount	= 0;
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
		
		ASSERT( not _imageToLocal.IsCreated() );
		ASSERT( not _bufferToLocal.IsCreated() );
		ASSERT( not _localImages.IsCreated() );
		ASSERT( not _localBuffers.IsCreated() );
		ASSERT( not _logicalRenderPasses.IsCreated() );
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
	_CreatePipelineLayout
=================================================
*/
	bool  VResourceManagerThread::_CreatePipelineLayout (OUT RawPipelineLayoutID &id, OUT ResourceBase<VPipelineLayout> const* &layoutPtr,
														 PipelineDescription::PipelineLayout &&desc, bool isAsync)
	{
		using DSLayouts_t = FixedArray<Pair< RawDescriptorSetLayoutID, const ResourceBase<VDescriptorSetLayout> *>, FG_MaxDescriptorSets >;
		
		CHECK_ERR( _Assign( OUT id ));
		
		// init pipeline layout create info
		DSLayouts_t  ds_layouts;
		for (auto& ds : desc.descriptorSets)
		{
			RawDescriptorSetLayoutID					ds_id;
			ResourceBase<VDescriptorSetLayout> const*	ds_layout = null;
			CHECK_ERR( _CreateDescriptorSetLayout( OUT ds_id, OUT ds_layout, ds.uniforms, isAsync ));

			ds_layouts.push_back({ ds_id, ds_layout });
		}

		auto&	pool	= _GetResourcePool( id );
		auto&	layout	= pool[ id.Index() ];
		Replace( layout, desc, ds_layouts );


		if ( not isAsync )
		{
			auto	inserted = pool.AddToCache( id.Index() );
		
			// if not unique
			if ( not inserted.second )
			{
				_Unassign( id );
				layoutPtr	= &pool[ inserted.first ];
				id			= RawPipelineLayoutID( inserted.first, layoutPtr->GetInstanceID() );
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
		
			auto	inserted = _pplnLayoutMap->insert_or_assign( &layout, id );
			ASSERT( inserted.second );
		}


		// create new pipeline layout
		if ( not layout.Create( GetDevice() ))
		{
			if ( not isAsync )
				pool.RemoveFromCache( id.Index() );

			_Unassign( id );
			RETURN_ERR( "failed when creating pipeline layout" );
		}

		for (auto& ds : ds_layouts) {
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
			auto	inserted = pool.AddToCache( id.Index() );

			// if not unique
			if ( not inserted.second )
			{
				_Unassign( id );
				layoutPtr	= &pool[ inserted.first ];
				id			= RawDescriptorSetLayoutID( inserted.first, layoutPtr->GetInstanceID() );
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
		
			auto	inserted = _dsLayoutMap->insert_or_assign( &ds_layout, id );
			ASSERT( inserted.second );
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
	RawMPipelineID  VResourceManagerThread::CreatePipeline (MeshPipelineDesc &&desc, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _pipelineCache.CompileShaders( INOUT desc, GetDevice() ));

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
	RawGPipelineID  VResourceManagerThread::CreatePipeline (GraphicsPipelineDesc &&desc, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _pipelineCache.CompileShaders( INOUT desc, GetDevice() ));
		
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
	RawCPipelineID  VResourceManagerThread::CreatePipeline (ComputePipelineDesc &&desc, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _pipelineCache.CompileShader( INOUT desc, GetDevice() ));
		
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
	RawRTPipelineID  VResourceManagerThread::CreatePipeline (RayTracingPipelineDesc &&desc, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _pipelineCache.CompileShaders( INOUT desc, GetDevice() ));
		
		RawPipelineLayoutID						layout_id;
		ResourceBase<VPipelineLayout> const*	layout	= null;
		CHECK_ERR( _CreatePipelineLayout( OUT layout_id, OUT layout, std::move(desc._pipelineLayout), isAsync ));

		RawRTPipelineID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( desc, layout_id, dbgName ) )
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
	bool  VResourceManagerThread::_CreateMemory (OUT RawMemoryID &id, OUT VMemoryObj* &memPtr, const MemoryDesc &desc, VMemoryManager &alloc, StringView dbgName)
	{
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );

		if ( not data.Create( desc, alloc, dbgName ) )
		{
			_Unassign( id );
			RETURN_ERR( "failed when creating memory object" );
		}
		
		memPtr = &data.Data();
		return true;
	}

/*
=================================================
	CreateImage
=================================================
*/
	RawImageID  VResourceManagerThread::CreateImage (const MemoryDesc &mem, const ImageDesc &desc, VMemoryManager &alloc,
													 EQueueFamily queueFamily, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );

		RawMemoryID		mem_id;
		VMemoryObj*		mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, mem, alloc, dbgName ));

		RawImageID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		
		ASSERT( id.InstanceID() == data.GetInstanceID() );
		Replace( data );
		ASSERT( id.InstanceID() == data.GetInstanceID() );

		if ( not data.Create( GetDevice(), desc, mem_id, *mem_obj, queueFamily, dbgName ))
		{
			DestroyResource( mem_id, isAsync );
			_Unassign( id );
			RETURN_ERR( "failed when creating image" );
		}

		ASSERT( id.InstanceID() == data.GetInstanceID() );
		return id;
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	RawBufferID  VResourceManagerThread::CreateBuffer (const MemoryDesc &mem, const BufferDesc &desc, VMemoryManager &alloc,
													   EQueueFamily queueFamily, StringView dbgName, bool isAsync)
	{
		SCOPELOCK( _rcCheck );

		RawMemoryID		mem_id;
		VMemoryObj*		mem_obj	= null;
		CHECK_ERR( _CreateMemory( OUT mem_id, OUT mem_obj, mem, alloc, dbgName ));

		RawBufferID		id;
		CHECK_ERR( _Assign( OUT id ));

		auto&	data = _GetResourcePool( id )[ id.Index() ];
		Replace( data );
		
		if ( not data.Create( GetDevice(), desc, mem_id, *mem_obj, queueFamily, dbgName ))
		{
			DestroyResource( mem_id, isAsync );
			_Unassign( id );
			RETURN_ERR( "failed when creating buffer" );
		}
		
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
		
		return id;
	}

/*
=================================================
	_CreateCachedResource
=================================================
*/
	template <typename DataT, typename ID, typename FnInitialize, typename FnCreate>
	inline ID  VResourceManagerThread::_CreateCachedResource (Storage<CachedResourceMap<ID, DataT>> &localCache, bool isAsync,
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
			auto	inserted = pool.AddToCache( id.Index() );
		
			// if not unique
			if ( not inserted.second )
			{
				auto&	temp = pool[ inserted.first ];
				temp.AddRef();
				_Unassign( id );
				return ID( inserted.first, temp.GetInstanceID() );
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
				return iter->second;
			}

			auto	inserted = localCache->insert_or_assign( &data, id );
			ASSERT( inserted.second );
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
	RawPipelineResourcesID  VResourceManagerThread::CreateDescriptorSet (const PipelineResources &desc, bool isAsync)
	{
		RawPipelineResourcesID	id = PipelineResourcesInitializer::GetCached( desc );
		
		if ( id )
		{
			auto&	res = _GetResourcePool( id )[ id.Index() ];

			if ( res.GetInstanceID() == id.InstanceID() )
			{
				res.AddRef();
				return id;
			}
		}

		RawPipelineResourcesID	result =
			_CreateCachedResource( _pplnResourcesMap, isAsync, "failed when creating descriptor set",
								   [&] (auto& data) { return Replace( data, desc ); },
								   [&] (auto& data) { return data.Create( *this, _pipelineCache ); });

		if ( result )
			PipelineResourcesInitializer::SetCache( desc, result );

		return result;
	}

/*
=================================================
	Remap (BufferID)
=================================================
*/
	LocalBufferID  VResourceManagerThread::Remap (RawBufferID id)
	{
		ASSERT( id );
		SCOPELOCK( _rcCheck );

		auto&	local = (*_bufferToLocal)[ id.Index() ];

		if ( local )
			return local;

		Index_t		index;
		CHECK_ERR( _localBuffers->Assign( OUT index ));

		auto&		data = (*_localBuffers)[ index ];
		CHECK_ERR( data.Create( GetResource( id ) ));

		_localBuffersCount = Max( index+1, _localBuffersCount );

		local = LocalBufferID( index, 0 );
		return local;
	}
	
/*
=================================================
	Remap (ImageID)
=================================================
*/
	LocalImageID  VResourceManagerThread::Remap (RawImageID id)
	{
		ASSERT( id );
		SCOPELOCK( _rcCheck );

		auto&	local = (*_imageToLocal)[ id.Index() ];

		if ( local )
			return local;

		Index_t		index;
		CHECK_ERR( _localImages->Assign( OUT index ));

		auto&		data = (*_localImages)[ index ];
		CHECK_ERR( data.Create( GetResource( id ) ));

		_localImagesCount = Max( index+1, _localImagesCount );

		local = LocalImageID( index, 0 );
		return local;
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
		CHECK_ERR( _logicalRenderPasses->Assign( OUT index ));
		
		auto&		data = (*_logicalRenderPasses)[ index ];
		CHECK_ERR( data.Create( *this, desc ));

		return LogicalPassID( index, 0 );
	}
	
/*
=================================================
	FlushLocalResourceStates
=================================================
*/
	void  VResourceManagerThread::FlushLocalResourceStates (ExeOrderIndex index, VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger)
	{
		for (size_t i = 0; i < _localImagesCount; ++i)
		{
			auto&	image = (*_localImages)[ Index_t(i) ];
			if ( not image.IsDestroyed() )
			{
				image.Data().ResetState( index, barrierMngr, debugger );
			}
		}
		
		for (size_t i = 0; i < _localBuffersCount; ++i)
		{
			auto&	buffer = (*_localBuffers)[ Index_t(i) ];
			if ( not buffer.IsDestroyed() )
			{
				buffer.Data().ResetState( index, barrierMngr, debugger );
			}
		}
	}
	

}	// FG
