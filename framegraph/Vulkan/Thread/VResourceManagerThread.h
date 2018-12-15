// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VResourceManager.h"
#include "VLocalBuffer.h"
#include "VLocalImage.h"
#include "VLogicalRenderPass.h"
#include "VPipelineCache.h"
#include "VDescriptorManager.h"
#include "VRenderPassCache.h"
#include "VLocalRTGeometry.h"
#include "VLocalRTScene.h"

namespace FG
{

	//
	// Resource Manager Thread
	//

	class VResourceManagerThread final
	{
	// types
	private:
		using Allocator_t		= LinearAllocator<>;
		using Index_t			= VResourceManager::Index_t;

		template <typename T, size_t ChunkSize, size_t MaxChunks>
		using PoolTmpl			= ChunkedIndexedPool< ResourceBase<T>, Index_t, ChunkSize, MaxChunks >;

		template <typename T, size_t MaxSize>
		using PoolTmpl2			= PoolTmpl< T, MaxSize/16, 16 >;

		template <typename Resource>
		struct ItemHash {
			size_t  operator () (const ResourceBase<Resource> *value) const  { return size_t(value->Data().GetHash()); }
		};
		
		template <typename Resource>
		struct ItemEqual {
			bool  operator () (const ResourceBase<Resource> *lhs, const ResourceBase<Resource> *rhs) const  { return (*lhs == *rhs); }
		};

		template <typename ID, typename Resource>
		using CachedResourceMap			= std::unordered_map< ResourceBase<Resource> const*, ID, ItemHash<Resource>, ItemEqual<Resource>,
																StdLinearAllocator<Pair<ResourceBase<Resource> const * const, ID>> >;

		using SamplerMap_t				= CachedResourceMap< RawSamplerID, VSampler >;
		using PipelineLayoutMap_t		= CachedResourceMap< RawPipelineLayoutID, VPipelineLayout >;
		using DescriptorSetLayoutMap_t	= CachedResourceMap< RawDescriptorSetLayoutID, VDescriptorSetLayout >;
		using RenderPassMap_t			= CachedResourceMap< RawRenderPassID, VRenderPass >;
		using FramebufferMap_t			= CachedResourceMap< RawFramebufferID, VFramebuffer >;
		using PipelineResourcesMap_t	= CachedResourceMap< RawPipelineResourcesID, VPipelineResources >;

		using ImageToLocal_t			= std::vector< LocalImageID >;
		using BufferToLocal_t			= std::vector< LocalBufferID >;
		using RTGeometryToLocal_t		= std::vector< LocalRTGeometryID >;
		using RTSceneToLocal_t			= std::vector< LocalRTSceneID >;

		static constexpr uint			MaxLocalResources = FG_MaxResources/4;

		using LocalImages_t				= PoolTmpl2< VLocalImage, MaxLocalResources >;
		using LocalBuffers_t			= PoolTmpl2< VLocalBuffer, MaxLocalResources >;
		using LogicalRenderPasses_t		= PoolTmpl2< VLogicalRenderPass, MaxLocalResources >;
		using LocalRTGeometries_t		= PoolTmpl2< VLocalRTGeometry, MaxLocalResources >;
		using LocalRTScenes_t			= PoolTmpl2< VLocalRTScene, MaxLocalResources >;

		using ResourceIDQueue_t			= std::vector< UntypedResourceID_t, StdLinearAllocator<UntypedResourceID_t> >;
		using ResourceIndexCache_t		= StaticArray< FixedArray<Index_t, 128>, 20 >;


	// variables
	private:
		Allocator_t&						_allocator;
		VResourceManager&					_mainRM;
		VPipelineCache						_pipelineCache;
		VRenderPassCache					_renderPassCache;
		VDescriptorManager					_descriptorMngr;

		ResourceIndexCache_t				_indexCache;

		ImageToLocal_t						_imageToLocal;
		BufferToLocal_t						_bufferToLocal;
		RTGeometryToLocal_t					_rtGeometryToLocal;
		RTSceneToLocal_t					_rtSceneToLocal;

		LocalImages_t						_localImages;
		LocalBuffers_t						_localBuffers;
		LogicalRenderPasses_t				_logicalRenderPasses;
		LocalRTGeometries_t					_localRTGeometries;
		LocalRTScenes_t						_localRTScenes;

		uint								_localImagesCount		= 0;
		uint								_localBuffersCount		= 0;
		uint								_logicalRenderPassCount	= 0;
		uint								_localRTGeometryCount	= 0;
		uint								_localRTSceneCount		= 0;

		Storage<SamplerMap_t>				_samplerMap;
		Storage<PipelineLayoutMap_t>		_pplnLayoutMap;
		Storage<DescriptorSetLayoutMap_t>	_dsLayoutMap;
		Storage<RenderPassMap_t>			_renderPassMap;
		Storage<FramebufferMap_t>			_framebufferMap;
		Storage<PipelineResourcesMap_t>		_pplnResourcesMap;

