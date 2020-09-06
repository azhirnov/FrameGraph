// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Shared/PipelineResourcesHelper.h"
#include "VPipelineResources.h"
#include "VResourceManager.h"
#include "VDevice.h"
#include "VEnumCast.h"
#include "stl/Algorithms/StringUtils.h"
#include "Shared/EnumToString.h"

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
	bool  VPipelineResources::Create (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _descriptorSet.first );
		CHECK_ERR( _dataPtr );

		VDevice const&	dev			= resMngr.GetDevice();
		auto const*		ds_layout	= resMngr.GetResource( _layoutId );
		
		// Without the nullDescriptor feature enabled, when updating a VkDescriptorSet, all the resources backing it must be non-null,
		// even if the descriptor is statically not used by the shader. This feature allows descriptors to be backed by null resources or views.
		// Loads from a null descriptor return zero values and stores and atomics to a null descriptor are discarded.
		// https://github.com/KhronosGroup/Vulkan-Guide/blob/master/chapters/robustness.md
	#ifdef VK_EXT_robustness2
		if ( dev.GetFeatures().robustness2 and dev.GetProperties().robustness2Features.nullDescriptor )
		{
			// '_allowEmptyResources' can be 'true'
		}
		else
	#endif
		{
			_allowEmptyResources = false;
		}

		CHECK_ERR( ds_layout );
		CHECK_ERR( ds_layout->AllocDescriptorSet( resMngr, OUT _descriptorSet ));
		
		UpdateDescriptors	update;
		update.descriptors		= update.allocator.Alloc< VkWriteDescriptorSet >( ds_layout->GetMaxIndex() + 1 );
		update.descriptorIndex	= 0;

		_dataPtr->ForEachUniform( [&](auto& un, auto& data) { _AddResource( resMngr, un, data, INOUT update ); });
		
		dev.vkUpdateDescriptorSets( dev.GetVkDevice(), update.descriptorIndex, update.descriptors, 0, null );
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  VPipelineResources::Destroy (VResourceManager &resMngr)
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
			#ifdef VK_NV_ray_tracing
				for (uint i = 0; i < rts.elementCount; ++i)
				{
					auto&	elem = rts.elements[i];
					alive &= not elem.sceneId or resMngr.IsResourceAlive( elem.sceneId );
				}
			#else
				Unused( rts );
			#endif
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
	_LogUniform
=================================================
*/
	inline void  VPipelineResources::_LogUniform (const UniformID &un, uint idx) const
	{
		#if FG_OPTIMIZE_IDS
			Unused( un, idx );
			CHECK( _allowEmptyResources );
		#else
			if ( not _allowEmptyResources )
				FG_LOGE( "Uniform '"s << un.GetName() << "[" << ToString(idx) << "]' contains invalid resource!" );
		#endif
	}

