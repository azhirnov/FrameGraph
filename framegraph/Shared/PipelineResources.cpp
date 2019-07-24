// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/Public/PipelineResources.h"
#include "PipelineResourcesHelper.h"
#include "stl/Memory/UntypedAllocator.h"
#include "stl/Memory/MemWriter.h"

#if 1
#	define UNIFORM_EXISTS	ASSERT
#else
#	define UNIFORM_EXISTS( ... )
#endif

namespace FGC
{
namespace {

/*
=================================================
	HashOf (Buffer)
=================================================
*/
	inline HashVal  HashOf (const FG::PipelineResources::Buffer &buf)
	{
		HashVal	result = FGC::HashOf( buf.index ) + FGC::HashOf( buf.state ) +
						 FGC::HashOf( buf.dynamicOffsetIndex ) + FGC::HashOf( buf.elementCount );

		for (uint16_t i = 0; i < buf.elementCount; ++i)
		{
			auto&	elem = buf.elements[i];
			result << (FGC::HashOf( elem.bufferId ) + FGC::HashOf( elem.offset ) + FGC::HashOf( elem.size ));
		}
		return result;
	}
	
/*
=================================================
	HashOf (Image)
=================================================
*/
	inline HashVal  HashOf (const FG::PipelineResources::Image &img)
	{
		HashVal	result = FGC::HashOf( img.index ) + FGC::HashOf( img.state ) + FGC::HashOf( img.elementCount );

		for (uint16_t i = 0; i < img.elementCount; ++i)
		{
			auto&	elem = img.elements[i];
			result << (FGC::HashOf( elem.imageId ) + (elem.hasDesc ? FGC::HashOf( elem.desc ) : HashVal{}));
		}
		return result;
	}
	
/*
=================================================
	HashOf (Texture)
=================================================
*/
	inline HashVal  HashOf (const FG::PipelineResources::Texture &tex)
	{
		HashVal	result = FGC::HashOf( tex.index ) + FGC::HashOf( tex.state ) + FGC::HashOf( tex.elementCount );

		for (uint16_t i = 0; i < tex.elementCount; ++i)
		{
			auto&	elem = tex.elements[i];
			result << (FGC::HashOf( elem.imageId ) + FGC::HashOf( elem.samplerId ) +
					   (elem.hasDesc ? FGC::HashOf( elem.desc ) : HashVal{}));
		}
		return result;
	}
	
/*
=================================================
	HashOf (Sampler)
=================================================
*/
	inline HashVal  HashOf (const FG::PipelineResources::Sampler &samp)
	{
		HashVal	result = FGC::HashOf( samp.index ) + FGC::HashOf( samp.elementCount );

		for (uint16_t i = 0; i < samp.elementCount; ++i)
		{
			result << FGC::HashOf( samp.elements[i].samplerId );
		}
		return result;
	}

/*
=================================================
	HashOf (RayTracingScene)
=================================================
*/
	inline HashVal  HashOf (const FG::PipelineResources::RayTracingScene &rts)
	{
		HashVal	result = FGC::HashOf( rts.index ) + FGC::HashOf( rts.elementCount );

		for (uint16_t i = 0; i < rts.elementCount; ++i)
		{
			result << FGC::HashOf( rts.elements[i].sceneId );
		}
		return result;
	}

}	// namespace
}	// FGC

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	PipelineResources::PipelineResources (const PipelineResources &other) :
		_dataPtr{ PipelineResourcesHelper::CloneDynamicData( other )},
		_allowEmptyResources{ other._allowEmptyResources }
	{
		SHAREDLOCK( _drCheck );
		STATIC_ASSERT( CachedID::is_always_lock_free );
		//STATIC_ASSERT( sizeof(CachedID::value_type) == sizeof(RawPipelineResourcesID) );

		_SetCachedID(other._GetCachedID());
	}

/*
=================================================
	destructor
=================================================
*/
	PipelineResources::~PipelineResources ()
	{
		EXLOCK( _drCheck );
	}

