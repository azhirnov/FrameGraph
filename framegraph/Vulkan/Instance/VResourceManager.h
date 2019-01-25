// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphThread.h"
#include "framegraph/Shared/ResourceBase.h"
#include "stl/Memory/LinearAllocator.h"
#include "stl/Containers/ChunkedIndexedPool.h"
#include "stl/Containers/CachedIndexedPool.h"
#include "VBuffer.h"
#include "VImage.h"
#include "VSampler.h"
#include "VMemoryObj.h"
#include "VPipelineCache.h"
#include "VRenderPass.h"
#include "VFramebuffer.h"
#include "VPipelineResources.h"
#include "VRayTracingGeometry.h"
#include "VRayTracingScene.h"

namespace FG
{

	//
	// Resource Manager
	//

	class VResourceManager final
	{
		friend class VResourceManagerThread;

	// types
	private:
		using Index_t		= uint;
		using Lock_t		= std::mutex;

		template <typename T, size_t ChunkSize, size_t MaxChunks>
		using PoolTmpl		= ChunkedIndexedPool< T, Index_t, ChunkSize, MaxChunks, UntypedAlignedAllocator, Lock_t, AtomicPtr >;

		template <typename T, size_t ChunkSize, size_t MaxChunks>
		using CachedPoolTmpl = CachedIndexedPool< T, Index_t, ChunkSize, MaxChunks, UntypedAlignedAllocator, Lock_t, AtomicPtr >;

		template <typename T, size_t MaxSize, template <typename, size_t, size_t> class PoolT>
		using PoolHelper	= PoolT< ResourceBase<T>, MaxSize/16, 16 >;

		using ImagePool_t				= PoolHelper< VImage,				FG_MaxResources,	PoolTmpl >;
		using BufferPool_t				= PoolHelper< VBuffer,				FG_MaxResources,	PoolTmpl >;
		using MemoryPool_t				= PoolHelper< VMemoryObj,			FG_MaxResources*2,	PoolTmpl >;

		using SamplerPool_t				= PoolHelper< VSampler,				FG_MaxResources,	CachedPoolTmpl >;
		using GPipelinePool_t			= PoolHelper< VGraphicsPipeline,	FG_MaxResources,	PoolTmpl >;
		using CPipelinePool_t			= PoolHelper< VComputePipeline,		FG_MaxResources,	PoolTmpl >;
		using MPipelinePool_t			= PoolHelper< VMeshPipeline,		FG_MaxResources,	PoolTmpl >;
		using RTPipelinePool_t			= PoolHelper< VRayTracingPipeline,	FG_MaxResources,	PoolTmpl >;
		using PplnLayoutPool_t			= PoolHelper< VPipelineLayout,		FG_MaxResources,	CachedPoolTmpl >;
		using DescriptorSetLayoutPool_t	= PoolHelper< VDescriptorSetLayout,	FG_MaxResources,	CachedPoolTmpl >;
		using RenderPassPool_t			= PoolHelper< VRenderPass,			FG_MaxResources,	CachedPoolTmpl >;
		using FramebufferPool_t			= PoolHelper< VFramebuffer,			FG_MaxResources,	CachedPoolTmpl >;
		using PipelineResourcesPool_t	= PoolHelper< VPipelineResources,	FG_MaxResources,	CachedPoolTmpl >;
		using RTGeometryPool_t			= PoolHelper< VRayTracingGeometry,	FG_MaxResources,	PoolTmpl >;
		using RTScenePool_t				= PoolHelper< VRayTracingScene,		FG_MaxResources,	PoolTmpl >;

		using VkResourceQueue_t			= Array< UntypedVkResource_t >;
		using ResourceIDQueue_t			= std::vector< Pair<UntypedResourceID_t, bool> >;

		using TimePoint_t				= std::chrono::high_resolution_clock::time_point;


		struct PerFrame
		{
			VkResourceQueue_t		readyToDelete;
		};
		using PerFrameArray_t			= FixedArray< PerFrame, FG_MaxRingBufferSize >;


		struct TaskData
		{
			using TaskFn = std::function< void (VResourceManagerThread &) >;

			//std::atomic_flag	lock  { ATOMIC_FLAG_INIT };
			TaskFn				task;
		};

		struct PoolRanges
		{
			std::atomic<uint64_t>	bits { 0 };		// 0 - require validation, 1 - valid
		};

		enum class ECachedResource
		{
			Begin = 0,
			//Sampler = Begin,
			//PipelineLayout,
			//DescriptorSetLayout,
			//RenderPass,
			Framebuffer = Begin,
			PipelineResources,
			End
		};
		
		using ValidationTasks_t		= FixedArray< TaskData, 128 >;
		using ValidationRanges_t	= StaticArray< PoolRanges, uint(ECachedResource::End) >;


	// variables
	private:
		VDevice const&				_device;

