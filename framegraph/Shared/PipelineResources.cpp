// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/Public/PipelineResources.h"
#include "PipelineResourcesHelper.h"
#include "stl/Memory/UntypedAllocator.h"
#include "stl/Memory/MemWriter.h"

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
		SHAREDLOCK( _rcCheck );
		STATIC_ASSERT( CachedID::is_always_lock_free );
		STATIC_ASSERT( sizeof(CachedID::value_type) == sizeof(RawPipelineResourcesID) );

		_SetCachedID(other._GetCachedID());
	}

/*
=================================================
	destructor
=================================================
*/
	PipelineResources::~PipelineResources ()
	{
		EXLOCK( _rcCheck );
	}
	
/*
=================================================
	_ResetCachedID
=================================================
*/
	inline void  PipelineResources::_ResetCachedID () const
	{
		_cachedId.store( BitCast<uint64_t>(RawPipelineResourcesID()), memory_order_release );
	}

/*
=================================================
	_GetResource
=================================================
*/
	template <typename T>
	ND_ inline T &  PipelineResources::_GetResource (const UniformID &id)
	{
		SHAREDLOCK( _rcCheck );
		ASSERT( _dataPtr );

		auto*	uniforms = _dataPtr->Uniforms();
		size_t	index	 = BinarySearch( ArrayView<Uniform>{uniforms, _dataPtr->uniformCount}, id );
		
		ASSERT( index < _dataPtr->uniformCount );
		auto&	un = uniforms[ index ];

		ASSERT( un.resType == T::TypeId );

		return *Cast<T>( _dataPtr.get() + BytesU{un.offset} );
	}
	
/*
=================================================
	_HasResource
=================================================
*/
	template <typename T>
	ND_ inline bool  PipelineResources::_HasResource (const UniformID &id) const
	{
		SHAREDLOCK( _rcCheck );

		if ( not _dataPtr )
			return false;
		
		auto*	uniforms = _dataPtr->Uniforms();
		size_t	index	 = BinarySearch( ArrayView<Uniform>{uniforms, _dataPtr->uniformCount}, id );

		if ( index >= _dataPtr->uniformCount )
			return false;

		return uniforms[ index ].resType == T::TypeId;
	}

/*
=================================================
	BindImage
=================================================
*/
	PipelineResources&  PipelineResources::BindImage (const UniformID &id, RawImageID image, uint index)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasImage( id ));

		auto&	res = _GetResource<Image>( id );
		auto&	img = res.elements[ index ];

		ASSERT( index < res.elementCapacity );
		res.elementCount = Max( uint16_t(index+1), res.elementCount );

		if ( img.imageId != image or img.hasDesc )
			_ResetCachedID();

		img.imageId	= image;
		img.hasDesc	= false;

		return *this;
	}
	
	PipelineResources&  PipelineResources::BindImage (const UniformID &id, RawImageID image, const ImageViewDesc &desc, uint index)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasImage( id ));

		auto&	res = _GetResource<Image>( id );
		auto&	img = res.elements[ index ];

		ASSERT( index < res.elementCapacity );
		res.elementCount = Max( uint16_t(index+1), res.elementCount );

		if ( img.imageId != image or not img.hasDesc or not (img.desc == desc) )
			_ResetCachedID();

		img.imageId	= image;
		img.desc	= desc;
		img.hasDesc	= true;

		return *this;
	}
	
/*
=================================================
	BindImages
=================================================
*/
	PipelineResources&  PipelineResources::BindImages (const UniformID &id, ArrayView<ImageID> images)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasImage( id ));

		auto&	res		= _GetResource<Image>( id );
		bool	changed	= false;
		
		ASSERT( images.size() <= res.elementCapacity );
		res.elementCount = Max( uint16_t(images.size()), res.elementCount );

		for (size_t i = 0; i < images.size(); ++i)
		{
			auto&	img = res.elements[i];

			changed |= (img.imageId != images[i].Get() or img.hasDesc);

			img.imageId	= images[i].Get();
			img.hasDesc	= false;
		}

		if ( changed )
			_ResetCachedID();

		return *this;
	}

