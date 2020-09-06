// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Pipeline.h"
#include "VCommon.h"

#ifdef VK_NV_ray_tracing

namespace FG
{

	//
	// Ray Tracing Shader Binding Table
	//

	class VRayTracingShaderTable final
	{
		friend class VPipelineCache;

	// types
	private:
		struct ShaderTable
		{
			VkPipeline			pipeline	= VK_NULL_HANDLE;
			PipelineLayoutID	layoutId;
			BytesU				bufferOffset;
			EShaderDebugMode	mode		= Default;
		};

		using Tables_t	= FixedArray< ShaderTable, uint(EShaderDebugMode::_Count) >;


	// variables
	private:
		mutable std::shared_mutex	_guard;

		BufferID					_bufferId;
		RTPipelineID				_pipelineId;
		Tables_t					_tables;
		
		BytesU						_rayGenOffset;
		BytesU						_rayMissOffset;
		BytesU						_rayHitOffset;
		BytesU						_callableOffset;
		BytesU						_blockSize;

		Bytes<uint16_t>				_rayMissStride;
		Bytes<uint16_t>				_rayHitStride;
		Bytes<uint16_t>				_callableStride;

		// in ray gen shader the 'sbtRecordStride' parameter of 'traceNV()' must equal to 'hitShadersPerInstance'
		// and 'sbtRecordOffset' must be less then 'hitShadersPerInstance'
		//uint16_t					_hitShadersPerInstance	= 0;
		//uint16_t					_maxMissShaders	= 0;

		DebugName_t					_debugName;

		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VRayTracingShaderTable () {}
		VRayTracingShaderTable (VRayTracingShaderTable &&) = delete;
		VRayTracingShaderTable (const VRayTracingShaderTable &) = delete;
		~VRayTracingShaderTable ();

		bool Create (StringView dbgName);
		void Destroy (VResourceManager &);

		bool GetBindings (EShaderDebugMode mode,
						  OUT RawPipelineLayoutID &layout, OUT VkPipeline &pipeline,
						  OUT VkDeviceSize &blockSize, OUT VkDeviceSize &rayGenOffset,
						  OUT VkDeviceSize &rayMissOffset, OUT VkDeviceSize &rayMissStride,
						  OUT VkDeviceSize &rayHitOffset, OUT VkDeviceSize &rayHitStride,
						  OUT VkDeviceSize &callableOffset, OUT VkDeviceSize &callableStride) const;
		
		ND_ RawRTPipelineID	GetPipeline ()	const	{ SHAREDLOCK( _drCheck );  return _pipelineId.Get(); }
		ND_ RawBufferID		GetBuffer ()	const	{ SHAREDLOCK( _drCheck );  return _bufferId.Get(); }
		ND_ StringView		GetDebugName ()	const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};


}	// FG

#endif	// VK_NV_ray_tracing
