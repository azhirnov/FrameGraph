// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/CommandBuffer.h"
#include "VCommon.h"

namespace FG
{

	//
	// Command Batch pointer
	//

	struct VCmdBatchPtr
	{
	// variables
	private:
		class VCmdBatch *	_ptr	= null;

	// methods
	public:
		VCmdBatchPtr ()												{}
		VCmdBatchPtr (VCmdBatch* ptr) : _ptr{ptr}					{ _IncRef(); }
		VCmdBatchPtr (const VCmdBatchPtr &other) : _ptr{other._ptr}	{ _IncRef(); }
		VCmdBatchPtr (VCmdBatchPtr &&other) : _ptr{other._ptr}		{ other._ptr = null; }
		~VCmdBatchPtr ()											{ _DecRef(); }

		VCmdBatchPtr&	operator = (const VCmdBatchPtr &rhs)		{ _DecRef();  _ptr = rhs._ptr;  _IncRef();  return *this; }
		VCmdBatchPtr&	operator = (VCmdBatchPtr &&rhs)				{ _DecRef();  _ptr = rhs._ptr;  rhs._ptr = null;  return *this; }

		ND_ VCmdBatch*	operator -> ()		const					{ ASSERT( _ptr );  return _ptr; }
		ND_ explicit	operator bool ()	const					{ return _ptr != null; }

		ND_ VCmdBatch*	get ()				const					{ return _ptr; }


	private:
		void _IncRef ();
		void _DecRef ();
	};



	//
	// Vulkan Command Batch
	//

	class VCmdBatch final : public CommandBuffer::Batch
	{
		friend struct VCmdBatchPtr;
		friend class  VCommandBuffer;	// TODO: remove

	// types
	public:
		struct Resource
		{
		// constants
			static constexpr uint		IndexOffset		= 0;
			static constexpr uint		InstanceOffset	= sizeof(_fg_hidden_::ResourceID<0>::Index_t) * 8;
			static constexpr uint		IDOffset		= sizeof(_fg_hidden_::ResourceID<0>::InstanceID_t) * 8;
			static constexpr uint64_t	IndexMask		= uint64_t(InstanceOffset) - 1;
			static constexpr uint64_t	InstanceMask	= (uint64_t(IDOffset) - 1) & ~IndexMask;
			static constexpr uint64_t	IDMask			= 0xFF << IDOffset;
			using Index_t				= RawImageID::Index_t;
			using InstanceID_t			= RawImageID::InstanceID_t;

		// variables
			uint64_t	value	= UMax;
			
		// methods
			Resource () {}

			template <uint UID>
			explicit Resource (_fg_hidden_::ResourceID<UID> id) :
				value{ (uint64_t(id.Index()) << IndexOffset) | (uint64_t(id.InstanceID()) << InstanceOffset) | (uint64_t(id.GetUID()) << IDOffset) }
			{}

			ND_ bool  operator == (const Resource &rhs)	const	{ return value == rhs.value; }

			ND_ Index_t			Index ()				const	{ return uint((value & IndexMask) >> IndexOffset); }
			ND_ InstanceID_t	InstanceID ()			const	{ return uint((value & InstanceMask) >> InstanceOffset); }
			ND_ uint			GetUID ()				const	{ return uint((value & IDMask) >> IDOffset); }
		};

		struct ResourceHash {
			ND_ size_t  operator () (const Resource &x) const noexcept {
				return std::hash<decltype(x.value)>{}( x.value );
			}
		};
		
		using ResourceMap_t		= std::unordered_set< Resource, ResourceHash >;		// TODO: custom allocator
		

		static constexpr uint	MaxBufferParts	= 3;
		static constexpr uint	MaxImageParts	= 4;


		struct StagingBuffer
		{
		// variables
			BufferID		bufferId;
			RawMemoryID		memoryId;
			BytesU			capacity;
			BytesU			size;
			
			void *			mappedPtr	= null;
			BytesU			memOffset;					// can be used to flush memory ranges
			VkDeviceMemory	mem			= VK_NULL_HANDLE;
			bool			isCoherent	= false;

		// methods
			StagingBuffer () {}

			StagingBuffer (BufferID &&buf, RawMemoryID mem, BytesU capacity) :
				bufferId{std::move(buf)}, memoryId{mem}, capacity{capacity} {}

			ND_ bool	IsFull ()	const	{ return size >= capacity; }
			ND_ bool	Empty ()	const	{ return size == 0_b; }
		};


		struct OnBufferDataLoadedEvent
		{
		// types
			struct Range
			{
				StagingBuffer const*	buffer;
				BytesU					offset;
				BytesU					size;
			};

			using DataParts_t	= FixedArray< Range, MaxBufferParts >;
			using Callback_t	= ReadBuffer::Callback_t;
			
