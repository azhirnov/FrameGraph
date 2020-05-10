// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Shared/PipelineResourcesHelper.h"
#include "VPipelineResources.h"
#include "VResourceManager.h"
#include "VDevice.h"
#include "VEnumCast.h"

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
		EXLOCK( _drCheck );
		
		_dataPtr	= PipelineResourcesHelper::CloneDynamicData( desc );
		_layoutId	= desc.GetLayout();
		_hash		= HashOf( _layoutId ) + _dataPtr->CalcHash();
	}
	
/*
=================================================
	constructor
=================================================
*/
	VPipelineResources::VPipelineResources (INOUT PipelineResources &desc) :
		_dataPtr{ PipelineResourcesHelper::RemoveDynamicData( INOUT desc )},
		_allowEmptyResources{ desc.IsEmptyResourcesAllowed() }
	{
		EXLOCK( _drCheck );
		
		_layoutId	= _dataPtr->layoutId;
		_hash		= HashOf( _layoutId ) + _dataPtr->CalcHash();
	}

/*
=================================================
	destructor
=================================================
*/
	VPipelineResources::~VPipelineResources ()
	{
		CHECK( not _descriptorSet.first );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VPipelineResources::Create (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _descriptorSet.first );
		CHECK_ERR( _dataPtr );

		VDevice const&	dev			= resMngr.GetDevice();
		auto const*		ds_layout	= resMngr.GetResource( _layoutId );
		
		CHECK_ERR( ds_layout );
		CHECK_ERR( ds_layout->AllocDescriptorSet( resMngr, OUT _descriptorSet ));
		
		UpdateDescriptors	update;
		update.descriptors		= update.allocator.Alloc< VkWriteDescriptorSet >( ds_layout->GetMaxIndex() + 1 );
		update.descriptorIndex	= 0;

		_dataPtr->ForEachUniform( [&](auto&, auto& data) { _AddResource( resMngr, data, INOUT update ); });
		
		dev.vkUpdateDescriptorSets( dev.GetVkDevice(), update.descriptorIndex, update.descriptors, 0, null );
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VPipelineResources::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		auto*	ds_layout = resMngr.GetResource( _layoutId, false, true );

		if ( ds_layout and _descriptorSet.first ) {
			ds_layout->ReleaseDescriptorSet( resMngr, _descriptorSet );
		}

		// release reference only if descriptor set was created
		if ( _layoutId and _descriptorSet.first ) {
			resMngr.ReleaseResource( _layoutId );
		}

		_dataPtr.reset();
		_descriptorSet	= { VK_NULL_HANDLE, UMax };
		_layoutId		= Default;
		_hash			= Default;
	}
	
