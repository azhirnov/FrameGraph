// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/IDs.h"
#include "framegraph/Public/ImageDesc.h"
#include "framegraph/Public/Pipeline.h"
#include "stl/ThreadSafe/RaceConditionCheck.h"

namespace FG
{

	//
	// Pipeline Resources
	//

	struct PipelineResources final
	{
		friend class PipelineResourcesHelper;

	// types
	public:
		using Self	= PipelineResources;
		
		enum class EDescriptorType : uint16_t
		{
			Unknown		= 0,
			Buffer,
			Image,
			Texture,
			SubpassInput,
			Sampler,
			RayTracingScene
		};

		struct Buffer
		{
			static constexpr EDescriptorType	TypeId = EDescriptorType::Buffer;

			struct Element {
				RawBufferID		bufferId;
				BytesU			offset;
				BytesU			size;
			};

			BindingIndex		index;
			EResourceState		state;
			uint				dynamicOffsetIndex;
			uint				elementCount;
			Element				elements[1];
		};

		struct Image
		{
			static constexpr EDescriptorType	TypeId = EDescriptorType::Image;

			struct Element {
				RawImageID		imageId;
				ImageViewDesc	desc;
				bool			hasDesc;
			};

			BindingIndex		index;
			EResourceState		state;
			uint				elementCount;
			Element				elements[1];
		};

		struct Texture
		{
			static constexpr EDescriptorType	TypeId = EDescriptorType::Texture;

			struct Element {
				RawImageID		imageId;
				RawSamplerID	samplerId;
				ImageViewDesc	desc;
				bool			hasDesc;
			};
			
			BindingIndex		index;
			EResourceState		state;
			uint				elementCount;
			Element				elements[1];
		};

		/*struct SubpassInput : Image
		{
		};*/

		struct Sampler
		{
			static constexpr EDescriptorType	TypeId = EDescriptorType::Sampler;

			struct Element {
				RawSamplerID	samplerId;
			};

			BindingIndex		index;
			uint				elementCount;
			Element				elements[1];
		};

		struct RayTracingScene
		{
			static constexpr EDescriptorType	TypeId = EDescriptorType::RayTracingScene;

			struct Element {
				RawRTSceneID	sceneId;
			};

			BindingIndex		index;
			uint				elementCount;
			Element				elements[1];
		};

		using CachedID			= std::atomic< uint64_t >;
		using DeallocatorFn_t	= void (*) (void*, void*, BytesU);

		struct Uniform
		{
			UniformID		id;
			EDescriptorType	resType		= Default;
			uint16_t		offset		= 0;

			ND_ bool  operator == (const UniformID &rhs) const	{ return id == rhs; }
			ND_ bool  operator <  (const UniformID &rhs) const	{ return id < rhs; }
		};

		struct DynamicData
		{
			RawDescriptorSetLayoutID	layoutId;
			uint						uniformCount		= 0;
			uint						uniformsOffset		= 0;
			uint						dynamicOffsetsCount	= 0;
			uint						dynamicOffsetsOffset= 0;
			DeallocatorFn_t				deallocate			= null;
			void *						allocatorPtr		= null;
			BytesU						memSize;

			ND_ Uniform	*	Uniforms ()			{ return Cast<Uniform>(this + BytesU{uniformsOffset}); }
			ND_ uint*		DynamicOffsets ()	{ return Cast<uint>(this + BytesU{dynamicOffsetsOffset}); }
				void		Dealloc ()			{ deallocate( allocatorPtr, this, memSize ); }
				
			template <typename T, typename Fn>	static void  _ForEachUniform (T&&, Fn &&);
			template <typename Fn>				void ForEachUniform (Fn&& fn)				{ _ForEachUniform( *this, fn ); }
			template <typename Fn>				void ForEachUniform (Fn&& fn) const			{ _ForEachUniform( *this, fn ); }
		};