/*
=================================================
	BindTexture
=================================================
*/
	PipelineResources&  PipelineResources::BindTexture (const UniformID &id, RawImageID image, const SamplerID &sampler, uint index)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasTexture( id ));

		auto&	res = _GetResource<Texture>( id );
		auto&	tex = res.elements[ index ];
		
		ASSERT( index < res.elementCapacity );
		res.elementCount = Max( uint16_t(index+1), res.elementCount );

		if ( tex.imageId != image or tex.samplerId != sampler.Get() or tex.hasDesc )
			_ResetCachedID();

		tex.imageId		= image;
		tex.samplerId	= sampler.Get();
		tex.hasDesc		= false;

		return *this;
	}
	
	PipelineResources&  PipelineResources::BindTexture (const UniformID &id, RawImageID image, const SamplerID &sampler, const ImageViewDesc &desc, uint index)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasTexture( id ));

		auto&	res = _GetResource<Texture>( id );
		auto&	tex = res.elements[ index ];
		
		ASSERT( index < res.elementCapacity );
		res.elementCount = Max( uint16_t(index+1), res.elementCount );

		if ( tex.imageId != image or tex.samplerId != sampler.Get() or not tex.hasDesc or not (tex.desc == desc) )
			_ResetCachedID();

		tex.imageId		= image;
		tex.samplerId	= sampler.Get();
		tex.desc		= desc;
		tex.hasDesc		= true;
		
		return *this;
	}
	
/*
=================================================
	BindTextures
=================================================
*/
	PipelineResources&  PipelineResources::BindTextures (const UniformID &id, ArrayView<ImageID> images, const SamplerID &sampler)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasTexture( id ));

		auto&	res		= _GetResource<Texture>( id );
		bool	changed = false;

		ASSERT( images.size() <= res.elementCapacity );
		res.elementCount = Max( uint16_t(images.size()), res.elementCount );

		for (size_t i = 0; i < images.size(); ++i)
		{
			auto&	tex = res.elements[i];

			changed |= (tex.imageId != images[i].Get() or tex.samplerId != sampler.Get() or tex.hasDesc);

			tex.imageId		= images[i].Get();
			tex.samplerId	= sampler.Get();
			tex.hasDesc		= false;
		}

		if ( changed )
			_ResetCachedID();

		return *this;
	}

/*
=================================================
	BindSampler
=================================================
*/
	PipelineResources&  PipelineResources::BindSampler (const UniformID &id, const SamplerID &sampler, uint index)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasSampler( id ));

		auto&	res  = _GetResource<Sampler>( id );
		auto&	samp = res.elements[ index ];

		ASSERT( index < res.elementCapacity );
		res.elementCount = Max( uint16_t(index+1), res.elementCount );

		if ( samp.samplerId != sampler.Get() )
			_ResetCachedID();

		samp.samplerId = sampler.Get();

		return *this;
	}
	
/*
=================================================
	BindSamplers
=================================================
*/
	PipelineResources&  PipelineResources::BindSamplers (const UniformID &id, ArrayView<SamplerID> samplers)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasSampler( id ));

		auto&	res		= _GetResource<Sampler>( id );
		bool	changed = false;
		
		ASSERT( samplers.size() <= res.elementCapacity );
		res.elementCount = uint16_t(samplers.size());

		for (size_t i = 0; i < samplers.size(); ++i)
		{
			auto&	samp = res.elements[i];

			changed |= (samp.samplerId != samplers[i].Get());

			samp.samplerId = samplers[i].Get();
		}

		if ( changed )
			_ResetCachedID();

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
		EXLOCK( _rcCheck );
		ASSERT( HasBuffer( id ));

		auto&	res	= _GetResource<Buffer>( id );
		auto&	buf	= res.elements[ index ];

		ASSERT( size == ~0_b or ((size >= res.staticSize) and (res.arrayStride == 0 or (size - res.staticSize) % res.arrayStride == 0)) );
		ASSERT( index < res.elementCapacity );
		res.elementCount = Max( uint16_t(index+1), res.elementCount );

		bool	changed = (buf.bufferId != buffer or buf.size != size);
		
		if ( res.dynamicOffsetIndex == PipelineDescription::STATIC_OFFSET )
		{
			changed		|= (buf.offset != offset);
			buf.offset	 = offset;
		}
		else
		{
			ASSERT( offset >= buf.offset and offset - buf.offset < std::numeric_limits<uint>::max() );
			_GetDynamicOffset( res.dynamicOffsetIndex + index ) = uint(offset - buf.offset);
		}
		
		if ( changed )
			_ResetCachedID();

		buf.bufferId = buffer;
		buf.size	 = size;

		return *this;
	}
	
