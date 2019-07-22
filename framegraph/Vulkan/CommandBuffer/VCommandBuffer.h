// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraph.h"
#include "VTaskGraph.h"
#include "VBarrierManager.h"
#include "VTaskProcessor.h"
#include "VPipelineCache.h"
#include "VDescriptorManager.h"
#include "VCmdBatch.h"
#include "VFrameGraph.h"

namespace FG
{

	//
	// Vulkan Command Buffer
	//

	class VCommandBuffer final : public ICommandBuffer
	{
	// types
	private:
		enum class EState
		{
			Initial,
			Recording,
			Compiling,
		};

		using TempTaskArray_t	= std::vector< VTask, StdLinearAllocator<VTask> >;
		using TaskGraph_t		= VTaskGraph< VTaskProcessor >;
		using Allocator_t		= LinearAllocator<>;
		using Statistic_t		= IFrameGraph::Statistics;
		using Debugger_t		= UniquePtr< VLocalDebugger >;

		using Resource_t		= VCmdBatch::Resource;
		using ResourceMap_t		= VCmdBatch::ResourceMap_t;
		using StagingBuffer		= VCmdBatch::StagingBuffer;
		
		static constexpr auto	MaxBufferParts	= VCmdBatch::MaxBufferParts;
		static constexpr auto	MaxImageParts	= VCmdBatch::MaxImageParts;
		static constexpr auto	MinBufferPart	= 4_Kb;

		using PerQueueArray_t	= FixedArray< VCommandPool, 4 >;
		
		using Index_t			= VResourceManager::Index_t;
		static constexpr uint	MaxLocalResources = Max( VResourceManager::MaxImages, VResourceManager::MaxBuffers );
		
		template <typename T, size_t CS, size_t MC>
		using PoolTmpl			= ChunkedIndexedPool< ResourceBase<T>, Index_t, CS, MC >;
		
		template <typename Res, typename MainPool, size_t MC>
		struct LocalResPool {
			PoolTmpl< Res, MainPool::capacity()/MC, MC >		pool;
			StaticArray< Index_t, MainPool::capacity() >		toLocal;
			uint												maxLocalIndex	= 0;
			uint												maxGlobalIndex	= uint(MainPool::capacity());
		};

		using LocalImages_t			= LocalResPool< VLocalImage,		VResourceManager::ImagePool_t,		16 >;
		using LocalBuffers_t		= LocalResPool< VLocalBuffer,		VResourceManager::BufferPool_t,		16 >;
		using LocalRTScenes_t		= LocalResPool< VLocalRTScene,		VResourceManager::RTScenePool_t,	16 >;
		using LocalRTGeometries_t	= LocalResPool< VLocalRTGeometry,	VResourceManager::RTGeometryPool_t,	16 >;
		using LogicalRenderPasses_t	= PoolTmpl< VLogicalRenderPass,		1u<<10,								16 >;
		


	// variables
	private:
		Allocator_t				_mainAllocator;
		TaskGraph_t				_taskGraph;
		EState					_state;
		VCmdBatchPtr			_batch;
		EQueueFamily			_queueIndex;

		VFrameGraph &			_instance;
		const uint				_indexInPool;
		VBarrierManager			_barrierMngr;
		VPipelineCache			_pipelineCache;
		Debugger_t				_debugger;

		struct {
			ResourceMap_t			resourceMap;
			LocalImages_t			images;
			LocalBuffers_t			buffers;
			LocalRTScenes_t			rtScenes;
			LocalRTGeometries_t		rtGeometries;
			LogicalRenderPasses_t	logicalRenderPasses;
			uint					logicalRenderPassCount	= 0;
		}						_rm;
		
		PerQueueArray_t			_perQueue;
		DebugName_t				_dbgName;

		DataRaceCheck			_drCheck;


	// methods
	public:
		explicit VCommandBuffer (VFrameGraph &, uint);
		~VCommandBuffer ();