	// variables
	private:
		DynamicData *			_dataPtr				= null;
		bool					_allowEmptyResources	= false;
		mutable CachedID		_cachedId;
		RWRaceConditionCheck	_rcCheck;


	// methods
	public:
		PipelineResources ()
		{
			_ResetCachedID();
		}

		~PipelineResources ();
		
		explicit PipelineResources (const PipelineResources &other);
		
		PipelineResources (PipelineResources &&other) :
			_dataPtr{ other._dataPtr },
			_allowEmptyResources{ other._allowEmptyResources }
		{
			other._dataPtr = null;
			_SetCachedID( other._GetCachedID() );
		}

		Self&  BindImage (const UniformID &id, const ImageID &image, uint elementIndex = 0);
		Self&  BindImage (const UniformID &id, const ImageID &image, const ImageViewDesc &desc, uint elementIndex = 0);
		Self&  BindImage (const UniformID &id, RawImageID image, uint elementIndex = 0);
		Self&  BindImage (const UniformID &id, RawImageID image, const ImageViewDesc &desc, uint elementIndex = 0);
		Self&  BindImages (const UniformID &id, ArrayView<ImageID> images);

		Self&  BindTexture (const UniformID &id, const ImageID &image, const SamplerID &sampler, uint elementIndex = 0);
		Self&  BindTexture (const UniformID &id, const ImageID &image, const SamplerID &sampler, const ImageViewDesc &desc, uint elementIndex = 0);
		Self&  BindTexture (const UniformID &id, RawImageID image, const SamplerID &sampler, uint elementIndex = 0);
		Self&  BindTexture (const UniformID &id, RawImageID image, const SamplerID &sampler, const ImageViewDesc &desc, uint elementIndex = 0);
		Self&  BindTextures (const UniformID &id, ArrayView<ImageID> images, const SamplerID &sampler);

		Self&  BindSampler (const UniformID &id, const SamplerID &sampler, uint elementIndex = 0);

		Self&  BindBuffer (const UniformID &id, const BufferID &buffer, uint elementIndex = 0);
		Self&  BindBuffer (const UniformID &id, const BufferID &buffer, BytesU offset, BytesU size, uint elementIndex = 0);
		Self&  BindBuffer (const UniformID &id, RawBufferID buffer, uint elementIndex = 0);
		Self&  BindBuffer (const UniformID &id, RawBufferID buffer, BytesU offset, BytesU size, uint elementIndex = 0);
		Self&  SetBufferBase (const UniformID &id, BytesU offset, uint elementIndex = 0);

		Self&  BindRayTracingScene (const UniformID &id, const RTSceneID &scene, uint elementIndex = 0);

		void  AllowEmptyResources (bool value)								{ EXLOCK(_rcCheck);  _allowEmptyResources = value; }

		ND_ bool  HasImage (const UniformID &id)					const	{ SHAREDLOCK(_rcCheck);  return _HasResource< Image >( id ); }
		ND_ bool  HasSampler (const UniformID &id)					const	{ SHAREDLOCK(_rcCheck);  return _HasResource< Sampler >( id ); }
		ND_ bool  HasTexture (const UniformID &id)					const	{ SHAREDLOCK(_rcCheck);  return _HasResource< Texture >( id ); }
		ND_ bool  HasBuffer (const UniformID &id)					const	{ SHAREDLOCK(_rcCheck);  return _HasResource< Buffer >( id ); }
		ND_ bool  HasRayTracingScene (const UniformID &id)			const	{ SHAREDLOCK(_rcCheck);  return _HasResource< RayTracingScene >( id ); }

		ND_ RawDescriptorSetLayoutID	GetLayout ()				const	{ SHAREDLOCK(_rcCheck);  return _dataPtr->layoutId; }
		ND_ ArrayView< uint >			GetDynamicOffsets ()		const	{ SHAREDLOCK(_rcCheck);  return {_dataPtr->DynamicOffsets(), _dataPtr->dynamicOffsetsCount}; }
		ND_ bool						IsEmptyResourcesAllowed ()	const	{ SHAREDLOCK(_rcCheck);  return _allowEmptyResources; }
		ND_ bool						IsInitialized ()			const	{ SHAREDLOCK(_rcCheck);  return _dataPtr != null; }


