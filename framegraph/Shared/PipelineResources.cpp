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
		_dataPtr{ other._dataPtr },
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

		if ( _dataPtr )
			_dataPtr->Dealloc();
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

		return *Cast<T>( _dataPtr + BytesU{un.offset} );
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

		ASSERT( index < res.elementCount );

		if ( img.imageId != image or img.hasDesc )
			_ResetCachedID();

		img.imageId	= image;
		img.hasDesc	= false;

		//curr.hash = HashOf( img.imageId );
		return *this;
	}
	
	PipelineResources&  PipelineResources::BindImage (const UniformID &id, RawImageID image, const ImageViewDesc &desc, uint index)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasImage( id ));

		auto&	res = _GetResource<Image>( id );
		auto&	img = res.elements[ index ];

		ASSERT( index < res.elementCount );

		if ( img.imageId != image or not img.hasDesc or not (img.desc == desc) )
			_ResetCachedID();

		img.imageId	= image;
		img.desc	= desc;
		img.hasDesc	= true;

		//curr.hash = HashOf( img.imageId ) + HashOf( img.desc );
		return *this;
	}
	
	PipelineResources&  PipelineResources::BindImages (const UniformID &id, ArrayView<ImageID> images)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasImage( id ));

		auto&	res = _GetResource<Image>( id );
		
		ASSERT( images.size() < res.elementCount );

		for (size_t i = 0; i < images.size(); ++i)
		{
			auto&	img = res.elements[i];

			if ( img.imageId != images[i].Get() or img.hasDesc )
				_ResetCachedID();

			img.imageId	= images[i].Get();
			img.hasDesc	= false;
		}
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
		
		ASSERT( index < res.elementCount );

		if ( tex.imageId != image or tex.samplerId != sampler.Get() or tex.hasDesc )
			_ResetCachedID();

		tex.imageId		= image;
		tex.samplerId	= sampler.Get();
		tex.hasDesc		= false;

		//curr.hash = HashOf( tex.imageId ) + HashOf( tex.samplerId );
		return *this;
	}
	
	PipelineResources&  PipelineResources::BindTexture (const UniformID &id, RawImageID image, const SamplerID &sampler, const ImageViewDesc &desc, uint index)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasTexture( id ));

		auto&	res = _GetResource<Texture>( id );
		auto&	tex = res.elements[ index ];
		
		ASSERT( index < res.elementCount );

		if ( tex.imageId != image or tex.samplerId != sampler.Get() or not tex.hasDesc or not (tex.desc == desc) )
			_ResetCachedID();

		tex.imageId		= image;
		tex.samplerId	= sampler.Get();
		tex.desc		= desc;
		tex.hasDesc		= true;
		
		//curr.hash = HashOf( tex.imageId ) + HashOf( tex.samplerId ) + HashOf( tex.desc );
		return *this;
	}
	
	PipelineResources&  PipelineResources::BindTextures (const UniformID &id, ArrayView<ImageID> images, const SamplerID &sampler)
	{
		EXLOCK( _rcCheck );
		ASSERT( HasTexture( id ));

		auto&	res = _GetResource<Texture>( id );

		ASSERT( images.size() == res.elementCount );

		for (size_t i = 0; i < images.size(); ++i)
		{
			auto&	tex = res.elements[i];

			if ( tex.imageId != images[i].Get() or tex.samplerId != sampler.Get() or tex.hasDesc )
				_ResetCachedID();

			tex.imageId		= images[i].Get();
			tex.samplerId	= sampler.Get();
			tex.hasDesc		= false;
		}
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

		ASSERT( index < res.elementCount );

		if ( samp.samplerId != sampler.Get() )
			_ResetCachedID();

		samp.samplerId = sampler.Get();

		//.hash = HashOf( samp.samplerId );
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
		//ASSERT( size == ~0_b or ((size >= un.staticSize) and (un.arrayStride == 0 or (size - un.staticSize) % un.arrayStride == 0)) );
		ASSERT( index < res.elementCount );

		bool	changed = (buf.bufferId	!= buffer or buf.size != size);
		
		if ( res.dynamicOffsetIndex == PipelineDescription::STATIC_OFFSET )
		{
			changed		|= (buf.offset != offset);
			buf.offset	 = offset;
		}
		else
		{
			ASSERT( offset >= buf.offset and offset - buf.offset < std::numeric_limits<uint>::max() );
			_GetDynamicOffset( res.dynamicOffsetIndex ) = uint(offset - buf.offset);
		}
		
		if ( changed )
			_ResetCachedID();

		buf.bufferId	= buffer;
		buf.size		= size;

		//curr.hash = HashOf( buf.bufferId ) + HashOf( buf.size ) + HashOf( buf.offset );	
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
		
		ASSERT( index < res.elementCount );

		if ( res.dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET )
		{
			if ( buf.offset != offset )
				_ResetCachedID();
			
			_GetDynamicOffset( res.dynamicOffsetIndex + 0 ) = uint(_GetDynamicOffset( res.dynamicOffsetIndex + 0 ) + buf.offset - offset);
			buf.offset = offset;
		
			//curr.hash = HashOf( buf.bufferId ) + HashOf( buf.size ) + HashOf( buf.offset );
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
		
		ASSERT( index < res.elementCount );

		if ( rts.sceneId != scene.Get() )
			_ResetCachedID();

		rts.sceneId = scene.Get();
		
		//curr.hash = HashOf( rts.sceneId );
		return *this;
	}
//-----------------------------------------------------------------------------


	
	using Allocator = UntypedAllocator;
/*
=================================================
	PRs_DynamicData_Deallocator
=================================================
*/
	void PipelineResourcesHelper::PRs_DynamicData_Deallocator (void* alloc, void* ptr, BytesU size)
	{
		ASSERT( alloc == null );
		Allocator::Deallocate( ptr, size );
	}

/*
=================================================
	Initialize
=================================================
*/
	bool  PipelineResourcesHelper::Initialize (OUT PipelineResources &res, RawDescriptorSetLayoutID layoutId, const void *dataPtr)
	{
		EXLOCK( res._rcCheck );

		res._ResetCachedID();

		if ( res._dataPtr )
			res._dataPtr->Dealloc();


		auto&	data = *Cast<PipelineResources::DynamicData>( dataPtr );
		res._dataPtr = Cast<PipelineResources::DynamicData>( Allocator::Allocate( data.memSize ));

		memcpy( OUT res._dataPtr, &data, size_t(data.memSize) );
		res._dataPtr->layoutId = layoutId;

		return true;
	}
	
/*
=================================================
	CloneDynamicData
=================================================
*/
	PipelineResources::DynamicData*  PipelineResourcesHelper::CloneDynamicData (const PipelineResources &res)
	{
		SHAREDLOCK( res._rcCheck );

		auto&	data	= *Cast<PipelineResources::DynamicData>( res._dataPtr );
		auto*	result	= Cast<PipelineResources::DynamicData>( Allocator::Allocate( data.memSize ));
		
		memcpy( OUT result, &data, size_t(data.memSize) );
		return result;
	}

/*
=================================================
	CreateDynamicData
=================================================
*/
	PipelineResources::DynamicData*
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
		data->deallocate		= &PRs_DynamicData_Deallocator;
		data->allocatorPtr		= null;
		data->memSize			= req_size;
		data->uniformCount		= uint(uniforms->size());
		data->uniformsOffset	= uint(mem.Offset());
		auto*	uniforms_ptr	= mem.EmplaceArray<PRs::Uniform>( uniforms->size() );

		data->dynamicOffsetsCount	= bufferDynamicOffsetCount;
		data->dynamicOffsetsOffset	= uint(mem.Offset());
		mem.EmplaceArray<uint>( bufferDynamicOffsetCount );


		for (auto& un : *uniforms)
		{
			auto&		curr		= uniforms_ptr[ un_index++ ];
			const uint	array_size	= (un.second.arraySize ? un.second.arraySize : FG_MaxElementsInUnsizedDesc);
			
			curr.id = un.first;

			Visit(	un.second.data,
					[&] (const PipelineDescription::Texture &tex)
					{
						curr.resType	= PRs::Texture::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::Texture>( BytesU{sizeof(PRs::Texture) + sizeof(PRs::Texture::Element) * (array_size-1)},
														un.second.index, tex.state, array_size );
					},

					[&] (const PipelineDescription::Sampler &)
					{
						curr.resType	= PRs::Sampler::TypeId;
						curr.offset		= uint16_t(mem.Offset());
						
						mem.EmplaceSized<PRs::Sampler>( BytesU{sizeof(PRs::Sampler) + sizeof(PRs::Sampler::Element) * (array_size-1)},
													    un.second.index, array_size );
					},

					[&] (const PipelineDescription::SubpassInput &spi)
					{
						curr.resType	= PRs::Image::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::Image>( BytesU{sizeof(PRs::Image) + sizeof(PRs::Image::Element) * (array_size-1)},
													  un.second.index, spi.state, array_size );
					},

					[&] (const PipelineDescription::Image &img)
					{
						curr.resType	= PRs::Image::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::Image>( BytesU{sizeof(PRs::Image) + sizeof(PRs::Image::Element) * (array_size-1)},
													  un.second.index, img.state, array_size );
					},

					[&] (const PipelineDescription::UniformBuffer &ubuf)
					{
						dbo_count		+= uint(ubuf.dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET) * array_size;
						curr.resType	= PRs::Buffer::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::Buffer>( BytesU{sizeof(PRs::Buffer) + sizeof(PRs::Buffer::Element) * (array_size-1)},
													   un.second.index, ubuf.state, ubuf.dynamicOffsetIndex, array_size );
						// TODO: initialize array
					},

					[&] (const PipelineDescription::StorageBuffer &sbuf)
					{
						dbo_count		+= uint(sbuf.dynamicOffsetIndex != PipelineDescription::STATIC_OFFSET) * array_size;
						curr.resType	= PRs::Buffer::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::Buffer>( BytesU{sizeof(PRs::Buffer) + sizeof(PRs::Buffer::Element) * (array_size-1)},
													   un.second.index, sbuf.state, sbuf.dynamicOffsetIndex, array_size );
						// TODO: initialize array
					},

					[&] (const PipelineDescription::RayTracingScene &)
					{
						curr.resType	= PRs::RayTracingScene::TypeId;
						curr.offset		= uint16_t(mem.Offset());

						mem.EmplaceSized<PRs::RayTracingScene>( BytesU{sizeof(PRs::RayTracingScene) + sizeof(PRs::RayTracingScene::Element) * (array_size-1)},
																un.second.index, array_size );
					},

					[] (const NullUnion &) { ASSERT(false); }
				);
		}

		std::sort( uniforms_ptr, uniforms_ptr + data->uniformCount,
				   [] (auto& lhs, auto& rhs) { return lhs.id < rhs.id; } );

		ASSERT( dbo_count == bufferDynamicOffsetCount );
		return data;
	}
	

}	// FG