		// variables
			Callback_t		callback;
			DataParts_t		parts;
			BytesU			totalSize;

		// methods
			OnBufferDataLoadedEvent () {}
			OnBufferDataLoadedEvent (const Callback_t &cb, BytesU size) : callback{cb}, totalSize{size} {}
		};


		struct OnImageDataLoadedEvent
		{
		// types
			using Range			= OnBufferDataLoadedEvent::Range;
			using DataParts_t	= FixedArray< Range, MaxImageParts >;
			using Callback_t	= ReadImage::Callback_t;

		// variables
			Callback_t		callback;
			DataParts_t		parts;
			BytesU			totalSize;
			uint3			imageSize;
			BytesU			rowPitch;
			BytesU			slicePitch;
			EPixelFormat	format		= Default;
			EImageAspect	aspect		= EImageAspect::Color;

		// methods
			OnImageDataLoadedEvent () {}

			OnImageDataLoadedEvent (const Callback_t &cb, BytesU size, const uint3 &imageSize,
									BytesU rowPitch, BytesU slicePitch, EPixelFormat fmt, EImageAspect asp) :
				callback{cb}, totalSize{size}, imageSize{imageSize}, rowPitch{rowPitch},
				slicePitch{slicePitch}, format{fmt}, aspect{asp} {}
		};

		static constexpr uint	MaxDependencies	= 16;
		using Dependencies_t	= FixedArray< VCmdBatchPtr, MaxDependencies >;


	public:
		enum class EState : uint
		{
			Recording,		// build command buffers
			Backed,			// command buffers builded, all data locked
			Ready,			// all dependencies in 'Ready', 'Submitted' or 'Complete' states
			Submitted,		// commands was submitted to the GPU
			Complete,		// commands complete execution on the GPU
		};


	// variables
	private:
		std::atomic<EState>						_state;
		const VDeviceQueueInfoPtr				_queue;
		const EQueueUsage						_usage;

		Dependencies_t							_dependencies;
		
		// command batch data
		FixedArray< VkCommandBuffer, 8 >		_commands;
		FixedArray< VkSemaphore, 8 >			_signalSemaphores;
		FixedArray< VkSemaphore, 8 >			_waitSemaphores;
		FixedArray< VkPipelineStageFlags, 8 >	_waitDstStages;

		// staging buffers
		FixedArray< StagingBuffer, 8 >			_hostToDevice;	// CPU write, GPU read
		FixedArray< StagingBuffer, 8 >			_deviceToHost;	// CPU read, GPU write
		Array< OnBufferDataLoadedEvent >		_onBufferLoadedEvents;
		Array< OnImageDataLoadedEvent >			_onImageLoadedEvents;

		// resources
		ResourceMap_t							_resourcesToRelease;
		
		VSubmittedPtr							_submitted;


	// methods
	public:
		VCmdBatch (VDeviceQueueInfoPtr queue, EQueueUsage usage, ArrayView<CommandBuffer> dependsOn);
		~VCmdBatch ();

		void Release () override;
		
		bool OnBacked (INOUT ResourceMap_t &);
		bool OnReadyToSubmit ();
		bool OnSubmit (OUT VkSubmitInfo &, const VSubmittedPtr &);
		bool OnComplete (VResourceManager &);

		void SignalSemaphore (VkSemaphore sem);
		void WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage);
		void AddCommandBuffer (VkCommandBuffer cmd);
		void AddDependency (VCmdBatch *);

		ND_ VDeviceQueueInfoPtr		GetQueue ()			const	{ return _queue; }
		ND_ EQueueUsage				GetQueueUsage ()	const	{ return _usage; }
		ND_ EState					GetState ()					{ return _state.load( memory_order_relaxed ); }
		ND_ ArrayView<VCmdBatchPtr>	GetDependencies ()	const	{ return _dependencies; }
		ND_ VSubmittedPtr const&	GetSubmitted ()		const	{ return _submitted; }		// TODO: rename

	private:
		void _SetState (EState newState);
		void _ReleaseResources (VResourceManager &);
		
		bool _MapMemory (VResourceManager &, INOUT StagingBuffer &) const;
	};


	
/*
=================================================
	_IncRef
=================================================
*/
	inline void VCmdBatchPtr::_IncRef ()
	{
		if ( _ptr )
			_ptr->_counter.fetch_add( 1, memory_order_relaxed );
	}

/*
=================================================
	_DecRef
=================================================
*/
	inline void VCmdBatchPtr::_DecRef ()
	{
		if ( _ptr )
		{
			auto	count = _ptr->_counter.fetch_sub( 1, memory_order_relaxed );
			ASSERT( count > 0 );

			if ( count == 1 ) {
				std::atomic_thread_fence( std::memory_order_acquire );
				_ptr->Release();
			}
		}
	}


}	// FG
