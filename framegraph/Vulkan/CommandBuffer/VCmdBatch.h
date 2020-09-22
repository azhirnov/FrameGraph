// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/CommandBuffer.h"
#include "framegraph/Public/FrameGraph.h"
#include "VDescriptorSetLayout.h"
#include "VLocalDebugger.h"
#include "VCommandPool.h"
#include "stl/Containers/FixedTupleArray.h"

namespace FG
{
	enum class StagingBufferIdx : uint {};



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
		
		VCmdBatchPtr&	operator = (VCmdBatch* rhs)					{ _DecRef();  _ptr = rhs;  _IncRef();  return *this; }
		VCmdBatchPtr&	operator = (const VCmdBatchPtr &rhs)		{ _DecRef();  _ptr = rhs._ptr;  _IncRef();  return *this; }
		VCmdBatchPtr&	operator = (VCmdBatchPtr &&rhs)				{ _DecRef();  _ptr = rhs._ptr;  rhs._ptr = null;  return *this; }
		VCmdBatchPtr&	operator = (std::nullptr_t)					{ _DecRef();  _ptr = null;  return *this; }

		ND_ VCmdBatch*	operator -> ()						const	{ ASSERT( _ptr );  return _ptr; }
		ND_ VCmdBatch&	operator *  ()						const	{ ASSERT( _ptr );  return *_ptr; }

		ND_ bool		operator == (VCmdBatch* rhs)		 const	{ return _ptr == rhs; }
		ND_ bool		operator == (const VCmdBatchPtr &rhs)const	{ return _ptr == rhs._ptr; }

		ND_ explicit	operator bool ()					const	{ return _ptr != null; }

		ND_ VCmdBatch*	get ()								const	{ return _ptr; }


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
		
		//---------------------------------------------------------------------------
		// acquired resources

		struct Resource
		{
		// constants
			static constexpr uint		IndexOffset		= 0;
			static constexpr uint		InstanceOffset	= sizeof(_fg_hidden_::ResourceID<0>::Index_t) * 8;
			static constexpr uint		IDOffset		= InstanceOffset + sizeof(_fg_hidden_::ResourceID<0>::InstanceID_t) * 8;
			static constexpr uint64_t	IndexMask		= (1ull << InstanceOffset) - 1;
			static constexpr uint64_t	InstanceMask	= ((1ull << IDOffset) - 1) & ~IndexMask;
			static constexpr uint64_t	IDMask			= 0xFFull << IDOffset;
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
			ND_ size_t  operator () (const Resource &x) const {
				return std::hash<decltype(x.value)>{}( x.value );
			}
		};
		
		using ResourceMap_t		= std::unordered_map< Resource, uint, ResourceHash >;		// TODO: custom allocator


		//---------------------------------------------------------------------------
		// staging buffers

		static constexpr uint	MaxBufferParts	= 3;
		static constexpr uint	MaxImageParts	= 4;

		struct StagingBuffer
		{
		// variables
			RawBufferID			bufferId;
			RawMemoryID			memoryId;
			BytesU				capacity;
			BytesU				size;
			StagingBufferIdx	index;
			
			void *				mappedPtr	= null;
			BytesU				memOffset;					// can be used to flush memory ranges
			VkDeviceMemory		mem			= VK_NULL_HANDLE;
			bool				isCoherent	= false;

		// methods
			StagingBuffer () {}

			StagingBuffer (StagingBufferIdx idx, RawBufferID buf, RawMemoryID mem, BytesU capacity) :
				bufferId{std::move(buf)}, memoryId{mem}, capacity{capacity}, index{idx} {}

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

		
		//---------------------------------------------------------------------------
		// shader debugger

		using SharedShaderPtr	= PipelineDescription::VkShaderPtr;
		using ShaderModules_t	= FixedArray< SharedShaderPtr, 8 >;
		using TaskName_t		= _fg_hidden_::TaskName_t;

		struct StorageBuffer
		{
			BufferID				shaderTraceBuffer;
			BufferID				readBackBuffer;
			BytesU					capacity;
			BytesU					size;
			VkPipelineStageFlags	stages	= 0;
		};

		struct DebugMode
		{
			ShaderModules_t			modules;
			VkDescriptorSet			descriptorSet	= VK_NULL_HANDLE;
			BytesU					offset;
			BytesU					size;
			uint					sbIndex			= UMax;
			EShaderDebugMode		mode			= Default;
			EShaderStages			shaderStages	= Default;
			TaskName_t				taskName;
			uint					data[4]			= {};
		};

