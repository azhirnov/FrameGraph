// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VPipelineResources.h"
#include "VResourceManagerThread.h"
#include "VEnumCast.h"
#include "VDevice.h"

namespace FG
{
/*
=================================================
	HashOf (VkDescriptorBufferInfo)
=================================================
*/
	inline HashVal  HashOf (const VkDescriptorBufferInfo &buffer)
	{
		return HashOf( buffer.buffer ) + HashOf( buffer.offset ) + HashOf( buffer.range );
	}
	
/*
=================================================
	HashOf (VkDescriptorImageInfo)
=================================================
*/
	inline HashVal  HashOf (const VkDescriptorImageInfo &image)
	{
		return HashOf( image.sampler ) + HashOf( image.imageView ) + HashOf( image.imageLayout );
	}
	
/*
=================================================
	HashOf (VkBufferView)
=================================================
*
	inline HashVal  HashOf (const VkBufferView &view)
	{
	}
	
/*
=================================================
	HashOf (VkWriteDescriptorSet)
=================================================
*
	inline HashVal  HashOf (const VkWriteDescriptorSet &ds)
	{
    VkStructureType                  sType;
    const void*                      pNext;
    VkDescriptorSet                  dstSet;
    uint32_t                         dstBinding;
    uint32_t                         dstArrayElement;
    uint32_t                         descriptorCount;
    VkDescriptorType                 descriptorType;
    const VkDescriptorImageInfo*     pImageInfo;
    const VkDescriptorBufferInfo*    pBufferInfo;
    const VkBufferView*              pTexelBufferView;

		
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VPipelineResources::~VPipelineResources ()
	{
	}
	
/*
=================================================
	Initialize
=================================================
*/
	void VPipelineResources::Initialize (const PipelineResources &desc)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( GetState() == EState::Initial );

		_layoutId	= DescriptorSetLayoutID{ desc.GetLayout() };
		_resources	= desc.GetData();
		_hash		= Default;

		for (auto& un : _resources)
		{
			_hash << un.hash;
		}
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VPipelineResources::Create (VResourceManagerThread &resMngr, VPipelineCache &pplnCache)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( GetState() == EState::Initial );
		CHECK_ERR( _descriptorSet == VK_NULL_HANDLE );
		
		VDevice const&					dev			= resMngr.GetDevice();
		VDescriptorSetLayout const*		ds_layout	= resMngr.GetResource( _layoutId.Get() );
		
		CHECK_ERR( pplnCache.AllocDescriptorSet( dev, ds_layout->Handle(), OUT _descriptorSet ));

		UpdateDescriptors	update;
		
		for (auto& un : _resources) {
			std::visit( [&] (auto& data) { _AddResource( resMngr, data, update ); }, un.res );
		}
		
		dev.vkUpdateDescriptorSets( dev.GetVkDevice(),
									uint(update.descriptors.size()),
									update.descriptors.data(),
									0, null );

		_OnCreate();
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VPipelineResources::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		SCOPELOCK( _rcCheck );

		// release reference only if descriptor set was created
		if ( _layoutId and _descriptorSet ) {
			unassignIDs.emplace_back( _layoutId.Release() );
		}

		if ( _descriptorSet ) {
			readyToDelete.emplace_back( VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, uint64_t(_descriptorSet) );
		}

		_descriptorSet		= VK_NULL_HANDLE;
		_layoutId			= Default;
		//_descriptorPoolId	= Default;
		_hash				= Default;
		_resources.clear();

		_OnDestroy();
	}
	
/*
=================================================
	IsAllResourcesAlive
=================================================
*/
	bool  VPipelineResources::IsAllResourcesAlive (const VResourceManagerThread &resMngr) const
	{
		ASSERT( IsCreated() );
		SHAREDLOCK( _rcCheck );

		for (auto& un : _resources)
		{
			bool	alive = Visit( un.res,
								[&resMngr] (const PipelineResources::Buffer &buf) {
									return resMngr.IsResourceAlive( buf.bufferId );
								},
								[&resMngr] (const PipelineResources::Image &img) {
									return resMngr.IsResourceAlive( img.imageId );
								},
								[&resMngr] (const PipelineResources::Texture &tex) {
									return resMngr.IsResourceAlive( tex.imageId ) and resMngr.IsResourceAlive( tex.samplerId );
								},
								[&resMngr] (const PipelineResources::Sampler &samp) {
									return resMngr.IsResourceAlive( samp.samplerId );
								},
								[] (const std::monostate &) {
									return true;
								}
							);

			if ( not alive )
				return false;
		}

		return true;
	}

