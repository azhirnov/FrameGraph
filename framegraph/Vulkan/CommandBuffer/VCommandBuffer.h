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
#include "VLocalDebugger.h"

namespace FG
{

	//
	// Vulkan Command Buffer
	//

	class VCommandBuffer final : public ICommandBuffer
	{
	// types
	private:
		struct PerQueueFamily
		{
			VkCommandPool		cmdPool;
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

		using PerQueueArray_t	= FixedArray< PerQueueFamily, 8 >;
		
		using Index_t			= VResourceManager::Index_t;
		static constexpr uint	MaxLocalResources = Max( VResourceManager::MaxImages, VResourceManager::MaxBuffers );
		
		template <typename T>	using PoolTmpl	= ChunkedIndexedPool< ResourceBase<T>, Index_t, MaxLocalResources/16, 16 >;

		using ToLocalImage_t		= StaticArray< Index_t, VResourceManager::MaxImages >;
		using ToLocalBuffer_t		= StaticArray< Index_t, VResourceManager::MaxBuffers >;
		using ToLocalRTScene_t		= StaticArray< Index_t, VResourceManager::MaxRTObjects >;
		using ToLocalRTGeometry_t	= StaticArray< Index_t, VResourceManager::MaxRTObjects >;

		using LocalImages_t			= PoolTmpl< VLocalImage >;
		using LocalBuffers_t		= PoolTmpl< VLocalBuffer >;
		using LogicalRenderPasses_t	= PoolTmpl< VLogicalRenderPass >;
		using LocalRTGeometries_t	= PoolTmpl< VLocalRTGeometry >;
		using LocalRTScenes_t		= PoolTmpl< VLocalRTScene >;


	// variables
	private:
		Allocator_t				_mainAllocator;
		TaskGraph_t				_taskGraph;
		Statistic_t				_statistic;
		EQueueUsage				_currUsage			= Default;
		VCmdBatchPtr			_batch;

		VFrameGraph &			_instance;
		VBarrierManager			_barrierMngr;
		VPipelineCache			_pipelineCache;
		Debugger_t				_debugger;

		struct {
			ResourceMap_t			resourceMap;

			ToLocalImage_t			imageToLocal;
			ToLocalBuffer_t			bufferToLocal;
			ToLocalRTScene_t		rtGeometryToLocal;
			ToLocalRTGeometry_t		rtSceneToLocal;

			LocalImages_t			localImages;
			LocalBuffers_t			localBuffers;
			LogicalRenderPasses_t	logicalRenderPasses;
			LocalRTGeometries_t		localRTGeometries;
			LocalRTScenes_t			localRTScenes;

			uint					localImagesCount		= 0;
			uint					localBuffersCount		= 0;
			uint					logicalRenderPassCount	= 0;
			uint					localRTGeometryCount	= 0;
			uint					localRTSceneCount		= 0;
		}						_rm;
		
		PerQueueArray_t			_perQueue;
		DebugName_t				_dbgName;

		RaceConditionCheck		_rcCheck;


	// methods
	public:
		explicit VCommandBuffer (VFrameGraph &);
		~VCommandBuffer ();

		bool Initialize ();
		void Deinitialize ();

		bool Begin (const CommandBufferDesc &desc, const VCmdBatchPtr &batch);
		bool Execute ();

		void SignalSemaphore (VkSemaphore sem);
		void WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage);

		RawImageID	GetSwapchainImage (RawSwapchainID swapchain, ESwapchainImage type) override;
		bool		AddExternalCommands (const ExternalCmdBatch_t &) override;
		bool		AddDependency (const CommandBuffer &) override;
		bool		AllocBuffer (BytesU size, OUT RawBufferID &id, OUT BytesU &offset, OUT void* &mapped) override;

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
		//Task		AddTask (const PresentVR &) override;
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

		
		ND_ bool					IsRecording ()				const	{ EXLOCK( _rcCheck );  return bool(_batch); }
		ND_ StringView				GetName ()					const	{ EXLOCK( _rcCheck );  return _dbgName; }
		ND_ VCmdBatch &				GetBatch ()					const	{ EXLOCK( _rcCheck );  return *_batch; }
		ND_ VCmdBatchPtr const&		GetBatchPtr ()				const	{ EXLOCK( _rcCheck );  return _batch; }
		ND_ Allocator_t &			GetAllocator ()						{ EXLOCK( _rcCheck );  return _mainAllocator; }
		ND_ Statistic_t &			EditStatistic ()					{ EXLOCK( _rcCheck );  return _statistic; }
		ND_ VPipelineCache &		GetPipelineCache ()					{ EXLOCK( _rcCheck );  return _pipelineCache; }
		ND_ VBarrierManager &		GetBarrierManager ()				{ EXLOCK( _rcCheck );  return _barrierMngr; }
		ND_ Ptr<VLocalDebugger>		GetDebugger ()						{ EXLOCK( _rcCheck );  return _debugger.get(); }
		ND_ VDevice const&			GetDevice ()				const	{ return _instance.GetDevice(); }
		ND_ VFrameGraph &			GetInstance ()				const	{ return _instance; }
		ND_ VResourceManager &		GetResourceManager ()		const	{ return _instance.GetResourceManager(); }
		ND_ VMemoryManager &		GetMemoryManager ()			const	{ return GetResourceManager().GetMemoryManager(); }


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
		

	// resource manager //
		template <typename ID, typename T, typename GtoL>
		ND_ T const*  _ToLocal (ID id, INOUT PoolTmpl<T> &, INOUT GtoL &, INOUT uint &counter, StringView msg);

		void  _FlushLocalResourceStates (ExeOrderIndex, ExeOrderIndex, VBarrierManager &, Ptr<VLocalDebugger>);
		void  _ResetLocalRemaping ();
	};

	
/*
=================================================
	AcquireTemporary
=================================================
*/
	template <uint UID>
	inline auto const*  VCommandBuffer::AcquireTemporary (_fg_hidden_::ResourceID<UID> id)
	{
		auto[iter, inserted] = _rm.resourceMap.insert(Resource_t{ id });

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
		_rm.resourceMap.insert(Resource_t{ id });
	}


}	// FG
