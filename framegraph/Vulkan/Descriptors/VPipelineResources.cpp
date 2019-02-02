// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VPipelineResources::VPipelineResources (const PipelineResources &desc) :
		_allowEmptyResources{ desc.IsEmptyResourcesAllowed() }
	{
		SCOPELOCK( _rcCheck );

		_resources	= desc.GetData();
		_layoutId	= desc.GetLayout();
		_hash		= HashOf( _layoutId );

		for (auto& un : _resources)
		{
			_hash << un.hash;
		}
	}

/*
=================================================
	destructor
=================================================
*/
	VPipelineResources::~VPipelineResources ()
	{
		CHECK( not _descriptorSet );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VPipelineResources::Create (VResourceManagerThread &resMngr)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( not _descriptorSet );

		VDevice const&					dev			= resMngr.GetDevice();
		VDescriptorSetLayout const*		ds_layout	= resMngr.GetResource( _layoutId );
		
		CHECK_ERR( resMngr.GetDescriptorManager()->AllocDescriptorSet( dev, ds_layout->Handle(), OUT _descriptorSet ));

		UpdateDescriptors	update;
		update.descriptors		= update.allocator.Alloc< VkWriteDescriptorSet >( _resources.size() + 1 );
		update.descriptorIndex	= 0;

		for (auto& un : _resources) {
			Visit( un.res, [&](auto& data) { _AddResource( resMngr, data, update ); });
		}
		
		dev.vkUpdateDescriptorSets( dev.GetVkDevice(), update.descriptorIndex, update.descriptors, 0, null );
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
			unassignIDs.emplace_back( _layoutId );
		}

		if ( _descriptorSet ) {
			// TODO
		//	readyToDelete.emplace_back( VK_OBJECT_TYPE_DESCRIPTOR_SET_EXT, uint64_t(_descriptorSet) );
		}

		_descriptorSet		= VK_NULL_HANDLE;
		_layoutId			= Default;
		//_descriptorPoolId	= Default;
		_hash				= Default;
		_resources.clear();
	}
	