/*
=================================================
	operator ==
=================================================
*/
	bool  VPipelineResources::operator == (const VPipelineResources &rhs) const
	{
		SHAREDLOCK( _rcCheck );
		SHAREDLOCK( rhs._rcCheck );

		if ( _hash				!= rhs._hash		or
			 _layoutId			!= rhs._layoutId	or
			 _resources.size()	!= rhs._resources.size() )
			return false;

		for (size_t i = 0, count = _resources.size(); i < count; ++i)
		{
			const auto&  left	= _resources[i];
			const auto&  right	= rhs._resources[i];

			if ( (left.hash != right.hash) or not (left.res == right.res) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &rm, INOUT PipelineResources::Buffer &buf, INOUT UpdateDescriptors &list)
	{
		VkDescriptorBufferInfo	info = {};
		info.buffer	= rm.GetResource( buf.bufferId )->Handle();
		info.offset	= VkDeviceSize(buf.offset);
		info.range	= VkDeviceSize(buf.size);

		list.buffers.push_back( std::move(info) );


		VkWriteDescriptorSet	wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= null;
		wds.descriptorType	= ((buf.state & EResourceState::_StateMask) == EResourceState::UniformRead) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		wds.descriptorCount	= 1;
		wds.dstArrayElement	= 0;
		wds.dstBinding		= buf.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pBufferInfo		= &list.buffers.back();

		list.descriptors.push_back( std::move(wds) );
		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &rm, INOUT PipelineResources::Image &img, INOUT UpdateDescriptors &list)
	{
		VLocalImage const*	local_img	= rm.GetState( img.imageId );

		VkDescriptorImageInfo	info = {};
		info.imageLayout	= EResourceState_ToImageLayout( img.state );
		info.imageView		= local_img->GetView( rm.GetDevice(), INOUT img.desc );
		info.sampler		= VK_NULL_HANDLE;

		list.images.push_back( std::move(info) );
		

		VkWriteDescriptorSet	wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= null;
		wds.descriptorType	= ((img.state & EResourceState::_StateMask) == EResourceState::InputAttachment) ? VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		wds.descriptorCount	= 1;
		wds.dstArrayElement	= 0;
		wds.dstBinding		= img.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pImageInfo		= &list.images.back();

		list.descriptors.push_back( std::move(wds) );
		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &rm, INOUT PipelineResources::Texture &tex, INOUT UpdateDescriptors &list)
	{
		VLocalImage const*	local_img	= rm.GetState( tex.imageId );

		VkDescriptorImageInfo	info = {};
		info.imageLayout	= EResourceState_ToImageLayout( tex.state );
		info.imageView		= local_img->GetView( rm.GetDevice(), INOUT tex.desc );
		info.sampler		= rm.GetResource( tex.samplerId )->Handle();

		list.images.push_back( std::move(info) );
		

		VkWriteDescriptorSet	wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= null;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		wds.descriptorCount	= 1;
		wds.dstArrayElement	= 0;
		wds.dstBinding		= tex.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pImageInfo		= &list.images.back();

		list.descriptors.push_back( std::move(wds) );
		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &rm, const PipelineResources::Sampler &samp, INOUT UpdateDescriptors &list)
	{
		VkDescriptorImageInfo	info = {};
		info.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.imageView		= VK_NULL_HANDLE;
		info.sampler		= rm.GetResource( samp.samplerId )->Handle();

		list.images.push_back( std::move(info) );
		

		VkWriteDescriptorSet	wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= null;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_SAMPLER;
		wds.descriptorCount	= 1;
		wds.dstArrayElement	= 0;
		wds.dstBinding		= samp.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pImageInfo		= &list.images.back();

		list.descriptors.push_back( std::move(wds) );
		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &, const std::monostate &, INOUT UpdateDescriptors &)
	{
		RETURN_ERR( "uninitialized uniform!" );
	}


}	// FG