		BufferPool_t				_bufferPool;
		ImagePool_t					_imagePool;
		SamplerPool_t				_samplerCache;
		MemoryPool_t				_memoryObjPool;

		GPipelinePool_t				_graphicsPplnPool;
		CPipelinePool_t				_computePplnPool;
		MPipelinePool_t				_meshPplnPool;
		RTPipelinePool_t			_rayTracingPplnPool;

		PplnLayoutPool_t			_pplnLayoutCache;
		DescriptorSetLayoutPool_t	_dsLayoutCache;
		PipelineResourcesPool_t		_pplnResourcesCache;

		RenderPassPool_t			_renderPassCache;
		FramebufferPool_t			_framebufferCache;

		RTGeometryPool_t			_rtGeometryPool;
		RTScenePool_t				_rtScenePool;

		ResourceIDQueue_t			_unassignIDs;
		PerFrameArray_t				_perFrame;
		uint						_frameId		= 0;

		TimePoint_t					_startTime;
		uint						_currTime		= 0;		// in seconds
		static constexpr uint		_maxTimeDelta	= 60*5;		// in seconds
		uint						_frameCounter	= 0;

		// cached resources validation
		ValidationRanges_t			_validationRanges;
		ECachedResource				_currentValidationPos = ECachedResource::Begin;

		ValidationTasks_t			_validationTasks;
		std::atomic<uint>			_validationTaskPos	= 0;
		
		// dummy resource descriptions
		const BufferDesc			_dummyBufferDesc;
		const ImageDesc				_dummyImageDesc;

		RawDescriptorSetLayoutID	_emptyDSLayout;

		RWRaceConditionCheck		_rcCheck;


	// methods
	public:
		explicit VResourceManager (const VDevice &dev);
		~VResourceManager ();

		bool Initialize (uint ringBufferSize);
		void Deinitialize ();
		
		void OnBeginFrame (uint frameId);
		void OnEndFrame ();

		template <typename ID>
		ND_ auto const*		GetResource (ID id)		const;

		template <typename ID>
		ND_ auto const&		GetDescription (ID id)	const;

		ND_ uint			GetFrameIndex ()		const	{ return _frameCounter; }


	private:
		void  _CreateValidationTasks ();
		void  _DestroyValidationTasks ();
		
		bool  _ProcessValidationTask (VResourceManagerThread &);

		template <typename T, size_t ChunkSize, size_t MaxChunks>
		bool  _AddValidationTasks (const CachedPoolTmpl<T, ChunkSize, MaxChunks> &, INOUT PoolRanges &,
								   void (VResourceManager::*) (VResourceManagerThread &, Index_t, Index_t) const);

		void  _ValidateSamplers (VResourceManagerThread &, Index_t, Index_t) const;
		void  _ValidatePipelineLayouts (VResourceManagerThread &, Index_t, Index_t) const;
		void  _ValidateDescriptorSetLayouts (VResourceManagerThread &, Index_t, Index_t) const;
		void  _ValidateRenderPasses (VResourceManagerThread &, Index_t, Index_t) const;
		void  _ValidateFramebuffers (VResourceManagerThread &, Index_t, Index_t) const;
		void  _ValidatePipelineResources (VResourceManagerThread &, Index_t, Index_t) const;

		void  _DeleteResources (INOUT VkResourceQueue_t &) const;
		void  _UnassignResourceIDs ();

		template <typename DataT, size_t CS, size_t MC, typename ID>
		void _UnassignResource (INOUT PoolTmpl<DataT,CS,MC> &pool, ID id, bool);

		template <typename DataT, size_t CS, size_t MC, typename ID>
		void _UnassignResource (INOUT CachedPoolTmpl<DataT,CS,MC> &pool, ID id, bool force);

		template <typename DataT, size_t CS, size_t MC>
		void _DestroyResourceCache (INOUT CachedPoolTmpl<DataT,CS,MC> &pool);
		
		bool _CreateEmptyDescriptorSetLayout ();

		template <typename ID>
		ND_ bool  _AddToResourceCache (const ID &id);