/*
=================================================
	_GetResource
=================================================
*/
	template <typename T>
	ND_ inline T*  PipelineResources::_GetResource (const UniformID &id)
	{
		SHAREDLOCK( _drCheck );
		
		if ( _dataPtr )
		{
			auto*	uniforms = _dataPtr->Uniforms();
			size_t	index	 = BinarySearch( ArrayView<Uniform>{uniforms, _dataPtr->uniformCount}, id );
		
			if ( index < _dataPtr->uniformCount )
			{
				auto&	un = uniforms[ index ];
				ASSERT( un.resType == T::TypeId );

				return Cast<T>( _dataPtr.get() + BytesU{un.offset} );
			}
		}
		return null;
	}
	
/*
=================================================
	_HasResource
=================================================
*/
	template <typename T>
	ND_ bool  PipelineResources::_HasResource (const UniformID &id) const
	{
		SHAREDLOCK( _drCheck );

		if ( not _dataPtr )
			return false;
		
		auto*	uniforms = _dataPtr->Uniforms();
		size_t	index	 = BinarySearch( ArrayView<Uniform>{uniforms, _dataPtr->uniformCount}, id );

		if ( index >= _dataPtr->uniformCount )
			return false;

		return uniforms[ index ].resType == T::TypeId;
	}

	bool  PipelineResources::HasImage (const UniformID &id)				const	{ SHAREDLOCK(_drCheck);  return _HasResource< Image >( id ); }
	bool  PipelineResources::HasSampler (const UniformID &id)			const	{ SHAREDLOCK(_drCheck);  return _HasResource< Sampler >( id ); }
	bool  PipelineResources::HasTexture (const UniformID &id)			const	{ SHAREDLOCK(_drCheck);  return _HasResource< Texture >( id ); }
	bool  PipelineResources::HasBuffer (const UniformID &id)			const	{ SHAREDLOCK(_drCheck);  return _HasResource< Buffer >( id ); }
	bool  PipelineResources::HasRayTracingScene (const UniformID &id)	const	{ SHAREDLOCK(_drCheck);  return _HasResource< RayTracingScene >( id ); }

/*
=================================================
	BindImage
=================================================
*/
	PipelineResources&  PipelineResources::BindImage (const UniformID &id, RawImageID image, uint index)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasImage( id ));

		if ( auto* res = _GetResource<Image>( id ))
		{
			auto&	img = res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( img.imageId != image or img.hasDesc or res->elementCount <= index )
				_ResetCachedID();
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			img.imageId			= image;
			img.hasDesc			= false;
		}
		return *this;
	}
	
	PipelineResources&  PipelineResources::BindImage (const UniformID &id, RawImageID image, const ImageViewDesc &desc, uint index)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasImage( id ));

		if ( auto* res = _GetResource<Image>( id ))
		{
			auto&	img = res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( img.imageId != image or not img.hasDesc or not (img.desc == desc) or res->elementCount <= index )
				_ResetCachedID();
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			img.imageId			= image;
			img.desc			= desc;
			img.hasDesc			= true;
		}
		return *this;
	}
	
/*
=================================================
	BindImages
=================================================
*/
	PipelineResources&  PipelineResources::BindImages (const UniformID &id, ArrayView<RawImageID> images)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasImage( id ));

		if ( auto* res = _GetResource<Image>( id ))
		{
			bool	changed	= res->elementCount != images.size();
		
			ASSERT( images.size() <= res->elementCapacity );
			res->elementCount = uint16_t(images.size());

			for (size_t i = 0; i < images.size(); ++i)
			{
				auto&	img = res->elements[i];

				changed |= (img.imageId != images[i] or img.hasDesc);

				img.imageId	= images[i];
				img.hasDesc	= false;
			}

			if ( changed )
				_ResetCachedID();
		}
		return *this;
	}

	PipelineResources&  PipelineResources::BindImages (const UniformID &id, ArrayView<ImageID> images)
	{
		STATIC_ASSERT( sizeof(ImageID) == sizeof(RawImageID) );
		return BindImages( id, ArrayView<RawImageID>{ Cast<RawImageID>(images.data()), images.size() });
	}

