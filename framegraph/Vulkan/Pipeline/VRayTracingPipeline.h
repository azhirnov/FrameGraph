// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VPipelineLayout.h"

namespace FG
{

	//
	// Ray Tracing Pipeline
	//

	class VRayTracingPipeline final
	{
		friend class VPipelineCache;

	// types
	private:
		struct GroupInfo
		{
			RTShaderGroupID		id;
			uint				offset	= UMax;

			ND_ bool  operator == (const RTShaderGroupID &rhs)	const	{ return id == rhs; }
			ND_ bool  operator <  (const RTShaderGroupID &rhs)	const	{ return id < rhs; }
			ND_ bool  operator <  (const GroupInfo &rhs)		const	{ return id < rhs.id; }
		};


	// variables
	private:
		PipelineLayoutID		_layoutId;
		VkPipeline				_pipeline			= VK_NULL_HANDLE;
		
		Array<uint8_t>			_groupData;
		uint					_groupInfoOffset	= 0;
		uint					_shaderHandleSize	= 0;

		DebugName_t				_debugName;
		
		RWRaceConditionCheck	_rcCheck;


	// methods
	public:
		VRayTracingPipeline () {}
		VRayTracingPipeline (VRayTracingPipeline &&) = default;
		~VRayTracingPipeline ();
		
		bool Create (const VResourceManagerThread &, const RayTracingPipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		ND_ ArrayView<uint8_t>		GetShaderGroupHandle (const RTShaderGroupID &id) const;
		
		ND_ RawPipelineLayoutID		GetLayoutID ()		const	{ SHAREDLOCK( _rcCheck );  return _layoutId.Get(); }
		ND_ VkPipeline				Handle ()			const	{ SHAREDLOCK( _rcCheck );  return _pipeline; }
		ND_ BytesU					ShaderHandleSize ()	const	{ SHAREDLOCK( _rcCheck );  return BytesU{_shaderHandleSize}; }
		ND_ StringView				GetDebugName ()		const	{ SHAREDLOCK( _rcCheck );  return _debugName; }
	};


}	// FG
