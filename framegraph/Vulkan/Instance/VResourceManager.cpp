// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VResourceManager.h"
#include "VResourceManagerThread.h"
#include "VDevice.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VResourceManager::VResourceManager (const VDevice &dev) :
		_device{ dev }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VResourceManager::~VResourceManager ()
	{
		EXLOCK( _rcCheck );
		ASSERT( _perFrame.empty() );
		//ASSERT( _unassignIDs.empty() );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VResourceManager::Initialize (uint ringBufferSize)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _perFrame.empty() );

		_perFrame.resize( ringBufferSize );

		for (auto& frame : _perFrame)
		{
			frame.readyToDelete.reserve( 256 );
		}

		_CreateEmptyDescriptorSetLayout();

		_startTime = TimePoint_t::clock::now();
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VResourceManager::Deinitialize ()
	{
		EXLOCK( _rcCheck );

		_UnassignResource( _GetResourcePool(_emptyDSLayout), _emptyDSLayout, true );

		_DestroyResourceCache( INOUT _samplerCache );
		_DestroyResourceCache( INOUT _pplnLayoutCache );
		_DestroyResourceCache( INOUT _dsLayoutCache );
		_DestroyResourceCache( INOUT _renderPassCache );
		_DestroyResourceCache( INOUT _framebufferCache );
		_DestroyResourceCache( INOUT _pplnResourcesCache );

		for (auto& frame : _perFrame) {
			_DeleteResources( INOUT frame.readyToDelete );
		}
		_perFrame.clear();
	}

/*
=================================================
	OnBeginFrame
=================================================
*/
	void VResourceManager::OnBeginFrame (uint frameId)
	{
		EXLOCK( _rcCheck );

		_frameId	= frameId;
		_currTime	= uint(std::chrono::duration_cast< std::chrono::seconds >( TimePoint_t::clock::now() - _startTime ).count());
		++_frameCounter;

		_DeleteResources( INOUT _perFrame[_frameId].readyToDelete );

		_CreateValidationTasks();
	}
	
/*
=================================================
	OnEndFrame
=================================================
*/
	void VResourceManager::OnEndFrame ()
	{
		EXLOCK( _rcCheck );

		_UnassignResourceIDs();
		_DestroyValidationTasks();
	}
	
/*
=================================================
	_UnassignResourceIDs
=================================================
*/
	void VResourceManager::_UnassignResourceIDs ()
	{
		ResourceIDQueue_t	temp;

		for (; not _unassignIDs.empty();)
		{
			std::swap( _unassignIDs, temp );

			for (auto& vid : temp)
			{
				Visit( vid.first, [this, force = vid.second] (auto id) { _UnassignResource( _GetResourcePool(id), id, force ); });
			}
			temp.clear();
		}

		if ( temp.capacity() > _unassignIDs.capacity() )
			std::swap( _unassignIDs, temp );
	}
	
/*
=================================================
	_DeleteResources
=================================================
*/
	void VResourceManager::_DeleteResources (INOUT VkResourceQueue_t &readyToDelete) const
	{
		VkDevice	dev = _device.GetVkDevice();
		
		for (auto& pair : readyToDelete)
		{
			switch ( pair.first )
			{
				case VK_OBJECT_TYPE_SEMAPHORE :
					_device.vkDestroySemaphore( dev, VkSemaphore(pair.second), null );
					break;

				case VK_OBJECT_TYPE_FENCE :
					_device.vkDestroyFence( dev, VkFence(pair.second), null );
					break;

				case VK_OBJECT_TYPE_DEVICE_MEMORY :
					_device.vkFreeMemory( dev, VkDeviceMemory(pair.second), null );
					break;

				case VK_OBJECT_TYPE_IMAGE :
					_device.vkDestroyImage( dev, VkImage(pair.second), null );
					break;

				case VK_OBJECT_TYPE_EVENT :
					_device.vkDestroyEvent( dev, VkEvent(pair.second), null );
					break;

				case VK_OBJECT_TYPE_QUERY_POOL :
					_device.vkDestroyQueryPool( dev, VkQueryPool(pair.second), null );
					break;

				case VK_OBJECT_TYPE_BUFFER :
					_device.vkDestroyBuffer( dev, VkBuffer(pair.second), null );
					break;

				case VK_OBJECT_TYPE_BUFFER_VIEW :
					_device.vkDestroyBufferView( dev, VkBufferView(pair.second), null );
					break;

				case VK_OBJECT_TYPE_IMAGE_VIEW :
					_device.vkDestroyImageView( dev, VkImageView(pair.second), null );
					break;

				case VK_OBJECT_TYPE_PIPELINE_LAYOUT :
					_device.vkDestroyPipelineLayout( dev, VkPipelineLayout(pair.second), null );
					break;

				case VK_OBJECT_TYPE_RENDER_PASS :
					_device.vkDestroyRenderPass( dev, VkRenderPass(pair.second), null );
					break;

				case VK_OBJECT_TYPE_PIPELINE :
					_device.vkDestroyPipeline( dev, VkPipeline(pair.second), null );
					break;

				case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT :
					_device.vkDestroyDescriptorSetLayout( dev, VkDescriptorSetLayout(pair.second), null );
					break;

				case VK_OBJECT_TYPE_SAMPLER :
					_device.vkDestroySampler( dev, VkSampler(pair.second), null );
					break;

				case VK_OBJECT_TYPE_DESCRIPTOR_POOL :
					_device.vkDestroyDescriptorPool( dev, VkDescriptorPool(pair.second), null );
					break;

				case VK_OBJECT_TYPE_FRAMEBUFFER :
					_device.vkDestroyFramebuffer( dev, VkFramebuffer(pair.second), null );
					break;

				case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION :
					_device.vkDestroySamplerYcbcrConversion( dev, VkSamplerYcbcrConversion(pair.second), null );
					break;

				case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE :
					_device.vkDestroyDescriptorUpdateTemplate( dev, VkDescriptorUpdateTemplate(pair.second), null );
					break;

				case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV :
					_device.vkDestroyAccelerationStructureNV( dev, VkAccelerationStructureNV(pair.second), null );
					break;

				default :
					FG_LOGE( "resource type is not supported" );
					break;
			}
		}
		readyToDelete.clear();
	}
	
/*
=================================================
	_DestroyResourceCache
=================================================
*/
	template <typename DataT, size_t CS, size_t MC>
	inline void VResourceManager::_DestroyResourceCache (INOUT CachedPoolTmpl<DataT,CS,MC> &res)
	{
		for (size_t i = 0, count = res.size(); i < count; ++i)
		{
			Index_t	id	 = Index_t(i);
			auto&	data = res[id];
				
			if ( data.IsCreated() )
			{
				res.RemoveFromCache( id );
				data.Destroy( OUT _GetReadyToDeleteQueue(), OUT _GetResourceIDs() );
				res.Unassign( id );
			}
		}
	}
	
/*
=================================================
	_AddValidationTasks
=================================================
*/
	template <typename T, size_t ChunkSize, size_t MaxChunks>
	inline bool  VResourceManager::_AddValidationTasks (const CachedPoolTmpl<T, ChunkSize, MaxChunks> &cache, INOUT PoolRanges &ranges,
														void (VResourceManager::*fn) (VResourceManagerThread &, Index_t, Index_t) const)
	{
		constexpr uint		MaxBits	= sizeof(ranges.bits) * 8;
		constexpr Index_t	Step	= (MaxChunks * ChunkSize) / MaxBits;
		const auto			bits	= ranges.bits.load( memory_order_relaxed );

		STATIC_ASSERT( (MaxChunks * ChunkSize) % MaxBits == 0 );


		for (uint i = 0; i < MaxBits; ++i)
		{
			if ( _validationTasks.size() == _validationTasks.capacity() )
				return false;	// array is full

			if ( i*Step >= cache.size() )
			{
				const uint64_t	mask = (1ull << i)-1;
				return (bits & mask) == mask;
			}

			if ( bits & (1ull << i) )
				continue;

			_validationTasks.emplace_back(	[this, i, Step, fn, &ranges] (VResourceManagerThread &rm)
											{
												(this->*fn)( rm, i*Step, i*Step + Step );
												ranges.bits.fetch_or( 1ull << i, memory_order_relaxed );
											});
		}
		return false;
	}

/*
=================================================
	_CreateValidationTasks
=================================================
*/
	void  VResourceManager::_CreateValidationTasks ()
	{
		for (ECachedResource pos = _currentValidationPos; pos < ECachedResource::End;)
		{
			if ( _validationTasks.size() == _validationTasks.capacity() )
				return;	// array is full

			auto&	range		= _validationRanges[ uint(pos) ];
			bool	is_complete	= false;

			ENABLE_ENUM_CHECKS();
			switch ( pos )
			{
				//case ECachedResource::Sampler :
				//	is_complete = _AddValidationTasks( _samplerCache, INOUT range, &VResourceManager::_ValidateSamplers );
				//	break;

				//case ECachedResource::PipelineLayout :
				//	is_complete = _AddValidationTasks( _pplnLayoutCache, INOUT range, &VResourceManager::_ValidatePipelineLayouts );
				//	break;

				//case ECachedResource::DescriptorSetLayout :
				//	is_complete = _AddValidationTasks( _dsLayoutCache, INOUT range, &VResourceManager::_ValidateDescriptorSetLayouts );
				//	break;

				//case ECachedResource::RenderPass :
				//	is_complete = _AddValidationTasks( _renderPassCache, INOUT range, &VResourceManager::_ValidateRenderPasses );
				//	break;

				case ECachedResource::Framebuffer :
					is_complete = _AddValidationTasks( _framebufferCache, INOUT range, &VResourceManager::_ValidateFramebuffers );
					break;

				//case ECachedResource::PipelineResources :
				//	is_complete = _AddValidationTasks( _pplnResourcesCache, INOUT range, &VResourceManager::_ValidatePipelineResources );
				//	break;

				case ECachedResource::End :
					break;
			}
			DISABLE_ENUM_CHECKS();

			pos = ECachedResource(uint(pos) + 1);

			if ( is_complete )
			{
				range.bits.store( 0, memory_order_relaxed );
				_currentValidationPos = (pos == ECachedResource::End ? ECachedResource::Begin : pos);
			}
		}
	}
	
/*
=================================================
	_DestroyValidationTasks
=================================================
*/
	void  VResourceManager::_DestroyValidationTasks ()
	{
		_validationTasks.clear();
		_validationTaskPos.store( 0, memory_order_release );
	}
	
/*
=================================================
	_ProcessValidationTask
=================================================
*/
	bool  VResourceManager::_ProcessValidationTask (VResourceManagerThread &resMngr)
	{
		SHAREDLOCK( _rcCheck );

		if ( _validationTasks.empty() )
			return false;

		uint	expected = 0;

		while ( not _validationTaskPos.compare_exchange_weak( INOUT expected, expected+1, memory_order_release, memory_order_relaxed ))
		{
			if ( expected >= _validationTasks.size() )
				return false;
		}

		_validationTasks[expected].task( resMngr );
		return true;
	}

/*
=================================================
	_ValidateSamplers
----
	find unused samplers
=================================================
*/
	void  VResourceManager::_ValidateSamplers (VResourceManagerThread &resMngr, Index_t first, Index_t last) const
	{
		for (Index_t i = first; i < last; ++i)
		{
			auto&	samp = _samplerCache[i];
			
			if ( not samp.IsCreated() )
				continue;
			
			const bool	is_deprecated	= samp.GetRefCount() <= 1 and (_currTime - samp.GetLastUsage() > _maxTimeDelta);

			if ( is_deprecated )
				resMngr.ReleaseResource( RawSamplerID{ i, samp.GetInstanceID() }, true, true );
		}
	}
	
/*
=================================================
	_ValidatePipelineLayouts
=================================================
*/
	void  VResourceManager::_ValidatePipelineLayouts (VResourceManagerThread &resMngr, Index_t first, Index_t last) const
	{
		for (Index_t i = first; i < last; ++i)
		{
			auto&	layout = _pplnLayoutCache[i];
			
			if ( not layout.IsCreated() )
				continue;
			
			const bool	is_deprecated	= layout.GetRefCount() <= 1 and (_currTime - layout.GetLastUsage() > _maxTimeDelta);
			const bool	is_invalid		= not layout.Data().IsAllResourcesAlive( resMngr );

			if ( is_deprecated or is_invalid )
				resMngr.ReleaseResource( RawPipelineLayoutID{ i, layout.GetInstanceID() }, true, true );
		}
	}
	
/*
=================================================
	_ValidateDescriptorSetLayouts
=================================================
*/
	void  VResourceManager::_ValidateDescriptorSetLayouts (VResourceManagerThread &resMngr, Index_t first, Index_t last) const
	{
		for (Index_t i = first; i < last; ++i)
		{
			auto&	layout = _dsLayoutCache[i];
			
			if ( not layout.IsCreated() )
				continue;
			
			const bool	is_deprecated	= layout.GetRefCount() <= 1 and (_currTime - layout.GetLastUsage() > _maxTimeDelta);

			if ( is_deprecated )
				resMngr.ReleaseResource( RawDescriptorSetLayoutID{ i, layout.GetInstanceID() }, true, true );
		}
	}
	
/*
=================================================
	_ValidateRenderPasses
=================================================
*/
	void  VResourceManager::_ValidateRenderPasses (VResourceManagerThread &resMngr, Index_t first, Index_t last) const
	{
		for (Index_t i = first; i < last; ++i)
		{
			auto&	rp = _renderPassCache[i];
			
			if ( not rp.IsCreated() )
				continue;
			
			const bool	is_deprecated	= rp.GetRefCount() <= 1 and (_currTime - rp.GetLastUsage() > _maxTimeDelta);

			if ( is_deprecated )
				resMngr.ReleaseResource( RawRenderPassID{ i, rp.GetInstanceID() }, true, true );
		}
	}
	
/*
=================================================
	_ValidateFramebuffers
=================================================
*/
	void  VResourceManager::_ValidateFramebuffers (VResourceManagerThread &resMngr, Index_t first, Index_t last) const
	{
		for (Index_t i = first; i < last; ++i)
		{
			auto&	fb = _framebufferCache[i];
			
			if ( not fb.IsCreated() )
				continue;

			const bool	is_deprecated	= fb.GetRefCount() <= 1 and (_currTime - fb.GetLastUsage() > _maxTimeDelta);
			const bool	is_invalid		= not fb.Data().IsAllResourcesAlive( resMngr );

			if ( is_deprecated or is_invalid )
				resMngr.ReleaseResource( RawFramebufferID{ i, fb.GetInstanceID() }, true, true );
		}
	}
	
/*
=================================================
	_ValidatePipelineResources
=================================================
*/
	void  VResourceManager::_ValidatePipelineResources (VResourceManagerThread &resMngr, Index_t first, Index_t last) const
	{
		for (Index_t i = first; i < last; ++i)
		{
			auto&	res = _pplnResourcesCache[i];
			
			if ( not res.IsCreated() )
				continue;

			const bool	is_deprecated	= res.GetRefCount() <= 1 and (_currTime - res.GetLastUsage() > _maxTimeDelta);
			const bool	is_invalid		= not res.Data().IsAllResourcesAlive( resMngr );

			if ( is_deprecated or is_invalid )
				resMngr.ReleaseResource( RawPipelineResourcesID{ i, res.GetInstanceID() }, true, true );
		}
	}
	
/*
=================================================
	_CreateEmptyDescriptorSetLayout
=================================================
*/
	bool  VResourceManager::_CreateEmptyDescriptorSetLayout ()
	{
		auto&		pool = _GetResourcePool( RawDescriptorSetLayoutID{} );
		Index_t		index;
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


}	// FG