/*
=================================================
	BindBuffers
=================================================
*/
	PipelineResources&  PipelineResources::BindBuffers (const UniformID &id, ArrayView<BufferID> buffers)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasBuffer( id ));
		
		auto&	res		= _GetResource<Buffer>( id );
		bool	changed = false;
		BytesU	offset	= 0_b;
		BytesU	size	= ~0_b;
		
		ASSERT( buffers.size() <= res.elementCapacity );
		res.elementCount = uint16_t(buffers.size());

		for (size_t i = 0; i < buffers.size(); ++i)
		{
			auto&	buf = res.elements[i];

			changed |= (buf.bufferId != buffers[i].Get() or buf.size != size);
		
			if ( res.dynamicOffsetIndex == PipelineDescription::STATIC_OFFSET )
			{
				changed		|= (buf.offset != offset);
				buf.offset	 = offset;
			}
			else
			{
				ASSERT( offset >= buf.offset and offset - buf.offset < std::numeric_limits<uint>::max() );
				_GetDynamicOffset( res.dynamicOffsetIndex + uint(i) ) = uint(offset - buf.offset);
			}

			buf.bufferId = buffers[i].Get();
			buf.size	 = size;
		}
		
		if ( changed )
			_ResetCachedID();

		return *this;
	}

/*
=================================================
	SetBufferBase
=================================================
*/
	PipelineResources&  PipelineResources::SetBufferBase (const UniformID &id, BytesU offset, uint index)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasBuffer( id ));

		auto&	res	= _GetResource<Buffer>( id );
		auto&	buf	= res.elements[ index ];
		
		ASSERT( index < res.elementCapacity );
		res.elementCount = Max( uint16_t(index+1), res.elementCount );

		if ( res.dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET )
		{
			if ( buf.offset != offset )
				_ResetCachedID();
			
			_GetDynamicOffset( res.dynamicOffsetIndex + index ) = uint(_GetDynamicOffset( res.dynamicOffsetIndex + index ) + buf.offset - offset);
			buf.offset = offset;
		}
		return *this;
	}
	
/*
=================================================
	BindRayTracingScene
=================================================
*/
	PipelineResources&  PipelineResources::BindRayTracingScene (const UniformID &id, const RTSceneID &scene, uint index)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasRayTracingScene( id ));

		auto&	res	= _GetResource<RayTracingScene>( id );
		auto&	rts	= res.elements[ index ];
		
		ASSERT( index < res.elementCapacity );
		res.elementCount = Max( uint16_t(index+1), res.elementCount );

		if ( rts.sceneId != scene.Get() )
			_ResetCachedID();

		rts.sceneId = scene.Get();
		
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
	HashOf (Buffer)