/*
=================================================
	IsAllResourcesAlive
=================================================
*/
	bool  VPipelineResources::IsAllResourcesAlive (const VResourceManagerThread &resMngr) const
	{
		SHAREDLOCK( _rcCheck );

		for (auto& un : _resources)
		{
			bool	alive = Visit( un.res,
								[&resMngr] (const PipelineResources::Buffer &buf) {
									return not buf.bufferId or resMngr.IsResourceAlive( buf.bufferId );
								},
								[&resMngr] (const PipelineResources::Image &img) {
									return not img.imageId or resMngr.IsResourceAlive( img.imageId );
								},
								[&resMngr] (const PipelineResources::Texture &tex) {
									return	not (tex.imageId and tex.samplerId) or
											(resMngr.IsResourceAlive( tex.imageId ) and resMngr.IsResourceAlive( tex.samplerId ));
								},
								[&resMngr] (const PipelineResources::Sampler &samp) {
									return not samp.samplerId or resMngr.IsResourceAlive( samp.samplerId );
								},
								[&resMngr] (const PipelineResources::RayTracingScene &rts) {
									return not rts.sceneId or resMngr.IsResourceAlive( rts.sceneId );
								},
								[] (const NullUnion &) {
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
	bool VPipelineResources::_AddResource (VResourceManagerThread &resMngr, INOUT PipelineResources::Buffer &buf, INOUT UpdateDescriptors &list)
	{
		VBuffer const*	buffer = resMngr.GetResource( buf.bufferId );
		
		if ( not buffer ) {
			CHECK( _allowEmptyResources );
			return false;
		}

		auto&	info = *list.allocator.Alloc< VkDescriptorBufferInfo >( 1 );
		info = {};
		info.buffer		= buffer->Handle();
		info.offset		= VkDeviceSize(buf.offset);
		info.range		= VkDeviceSize(buf.size);

		const bool	is_uniform	= ((buf.state & EResourceState::_StateMask) == EResourceState::UniformRead);
		const bool	is_dynamic	= EnumEq( buf.state, EResourceState::_BufferDynamicOffset );

		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= (is_uniform ?
								(is_dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) :
								(is_dynamic ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
		wds.descriptorCount	= 1;
		wds.dstBinding		= buf.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pBufferInfo		= &info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &resMngr, INOUT PipelineResources::Image &img, INOUT UpdateDescriptors &list)
	{
		VLocalImage const*	local_img	= resMngr.ToLocal( img.imageId );
		
		if ( not local_img ) {
			CHECK( _allowEmptyResources );
			return false;
		}

		auto&	info = *list.allocator.Alloc< VkDescriptorImageInfo >( 1 );
		info = {};
		info.imageLayout	= EResourceState_ToImageLayout( img.state, local_img->AspectMask() );
		info.imageView		= local_img->GetView( resMngr.GetDevice(), not img.hasDesc, INOUT img.desc );
		info.sampler		= VK_NULL_HANDLE;
		
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= ((img.state & EResourceState::_StateMask) == EResourceState::InputAttachment) ?
								VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		wds.descriptorCount	= 1;
		wds.dstBinding		= img.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pImageInfo		= &info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &resMngr, INOUT PipelineResources::Texture &tex, INOUT UpdateDescriptors &list)
	{
		VLocalImage const*	local_img	= resMngr.ToLocal( tex.imageId );
		VSampler const*		sampler		= resMngr.GetResource( tex.samplerId );
		
		if ( not local_img or not sampler ) {
			CHECK( _allowEmptyResources );
			return false;
		}

		auto&	info = *list.allocator.Alloc< VkDescriptorImageInfo >( 1 );
		info = {};
		info.imageLayout	= EResourceState_ToImageLayout( tex.state, local_img->AspectMask() );
		info.imageView		= local_img->GetView( resMngr.GetDevice(), not tex.hasDesc, INOUT tex.desc );
		info.sampler		= sampler->Handle();
		
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		wds.descriptorCount	= 1;
		wds.dstBinding		= tex.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pImageInfo		= &info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &resMngr, const PipelineResources::Sampler &samp, INOUT UpdateDescriptors &list)
	{
		VSampler const*	sampler = resMngr.GetResource( samp.samplerId );

		if ( not sampler ) {
			CHECK( _allowEmptyResources );
			return false;
		}

		auto&	info = *list.allocator.Alloc< VkDescriptorImageInfo >( 1 );
		info = {};
		info.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.imageView		= VK_NULL_HANDLE;
		info.sampler		= sampler->Handle();
		
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_SAMPLER;
		wds.descriptorCount	= 1;
		wds.dstBinding		= samp.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pImageInfo		= &info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &resMngr, const PipelineResources::RayTracingScene &rtScene, INOUT UpdateDescriptors &list)
	{
		VRayTracingScene const*	scene = resMngr.GetResource( rtScene.sceneId );

		if ( not scene ) {
			CHECK( _allowEmptyResources );
			return false;
		}

		auto&	tlas = *list.allocator.Alloc<VkAccelerationStructureNV>( 1 );
		tlas = scene->Handle();

		auto& 	top_as = *list.allocator.Alloc<VkWriteDescriptorSetAccelerationStructureNV>( 1 );
		top_as = {};
		top_as.sType						= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
		top_as.accelerationStructureCount	= 1;
		top_as.pAccelerationStructures		= &tlas;
		
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= &top_as;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		wds.descriptorCount	= 1;
		wds.dstBinding		= rtScene.index.VKBinding();
		wds.dstSet			= _descriptorSet;

		return true;
	}

/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &, const NullUnion &, INOUT UpdateDescriptors &)
	{
		//RETURN_ERR( "uninitialized uniform!" );
		return false;
	}


}	// FG
