// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SpirvCompiler.h"
#include "extensions/vulkan_loader/VulkanLoader.h"

namespace FG
{

/*
=================================================
	_GenerateResources
=================================================
*/
	void SpirvCompiler::_GenerateResources (OUT TBuiltInResource& res)
	{
		res = {};

		res.maxLights = 0;
		res.maxClipPlanes = 6;
		res.maxTextureUnits = 32;
		res.maxTextureCoords = 32;
		res.maxVertexAttribs = FG_MaxVertexAttribs;
		res.maxVertexUniformComponents = 4096;
		res.maxVaryingFloats = 64;
		res.maxVertexTextureImageUnits = 32;
		res.maxCombinedTextureImageUnits = 80;
		res.maxTextureImageUnits = 32;
		res.maxFragmentUniformComponents = 4096;
		res.maxDrawBuffers = FG_MaxColorBuffers;
		res.maxVertexUniformVectors = 128;
		res.maxVaryingVectors = 8;
		res.maxFragmentUniformVectors = 16;
		res.maxVertexOutputVectors = 16;
		res.maxFragmentInputVectors = 15;
		res.minProgramTexelOffset = -8;
		res.maxProgramTexelOffset = 7;
		res.maxClipDistances = 8;
		res.maxComputeWorkGroupCountX = 65535;
		res.maxComputeWorkGroupCountY = 65535;
		res.maxComputeWorkGroupCountZ = 65535;
		res.maxComputeWorkGroupSizeX = 1024;
		res.maxComputeWorkGroupSizeY = 1024;
		res.maxComputeWorkGroupSizeZ = 64;
		res.maxComputeUniformComponents = 1024;
		res.maxComputeTextureImageUnits = 16;
		res.maxComputeImageUniforms = 8;
		res.maxComputeAtomicCounters = 0;
		res.maxComputeAtomicCounterBuffers = 0;
		res.maxVaryingComponents = 60;
		res.maxVertexOutputComponents = 64;
		res.maxGeometryInputComponents = 64;
		res.maxGeometryOutputComponents = 128;
		res.maxFragmentInputComponents = 128;
		res.maxImageUnits = 8;
		res.maxCombinedImageUnitsAndFragmentOutputs = 8;
		res.maxImageSamples = 0;
		res.maxVertexImageUniforms = 0;
		res.maxTessControlImageUniforms = 0;
		res.maxTessEvaluationImageUniforms = 0;
		res.maxGeometryImageUniforms = 0;
		res.maxFragmentImageUniforms = 8;
		res.maxCombinedImageUniforms = 8;
		res.maxGeometryTextureImageUnits = 16;
		res.maxGeometryOutputVertices = 256;
		res.maxGeometryTotalOutputComponents = 1024;
		res.maxGeometryUniformComponents = 1024;
		res.maxGeometryVaryingComponents = 64;
		res.maxTessControlInputComponents = 128;
		res.maxTessControlOutputComponents = 128;
		res.maxTessControlTextureImageUnits = 16;
		res.maxTessControlUniformComponents = 1024;
		res.maxTessControlTotalOutputComponents = 4096;
		res.maxTessEvaluationInputComponents = 128;
		res.maxTessEvaluationOutputComponents = 128;
		res.maxTessEvaluationTextureImageUnits = 16;
		res.maxTessEvaluationUniformComponents = 1024;
		res.maxTessPatchComponents = 120;
		res.maxPatchVertices = 32;
		res.maxTessGenLevel = 64;
		res.maxViewports = FG_MaxViewports;
		res.maxVertexAtomicCounters = 0;
		res.maxTessControlAtomicCounters = 0;
		res.maxTessEvaluationAtomicCounters = 0;
		res.maxGeometryAtomicCounters = 0;
		res.maxFragmentAtomicCounters = 0;
		res.maxCombinedAtomicCounters = 0;
		res.maxAtomicCounterBindings = 0;
		res.maxVertexAtomicCounterBuffers = 0;
		res.maxTessControlAtomicCounterBuffers = 0;
		res.maxTessEvaluationAtomicCounterBuffers = 0;
		res.maxGeometryAtomicCounterBuffers = 0;
		res.maxFragmentAtomicCounterBuffers = 0;
		res.maxCombinedAtomicCounterBuffers = 0;
		res.maxAtomicCounterBufferSize = 0;
		res.maxTransformFeedbackBuffers = 0;
		res.maxTransformFeedbackInterleavedComponents = 0;
		res.maxCullDistances = 8;
		res.maxCombinedClipAndCullDistances = 8;
		res.maxSamples = 4;
		res.maxMeshOutputVerticesNV = 256;
		res.maxMeshOutputPrimitivesNV = 512;
		res.maxMeshWorkGroupSizeX_NV = 32;
		res.maxMeshWorkGroupSizeY_NV = 1;
		res.maxMeshWorkGroupSizeZ_NV = 1;
		res.maxTaskWorkGroupSizeX_NV = 32;
		res.maxTaskWorkGroupSizeY_NV = 1;
		res.maxTaskWorkGroupSizeZ_NV = 1;
		res.maxMeshViewCountNV = 4;

		res.limits.nonInductiveForLoops = 1;
		res.limits.whileLoops = 1;
		res.limits.doWhileLoops = 1;
		res.limits.generalUniformIndexing = 1;
		res.limits.generalAttributeMatrixVectorIndexing = 1;
		res.limits.generalVaryingIndexing = 1;
		res.limits.generalSamplerIndexing = 1;
		res.limits.generalVariableIndexing = 1;
		res.limits.generalConstantMatrixVectorIndexing = 1;
	}
	
/*
=================================================
	SetDefaultResourceLimits
=================================================
*/
	bool SpirvCompiler::SetDefaultResourceLimits ()
	{
		_GenerateResources( OUT _builtinResource );
		return true;
	}

/*
=================================================
	SetCurrentResourceLimits
=================================================
*/
	bool SpirvCompiler::SetCurrentResourceLimits (PhysicalDeviceVk_t physicalDevice)
	{
		CHECK_ERR( physicalDevice );

		auto&	res = _builtinResource;			_GenerateResources( OUT res );
		
		VkPhysicalDevice						dev					= BitCast<VkPhysicalDevice>( physicalDevice );
		VkPhysicalDeviceProperties				props				= {};

		#ifdef VK_NV_mesh_shader
		VkPhysicalDeviceMeshShaderFeaturesNV	mesh_shader_feats	= {};
		VkPhysicalDeviceMeshShaderPropertiesNV	mesh_shader_props	= {};
		#endif

		vkGetPhysicalDeviceProperties( dev, OUT &props );
		
		if ( VK_VERSION_MAJOR( props.apiVersion ) >= 1 and
			 VK_VERSION_MINOR( props.apiVersion ) >  0 )
		{
			void **						next_props	= null;
			void **						next_feat	= null;
			VkPhysicalDeviceProperties2	props2		= {};
			VkPhysicalDeviceFeatures2	feat2		= {};

			props2.sType			= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			next_props				= &props2.pNext;

			feat2.sType				= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			next_feat				= &feat2.pNext;
			
		#ifdef VK_NV_mesh_shader
			*next_feat				= &mesh_shader_feats;
			next_feat				= &mesh_shader_feats.pNext;
			mesh_shader_feats.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;

			*next_props				= &mesh_shader_props;
			next_props				= &mesh_shader_props.pNext;
			mesh_shader_props.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
		#endif
		
			vkGetPhysicalDeviceFeatures2KHR( dev, OUT &feat2 );
			vkGetPhysicalDeviceProperties2KHR( dev, OUT &props2 );
		}


		res.maxVertexAttribs = Min( FG_MaxVertexAttribs, props.limits.maxVertexInputAttributes );
		res.maxDrawBuffers = Min( FG_MaxColorBuffers, props.limits.maxColorAttachments );
		res.minProgramTexelOffset = props.limits.minTexelOffset;
		res.maxProgramTexelOffset = props.limits.maxTexelOffset;

		res.maxComputeWorkGroupCountX = props.limits.maxComputeWorkGroupCount[0];
		res.maxComputeWorkGroupCountY = props.limits.maxComputeWorkGroupCount[1];
		res.maxComputeWorkGroupCountZ = props.limits.maxComputeWorkGroupCount[2];
		res.maxComputeWorkGroupSizeX = props.limits.maxComputeWorkGroupSize[0];
		res.maxComputeWorkGroupSizeY = props.limits.maxComputeWorkGroupSize[1];
		res.maxComputeWorkGroupSizeZ = props.limits.maxComputeWorkGroupSize[2];

		res.maxVertexOutputComponents = props.limits.maxVertexOutputComponents;
		res.maxGeometryInputComponents = props.limits.maxGeometryInputComponents;
		res.maxGeometryOutputComponents = props.limits.maxGeometryOutputComponents;
		res.maxFragmentInputComponents = props.limits.maxFragmentInputComponents;

		res.maxCombinedImageUnitsAndFragmentOutputs = props.limits.maxFragmentCombinedOutputResources;
		res.maxGeometryOutputVertices = props.limits.maxGeometryOutputVertices;
		res.maxGeometryTotalOutputComponents = props.limits.maxGeometryTotalOutputComponents;

		res.maxTessControlInputComponents = props.limits.maxTessellationControlPerVertexInputComponents;
		res.maxTessControlOutputComponents = props.limits.maxTessellationControlPerVertexOutputComponents;
		res.maxTessControlTotalOutputComponents = props.limits.maxTessellationControlTotalOutputComponents;
		res.maxTessEvaluationInputComponents = props.limits.maxTessellationEvaluationInputComponents;
		res.maxTessEvaluationOutputComponents = props.limits.maxTessellationEvaluationOutputComponents;
		res.maxTessPatchComponents = props.limits.maxTessellationControlPerPatchOutputComponents;
		res.maxPatchVertices = props.limits.maxTessellationPatchSize;
		res.maxTessGenLevel = props.limits.maxTessellationGenerationLevel;

		res.maxViewports = Min( FG_MaxViewports, props.limits.maxViewports );
		res.maxClipDistances = props.limits.maxClipDistances;
		res.maxCullDistances = props.limits.maxCullDistances;
		res.maxCombinedClipAndCullDistances = props.limits.maxCombinedClipAndCullDistances;
		
	#ifdef VK_NV_mesh_shader
		if ( mesh_shader_feats.meshShader and mesh_shader_feats.taskShader )
		{
			res.maxMeshOutputVerticesNV = mesh_shader_props.maxMeshOutputVertices;
			res.maxMeshOutputPrimitivesNV = mesh_shader_props.maxMeshOutputPrimitives;
			res.maxMeshWorkGroupSizeX_NV = mesh_shader_props.maxMeshWorkGroupSize[0];
			res.maxMeshWorkGroupSizeY_NV = mesh_shader_props.maxMeshWorkGroupSize[1];
			res.maxMeshWorkGroupSizeZ_NV = mesh_shader_props.maxMeshWorkGroupSize[2];
			res.maxTaskWorkGroupSizeX_NV = mesh_shader_props.maxTaskWorkGroupSize[0];
			res.maxTaskWorkGroupSizeY_NV = mesh_shader_props.maxTaskWorkGroupSize[1];
			res.maxTaskWorkGroupSizeZ_NV = mesh_shader_props.maxTaskWorkGroupSize[2];
			res.maxMeshViewCountNV = mesh_shader_props.maxMeshMultiviewViewCount;
		}
	#endif

		return true;
	}


}	// FG