	private:
		void _SetCachedID (RawPipelineResourcesID id)		const	{ _cachedId.store( BitCast<uint64_t>(id), memory_order_release ); }
		void _ResetCachedID ()								const;
		
		ND_ RawPipelineResourcesID	_GetCachedID ()			const	{ return BitCast<RawPipelineResourcesID>( _cachedId.load( memory_order_acquire )); }

		ND_ uint &					_GetDynamicOffset (uint i)		{ ASSERT( i < _dataPtr->dynamicOffsetsCount );  return _dataPtr->DynamicOffsets()[i]; }

		template <typename T> T &	_GetResource (const UniformID &id);
		template <typename T> bool	_HasResource (const UniformID &id) const;
	};


	using PipelineResourceSet	= FixedMap< DescriptorSetID, Ptr<const PipelineResources>, FG_MaxDescriptorSets >;

	

/*
=================================================
	BindImage
=================================================
*/
	inline PipelineResources&  PipelineResources::BindImage (const UniformID &id, const ImageID &image, uint index)
	{
		return BindImage( id, image.Get(), index );
	}
	
	inline PipelineResources&  PipelineResources::BindImage (const UniformID &id, const ImageID &image, const ImageViewDesc &desc, uint index)
	{
		return BindImage( id, image.Get(), desc, index );
	}

/*
=================================================
	BindTexture
=================================================
*/
	inline PipelineResources&  PipelineResources::BindTexture (const UniformID &id, const ImageID &image, const SamplerID &sampler, uint index)
	{
		return BindTexture( id, image.Get(), sampler, index );
	}
	
	inline PipelineResources&  PipelineResources::BindTexture (const UniformID &id, const ImageID &image, const SamplerID &sampler, const ImageViewDesc &desc, uint index)
	{
		return BindTexture( id, image.Get(), sampler, desc, index );
	}

/*
=================================================
	BindBuffer
=================================================
*/
	inline PipelineResources&  PipelineResources::BindBuffer (const UniformID &id, const BufferID &buffer, uint index)
	{
		return BindBuffer( id, buffer.Get(), index );
	}
	
	inline PipelineResources&  PipelineResources::BindBuffer (const UniformID &id, const BufferID &buffer, BytesU offset, BytesU size, uint index)
	{
		return BindBuffer( id, buffer.Get(), offset, size, index );
	}
	
/*
=================================================
	_ForEachUniform
=================================================
*/
	template <typename T, typename Fn>
	inline void  PipelineResources::DynamicData::_ForEachUniform (T&& self, Fn&& fn)
	{
		STATIC_ASSERT( IsSameTypes< std::remove_cv_t<std::remove_reference_t<T>>, DynamicData > );

		for (uint i = 0, cnt = self.uniformCount; i < cnt; ++i)
		{
			auto&	un  = self.Uniforms()[i];
			auto*	ptr = (&self + BytesU{un.offset});

			ENABLE_ENUM_CHECKS();
			switch ( un.resType )
			{
				case EDescriptorType::Unknown :			break;
				case EDescriptorType::Buffer :			fn( *Cast<Buffer>(ptr) );			break;
				case EDescriptorType::SubpassInput :
				case EDescriptorType::Image :			fn( *Cast<Image>(ptr) );			break;
				case EDescriptorType::Texture :			fn( *Cast<Texture>(ptr) );			break;
				case EDescriptorType::Sampler :			fn( *Cast<Sampler>(ptr) );			break;
				case EDescriptorType::RayTracingScene :	fn( *Cast<RayTracingScene>(ptr) );	break;
			}
			DISABLE_ENUM_CHECKS();
		}
	}


}	// FG