		bool Begin (const CommandBufferDesc &desc, const VCmdBatchPtr &batch, VDeviceQueueInfoPtr queue);
		bool Execute ();

		void SignalSemaphore (VkSemaphore sem);
		void WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage);
		
		FrameGraph	GetFrameGraph () override	{ return _instance.shared_from_this(); }

		RawImageID	GetSwapchainImage (RawSwapchainID swapchain, ESwapchainImage type) override;
		bool		AddExternalCommands (const ExternalCmdBatch_t &) override;
		bool		AddDependency (const CommandBuffer &) override;
		bool		AllocBuffer (BytesU size, BytesU align, OUT RawBufferID &id, OUT BytesU &offset, OUT void* &mapped) override;

		void		AcquireImage (RawImageID id, bool makeMutable, bool invalidate);
		void		AcquireBuffer (RawBufferID id, bool makeMutable);


		// tasks //
		Task		AddTask (const SubmitRenderPass &) override;
		Task		AddTask (const DispatchCompute &) override;
		Task		AddTask (const DispatchComputeIndirect &) override;
		Task		AddTask (const CopyBuffer &) override;
		Task		AddTask (const CopyImage &) override;
		Task		AddTask (const CopyBufferToImage &) override;
		Task		AddTask (const CopyImageToBuffer &) override;
		Task		AddTask (const BlitImage &) override;
		Task		AddTask (const ResolveImage &) override;
		Task		AddTask (const GenerateMipmaps &) override;
		Task		AddTask (const FillBuffer &) override;
		Task		AddTask (const ClearColorImage &) override;
		Task		AddTask (const ClearDepthStencilImage &) override;
		Task		AddTask (const UpdateBuffer &) override;
		Task		AddTask (const UpdateImage &) override;
		Task		AddTask (const ReadBuffer &) override;
		Task		AddTask (const ReadImage &) override;
		Task		AddTask (const Present &) override;
		Task		AddTask (const UpdateRayTracingShaderTable &) override;
		Task		AddTask (const BuildRayTracingGeometry &) override;
		Task		AddTask (const BuildRayTracingScene &) override;
		Task		AddTask (const TraceRays &) override;


		// draw tasks //
		LogicalPassID  CreateRenderPass (const RenderPassDesc &desc) override;

		void		AddTask (LogicalPassID, const DrawVertices &) override;
		void		AddTask (LogicalPassID, const DrawIndexed &) override;
		void		AddTask (LogicalPassID, const DrawVerticesIndirect &) override;
		void		AddTask (LogicalPassID, const DrawIndexedIndirect &) override;
		void		AddTask (LogicalPassID, const DrawMeshes &) override;
		void		AddTask (LogicalPassID, const DrawMeshesIndirect &) override;
		void		AddTask (LogicalPassID, const CustomDraw &) override;


		// resource manager //
		template <uint UID>
		ND_ auto const*				AcquireTemporary (_fg_hidden_::ResourceID<UID> id);
		
		template <uint UID>
		void						ReleaseResource (_fg_hidden_::ResourceID<UID> id);