		ND_ auto&  _GetResourcePool (const RawBufferID &)				{ return _bufferPool; }
		ND_ auto&  _GetResourcePool (const RawImageID &)				{ return _imagePool; }
		ND_ auto&  _GetResourcePool (const RawSamplerID &)				{ return _samplerCache; }
		ND_ auto&  _GetResourcePool (const RawMemoryID &)				{ return _memoryObjPool; }
		ND_ auto&  _GetResourcePool (const RawGPipelineID &)			{ return _graphicsPplnPool; }
		ND_ auto&  _GetResourcePool (const RawCPipelineID &)			{ return _computePplnPool; }
		ND_ auto&  _GetResourcePool (const RawMPipelineID &)			{ return _meshPplnPool; }
		ND_ auto&  _GetResourcePool (const RawRTPipelineID &)			{ return _rayTracingPplnPool; }
		ND_ auto&  _GetResourcePool (const RawPipelineLayoutID &)		{ return _pplnLayoutCache; }
		ND_ auto&  _GetResourcePool (const RawDescriptorSetLayoutID &)	{ return _dsLayoutCache; }
		ND_ auto&  _GetResourcePool (const RawRenderPassID &)			{ return _renderPassCache; }
		ND_ auto&  _GetResourcePool (const RawFramebufferID &)			{ return _framebufferCache; }
		ND_ auto&  _GetResourcePool (const RawPipelineResourcesID &)	{ return _pplnResourcesCache; }
		ND_ auto&  _GetResourcePool (const RawRTGeometryID &)			{ return _rtGeometryPool; }
		ND_ auto&  _GetResourcePool (const RawRTSceneID &)				{ return _rtScenePool; }

		template <typename ID>
		ND_ const auto&  _GetResourceCPool (const ID &id)		const	{ return const_cast<VResourceManager *>(this)->_GetResourcePool( id ); }

		ND_ VkResourceQueue_t&  _GetReadyToDeleteQueue ()				{ return _perFrame[_frameId].readyToDelete; }

		ND_ AppendableResourceIDs_t				_GetResourceIDs ()		{ return AppendableResourceIDs_t{ _unassignIDs, std::integral_constant< decltype(&_AppendResourceID), &_AppendResourceID >{}}; }
		static Pair<UntypedResourceID_t, bool>  _AppendResourceID (UntypedResourceID_t &&id)	{ return { std::move(id), false }; }
	};

	
	
/*
=================================================
	GetResource
=================================================
*/
	template <typename ID>
	inline auto const*  VResourceManager::GetResource (ID id) const
	{
		SHAREDLOCK( _rcCheck );

		auto&	pool = _GetResourceCPool( id );

		using Result_t = typename std::remove_reference_t<decltype(pool)>::Value_t::Resource_t const*;

		if ( id.Index() < pool.size() )
		{
			auto&	data = pool[ id.Index() ];

			if ( data.IsCreated() and data.GetInstanceID() == id.InstanceID() )
				return &data.Data();
			
			ASSERT( data.IsCreated() );
			ASSERT( data.GetInstanceID() == id.InstanceID() );
		}

		ASSERT(false);
		return static_cast< Result_t >(null);
	}
	
/*
=================================================
	GetDescription
=================================================
*/
	template <>
	inline auto const&  VResourceManager::GetDescription (RawBufferID id) const
	{
		auto*	res = GetResource( id );
		return res ? res->Description() : _dummyBufferDesc;
	}

	template <>
	inline auto const&  VResourceManager::GetDescription (RawImageID id) const
	{
		auto*	res = GetResource( id );
		return res ? res->Description() : _dummyImageDesc;
	}

/*
=================================================
	_UnassignResource
=================================================
*/
	template <typename DataT, size_t CS, size_t MC, typename ID>
	inline void  VResourceManager::_UnassignResource (INOUT PoolTmpl<DataT,CS,MC> &pool, ID id, bool)
	{
		SCOPELOCK( _rcCheck );
		
		if ( id.Index() >= pool.size() )
			return;

		// destroy if needed
		auto&	data = pool[ id.Index() ];

		if ( data.GetInstanceID() != id.InstanceID() )
			return;	// this instance is already destroyed

		if ( data.IsCreated() )
			data.Destroy( OUT _GetReadyToDeleteQueue(), OUT _GetResourceIDs() );

		pool.Unassign( id.Index() );
	}
	
/*
=================================================
	_UnassignResource
=================================================
*/
	template <typename DataT, size_t CS, size_t MC, typename ID>
	inline void  VResourceManager::_UnassignResource (INOUT CachedPoolTmpl<DataT,CS,MC> &pool, ID id, bool force)
	{
		SCOPELOCK( _rcCheck );
		
		if ( id.Index() >= pool.size() )
			return;

		// destroy if needed
		auto&	data = pool[ id.Index() ];
		
		if ( data.GetInstanceID() != id.InstanceID() )
			return;	// this instance is already destroyed

		const bool	destroy = data.IsCreated() and (force or data.ReleaseRef( _currTime ));

		if ( not destroy )
			return;	// don't unassign ID

		pool.RemoveFromCache( id.Index() );
		data.Destroy( OUT _GetReadyToDeleteQueue(), OUT _GetResourceIDs() );
		pool.Unassign( id.Index() );
	}
	
/*
=================================================
	_AddToResourceCache
=================================================
*/
	template <typename ID>
	inline bool  VResourceManager::_AddToResourceCache (const ID &id)
	{
		SCOPELOCK( _rcCheck );

		auto&	pool = _GetResourcePool( id );

		return pool.AddToCache( id.Index() ).second;
	}


}	// FG
