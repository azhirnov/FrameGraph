// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Shared/PipelineResourcesHelper.h"
#include "VPipelineResources.h"
#include "VResourceManagerThread.h"
#include "VEnumCast.h"
#include "VDevice.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VPipelineResources::VPipelineResources (const PipelineResources &desc) :
		_allowEmptyResources{ desc.IsEmptyResourcesAllowed() }
	{
		EXLOCK( _rcCheck );

		_layoutId	= desc.GetLayout();
		_hash		= HashOf( _layoutId );

		/*for (auto& un : _resources)
		{
			_hash << un.hash;
		}*/
	}

/*
=================================================
	destructor
=================================================
*/
	VPipelineResources::~VPipelineResources ()
	{
		CHECK( not _descriptorSet );
		CHECK( not _dataPtr );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VPipelineResources::Create (VResourceManagerThread &resMngr, const PipelineResources &desc)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( not _descriptorSet );

		VDevice const&					dev			= resMngr.GetDevice();
		VDescriptorSetLayout const*		ds_layout	= resMngr.GetResource( _layoutId );
		
		CHECK_ERR( resMngr.GetDescriptorManager()->AllocDescriptorSet( dev, ds_layout->Handle(), OUT _descriptorSet ));

		UpdateDescriptors	update;
		update.descriptors		= update.allocator.Alloc< VkWriteDescriptorSet >( ds_layout->GetMaxIndex() + 1 );
		update.descriptorIndex	= 0;

		_dataPtr = PipelineResourcesHelper::CloneDynamicData( desc );
		_dataPtr->ForEachUniform( [&](auto& data) { _AddResource( resMngr, data, INOUT update ); });
		
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
		EXLOCK( _rcCheck );

		// release reference only if descriptor set was created
		if ( _layoutId and _descriptorSet ) {
			unassignIDs.emplace_back( _layoutId );
		}

		if ( _descriptorSet ) {
			// TODO
		//	readyToDelete.emplace_back( VK_OBJECT_TYPE_DESCRIPTOR_SET_EXT, uint64_t(_descriptorSet) );
		}

		if ( _dataPtr ) {
			_dataPtr->Dealloc();
		}

		_dataPtr			= null;
		_descriptorSet		= VK_NULL_HANDLE;
		_layoutId			= Default;
		//_descriptorPoolId	= Default;
		_hash				= Default;
		//_resources.clear();
	}
	
/*
=================================================
	IsAllResourcesAlive
=================================================
*/
	bool  VPipelineResources::IsAllResourcesAlive (const VResourceManagerThread &resMngr) const
	{
		SHAREDLOCK( _rcCheck );

		struct Visitor
		{
			VResourceManagerThread const&	resMngr;
			bool							alive	= true;
			
			Visitor (const VResourceManagerThread &resMngr) : resMngr{resMngr}
			{}

			void operator () (const PipelineResources::Buffer &buf)
			{
				for (uint i = 0; i < buf.elementCount; ++i)
				{
					auto&	elem = buf.elements[i];
					alive &= not elem.bufferId or resMngr.IsResourceAlive( elem.bufferId );
				}
			}

			void operator () (const PipelineResources::Image &img)
			{
				for (uint i = 0; i < img.elementCount; ++i)
				{
					auto&	elem = img.elements[i];
					alive &= not elem.imageId or resMngr.IsResourceAlive( elem.imageId );
				}
			}

			void operator () (const PipelineResources::Texture &tex)
			{
				for (uint i = 0; i < tex.elementCount; ++i)
				{
					auto&	elem = tex.elements[i];
					alive &= not (elem.imageId and elem.samplerId) or
							 (resMngr.IsResourceAlive( elem.imageId ) and resMngr.IsResourceAlive( elem.samplerId ));
				}
			}

			void operator () (const PipelineResources::Sampler &samp)
			{
				for (uint i = 0; i < samp.elementCount; ++i)
				{
					auto&	elem = samp.elements[i];
					alive &= not elem.samplerId or resMngr.IsResourceAlive( elem.samplerId );
				}
			}

			void operator () (const PipelineResources::RayTracingScene &rts)
			{
				for (uint i = 0; i < rts.elementCount; ++i)
				{
					auto&	elem = rts.elements[i];
					alive &= not elem.sceneId or resMngr.IsResourceAlive( elem.sceneId );
				}
			}
		};

		Visitor	vis{ resMngr };
		ForEachUniform( vis );

		return vis.alive;
	}