/*
=================================================
	BindTexture
=================================================
*/
	PipelineResources&  PipelineResources::BindTexture (const UniformID &id, RawImageID image, RawSamplerID sampler, uint index)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasTexture( id ));

		if ( auto* res = _GetResource<Texture>( id ))
		{
			auto&	tex = res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( tex.imageId != image or tex.samplerId != sampler or tex.hasDesc or res->elementCount <= index )
				_ResetCachedID();
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			tex.imageId			= image;
			tex.samplerId		= sampler;
			tex.hasDesc			= false;
		}
		return *this;
	}
	
	PipelineResources&  PipelineResources::BindTexture (const UniformID &id, RawImageID image, RawSamplerID sampler, const ImageViewDesc &desc, uint index)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasTexture( id ));

		if ( auto* res = _GetResource<Texture>( id ))
		{
			auto&	tex = res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( tex.imageId != image or tex.samplerId != sampler or not tex.hasDesc or not (tex.desc == desc) or res->elementCount <= index )
				_ResetCachedID();
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			tex.imageId			= image;
			tex.samplerId		= sampler;
			tex.desc			= desc;
			tex.hasDesc			= true;
		}
		return *this;
	}
	
/*
=================================================
	BindTextures
=================================================
*/
	PipelineResources&  PipelineResources::BindTextures (const UniformID &id, ArrayView<ImageID> images, RawSamplerID sampler)
	{
		STATIC_ASSERT( sizeof(ImageID) == sizeof(RawImageID) );
		return BindTextures( id, ArrayView<RawImageID>{ Cast<RawImageID>(images.data()), images.size() }, sampler );
	}

	PipelineResources&  PipelineResources::BindTextures (const UniformID &id, ArrayView<RawImageID> images, RawSamplerID sampler)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasTexture( id ));

		if ( auto* res = _GetResource<Texture>( id ))
		{
			bool	changed = res->elementCount != images.size();

			ASSERT( images.size() <= res->elementCapacity );
			res->elementCount = uint16_t(images.size());

			for (size_t i = 0; i < images.size(); ++i)
			{
				auto&	tex = res->elements[i];

				changed |= (tex.imageId != images[i] or tex.samplerId != sampler or tex.hasDesc);

				tex.imageId		= images[i];
				tex.samplerId	= sampler;
				tex.hasDesc		= false;
			}

			if ( changed )
				_ResetCachedID();
		}
		return *this;
	}

/*
=================================================
	BindSampler
=================================================
*/
	PipelineResources&  PipelineResources::BindSampler (const UniformID &id, RawSamplerID sampler, uint index)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasSampler( id ));

		if ( auto* res = _GetResource<Sampler>( id ))
		{
			auto&	samp = res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( samp.samplerId != sampler or res->elementCount <= index )
				_ResetCachedID();
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			samp.samplerId		= sampler;
		}
		return *this;
	}
	
/*
=================================================
	BindSamplers
=================================================
*/
	PipelineResources&  PipelineResources::BindSamplers (const UniformID &id, ArrayView<SamplerID> samplers)
	{
		STATIC_ASSERT( sizeof(SamplerID) == sizeof(RawSamplerID) );
		return BindSamplers( id, ArrayView<RawSamplerID>{ Cast<RawSamplerID>(samplers.data()), samplers.size() });
	}

	PipelineResources&  PipelineResources::BindSamplers (const UniformID &id, ArrayView<RawSamplerID> samplers)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasSampler( id ));

		if ( auto* res = _GetResource<Sampler>( id ))
		{
			bool	changed = res->elementCount != samplers.size();
		
			ASSERT( samplers.size() <= res->elementCapacity );
			res->elementCount = uint16_t(samplers.size());

			for (size_t i = 0; i < samplers.size(); ++i)
			{
				auto&	samp = res->elements[i];

				changed |= (samp.samplerId != samplers[i]);

				samp.samplerId = samplers[i];
			}

			if ( changed )
				_ResetCachedID();
		}
		return *this;
	}

/*
=================================================
	BindBuffer
=================================================
*/
	PipelineResources&  PipelineResources::BindBuffer (const UniformID &id, RawBufferID buffer, uint index)
	{
		return BindBuffer( id, buffer, 0_b, ~0_b, index );
	}

	PipelineResources&  PipelineResources::BindBuffer (const UniformID &id, RawBufferID buffer, BytesU offset, BytesU size, uint index)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasBuffer( id ));

		if ( auto* res = _GetResource<Buffer>( id ))
		{
			auto&	buf	= res->elements[ index ];

			ASSERT( size == ~0_b or ((size >= res->staticSize) and (res->arrayStride == 0 or (size - res->staticSize) % res->arrayStride == 0)) );
			ASSERT( index < res->elementCapacity );

			bool	changed = (buf.bufferId != buffer or buf.size != size or res->elementCount <= index);
		
			if ( res->dynamicOffsetIndex == PipelineDescription::STATIC_OFFSET )
			{
				changed		|= (buf.offset != offset);
				buf.offset	 = offset;
			}
			else
			{
				ASSERT( offset >= buf.offset and offset - buf.offset < std::numeric_limits<uint>::max() );
				_GetDynamicOffset( res->dynamicOffsetIndex + index ) = uint(offset - buf.offset);
			}
		
			if ( changed )
				_ResetCachedID();
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			buf.bufferId		= buffer;
			buf.size			= size;
		}
		return *this;
	}
	