		Storage<ResourceIDQueue_t>			_unassignIDs;

		RaceConditionCheck					_rcCheck;


	// methods
	public:
		explicit VResourceManagerThread (Allocator_t& alloc, VResourceManager &rm);
		~VResourceManagerThread ();
		
		bool Initialize ();
		void Deinitialize ();

		void OnBeginFrame ();
		void OnEndFrame ();
		void AfterFrameCompilation ();
		void OnDiscardMemory ();
		
		ND_ RawMPipelineID		CreatePipeline (MeshPipelineDesc &&desc, StringView dbgName, bool isAsync);
		ND_ RawGPipelineID		CreatePipeline (GraphicsPipelineDesc &&desc, StringView dbgName, bool isAsync);
		ND_ RawCPipelineID		CreatePipeline (ComputePipelineDesc &&desc, StringView dbgName, bool isAsync);
		ND_ RawRTPipelineID		CreatePipeline (RayTracingPipelineDesc &&desc, StringView dbgName, bool isAsync);
		
		ND_ RawImageID			CreateImage (const ImageDesc &desc, const MemoryDesc &mem, VMemoryManager &alloc, EQueueFamily queueFamily, StringView dbgName, bool isAsync);
		ND_ RawBufferID			CreateBuffer (const BufferDesc &desc, const MemoryDesc &mem, VMemoryManager &alloc, EQueueFamily queueFamily, StringView dbgName, bool isAsync);
		ND_ RawSamplerID		CreateSampler (const SamplerDesc &desc, StringView dbgName, bool isAsync);
		
		ND_ RawImageID			CreateImage (const VulkanImageDesc &desc, FrameGraphThread::OnExternalImageReleased_t &&onRelease, StringView dbgName);
		ND_ RawBufferID			CreateBuffer (const VulkanBufferDesc &desc, FrameGraphThread::OnExternalBufferReleased_t &&onRelease, StringView dbgName);

		ND_ RawRenderPassID		CreateRenderPass (ArrayView<VLogicalRenderPass*> logicalPasses, ArrayView<GraphicsPipelineDesc::FragmentOutput> fragOutput,
												  StringView dbgName, bool isAsync);
		ND_ RawFramebufferID	CreateFramebuffer (ArrayView<Pair<RawImageID, ImageViewDesc>> attachments, RawRenderPassID rp, uint2 dim, uint layers,
												   StringView dbgName, bool isAsync);
		ND_ RawPipelineResourcesID	CreateDescriptorSet (const PipelineResources &desc, bool isAsync);
		
		ND_ RawRTGeometryID		CreateRayTracingGeometry (const RayTracingGeometryDesc &desc, const MemoryDesc &mem, VMemoryManager &alloc, EQueueFamily queueFamily, StringView dbgName, bool isAsync);
		ND_ RawRTSceneID		CreateRayTracingScene (const RayTracingSceneDesc &desc, const MemoryDesc &mem, VMemoryManager &alloc, EQueueFamily queueFamily, StringView dbgName, bool isAsync);

		ND_ LocalBufferID		Remap (RawBufferID id);
		ND_ LocalImageID		Remap (RawImageID id);
		ND_ LocalRTGeometryID	Remap (RawRTGeometryID id);
		ND_ LocalRTSceneID		Remap (RawRTSceneID id);
		ND_ LogicalPassID		CreateLogicalRenderPass (const RenderPassDesc &desc);
		
		template <typename ID>
		ND_ auto const*			GetResource (ID id)			const	{ return _mainRM.GetResource( id ); }

		template <typename ID>
		ND_ bool				IsResourceAlive (ID id)		const;

		ND_ VDevice const&		GetDevice ()				const	{ return _mainRM._device; }

		ND_ VLocalBuffer const*		GetState (LocalBufferID id)		{ return _GetState( _localBuffers, id ); }
		ND_ VLocalImage  const*		GetState (LocalImageID id)		{ return _GetState( _localImages,  id ); }
		ND_ VLogicalRenderPass*		GetState (LogicalPassID id)		{ return _GetState( _logicalRenderPasses, id ); }
		ND_ VLocalRTGeometry const*	GetState (LocalRTGeometryID id)	{ return _GetState( _localRTGeometries, id ); }
		ND_ VLocalRTScene const*	GetState (LocalRTSceneID id)	{ return _GetState( _localRTScenes, id ); }

		ND_ VLocalBuffer const*		GetState (RawBufferID id)		{ return _GetState( _localBuffers, Remap( id )); }
		ND_ VLocalImage  const*		GetState (RawImageID id)		{ return _GetState( _localImages,  Remap( id )); }
		ND_ VLocalRTGeometry const*	GetState (RawRTGeometryID id)	{ return _GetState( _localRTGeometries, Remap( id )); }
		ND_ VLocalRTScene const*	GetState (RawRTSceneID id)		{ return _GetState( _localRTScenes, Remap( id )); }