		using StorageBuffers_t		= Array< StorageBuffer >;
		using DebugModes_t			= Array< DebugMode >;
		using DescriptorCache_t		= HashMap< Pair<RawBufferID, RawDescriptorSetLayoutID>, VDescriptorSetLayout::DescriptorSet >;
		using ShaderDebugCallback_t	= IFrameGraph::ShaderDebugCallback_t;
		

		//---------------------------------------------------------------------------
		
		static constexpr uint		MaxDependencies	= 16;
		using Dependencies_t		= FixedArray< VCmdBatchPtr, MaxDependencies >;

		static constexpr uint		MaxSwapchains = 8;
		using Swapchains_t			= FixedArray< VSwapchain const*, MaxSwapchains >;

		using BatchGraph			= VLocalDebugger::BatchGraph;

		static constexpr uint		MaxBatchItems = 8;
		using CmdBuffers_t			= FixedTupleArray< MaxBatchItems, VkCommandBuffer, VCommandPool const* >;
		using SignalSemaphores_t	= FixedArray< VkSemaphore, MaxBatchItems >;
		using WaitSemaphores_t		= FixedTupleArray< MaxBatchItems, VkSemaphore, VkPipelineStageFlags >;
		
		using VkResourceArray_t		= Array<Pair< VkObjectType, uint64_t >>;

		using Statistic_t			= IFrameGraph::Statistics;


	public:
		enum class EState : uint
		{
			Recording,		// build command buffers
			Backed,			// command buffers builded, all data locked
			Ready,			// all dependencies in 'Ready', 'Submitted' or 'Complete' states
			Submitted,		// commands was submitted to the GPU
			Complete,		// commands complete execution on the GPU
			Initial,
		};


	// variables
	private:
		Atomic<EState>						_state;
		VFrameGraph &						_frameGraph;

		const uint							_indexInPool;		// index in VFrameGraph::_cmdBatchPool
		EQueueType							_queueType			= Default;

		Dependencies_t						_dependencies;
		bool								_submitImmediately	= false;
		bool								_supportsQuery		= false;
		bool								_dbgQueueSync		= false;

		// command batch data
		struct {
			CmdBuffers_t						commands;
			SignalSemaphores_t					signalSemaphores;
			WaitSemaphores_t					waitSemaphores;
		}									_batch;

		// staging buffers
		struct {
			FixedArray< StagingBuffer, 8 >		hostToDevice;	// CPU write, GPU read
			FixedArray< StagingBuffer, 8 >		deviceToHost;	// CPU read, GPU write
			Array< OnBufferDataLoadedEvent >	onBufferLoadedEvents;
			Array< OnImageDataLoadedEvent >		onImageLoadedEvents;
		}									_staging;

		// resources
		ResourceMap_t						_resourcesToRelease;
		Swapchains_t						_swapchains;
		VkResourceArray_t					_readyToDelete;

		// shader debugger
		struct {
			StorageBuffers_t					buffers;
			DebugModes_t						modes;
			DescriptorCache_t					descCache;
			BytesU								bufferAlign;
			const BytesU						bufferSize		= 64_Mb;
		}									_shaderDebugger;

		// frame debugger
		String								_debugDump;
		BatchGraph							_debugGraph;
		DebugName_t							_debugName;
		
		Statistic_t							_statistic;

		VSubmitted *						_submitted	= null;	// TODO: should be atomic
		
		RWDataRaceCheck						_drCheck;


	// methods
	public:
		VCmdBatch (VFrameGraph &fg, uint indexInPool);
		~VCmdBatch ();

		void  Initialize (EQueueType type, ArrayView<CommandBuffer> dependsOn);
		void  Release () override;
		
		bool  OnBegin (const CommandBufferDesc &);
		void  OnBeginRecording (VkCommandBuffer cmd);
		void  OnEndRecording (VkCommandBuffer cmd);
		bool  OnBaked (INOUT ResourceMap_t &);
		bool  OnReadyToSubmit ();
		bool  BeforeSubmit (OUT VkSubmitInfo &);
		bool  AfterSubmit (OUT Appendable<VSwapchain const*>, VSubmitted *);
		bool  OnComplete (VDebugger &, const ShaderDebugCallback_t &, INOUT Statistic_t &);

		void  SignalSemaphore (VkSemaphore sem);
		void  WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage);
		void  PushFrontCommandBuffer (VkCommandBuffer, const VCommandPool *);
		void  PushBackCommandBuffer (VkCommandBuffer, const VCommandPool *);
		void  AddDependency (VCmdBatch *);
		void  DestroyPostponed (VkObjectType type, uint64_t handle);
	

