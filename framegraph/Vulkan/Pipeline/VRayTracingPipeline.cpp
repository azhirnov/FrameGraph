// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingPipeline.h"
#include "VResourceManagerThread.h"
#include "VEnumCast.h"
#include "VDevice.h"

namespace FG
{
	
/*
=================================================
	destructor
=================================================
*/
	VRayTracingPipeline::~VRayTracingPipeline ()
	{
		CHECK( not _pipeline );
		CHECK( not _layoutId );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VRayTracingPipeline::Create (const VResourceManagerThread &resMngr, const RayTracingPipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( not _pipeline );
		CHECK_ERR( desc._groups.size() );
		
		// TODO: custom allocator
		Array< VkPipelineShaderStageCreateInfo >		shader_stages;
		Array< VkRayTracingShaderGroupCreateInfoNV >	shader_groups;

		const auto	GetShaderStage = [&desc] (const RTShaderID &id, OUT VkPipelineShaderStageCreateInfo &stage) -> bool
		{
			ASSERT( id.IsDefined() );

			auto	iter = desc._shaders.find( id );
			CHECK_ERR( iter->second.data.size() == 1 );

			auto* vk_shader = std::get_if< PipelineDescription::VkShaderPtr >( &iter->second.data.begin()->second );
			CHECK_ERR( vk_shader );
			
			stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.pNext  = null;
			stage.flags  = 0;
			stage.module = BitCast<VkShaderModule>((*vk_shader)->GetData());
			stage.pName  = (*vk_shader)->GetEntry().data();
			stage.stage  = VEnumCast( iter->second.shaderType );
			stage.pSpecializationInfo = null;
			return true;
		};

		for (auto& group : desc._groups)
		{
			auto&	group_ci = shader_groups.emplace_back();
			
			group_ci.sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
			group_ci.pNext				= null;
			group_ci.generalShader		= VK_SHADER_UNUSED_NV;
			group_ci.closestHitShader	= VK_SHADER_UNUSED_NV;
			group_ci.anyHitShader		= VK_SHADER_UNUSED_NV;
			group_ci.intersectionShader	= VK_SHADER_UNUSED_NV;

			CHECK_ERR( Visit( group.second,
							[&] (const RayTracingPipelineDesc::GeneralGroup &gen) -> bool
							{
								auto&	stage = shader_stages.emplace_back();
								CHECK_ERR( GetShaderStage( gen.shader, OUT stage ));

								group_ci.type			= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
								group_ci.generalShader	= uint(shader_stages.size()-1);
								return true;
							},

							[&] (const RayTracingPipelineDesc::TriangleHitGroup &tri) -> bool
							{
								group_ci.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
								if ( tri.closestHitShader.IsDefined() )
								{
									auto&	stage = shader_stages.emplace_back();
									CHECK_ERR( GetShaderStage( tri.closestHitShader, OUT stage ));
									group_ci.closestHitShader = uint(shader_stages.size()-1);
								}
								if ( tri.anyHitShader.IsDefined() )
								{
									auto&	stage = shader_stages.emplace_back();
									CHECK_ERR( GetShaderStage( tri.anyHitShader, OUT stage ));
									group_ci.anyHitShader = uint(shader_stages.size()-1);
								}
								return true;
							},

							[&] (const RayTracingPipelineDesc::ProceduralHitGroup &proc) -> bool
							{
								group_ci.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_NV;
								{
									auto&	stage = shader_stages.emplace_back();
									CHECK_ERR( GetShaderStage( proc.intersectionShader, OUT stage ));
									group_ci.intersectionShader = uint(shader_stages.size()-1);
								}
								if ( proc.anyHitShader.IsDefined() )
								{
									auto&	stage = shader_stages.emplace_back();
									CHECK_ERR( GetShaderStage( proc.anyHitShader, OUT stage ));
									group_ci.anyHitShader = uint(shader_stages.size()-1);
								}
								if ( proc.closestHitShader.IsDefined() )
								{
									auto&	stage = shader_stages.emplace_back();
									CHECK_ERR( GetShaderStage( proc.closestHitShader, OUT stage ));
									group_ci.closestHitShader = uint(shader_stages.size()-1);
								}
								return true;
							},

							[] (const std::monostate &) -> bool { return false; }
						));
		}
		
		auto&	dev = resMngr.GetDevice();

		VkRayTracingPipelineCreateInfoNV 	pipeline_info = {};
		pipeline_info.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
		pipeline_info.flags					= 0;
		pipeline_info.stageCount			= uint(shader_stages.size());
		pipeline_info.pStages				= shader_stages.data();
		pipeline_info.groupCount			= uint(shader_groups.size());
		pipeline_info.pGroups				= shader_groups.data();
		pipeline_info.maxRecursionDepth		= desc._maxRecursionDepth;
		pipeline_info.layout				= resMngr.GetResource( layoutId )->Handle();
		pipeline_info.basePipelineIndex		= -1;
		pipeline_info.basePipelineHandle	= VK_NULL_HANDLE;

		VK_CHECK( dev.vkCreateRayTracingPipelinesNV( dev.GetVkDevice(), VK_NULL_HANDLE, 1, &pipeline_info, null, OUT &_pipeline ));

		_layoutId	= PipelineLayoutID{ layoutId };
		_debugName	= dbgName;
		
						_shaderHandleSize	= dev.GetDeviceRayTracingProperties().shaderGroupHandleSize;
		const size_t	handles_size		= desc._groups.size() * _shaderHandleSize;
		const size_t	group_ids_size		= desc._groups.size() * sizeof(GroupInfo);

		_groupData.resize( handles_size + group_ids_size );

		VK_CALL( dev.vkGetRayTracingShaderGroupHandlesNV( dev.GetVkDevice(), _pipeline, 0, uint(desc._groups.size()), handles_size, OUT _groupData.data() ));
		
		resMngr.EditStatistic().newRayTracingPipelineCount++;

		GroupInfo*	group_info	= Cast<GroupInfo>( _groupData.data() + BytesU{handles_size} );
		uint		group_index	= 0;

		for (auto& group : desc._groups)
		{
			group_info[group_index].id		= group.first;
			group_info[group_index].offset	= group_index * _shaderHandleSize;
			++group_index;
		}

		std::sort( group_info, group_info + BytesU{group_ids_size} );
		_groupInfoOffset = uint(handles_size);

		return true;
	}
	
/*
=================================================
	GetShaderGroupHandle
=================================================
*/
	ArrayView<uint8_t>  VRayTracingPipeline::GetShaderGroupHandle (const RTShaderGroupID &id) const
	{
		GroupInfo const*	infos	= Cast<GroupInfo>( _groupData.data() + BytesU{_groupInfoOffset} );
		const uint			count	= uint(_groupData.size() - _groupInfoOffset) / sizeof(GroupInfo);

		const size_t		pos		= BinarySearch( ArrayView{ infos, count }, id );
		CHECK_ERR( pos < count );

		return ArrayView{ _groupData.data() + BytesU{infos[pos].offset}, _shaderHandleSize };
	}

/*
=================================================
	Destroy
=================================================
*/
	void VRayTracingPipeline::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		SCOPELOCK( _rcCheck );

		if ( _pipeline ) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_PIPELINE, uint64_t(_pipeline) );
		}

		if ( _layoutId ) {
			unassignIDs.push_back( _layoutId.Release() );
		}

		_debugName.clear();
		_groupData.clear();

		_pipeline			= VK_NULL_HANDLE;
		_layoutId			= Default;
		_groupInfoOffset	= 0;
		_shaderHandleSize	= 0;
	}


}	// FG