/*
=================================================
	IsAllResourcesAlive
=================================================
*/
	bool  VPipelineResources::IsAllResourcesAlive (const VResourceManager &resMngr) const
	{
		SHAREDLOCK( _drCheck );

		struct Visitor
		{
			VResourceManager const&	resMngr;
			bool					alive	= true;
			
			Visitor (const VResourceManager &resMngr) : resMngr{resMngr}
			{}

			void operator () (const UniformID &, const PipelineResources::Buffer &buf)
			{
				for (uint i = 0; i < buf.elementCount; ++i)
				{
					auto&	elem = buf.elements[i];
					alive &= not elem.bufferId or resMngr.IsResourceAlive( elem.bufferId );
				}
			}
			
			void operator () (const UniformID &, const PipelineResources::TexelBuffer &texbuf)
			{
				for (uint i = 0; i < texbuf.elementCount; ++i)
				{
					auto&	elem = texbuf.elements[i];
					alive &= not elem.bufferId or resMngr.IsResourceAlive( elem.bufferId );
				}
			}

			void operator () (const UniformID &, const PipelineResources::Image &img)
			{
				for (uint i = 0; i < img.elementCount; ++i)
				{
					auto&	elem = img.elements[i];
					alive &= not elem.imageId or resMngr.IsResourceAlive( elem.imageId );
				}
			}

			void operator () (const UniformID &, const PipelineResources::Texture &tex)
			{
				for (uint i = 0; i < tex.elementCount; ++i)
				{
					auto&	elem = tex.elements[i];
					alive &= not (elem.imageId and elem.samplerId) or
							 (resMngr.IsResourceAlive( elem.imageId ) and resMngr.IsResourceAlive( elem.samplerId ));
				}
			}

			void operator () (const UniformID &, const PipelineResources::Sampler &samp)
			{
				for (uint i = 0; i < samp.elementCount; ++i)
				{
					auto&	elem = samp.elements[i];
					alive &= not elem.samplerId or resMngr.IsResourceAlive( elem.samplerId );
				}
			}

			void operator () (const UniformID &, const PipelineResources::RayTracingScene &rts)
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
		SHAREDLOCK( _drCheck );
		SHAREDLOCK( rhs._drCheck );

		return	_hash		==  rhs._hash		and
				_layoutId	==  rhs._layoutId	and
				_dataPtr	and rhs._dataPtr	and
				*_dataPtr	== *rhs._dataPtr;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManager &resMngr, INOUT PipelineResources::Buffer &buf, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorBufferInfo >( buf.elementCount );

		for (uint i = 0; i < buf.elementCount; ++i)
		{
			auto&			elem	= buf.elements[i];
			VBuffer const*	buffer	= resMngr.GetResource( elem.bufferId, false, _allowEmptyResources );
			CHECK( buffer or _allowEmptyResources );

			if ( not buffer )
				return false;

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
		wds.dstSet			= _descriptorSet.first;
		wds.pBufferInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManager &resMngr, INOUT PipelineResources::TexelBuffer &texbuf, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkBufferView >( texbuf.elementCount );

		for (uint i = 0; i < texbuf.elementCount; ++i)
		{
			auto&			elem	= texbuf.elements[i];
			VBuffer const*	buffer	= resMngr.GetResource( elem.bufferId, false, _allowEmptyResources );
			CHECK( buffer or _allowEmptyResources );
			
			if ( not buffer )
				return false;

			info[i] = buffer->GetView( resMngr.GetDevice(), elem.desc );
		}
		
		const bool	is_uniform	= ((texbuf.state & EResourceState::_StateMask) == EResourceState::UniformRead);

		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType		= is_uniform ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		wds.descriptorCount		= texbuf.elementCount;
		wds.dstBinding			= texbuf.index.VKBinding();
		wds.dstSet				= _descriptorSet.first;
		wds.pTexelBufferView	= info;

		return true;
	}

/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManager &resMngr, INOUT PipelineResources::Image &img, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( img.elementCount );

		for (uint i = 0; i < img.elementCount; ++i)
		{
			auto&			elem	= img.elements[i];
			VImage const*	img_res	= resMngr.GetResource( elem.imageId, false, _allowEmptyResources );
			CHECK( img_res or _allowEmptyResources );
			
			if ( not img_res )
				return false;

			info[i].imageLayout	= EResourceState_ToImageLayout( img.state, img_res->AspectMask() );
			info[i].imageView	= img_res->GetView( resMngr.GetDevice(), not elem.hasDesc, INOUT elem.desc );
			info[i].sampler		= VK_NULL_HANDLE;
		}		
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= ((img.state & EResourceState::_StateMask) == EResourceState::InputAttachment) ?
								VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		wds.descriptorCount	= img.elementCount;
		wds.dstBinding		= img.index.VKBinding();
		wds.dstSet			= _descriptorSet.first;
		wds.pImageInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManager &resMngr, INOUT PipelineResources::Texture &tex, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( tex.elementCount );

		for (uint i = 0; i < tex.elementCount; ++i)
		{
			auto&			elem	= tex.elements[i];
			VImage const*	img_res	= resMngr.GetResource( elem.imageId, false, _allowEmptyResources );
			VSampler const*	sampler	= resMngr.GetResource( elem.samplerId, false, _allowEmptyResources );
			CHECK( (img_res and sampler) or _allowEmptyResources );

			if ( not (img_res and sampler) )
				return false;

			info[i].imageLayout	= EResourceState_ToImageLayout( tex.state, img_res->AspectMask() );
			info[i].imageView	= img_res->GetView( resMngr.GetDevice(), not elem.hasDesc, INOUT elem.desc );
			info[i].sampler		= sampler->Handle();
		}
		
		VkWriteDescriptorSet&	wds = list.descriptors[list.descriptorIndex++];
		wds = {};
		wds.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		wds.descriptorCount	= tex.elementCount;
		wds.dstBinding		= tex.index.VKBinding();
		wds.dstSet			= _descriptorSet.first;
		wds.pImageInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManager &resMngr, const PipelineResources::Sampler &samp, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( samp.elementCount );

		for (uint i = 0; i < samp.elementCount; ++i)
		{
			auto&			elem	= samp.elements[i];
			VSampler const*	sampler = resMngr.GetResource( elem.samplerId, false, _allowEmptyResources );
			CHECK( sampler or _allowEmptyResources );
			
			if ( not sampler )
				return false;

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
		wds.dstSet			= _descriptorSet.first;
		wds.pImageInfo		= info;

		return true;
	}
	
/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManager &resMngr, const PipelineResources::RayTracingScene &rtScene, INOUT UpdateDescriptors &list)
	{
		auto*	tlas = list.allocator.Alloc<VkAccelerationStructureNV>( rtScene.elementCount );

		for (uint i = 0; i < rtScene.elementCount; ++i)
		{
			auto&					elem	= rtScene.elements[i];
			VRayTracingScene const*	scene	= resMngr.GetResource( elem.sceneId, false, _allowEmptyResources );
			CHECK( scene or _allowEmptyResources );

			if ( not scene )
				return false;

			tlas[i] = scene->Handle();
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
		wds.dstSet			= _descriptorSet.first;

		return true;
	}

/*
=================================================
	_AddResource
=================================================
*/
	bool VPipelineResources::_AddResource (VResourceManager &, const NullUnion &, INOUT UpdateDescriptors &)
	{
		CHECK( _allowEmptyResources and "uninitialized uniform!" );
		return false;
	}


}	// FG