		template <typename ID>
		void ReleaseResource (ID id, bool isAsync);
		
		void FlushLocalResourceStates (ExeOrderIndex, VBarrierManager &, VFrameGraphDebugger *);

		ND_ VPipelineCache *	GetPipelineCache ()					{ return &_pipelineCache; }
		ND_ VRenderPassCache*	GetRenderPassCache ()				{ return &_renderPassCache; }


	private:
		bool  _CreateMemory (OUT RawMemoryID &id, OUT VMemoryObj* &memPtr, const MemoryDesc &desc, VMemoryManager &alloc, StringView dbgName);

		bool  _CreatePipelineLayout (OUT RawPipelineLayoutID &id, OUT ResourceBase<VPipelineLayout> const* &layoutPtr,
									 PipelineDescription::PipelineLayout &&desc, bool isAsync);

		bool  _CreateDescriptorSetLayout (OUT RawDescriptorSetLayoutID &id, OUT ResourceBase<VDescriptorSetLayout> const* &layoutPtr,
										  const PipelineDescription::UniformMapPtr &uniforms, bool isAsync);

		template <typename DataT, size_t CS, size_t MC, typename ID>
		ND_ DataT*  _GetState (PoolTmpl<DataT,CS,MC> &pool, ID id);

		template <typename DataT, typename ID, typename FnInitialize, typename FnCreate>
		ND_ ID  _CreateCachedResource (Storage<CachedResourceMap<ID, DataT>> &localCache, bool isAsync, StringView errorStr, FnInitialize&& fnInit, FnCreate&& fnCreate);

		template <typename ID, typename ...IDs>
		void  _FreeIndexCache (const Union<ID, IDs...> &);

		template <typename ID>	ND_ auto&  _GetIndexCache ();
		template <typename ID>	ND_ bool   _Assign (OUT ID &id);
		template <typename ID>		void   _Unassign (ID id);
		template <typename ID>	ND_ auto&  _GetResourcePool (const ID &id)		{ return _mainRM._GetResourcePool( id ); }
	};


/*
=================================================
	_GetState
=================================================
*/
	template <typename DataT, size_t CS, size_t MC, typename ID>
	inline DataT*  VResourceManagerThread::_GetState (PoolTmpl<DataT,CS,MC> &pool, ID id)
	{
		ASSERT( id );
		SCOPELOCK( _rcCheck );

		auto&	data = pool[ id.Index() ];
		ASSERT( data.IsCreated() );

		return &data.Data();
	}
	
/*
=================================================
	ReleaseResource
=================================================
*/
	template <typename ID>
	inline void  VResourceManagerThread::ReleaseResource (ID id, bool isAsync)
	{
		ASSERT( id );
		SCOPELOCK( _rcCheck );

		if ( isAsync )
			_unassignIDs->push_back( id );
		else
			_mainRM._UnassignResource( _GetResourcePool( id ), id );
	}
	
/*
=================================================
	_GetIndexCache
=================================================
*/
	template <typename ID>
	inline auto&  VResourceManagerThread::_GetIndexCache ()
	{
		STATIC_ASSERT( ID::GetUID() < std::tuple_size_v<decltype(_indexCache)> );

		return _indexCache[ ID::GetUID() ];
	}
	
/*
=================================================
	_Assign
----
	acquire free index from cache (cache is local in thread),
	if cache empty then acquire new indices from main pool (internaly synchronized),
	if pool is full then return error (false).
=================================================
*/
	template <typename ID>
	inline bool  VResourceManagerThread::_Assign (OUT ID &id)
	{
		auto&	cache	= _GetIndexCache<ID>();
		auto&	pool	= _mainRM._GetResourcePool( id );

		if ( cache.size() )	{}	// TODO: branch prediction optimization
		else {
			// acquire new indices
			CHECK_ERR( pool.Assign( cache.capacity()/2, INOUT cache ) > 0 );
		}

		Index_t		index = cache.back();
		cache.pop_back();

		id = ID( index, pool[index].GetInstanceID() );
		return true;
	}
	
/*
=================================================
	_Unassign
=================================================
*/
	template <typename ID>
	inline void  VResourceManagerThread::_Unassign (ID id)
	{
		ASSERT( id );
		auto&	cache	= _GetIndexCache<ID>();

		if ( cache.size() < cache.capacity() )
		{
			cache.push_back( id.Index() );
		}
		else
		{
			// cache overflow
			_mainRM._GetResourcePool( id ).Unassign( cache.capacity()/2, INOUT cache );
			cache.push_back( id.Index() );
		}
	}
	
/*
=================================================
	IsResourceAlive
=================================================
*/
	template <typename ID>
	inline bool  VResourceManagerThread::IsResourceAlive (ID id) const
	{
		ASSERT( id );
		auto&	res = _mainRM._GetResourceCPool(id)[ id.Index() ];

		return (res.GetInstanceID() == id.InstanceID());
	}


}	// FG