/*
=================================================
	BindBuffers
=================================================
*/
	PipelineResources&  PipelineResources::BindBuffers (const UniformID &id, ArrayView<BufferID> buffers)
	{
		STATIC_ASSERT( sizeof(BufferID) == sizeof(RawBufferID) );
		return BindBuffers( id, ArrayView<RawBufferID>{ Cast<RawBufferID>(buffers.data()), buffers.size() });
	}

	PipelineResources&  PipelineResources::BindBuffers (const UniformID &id, ArrayView<RawBufferID> buffers)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasBuffer( id ));
		
		if ( auto* res = _GetResource<Buffer>( id ))
		{
			bool	changed = res->elementCount != buffers.size();
			BytesU	offset	= 0_b;
			BytesU	size	= ~0_b;
		
			ASSERT( buffers.size() <= res->elementCapacity );
			res->elementCount = uint16_t(buffers.size());

			for (size_t i = 0; i < buffers.size(); ++i)
			{
				auto&	buf = res->elements[i];

				changed |= (buf.bufferId != buffers[i] or buf.size != size);
		
				if ( res->dynamicOffsetIndex == PipelineDescription::STATIC_OFFSET )
				{
					changed		|= (buf.offset != offset);
					buf.offset	 = offset;
				}
				else
				{
					ASSERT( offset >= buf.offset and offset - buf.offset < std::numeric_limits<uint>::max() );
					_GetDynamicOffset( res->dynamicOffsetIndex + uint(i) ) = uint(offset - buf.offset);
				}

				buf.bufferId = buffers[i];
				buf.size	 = size;
			}
		
			if ( changed )
				_ResetCachedID();
		}
		return *this;
	}

/*
=================================================
	SetBufferBase
=================================================
*/
	PipelineResources&  PipelineResources::SetBufferBase (const UniformID &id, BytesU offset, uint index)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasBuffer( id ));

		if ( auto* res = _GetResource<Buffer>( id ))
		{
			auto&	buf		= res->elements[ index ];
			bool	changed	= res->elementCount <= index;

			ASSERT( index < res->elementCapacity );
			res->elementCount = Max( uint16_t(index+1), res->elementCount );

			if ( res->dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET )
			{
				changed |= (buf.offset != offset);
				_GetDynamicOffset( res->dynamicOffsetIndex + index ) = uint(_GetDynamicOffset( res->dynamicOffsetIndex + index ) + buf.offset - offset);
				buf.offset = offset;
			}
		
			if ( changed )
				_ResetCachedID();
		}
		return *this;
	}
	
/*
=================================================
	BindRayTracingScene
=================================================
*/
	PipelineResources&  PipelineResources::BindRayTracingScene (const UniformID &id, RawRTSceneID scene, uint index)
	{
		EXLOCK( _drCheck );
		UNIFORM_EXISTS( HasRayTracingScene( id ));

		if ( auto* res = _GetResource<RayTracingScene>( id ))
		{
			auto&	rts	= res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( rts.sceneId != scene or res->elementCount <= index )
				_ResetCachedID();
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			rts.sceneId			= scene;
		}
		return *this;
	}
//-----------------------------------------------------------------------------


namespace {
/*
=================================================
	operator == (Buffer)
=================================================
*/
	inline bool  operator == (const PipelineResources::Buffer &lhs, const PipelineResources::Buffer &rhs)
	{
		if ( lhs.index				!= rhs.index				or
			 lhs.state				!= rhs.state				or
			 lhs.dynamicOffsetIndex	!= rhs.dynamicOffsetIndex	or
			// lhs.staticSize		!= rhs.staticSize			or
			// lhs.arrayStride		!= rhs.arrayStride			or
			 lhs.elementCount		!= rhs.elementCount )
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].bufferId	!= rhs.elements[i].bufferId or
				 lhs.elements[i].offset		!= rhs.elements[i].offset	or
				 lhs.elements[i].size		!= rhs.elements[i].size )
				return false;
		}
		return true;
	}
	