		ND_ VLogicalRenderPass*		ToLocal (LogicalPassID id);
		ND_ VLocalBuffer const*		ToLocal (RawBufferID id);
		ND_ VLocalImage  const*		ToLocal (RawImageID id);
		ND_ VLocalRTGeometry const*	ToLocal (RawRTGeometryID id);
		ND_ VLocalRTScene const*	ToLocal (RawRTSceneID id);
		ND_ VPipelineResources const* CreateDescriptorSet (const PipelineResources &desc);

		
		ND_ StringView				GetName ()					const	{ EXLOCK( _drCheck );  return _dbgName; }
		ND_ VCmdBatch &				GetBatch ()					const	{ EXLOCK( _drCheck );  return *_batch; }
		ND_ VCmdBatchPtr const&		GetBatchPtr ()				const	{ EXLOCK( _drCheck );  return _batch; }
		ND_ Allocator_t &			GetAllocator ()						{ EXLOCK( _drCheck );  return _mainAllocator; }
		ND_ Statistic_t &			EditStatistic ()					{ EXLOCK( _drCheck );  return _batch->_statistic; }
		ND_ VPipelineCache &		GetPipelineCache ()					{ EXLOCK( _drCheck );  return _pipelineCache; }
		ND_ VBarrierManager &		GetBarrierManager ()				{ EXLOCK( _drCheck );  return _barrierMngr; }
		ND_ Ptr<VLocalDebugger>		GetDebugger ()						{ EXLOCK( _drCheck );  return _debugger.get(); }
		ND_ VDevice const&			GetDevice ()				const	{ return _instance.GetDevice(); }
		ND_ VFrameGraph &			GetInstance ()				const	{ return _instance; }
		ND_ VResourceManager &		GetResourceManager ()		const	{ return _instance.GetResourceManager(); }
		ND_ VMemoryManager &		GetMemoryManager ()			const	{ return GetResourceManager().GetMemoryManager(); }
		ND_ uint					GetIndexInPool ()			const	{ return _indexInPool; }


	private:
	// staging buffer //
		template <typename T>
		bool  _AllocStorage (size_t count, OUT const VLocalBuffer* &buf, OUT VkDeviceSize &offset, OUT T* &ptr);
		bool  _StoreData (const void *dataPtr, BytesU dataSize, BytesU offsetAlign, OUT const VLocalBuffer* &buf, OUT VkDeviceSize &offset);
		bool  _StorePartialData (ArrayView<uint8_t> srcData, BytesU srcOffset, OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);
		bool  _StoreImageData (ArrayView<uint8_t> srcData, BytesU srcOffset, BytesU srcPitch, BytesU srcTotalSize,
							   OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);
		
		ND_ Task  _AddUpdateBufferTask (const UpdateBuffer &);
		ND_ Task  _AddUpdateImageTask (const UpdateImage &);
		ND_ Task  _AddReadBufferTask (const ReadBuffer &);
		ND_ Task  _AddReadImageTask (const ReadImage &);


	// task processor //
		bool  _BuildCommandBuffers ();
		bool  _ProcessTasks (VkCommandBuffer cmd);
		void  _AfterCompilation ();
		

	// resource manager //
		template <typename ID, typename Res, typename MainPool, size_t MC>
		ND_ Res const*  _ToLocal (ID id, INOUT LocalResPool<Res,MainPool,MC> &, StringView msg);

		void  _FlushLocalResourceStates (ExeOrderIndex, VBarrierManager &, Ptr<VLocalDebugger>);
		void  _ResetLocalRemaping ();


	// queue //
		ND_ EQueueUsage	_GetQueueUsage ()	const	{ return EQueueUsage(0) | _batch->GetQueueType(); }
		ND_ bool		_IsRecording ()		const	{ return _state == EState::Recording; }
	};

	
/*
=================================================
	AcquireTemporary
=================================================
*/
	template <uint UID>
	inline auto const*  VCommandBuffer::AcquireTemporary (_fg_hidden_::ResourceID<UID> id)
	{
		auto[iter, inserted] = _rm.resourceMap.insert({ Resource_t{ id }, 1 });

		return _instance.GetResourceManager().GetResource( id, inserted );
	}
		
/*
=================================================
	ReleaseResource
=================================================
*/
	template <uint UID>
	inline void  VCommandBuffer::ReleaseResource (_fg_hidden_::ResourceID<UID> id)
	{
		_rm.resourceMap.insert({ Resource_t{ id }, 0 }).first->second++;
	}
	
/*
=================================================
	CreateDescriptorSet
=================================================
*/
	inline VPipelineResources const*  VCommandBuffer::CreateDescriptorSet (const PipelineResources &desc)
	{
		return GetResourceManager().CreateDescriptorSet( desc, INOUT _rm.resourceMap );
	}


}	// FG
