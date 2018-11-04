// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphThread.h"
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
#include <shared_mutex>

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
		using Index_t	= uint;

		// shared mutex is much faster than mutex or spinlock
		using Lock_t	= std::shared_mutex;

		template <typename T, size_t ChunkSize, size_t MaxChunks>
		using PoolTmpl		= ChunkedIndexedPool< T, Index_t, ChunkSize, MaxChunks, UntypedAlignedAllocator, Lock_t, AtomicPtr >;

		template <typename T, size_t ChunkSize, size_t MaxChunks>
		using CachedPoolTmpl = CachedIndexedPool< T, Index_t, ChunkSize, MaxChunks, UntypedAlignedAllocator, Lock_t, AtomicPtr >;

		template <typename T, size_t MaxSize, template <typename, size_t, size_t> class PoolT>
		using PoolHelper	= PoolT< T, MaxSize/16, 16 >;

		using ImagePool_t				= PoolHelper< VImage,				FG_MaxImageResources,	PoolTmpl >;
		using BufferPool_t				= PoolHelper< VBuffer,				FG_MaxBufferResources,	PoolTmpl >;
		using MemoryPool_t				= PoolHelper< VMemoryObj,			(FG_MaxImageResources + FG_MaxBufferResources), PoolTmpl >;

		using SamplerPool_t				= PoolHelper< VSampler,				(1u<<10),			CachedPoolTmpl >;
		using GPipelinePool_t			= PoolHelper< VGraphicsPipeline,	FG_MaxPipelines,	PoolTmpl >;
		using CPipelinePool_t			= PoolHelper< VComputePipeline,		FG_MaxPipelines,	PoolTmpl >;
		using MPipelinePool_t			= PoolHelper< VMeshPipeline,		FG_MaxPipelines,	PoolTmpl >;
		using RTPipelinePool_t			= PoolHelper< VRayTracingPipeline,	FG_MaxPipelines,	PoolTmpl >;
		using PplnLayoutPool_t			= PoolHelper< VPipelineLayout,		FG_MaxPipelines,	CachedPoolTmpl >;
		using DescriptorSetLayoutPool_t	= PoolHelper< VDescriptorSetLayout,	FG_MaxPipelines,	CachedPoolTmpl >;
		using RenderPassPool_t			= PoolHelper< VRenderPass,			(1u<<10),			CachedPoolTmpl >;
		using FramebufferPool_t			= PoolHelper< VFramebuffer,			(1u<<10),			CachedPoolTmpl >;
		using PipelineResourcesPool_t	= PoolHelper< VPipelineResources,	(1u<<10),			CachedPoolTmpl >;

		using VkResourceQueue_t			= Array< UntypedVkResource_t >;
		using UnassignIDQueue_t			= Array< UntypedResourceID_t >;

		struct PerFrame
		{
			VkResourceQueue_t		readyToDelete;
		};
		using PerFrameArray_t			= FixedArray< PerFrame, FG_MaxRingBufferSize >;


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

		RenderPassPool_t			_renderPassCache;
		FramebufferPool_t			_framebufferCache;
		PipelineResourcesPool_t		_pplnResourcesCache;
		
		UnassignIDQueue_t			_unassignIDs;
		PerFrameArray_t				_perFrame;
		uint						_frameId		= 0;


	// methods
	public:
		explicit VResourceManager (const VDevice &dev);
		~VResourceManager ();

		bool Initialize (uint ringBufferSize);
		void Deinitialize ();
		
		void OnBeginFrame (uint frameId);
		void OnEndFrame ();

		template <typename ID>
		ND_ auto const*		GetResource (ID id)	const;


	private:
		void  _DeleteResources (INOUT VkResourceQueue_t &) const;
		void  _UnassignResourceIDs ();

		template <typename ID, typename PoolT>
		ND_ static typename PoolT::Value_t*  _GetResource (PoolT &res, ID id);

		template <typename DataT, size_t CS, size_t MC, typename ID>
		void _UnassignResource (INOUT PoolTmpl<DataT,CS,MC> &res, ID id);

		template <typename DataT, size_t CS, size_t MC, typename ID>
		void _UnassignResource (INOUT CachedPoolTmpl<DataT,CS,MC> &res, ID id);

		template <typename DataT, size_t CS, size_t MC>
		void _DestroyResourceCache (INOUT CachedPoolTmpl<DataT,CS,MC> &res);

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

		template <typename ID>
		ND_ const auto&  _GetResourceCPool (const ID &id)		const	{ return const_cast<VResourceManager *>(this)->_GetResourcePool( id ); }

		ND_ VkResourceQueue_t&  _GetReadyToDeleteQueue ()				{ return _perFrame[_frameId].readyToDelete; }
	};

	
	
/*
=================================================
	GetResource
=================================================
*/
	template <typename ID>
	inline auto const*  VResourceManager::GetResource (ID id) const
	{
		ASSERT( id );

		auto&	pool = _GetResourceCPool( id );
		auto*	data = &pool[ id.Index() ];

		ASSERT( data->IsCreated() );
		ASSERT( data->GetInstanceID() == id.InstanceID() );

		return data;
	}
	
/*
=================================================
	_GetResource
=================================================
*/
	template <typename ID, typename PoolT>
	inline typename PoolT::Value_t*  VResourceManager::_GetResource (PoolT &res, ID id)
	{
		ASSERT( id );
		
		auto*	data = &res[ id.Index() ];
		ASSERT( data->GetInstanceID() == id.InstanceID() );
		
		return data;
	}
	
/*
=================================================
	_UnassignResource
=================================================
*/
	template <typename DataT, size_t CS, size_t MC, typename ID>
	inline void  VResourceManager::_UnassignResource (INOUT PoolTmpl<DataT,CS,MC> &res, ID id)
	{
		// destroy if needed
		{
			auto&	data = res[ id.Index() ];
			ASSERT( data.GetInstanceID() == id.InstanceID() );

			if ( data.GetState() != ResourceBase::EState::Initial )
				data.Destroy( OUT _GetReadyToDeleteQueue(), OUT _unassignIDs );
		}

		res.Unassign( id.Index() );
	}
	
/*
=================================================
	_UnassignResource
=================================================
*/
	template <typename DataT, size_t CS, size_t MC, typename ID>
	inline void  VResourceManager::_UnassignResource (INOUT CachedPoolTmpl<DataT,CS,MC> &res, ID id)
	{
		// destroy if needed
		{
			auto&	data	= res[ id.Index() ];

			ASSERT( not data.IsCreated() or data.GetInstanceID() == id.InstanceID() );

			bool	destroy	= (data.IsCreated() and data.ReleaseRef()) or
							   data.GetState() == ResourceBase::EState::ReadyToDelete;

			if ( destroy )
				data.Destroy( OUT _GetReadyToDeleteQueue(), OUT _unassignIDs );
			else
				return;	// don't unassign ID
		}

		res.RemoveFromCache( id.Index() );
		res.Unassign( id.Index() );
	}


}	// FG