=================================================
*/
	inline HashVal  HashOf (const PipelineResources::Buffer &buf)
	{
		HashVal	result = FG::HashOf( buf.index ) + FG::HashOf( buf.state ) +
						 FG::HashOf( buf.dynamicOffsetIndex ) + FG::HashOf( buf.elementCount );

		for (uint16_t i = 0; i < buf.elementCount; ++i)
		{
			auto&	elem = buf.elements[i];
			result << (FG::HashOf( elem.bufferId ) + FG::HashOf( elem.offset ) + FG::HashOf( elem.size ));
		}
		return result;
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
	HashOf (Image)
=================================================
*/
	inline HashVal  HashOf (const PipelineResources::Image &img)
	{
		HashVal	result = FG::HashOf( img.index ) + FG::HashOf( img.state ) + FG::HashOf( img.elementCount );

		for (uint16_t i = 0; i < img.elementCount; ++i)
		{
			auto&	elem = img.elements[i];
			result << (FG::HashOf( elem.imageId ) + (elem.hasDesc ? FG::HashOf( elem.desc ) : HashVal{}));
		}
		return result;
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
	HashOf (Texture)
=================================================
*/
	inline HashVal  HashOf (const PipelineResources::Texture &tex)
	{
		HashVal	result = FG::HashOf( tex.index ) + FG::HashOf( tex.state ) + FG::HashOf( tex.elementCount );

		for (uint16_t i = 0; i < tex.elementCount; ++i)
		{
			auto&	elem = tex.elements[i];
			result << (FG::HashOf( elem.imageId ) + FG::HashOf( elem.samplerId ) +
					   (elem.hasDesc ? FG::HashOf( elem.desc ) : HashVal{}));
		}
		return result;
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
	HashOf (Sampler)
=================================================
*/
	inline HashVal  HashOf (const PipelineResources::Sampler &samp)
	{
		HashVal	result = FG::HashOf( samp.index ) + FG::HashOf( samp.elementCount );

		for (uint16_t i = 0; i < samp.elementCount; ++i)
		{
			result << FG::HashOf( samp.elements[i].samplerId );
		}
		return result;
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

/*
=================================================
	HashOf (RayTracingScene)
=================================================
*/
	inline HashVal  HashOf (const PipelineResources::RayTracingScene &rts)
	{
		HashVal	result = FG::HashOf( rts.index ) + FG::HashOf( rts.elementCount );

		for (uint16_t i = 0; i < rts.elementCount; ++i)
		{
			result << FG::HashOf( rts.elements[i].sceneId );
		}
		return result;
	}

}	// namespace
//-----------------------------------------------------------------------------



	using Allocator = UntypedAllocator;
/*
=================================================
	DynamicDataDeleter::operator ()
=================================================
*/
	void PipelineResources::DynamicDataDeleter::operator () (DynamicData *ptr) const noexcept
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
		EXLOCK( res._rcCheck );
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
		SHAREDLOCK( res._rcCheck );
		CHECK_ERR( res._dataPtr );

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
		data->memSize			= req_size;
		data->uniformCount		= uint(uniforms->size());
		data->uniformsOffset	= uint(mem.Offset());
		auto*	uniforms_ptr	= mem.EmplaceArray<PRs::Uniform>( uniforms->size() );

		data->dynamicOffsetsCount	= bufferDynamicOffsetCount;
		data->dynamicOffsetsOffset	= uint(mem.Offset());
		FG_UNUSED( mem.EmplaceArray<uint>( bufferDynamicOffsetCount ));


		for (auto& un : *uniforms)
		{
			auto&			curr			= uniforms_ptr[ un_index++ ];
			const uint16_t	array_capacity	= uint16_t(un.second.arraySize ? un.second.arraySize : FG_MaxElementsInUnsizedDesc);
			const uint16_t	array_size		= uint16_t(un.second.arraySize);
			
			curr.id = un.first;

			Visit(	un.second.data,
					[&] (const PipelineDescription::Texture &tex)
					{
						curr.resType	= PRs::Texture::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::Texture>( BytesU{sizeof(PRs::Texture) + sizeof(PRs::Texture::Element) * (array_capacity-1)},
														un.second.index, tex.state, array_capacity, array_size );
					},

					[&] (const PipelineDescription::Sampler &)
					{
						curr.resType	= PRs::Sampler::TypeId;
						curr.offset		= uint16_t(mem.Offset());
						
						mem.EmplaceSized<PRs::Sampler>( BytesU{sizeof(PRs::Sampler) + sizeof(PRs::Sampler::Element) * (array_capacity-1)},
													    un.second.index, array_capacity, array_size );
					},

					[&] (const PipelineDescription::SubpassInput &spi)
					{
						curr.resType	= PRs::Image::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::Image>( BytesU{sizeof(PRs::Image) + sizeof(PRs::Image::Element) * (array_capacity-1)},
													  un.second.index, spi.state, array_capacity, array_size );
					},

					[&] (const PipelineDescription::Image &img)
					{
						curr.resType	= PRs::Image::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::Image>( BytesU{sizeof(PRs::Image) + sizeof(PRs::Image::Element) * (array_capacity-1)},
													  un.second.index, img.state, array_capacity, array_size );
					},

					[&] (const PipelineDescription::UniformBuffer &ubuf)
					{
						dbo_count		+= uint(ubuf.dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET) * array_capacity;
						curr.resType	= PRs::Buffer::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::Buffer>( BytesU{sizeof(PRs::Buffer) + sizeof(PRs::Buffer::Element) * (array_capacity-1)},
													   un.second.index, ubuf.state, ubuf.dynamicOffsetIndex, ubuf.size, 0_b,
													   array_capacity, array_size );
					},

					[&] (const PipelineDescription::StorageBuffer &sbuf)
					{
						dbo_count		+= uint(sbuf.dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET) * array_capacity;
						curr.resType	= PRs::Buffer::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::Buffer>( BytesU{sizeof(PRs::Buffer) + sizeof(PRs::Buffer::Element) * (array_capacity-1)},
													   un.second.index, sbuf.state, sbuf.dynamicOffsetIndex, sbuf.staticSize, sbuf.arrayStride,
													   array_capacity, array_size );
					},

					[&] (const PipelineDescription::RayTracingScene &)
					{
						curr.resType	= PRs::RayTracingScene::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::RayTracingScene>( BytesU{sizeof(PRs::RayTracingScene) + sizeof(PRs::RayTracingScene::Element) * (array_capacity-1)},
																un.second.index, array_capacity, array_size );
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
