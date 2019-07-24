// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/IDs.h"
#include "framegraph/Public/ImageDesc.h"
#include "framegraph/Public/Pipeline.h"
#include "stl/ThreadSafe/DataRaceCheck.h"

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
			BytesU				staticSize;
			BytesU				arrayStride;
			const uint16_t		elementCapacity;
			uint16_t			elementCount;
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
			const uint16_t		elementCapacity;
			uint16_t			elementCount;
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
			const uint16_t		elementCapacity;
			uint16_t			elementCount;
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
			const uint16_t		elementCapacity;
			uint16_t			elementCount;
			Element				elements[1];
		};

		struct RayTracingScene
		{
			static constexpr EDescriptorType	TypeId = EDescriptorType::RayTracingScene;

			struct Element {
				RawRTSceneID	sceneId;
			};

			BindingIndex		index;
			const uint16_t		elementCapacity;
			uint16_t			elementCount;
			Element				elements[1];
		};

		using CachedID			= std::atomic< RawPipelineResourcesID::Value_t >;
		using DeallocatorFn_t	= void (*) (void*, void*, BytesU);

		struct Uniform
		{
			UniformID		id;
			EDescriptorType	resType		= Default;
			uint16_t		offset		= 0;

			// for sorting and searching
			ND_ bool  operator == (const UniformID &rhs) const	{ return id == rhs; }
			ND_ bool  operator <  (const UniformID &rhs) const	{ return id <  rhs; }
		};

		struct DynamicData
		{
			RawDescriptorSetLayoutID	layoutId;
			uint						uniformCount		= 0;
			uint						uniformsOffset		= 0;
			uint						dynamicOffsetsCount	= 0;
			uint						dynamicOffsetsOffset= 0;
			BytesU						memSize;

			DynamicData () {}

			ND_ Uniform	*		Uniforms ()					{ return Cast<Uniform>(this + BytesU{uniformsOffset}); }
			ND_ Uniform	const*	Uniforms ()			const	{ return Cast<Uniform>(this + BytesU{uniformsOffset}); }
			ND_ uint*			DynamicOffsets ()			{ return Cast<uint>(this + BytesU{dynamicOffsetsOffset}); }

			ND_ HashVal			CalcHash () const;
			ND_ bool			operator == (const DynamicData &) const;
				
			template <typename T, typename Fn>	static void  _ForEachUniform (T&&, Fn &&);
			template <typename Fn>				void ForEachUniform (Fn&& fn)				{ _ForEachUniform( *this, fn ); }
			template <typename Fn>				void ForEachUniform (Fn&& fn) const			{ _ForEachUniform( *this, fn ); }
		};
	
		struct DynamicDataDeleter
		{
            constexpr DynamicDataDeleter () {}

            DynamicDataDeleter (const DynamicDataDeleter &) {}

            void operator () (DynamicData *) const;
		};

		using DynamicDataPtr = std::unique_ptr< DynamicData, DynamicDataDeleter >;


	// variables
	private:
		DynamicDataPtr			_dataPtr;
		bool					_allowEmptyResources	= false;
		mutable CachedID		_cachedId;
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		PipelineResources ()
		{
			_ResetCachedID();
		}

		~PipelineResources ();
		
		explicit PipelineResources (const PipelineResources &other);
		
		PipelineResources (PipelineResources &&other) :
			_dataPtr{ std::move(other._dataPtr) },
			_allowEmptyResources{ other._allowEmptyResources }
		{
			_SetCachedID( other._GetCachedID() );
		}

		Self&  BindImage (const UniformID &id, RawImageID image, uint elementIndex = 0);
		Self&  BindImage (const UniformID &id, RawImageID image, const ImageViewDesc &desc, uint elementIndex = 0);
		Self&  BindImages (const UniformID &id, ArrayView<ImageID> images);
		Self&  BindImages (const UniformID &id, ArrayView<RawImageID> images);

		Self&  BindTexture (const UniformID &id, RawImageID image, RawSamplerID sampler, uint elementIndex = 0);
		Self&  BindTexture (const UniformID &id, RawImageID image, RawSamplerID sampler, const ImageViewDesc &desc, uint elementIndex = 0);
		Self&  BindTextures (const UniformID &id, ArrayView<ImageID> images, RawSamplerID sampler);
		Self&  BindTextures (const UniformID &id, ArrayView<RawImageID> images, RawSamplerID sampler);

		Self&  BindSampler (const UniformID &id, RawSamplerID sampler, uint elementIndex = 0);
		Self&  BindSamplers (const UniformID &id, ArrayView<SamplerID> samplers);
		Self&  BindSamplers (const UniformID &id, ArrayView<RawSamplerID> samplers);

		Self&  BindBuffer (const UniformID &id, RawBufferID buffer, uint elementIndex = 0);
		Self&  BindBuffer (const UniformID &id, RawBufferID buffer, BytesU offset, BytesU size, uint elementIndex = 0);
		Self&  BindBuffers (const UniformID &id, ArrayView<BufferID> buffers);
		Self&  BindBuffers (const UniformID &id, ArrayView<RawBufferID> buffers);
		Self&  SetBufferBase (const UniformID &id, BytesU offset, uint elementIndex = 0);

		Self&  BindRayTracingScene (const UniformID &id, RawRTSceneID scene, uint elementIndex = 0);

		void  AllowEmptyResources (bool value)								{ EXLOCK(_drCheck);  _allowEmptyResources = value; }

		ND_ bool  HasImage (const UniformID &id)					const;
		ND_ bool  HasSampler (const UniformID &id)					const;
		ND_ bool  HasTexture (const UniformID &id)					const;
		ND_ bool  HasBuffer (const UniformID &id)					const;
		ND_ bool  HasRayTracingScene (const UniformID &id)			const;

		ND_ RawDescriptorSetLayoutID	GetLayout ()				const	{ SHAREDLOCK(_drCheck); ASSERT(_dataPtr);  return _dataPtr->layoutId; }
		ND_ ArrayView< uint >			GetDynamicOffsets ()		const;
		ND_ bool						IsEmptyResourcesAllowed ()	const	{ SHAREDLOCK(_drCheck);  return _allowEmptyResources; }
		ND_ bool						IsInitialized ()			const	{ SHAREDLOCK(_drCheck);  return _dataPtr != null; }


	private:
		void _SetCachedID (RawPipelineResourcesID id)		const	{ _cachedId.store( BitCast<uint>(id), memory_order_relaxed ); }
		void _ResetCachedID ()								const	{ _cachedId.store( UMax, memory_order_relaxed ); }
		
		ND_ RawPipelineResourcesID	_GetCachedID ()			const	{ return BitCast<RawPipelineResourcesID>( _cachedId.load( memory_order_acquire )); }

		ND_ uint &					_GetDynamicOffset (uint i)		{ ASSERT( _dataPtr and i < _dataPtr->dynamicOffsetsCount );  return _dataPtr->DynamicOffsets()[i]; }

		template <typename T> T *	_GetResource (const UniformID &id);
		template <typename T> bool	_HasResource (const UniformID &id) const;
	};


	using PipelineResourceSet	= FixedMap< DescriptorSetID, Ptr<const PipelineResources>, FG_MaxDescriptorSets >;

	
	
/*
=================================================
	GetDynamicOffsets
=================================================
*/
	inline ArrayView<uint>  PipelineResources::GetDynamicOffsets () const
	{
		SHAREDLOCK( _drCheck );
		return _dataPtr ? ArrayView<uint>{ _dataPtr->DynamicOffsets(), _dataPtr->dynamicOffsetsCount } : ArrayView<uint>{};
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
				case EDescriptorType::Buffer :			fn( un.id, *Cast<Buffer>(ptr) );			break;
				case EDescriptorType::SubpassInput :
				case EDescriptorType::Image :			fn( un.id, *Cast<Image>(ptr) );				break;
				case EDescriptorType::Texture :			fn( un.id, *Cast<Texture>(ptr) );			break;
				case EDescriptorType::Sampler :			fn( un.id, *Cast<Sampler>(ptr) );			break;
				case EDescriptorType::RayTracingScene :	fn( un.id, *Cast<RayTracingScene>(ptr) );	break;
			}
			DISABLE_ENUM_CHECKS();
		}
	}


}	// FG