/*
=================================================
	operator ==
=================================================
*/
	bool  VPipelineResources::operator == (const VPipelineResources &rhs) const
	{
		/*SHAREDLOCK( _rcCheck );
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
		return true;*/
		return false;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &resMngr, INOUT PipelineResources::Buffer &buf, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorBufferInfo >( buf.elementCount );

		for (uint i = 0; i < buf.elementCount; ++i)
		{
			auto&			elem	= buf.elements[i];
			VBuffer const*	buffer	= resMngr.GetResource( elem.bufferId );		
			CHECK( buffer or _allowEmptyResources );

			if ( not buffer )
				continue;

			info[i].buffer	= buffer->Handle();
			info[i].offset	= VkDeviceSize(elem.offset);
			info[i].range	= VkDeviceSize(elem.size);
		}

		const bool	is_uniform	= ((buf.state & EResourceState::_StateMask) == EResourceState::UniformRead);
		const bool	is_dynamic	= EnumEq( buf.state, EResourceState::_BufferDynamicOffset );

		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= (is_uniform ?
								(is_dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) :
								(is_dynamic ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
		wds.descriptorCount	= buf.elementCount;
		wds.dstBinding		= buf.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pBufferInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &resMngr, INOUT PipelineResources::Image &img, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( img.elementCount );

		for (uint i = 0; i < img.elementCount; ++i)
		{
			auto&				elem		= img.elements[i];
			VLocalImage const*	local_img	= resMngr.ToLocal( elem.imageId );
			CHECK( local_img or _allowEmptyResources );

			if ( not local_img )
				continue;

			info[i].imageLayout	= EResourceState_ToImageLayout( img.state, local_img->AspectMask() );
			info[i].imageView	= local_img->GetView( resMngr.GetDevice(), not elem.hasDesc, INOUT elem.desc );
			info[i].sampler		= VK_NULL_HANDLE;
		}		
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= ((img.state & EResourceState::_StateMask) == EResourceState::InputAttachment) ?
								VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		wds.descriptorCount	= img.elementCount;
		wds.dstBinding		= img.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pImageInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &resMngr, INOUT PipelineResources::Texture &tex, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( tex.elementCount );

		for (uint i = 0; i < tex.elementCount; ++i)
		{
			auto&				elem		= tex.elements[i];
			VLocalImage const*	local_img	= resMngr.ToLocal( elem.imageId );
			VSampler const*		sampler		= resMngr.GetResource( elem.samplerId );
			CHECK( (local_img and sampler) or _allowEmptyResources );

			if ( not (local_img and sampler) )
				continue;

			info[i].imageLayout	= EResourceState_ToImageLayout( tex.state, local_img->AspectMask() );
			info[i].imageView	= local_img->GetView( resMngr.GetDevice(), not elem.hasDesc, INOUT elem.desc );
			info[i].sampler		= sampler->Handle();
		}
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		wds.descriptorCount	= tex.elementCount;
		wds.dstBinding		= tex.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pImageInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &resMngr, const PipelineResources::Sampler &samp, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( samp.elementCount );

		for (uint i = 0; i < samp.elementCount; ++i)
		{
			auto&			elem	= samp.elements[i];
			VSampler const*	sampler = resMngr.GetResource( elem.samplerId );
			CHECK( sampler or _allowEmptyResources );

			if ( not sampler )
				continue;

			info[i].imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			info[i].imageView	= VK_NULL_HANDLE;
			info[i].sampler		= sampler->Handle();
		}
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_SAMPLER;
		wds.descriptorCount	= samp.elementCount;
		wds.dstBinding		= samp.index.VKBinding();
		wds.dstSet			= _descriptorSet;
		wds.pImageInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManagerThread &resMngr, const PipelineResources::RayTracingScene &rtScene, INOUT UpdateDescriptors &list)
	{
		auto*	tlas = list.allocator.Alloc<VkAccelerationStructureNV>( rtScene.elementCount );

		for (uint i = 0; i < rtScene.elementCount; ++i)
		{
			auto&					elem	= rtScene.elements[i];
			VRayTracingScene const*	scene	= resMngr.GetResource( elem.sceneId );
			CHECK( scene or _allowEmptyResources );

			tlas[i] = scene ? scene->Handle() : VK_NULL_HANDLE;
		}

		auto& 	top_as = *list.allocator.Alloc<VkWriteDescriptorSetAccelerationStructureNV>( 1 );
		top_as = {};
		top_as.sType						= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
		top_as.accelerationStructureCount	= rtScene.elementCount;
		top_as.pAccelerationStructures		= tlas;
		
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext			= &top_as;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		wds.descriptorCount	= rtScene.elementCount;
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
		CHECK( _allowEmptyResources and "uninitialized uniform!" );
		return false;
	}


}	// FG