		// shader debugger //
		bool  SetShaderModule (ShaderDbgIndex id, const SharedShaderPtr &module);
		bool  GetDebugModeInfo (ShaderDbgIndex id, OUT EShaderDebugMode &mode, OUT EShaderStages &stages) const;
		bool  GetDescriptotSet (ShaderDbgIndex id, OUT uint &binding, OUT VkDescriptorSet &descSet, OUT uint &dynamicOffset) const;
		bool  GetShaderTimemap (ShaderDbgIndex id, OUT RawBufferID &buf, OUT BytesU &offset, OUT BytesU &size, OUT uint2 &dim) const;

		ND_ ShaderDbgIndex  AppendShader (INOUT ArrayView<RectI> &, const TaskName_t &name, const _fg_hidden_::GraphicsShaderDebugMode &mode, BytesU size = 8_Mb);
		ND_ ShaderDbgIndex  AppendShader (const TaskName_t &name, const _fg_hidden_::ComputeShaderDebugMode &mode, BytesU size = 8_Mb);
		ND_ ShaderDbgIndex  AppendShader (const TaskName_t &name, const _fg_hidden_::RayTracingShaderDebugMode &mode, BytesU size = 8_Mb);
		ND_ ShaderDbgIndex  AppendTimemap (const uint2 &dim, EShaderStages stages);


		// staging buffer //
		bool  GetWritable (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
							OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &outSize, OUT void* &mappedPtr);
		bool  AddPendingLoad (BytesU srcOffset, BytesU srcTotalSize, OUT RawBufferID &dstBuffer, OUT OnBufferDataLoadedEvent::Range &range);
		bool  AddPendingLoad (BytesU srcOffset, BytesU srcTotalSize, BytesU srcPitch, OUT RawBufferID &dstBuffer, OUT OnImageDataLoadedEvent::Range &range);
		bool  AddDataLoadedEvent (OnImageDataLoadedEvent &&);
		bool  AddDataLoadedEvent (OnBufferDataLoadedEvent &&);

		ND_ StringView				GetName ()						const	{ SHAREDLOCK( _drCheck );  return _debugName; }
		ND_ EQueueType				GetQueueType ()					const	{ SHAREDLOCK( _drCheck );  return _queueType; }
		ND_ EState					GetState ()								{ return _state.load( memory_order_relaxed ); }
		ND_ ArrayView<VCmdBatchPtr>	GetDependencies ()				const	{ SHAREDLOCK( _drCheck );  return _dependencies; }
		ND_ VSubmitted *			GetSubmitted ()					const	{ SHAREDLOCK( _drCheck );  return _submitted; }		// TODO: rename
		ND_ uint					GetIndexInPool ()				const	{ return _indexInPool; }
		ND_ bool					IsQueueSyncRequired ()			const	{ SHAREDLOCK( _drCheck );  return _dbgQueueSync; }


	private:
		void  _SetState (EState newState);
		void  _ReleaseResources ();
		void  _ReleaseVkObjects ();
		void  _FinalizeCommands ();

		
		// shader debugger //
		void  _BeginShaderDebugger (VkCommandBuffer cmd);
		void  _EndShaderDebugger (VkCommandBuffer cmd);
		bool  _AllocStorage (INOUT DebugMode &, BytesU);
		bool  _AllocDescriptorSet (EShaderDebugMode debugMode, EShaderStages stages,
								   RawBufferID storageBuffer, BytesU size, OUT VkDescriptorSet &descSet);
		void  _ParseDebugOutput (const ShaderDebugCallback_t &cb);
		bool  _ParseDebugOutput2 (const ShaderDebugCallback_t &cb, const DebugMode &dbg, Array<String>&) const;


		// staging buffer //
		bool  _AddPendingLoad (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
							   OUT RawBufferID &dstBuffer, OUT OnBufferDataLoadedEvent::Range &range);
		bool  _MapMemory (INOUT StagingBuffer &) const;
		void  _FinalizeStagingBuffers (const VDevice &);
	};


	
/*
=================================================
	_IncRef
=================================================
*/
	forceinline void  VCmdBatchPtr::_IncRef ()
	{
		if ( _ptr )
			_ptr->_counter.fetch_add( 1, memory_order_relaxed );
	}

/*
=================================================
	_DecRef
=================================================
*/
	forceinline void  VCmdBatchPtr::_DecRef ()
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