/*
=================================================
	operator == (Image)
=================================================
*/
	inline bool  operator == (const PipelineResources::Image &lhs, const PipelineResources::Image &rhs)
	{
		if ( lhs.index			!= rhs.index		or
			 lhs.state			!= rhs.state		or
			 lhs.elementCount	!= rhs.elementCount )
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].imageId	!= rhs.elements[i].imageId	or
				 lhs.elements[i].hasDesc	!= rhs.elements[i].hasDesc	or
				 (lhs.elements[i].hasDesc	?  not (lhs.elements[i].desc == rhs.elements[i].desc) : false) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	operator == (Texture)
=================================================
*/
	inline bool  operator == (const PipelineResources::Texture &lhs, const PipelineResources::Texture &rhs)
	{
		if ( lhs.index			!= rhs.index		or
			 lhs.state			!= rhs.state		or
			 lhs.elementCount	!= rhs.elementCount	)
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].imageId	!= rhs.elements[i].imageId		or
				 lhs.elements[i].samplerId	!= rhs.elements[i].samplerId	or
				 lhs.elements[i].hasDesc	!= rhs.elements[i].hasDesc		or
				 (lhs.elements[i].hasDesc	?  not (lhs.elements[i].desc == rhs.elements[i].desc) : false) )
				return false;
		}
		return true;
	}

/*
=================================================
	operator == (Sampler)
=================================================
*/
	inline bool  operator == (const PipelineResources::Sampler &lhs, const PipelineResources::Sampler &rhs)
	{
		if ( lhs.index			!= rhs.index		or
			 lhs.elementCount	!= rhs.elementCount )
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].samplerId != rhs.elements[i].samplerId )
				return false;
		}
		return true;
	}

/*
=================================================
	operator == (RayTracingScene)
=================================================
*/
	inline bool  operator == (const PipelineResources::RayTracingScene &lhs, const PipelineResources::RayTracingScene &rhs)
	{
		if ( lhs.index			!= rhs.index		or
			 lhs.elementCount	!= rhs.elementCount )
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].sceneId != rhs.elements[i].sceneId )
				return false;
		}
		return true;
	}

}	// namespace
//-----------------------------------------------------------------------------



	using Allocator = UntypedAllocator;
/*
=================================================
	DynamicDataDeleter::operator ()
=================================================
*/
    void PipelineResources::DynamicDataDeleter::operator () (DynamicData *ptr) const
	{
		ASSERT( ptr != null );
		Allocator::Deallocate( ptr, ptr->memSize );
	}

/*
=================================================
	CalcHash
=================================================
*/
	HashVal  PipelineResources::DynamicData::CalcHash () const
	{
		HashVal	result;
		ForEachUniform( [&result] (const UniformID &id, auto& res) { result << HashOf(id) << HashOf(res); });
		return result;
	}