/*
=================================================
	_AddResource
=================================================
*/
	bool  VPipelineResources::_AddResource (VResourceManager &resMngr, const UniformID &un, INOUT PipelineResources::Buffer &buf, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorBufferInfo >( buf.elementCount );

		for (uint i = 0; i < buf.elementCount; ++i)
		{
			auto&			elem	= buf.elements[i];
			VBuffer const*	buffer	= resMngr.GetResource( elem.bufferId, false, true );
			
			if_unlikely( not buffer )
			{
				_LogUniform( un, i );
				return false;
			}

			info[i].buffer	= buffer->Handle();
			info[i].offset	= VkDeviceSize(elem.offset);
			info[i].range	= VkDeviceSize(elem.size);

			_CheckBufferUsage( *buffer, buf.state );
		}

		const bool	is_uniform	= ((buf.state & EResourceState::_StateMask) == EResourceState::UniformRead);
		const bool	is_dynamic	= AllBits( buf.state, EResourceState::_BufferDynamicOffset );

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
	bool  VPipelineResources::_AddResource (VResourceManager &resMngr, const UniformID &un, INOUT PipelineResources::TexelBuffer &texbuf, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkBufferView >( texbuf.elementCount );

		for (uint i = 0; i < texbuf.elementCount; ++i)
		{
			auto&			elem	= texbuf.elements[i];
			VBuffer const*	buffer	= resMngr.GetResource( elem.bufferId, false, true );
			
			if_unlikely( not buffer )
			{
				_LogUniform( un, i );
				return false;
			}

			info[i] = buffer->GetView( resMngr.GetDevice(), elem.desc );
			
			if_unlikely( info[i] == VK_NULL_HANDLE )
			{
				_LogUniform( un, i );
				return false;
			}
			
			_CheckTexelBufferUsage( *buffer, texbuf.state );
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
	bool  VPipelineResources::_AddResource (VResourceManager &resMngr, const UniformID &un, INOUT PipelineResources::Image &img, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( img.elementCount );

		for (uint i = 0; i < img.elementCount; ++i)
		{
			auto&			elem	= img.elements[i];
			VImage const*	img_res	= resMngr.GetResource( elem.imageId, false, true );
			
			if_unlikely( not img_res )
			{
				_LogUniform( un, i );
				return false;
			}

			info[i].imageLayout	= EResourceState_ToImageLayout( img.state, img_res->AspectMask() );
			info[i].imageView	= img_res->GetView( resMngr.GetDevice(), not elem.hasDesc, INOUT elem.desc );
			info[i].sampler		= VK_NULL_HANDLE;
			
			if_unlikely( info[i].imageView == VK_NULL_HANDLE )
			{
				_LogUniform( un, i );
				return false;
			}

			_CheckImageType( un, i, *img_res, elem.desc, img.imageType );
			_CheckImageUsage( *img_res, img.state );
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
	bool  VPipelineResources::_AddResource (VResourceManager &resMngr, const UniformID &un, INOUT PipelineResources::Texture &tex, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( tex.elementCount );

		for (uint i = 0; i < tex.elementCount; ++i)
		{
			auto&			elem	= tex.elements[i];
			VImage const*	img_res	= resMngr.GetResource( elem.imageId, false, true );
			VSampler const*	sampler	= resMngr.GetResource( elem.samplerId, false, true );

			if_unlikely( not (img_res and sampler) )
			{
				_LogUniform( un, i );
				return false;
			}

			info[i].imageLayout	= EResourceState_ToImageLayout( tex.state, img_res->AspectMask() );
			info[i].imageView	= img_res->GetView( resMngr.GetDevice(), not elem.hasDesc, INOUT elem.desc );
			info[i].sampler		= sampler->Handle();
			
			if_unlikely( info[i].imageView == VK_NULL_HANDLE )
			{
				_LogUniform( un, i );
				return false;
			}
			
			_CheckTextureType( un, i, *img_res, elem.desc, tex.samplerType );
			_CheckImageUsage( *img_res, tex.state );
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
	bool  VPipelineResources::_AddResource (VResourceManager &resMngr, const UniformID &un, const PipelineResources::Sampler &samp, INOUT UpdateDescriptors &list)
	{
		auto*	info = list.allocator.Alloc< VkDescriptorImageInfo >( samp.elementCount );

		for (uint i = 0; i < samp.elementCount; ++i)
		{
			auto&			elem	= samp.elements[i];
			VSampler const*	sampler = resMngr.GetResource( elem.samplerId, false, true );
			
			if_unlikely( not sampler )
			{
				_LogUniform( un, i );
				return false;
			}

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
	bool  VPipelineResources::_AddResource (VResourceManager &resMngr, const UniformID &un, const PipelineResources::RayTracingScene &rtScene, INOUT UpdateDescriptors &list)
	{
	#ifdef VK_NV_ray_tracing
		auto*	tlas = list.allocator.Alloc<VkAccelerationStructureNV>( rtScene.elementCount );

		for (uint i = 0; i < rtScene.elementCount; ++i)
		{
			auto&					elem	= rtScene.elements[i];
			VRayTracingScene const*	scene	= resMngr.GetResource( elem.sceneId, false, true );

			if_unlikely( not scene )
			{
				_LogUniform( un, i );
				return false;
			}

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
	#else
		Unused( resMngr, un, rtScene, list );
		ASSERT( !"ray tracing is not supported" );
		return false;
	#endif
	}

/*
=================================================
	_AddResource
=================================================
*/
	bool  VPipelineResources::_AddResource (VResourceManager &, const UniformID &un, const NullUnion &, INOUT UpdateDescriptors &)
	{
		#if FG_OPTIMIZE_IDS
			Unused( un );
			CHECK( _allowEmptyResources and "uninitialized uniform!" );
		#else
			if ( not _allowEmptyResources )
				FG_LOGE( "Uninitialized uniform "s << un.GetName() );
		#endif
		return false;
	}
	
/*
=================================================
	_CheckBufferUsage
=================================================
*/
	inline void  VPipelineResources::_CheckBufferUsage (const VBuffer &buffer, EResourceState state)
	{
	#ifdef FG_DEBUG
		const EBufferUsage	usage = buffer.Description().usage;

		switch ( state & EResourceState::_AccessMask )
		{
			case EResourceState::_Access_ShaderStorage :	CHECK( AllBits( usage, EBufferUsage::Storage ));	break;
			case EResourceState::_Access_Uniform :			CHECK( AllBits( usage, EBufferUsage::Uniform ));	break;
			default :										CHECK( !"unknown resource state" );					break;
		}
	#else
		Unused( buffer, state );
	#endif
	}

/*
=================================================
	_CheckTexelBufferUsage
=================================================
*/
	inline void  VPipelineResources::_CheckTexelBufferUsage (const VBuffer &buffer, EResourceState state)
	{
	#ifdef FG_DEBUG
		const EBufferUsage	usage = buffer.Description().usage;

		switch ( state & EResourceState::_AccessMask )
		{
			case EResourceState::_Access_ShaderStorage :	CHECK( AnyBits( usage, EBufferUsage::StorageTexel | EBufferUsage::StorageTexelAtomic ));	break;
			case EResourceState::_Access_Uniform :			CHECK( AllBits( usage, EBufferUsage::UniformTexel ));										break;
			default :										CHECK( !"unknown resource state" );															break;
		}
	#else
		Unused( buffer, state );
	#endif
	}
	
/*
=================================================
	_CheckImageUsage
=================================================
*/
	inline void  VPipelineResources::_CheckImageUsage (const VImage &image, EResourceState state)
	{
	#ifdef FG_DEBUG
		const EImageUsage	usage = image.Description().usage;

		switch ( state & EResourceState::_AccessMask )
		{
			case EResourceState::_Access_ShaderStorage :	CHECK( AnyBits( usage, EImageUsage::Storage | EImageUsage::StorageAtomic ));	break;
			case EResourceState::_Access_ShaderSample :		CHECK( AllBits( usage, EImageUsage::Sampled ));									break;
			default :										CHECK( !"unknown resource state" );												break;
		}
	#else
		Unused( image, state );
	#endif
	}
	
/*
=================================================
	_CheckImageType
=================================================
*/
	inline void  VPipelineResources::_CheckImageType (const UniformID &un, uint idx, const VImage &img, const ImageViewDesc &desc, EImageSampler type)
	{
	#ifdef FG_DEBUG
		EPixelFormat	fmt = EPixelFormat(type & EImageSampler::_FormatMask);

		if ( fmt != Zero and fmt != desc.format )
		{
			FG_LOGE( "Uncompatible image formats in uniform "s << un.GetName() << "[" << ToString(idx) << "]:\n"
					 "    in shader: " << ToString( fmt ) << "\n"
					 "    in image:  " << ToString( desc.format ) << ", name: " << img.GetDebugName() );
		}
		_CheckTextureType( un, idx, img, desc, (type & ~EImageSampler::_FormatMask) );
	#else
		Unused( un, idx, img, desc, type );
	#endif
	}
	
/*
=================================================
	_CheckTextureType
=================================================
*/
	inline void  VPipelineResources::_CheckTextureType (const UniformID &un, uint idx, const VImage &img, const ImageViewDesc &desc, EImageSampler shaderType)
	{
	#ifdef FG_DEBUG
		using EType = PixelFormatInfo::EType;

		ASSERT( not AnyBits( shaderType, EImageSampler::_FormatMask ));

		EImageSampler	img_type = Zero;

		BEGIN_ENUM_CHECKS();
		switch ( desc.viewType )
		{
			case EImage_1D :			img_type = EImageSampler::_1D;			break;
			case EImage_2D :			img_type = EImageSampler::_2D;			break;
			case EImage_3D :			img_type = EImageSampler::_3D;			break;
			case EImage_1DArray :		img_type = EImageSampler::_1DArray;		break;
			case EImage_2DArray :		img_type = EImageSampler::_2DArray;		break;
			case EImage_Cube :			img_type = EImageSampler::_Cube;		break;
			case EImage_CubeArray :	img_type = EImageSampler::_CubeArray;	break;
			case EImage::Unknown :		ASSERT(false);							break;
		}
		END_ENUM_CHECKS();

		auto&	info = EPixelFormat_GetInfo( desc.format );

		if ( desc.aspectMask == EImageAspect::Stencil )
		{
			ASSERT( AnyBits( info.valueType, EType::Stencil | EType::DepthStencil ));
			img_type |= EImageSampler::_Int;
		}
		else
		if ( desc.aspectMask == EImageAspect::Depth )
		{
			ASSERT( AnyBits( info.valueType, EType::Depth | EType::DepthStencil ));
			img_type |= EImageSampler::_Float;
		}
		else
		if ( AllBits( info.valueType, EType::Int ))
			img_type |= EImageSampler::_Int;
		else
		if ( AllBits( info.valueType, EType::UInt ))
			img_type |= EImageSampler::_Uint;
		else
		if ( AnyBits( info.valueType, EType::SFloat | EType::UFloat | EType::UNorm | EType::SNorm ))
			img_type |= EImageSampler::_Float;
		else
			ASSERT( !"unknown value type" );

		if ( img_type != shaderType )
		{
			FG_LOGE( "Uncompatible image formats in uniform "s << un.GetName() << "[" << ToString(idx) << "]:\n"
					 "    in shader: " << ToString( shaderType ) << "\n"
					 "    in image:  " << ToString( img_type ) << ", name: " << img.GetDebugName() );
		}
	#else
		Unused( un, idx, img, desc, shaderType );
	#endif
	}

}	// FG