/*
=================================================
	operator ==
=================================================
*/
	bool  PipelineResources::DynamicData::operator == (const DynamicData &rhs) const
	{
		auto&	lhs = *this;

		if ( lhs.layoutId		!= rhs.layoutId		and
			 lhs.uniformCount	!= rhs.uniformCount	)
			return false;

		for (uint i = 0, cnt = lhs.uniformCount; i < cnt; ++i)
		{
			auto&		lhs_un  = lhs.Uniforms()[i];
			auto&		rhs_un	= rhs.Uniforms()[i];
			void const*	lhs_ptr	= (&lhs + BytesU{lhs_un.offset});
			void const*	rhs_ptr = (&rhs + BytesU{rhs_un.offset});
			bool		equals	= true;

			if ( lhs_un.id		!= rhs_un.id	or
				 lhs_un.resType != rhs_un.resType )
				return false;

			ENABLE_ENUM_CHECKS();
			switch ( lhs_un.resType )
			{
				case EDescriptorType::Unknown :			break;
				case EDescriptorType::Buffer :			equals = (*Cast<Buffer>(lhs_ptr)		  == *Cast<Buffer>(rhs_ptr));			break;
				case EDescriptorType::SubpassInput :
				case EDescriptorType::Image :			equals = (*Cast<Image>(lhs_ptr)			  == *Cast<Image>(rhs_ptr));			break;
				case EDescriptorType::Texture :			equals = (*Cast<Texture>(lhs_ptr)		  == *Cast<Texture>(rhs_ptr));			break;
				case EDescriptorType::Sampler :			equals = (*Cast<Sampler>(lhs_ptr)		  == *Cast<Sampler>(rhs_ptr));			break;
				case EDescriptorType::RayTracingScene :	equals = (*Cast<RayTracingScene>(lhs_ptr) == *Cast<RayTracingScene>(rhs_ptr));	break;
			}
			DISABLE_ENUM_CHECKS();

			if ( not equals )
				return false;
		}
		return true;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	Initialize
=================================================
*/
	bool  PipelineResourcesHelper::Initialize (OUT PipelineResources &res, RawDescriptorSetLayoutID layoutId, const DynamicDataPtr &dataPtr)
	{
		EXLOCK( res._drCheck );
		CHECK_ERR( dataPtr );

		res._ResetCachedID();
		res._dataPtr = DynamicDataPtr{ Cast<PipelineResources::DynamicData>( Allocator::Allocate( dataPtr->memSize ))};

		memcpy( OUT res._dataPtr.get(), dataPtr.get(), size_t(dataPtr->memSize) );
		res._dataPtr->layoutId = layoutId;

		return true;
	}
	
/*
=================================================
	CloneDynamicData
=================================================
*/
	PipelineResources::DynamicDataPtr  PipelineResourcesHelper::CloneDynamicData (const PipelineResources &res)
	{
		SHAREDLOCK( res._drCheck );

		if ( not res._dataPtr )
			return Default;

		auto&	data	= res._dataPtr;
		auto*	result	= Cast<PipelineResources::DynamicData>( Allocator::Allocate( data->memSize ));
		
		memcpy( OUT result, data.get(), size_t(data->memSize) );
		return DynamicDataPtr{ result };
	}
	
/*
=================================================
	RemoveDynamicData
=================================================
*/
	PipelineResources::DynamicDataPtr  PipelineResourcesHelper::RemoveDynamicData (INOUT PipelineResources &res)
	{
		auto	temp = std::move(res._dataPtr);
		return temp;
	}

/*
=================================================
	CreateDynamicData
=================================================
*/
	PipelineResources::DynamicDataPtr
		PipelineResourcesHelper::CreateDynamicData (const PipelineDescription::UniformMapPtr &uniforms,
													uint resourceCount, uint arrayElemCount, uint bufferDynamicOffsetCount)
	{
		using PRs			= PipelineResources;
		using AllResources	= Union< PRs::Buffer, PRs::Image, PRs::Texture, PRs::Sampler, PRs::RayTracingScene >;
		using AllElements	= Union< PRs::Buffer::Element, PRs::Image::Element, PRs::Texture::Element, PRs::Sampler::Element, PRs::RayTracingScene::Element >;

		const BytesU	req_size =	SizeOf<PRs::DynamicData> +
									SizeOf<PRs::Uniform> * uniforms->size() +
									SizeOf<uint> * bufferDynamicOffsetCount +
									SizeOf<AllResources> * resourceCount +
									SizeOf<AllElements> * arrayElemCount;
	
		MemWriter	mem			{ Allocator::Allocate( req_size ), req_size };
		uint		dbo_count	= 0;
		uint		un_index	= 0;

		auto*	data			= &mem.Emplace<PRs::DynamicData>();
		auto*	uniforms_ptr	= mem.EmplaceArray<PRs::Uniform>( uniforms->size() );
		data->memSize			= req_size;
		data->uniformCount		= uint(uniforms->size());
		data->uniformsOffset	= uint(mem.OffsetOf( uniforms_ptr ));

		auto*	dyn_offsets		= mem.EmplaceArray<uint>( bufferDynamicOffsetCount );
		data->dynamicOffsetsCount	= bufferDynamicOffsetCount;
		data->dynamicOffsetsOffset	= uint(mem.OffsetOf( dyn_offsets ));

		for (auto& un : *uniforms)
		{
			auto&			curr			= uniforms_ptr[ un_index++ ];
			const uint16_t	array_capacity	= uint16_t(un.second.arraySize ? un.second.arraySize : FG_MaxElementsInUnsizedDesc);
			const uint16_t	array_size		= uint16_t(un.second.arraySize);
			
			curr.id = un.first;

			Visit(	un.second.data,
					[&] (const PipelineDescription::Texture &tex)
					{
						void* ptr = &mem.EmplaceSized<PRs::Texture>(
											BytesU{sizeof(PRs::Texture) + sizeof(PRs::Texture::Element) * (array_capacity-1)},
											un.second.index, tex.state, array_capacity, array_size, Default );

						curr.resType	= PRs::Texture::TypeId;
						curr.offset		= uint16_t(mem.OffsetOf( ptr ));
					},

					[&] (const PipelineDescription::Sampler &)
					{
						void* ptr = &mem.EmplaceSized<PRs::Sampler>(
											BytesU{sizeof(PRs::Sampler) + sizeof(PRs::Sampler::Element) * (array_capacity-1)},
											un.second.index, array_capacity, array_size, Default );

						curr.resType	= PRs::Sampler::TypeId;
						curr.offset		= uint16_t(mem.OffsetOf( ptr ));
					},

					[&] (const PipelineDescription::SubpassInput &spi)
					{
						void* ptr = &mem.EmplaceSized<PRs::Image>(
											BytesU{sizeof(PRs::Image) + sizeof(PRs::Image::Element) * (array_capacity-1)},
											un.second.index, spi.state, array_capacity, array_size, Default );

						curr.resType	= PRs::Image::TypeId;
						curr.offset		= uint16_t(mem.OffsetOf( ptr ));
					},

					[&] (const PipelineDescription::Image &img)
					{
						void* ptr = &mem.EmplaceSized<PRs::Image>(
											BytesU{sizeof(PRs::Image) + sizeof(PRs::Image::Element) * (array_capacity-1)},
											un.second.index, img.state, array_capacity, array_size, Default );

						curr.resType	= PRs::Image::TypeId;
						curr.offset		= uint16_t(mem.OffsetOf( ptr ));
					},

					[&] (const PipelineDescription::UniformBuffer &ubuf)
					{
						void* ptr = &mem.EmplaceSized<PRs::Buffer>(
											BytesU{sizeof(PRs::Buffer) + sizeof(PRs::Buffer::Element) * (array_capacity-1)},
											un.second.index, ubuf.state, ubuf.dynamicOffsetIndex, ubuf.size, 0_b,
											array_capacity, array_size, Default );

						dbo_count		+= uint(ubuf.dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET) * array_capacity;
						curr.resType	= PRs::Buffer::TypeId;
						curr.offset		= uint16_t(mem.OffsetOf( ptr ));
					},

					[&] (const PipelineDescription::StorageBuffer &sbuf)
					{
						void* ptr = &mem.EmplaceSized<PRs::Buffer>(
											BytesU{sizeof(PRs::Buffer) + sizeof(PRs::Buffer::Element) * (array_capacity-1)},
											un.second.index, sbuf.state, sbuf.dynamicOffsetIndex, sbuf.staticSize, sbuf.arrayStride,
											array_capacity, array_size, Default );

						dbo_count		+= uint(sbuf.dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET) * array_capacity;
						curr.resType	= PRs::Buffer::TypeId;
						curr.offset		= uint16_t(mem.OffsetOf( ptr ));
					},

					[&] (const PipelineDescription::RayTracingScene &)
					{
						void* ptr = &mem.EmplaceSized<PRs::RayTracingScene>(
											BytesU{sizeof(PRs::RayTracingScene) + sizeof(PRs::RayTracingScene::Element) * (array_capacity-1)},
											un.second.index, array_capacity, array_size, Default );

						curr.resType	= PRs::RayTracingScene::TypeId;
						curr.offset		= uint16_t(mem.OffsetOf( ptr ));
					},

					[] (const NullUnion &) { ASSERT(false); }
				);
		}

		std::sort( uniforms_ptr, uniforms_ptr + data->uniformCount,
				   [] (auto& lhs, auto& rhs) { return lhs.id < rhs.id; } );

		ASSERT( dbo_count == bufferDynamicOffsetCount );
		return DynamicDataPtr{ data };
	}
	

}	// FG
