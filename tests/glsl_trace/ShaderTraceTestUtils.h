// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "glsl_trace/include/ShaderTrace.h"
#include "ShaderCompiler.h"
#include "framework/Vulkan/VulkanDeviceExt.h"
#include "stl/Algorithms/StringUtils.h"
using namespace FGC;


struct TestHelpers
{
	mutable ShaderCompiler	compiler;
	VkQueue					queue			= VK_NULL_HANDLE;
	uint					queueFamily		= UMax;
	VkCommandPool			cmdPool			= VK_NULL_HANDLE;
	VkCommandBuffer			cmdBuffer		= VK_NULL_HANDLE;
	VkDescriptorPool		descPool		= VK_NULL_HANDLE;
	VkBuffer				debugOutputBuf	= VK_NULL_HANDLE;
	VkBuffer				readBackBuf		= VK_NULL_HANDLE;
	VkDeviceMemory			debugOutputMem	= VK_NULL_HANDLE;
	VkDeviceMemory			readBackMem		= VK_NULL_HANDLE;
	void *					readBackPtr		= null;
	BytesU					debugOutputSize	= 128_Mb;
};

bool CreateDebugDescriptorSet (VulkanDevice &vulkan, const TestHelpers &helper, VkShaderStageFlags stages,
							   OUT VkDescriptorSetLayout &dsLayout, OUT VkDescriptorSet &descSet);

bool CreateRenderTarget (VulkanDeviceExt &vulkan, VkFormat colorFormat, uint width, uint height, VkImageUsageFlags imageUsage,
						 OUT VkRenderPass &outRenderPass, OUT VkImage &outImage,
						 OUT VkDeviceMemory &outImageMem, OUT VkFramebuffer &outFramebuffer);

bool CreateGraphicsPipelineVar1 (VulkanDevice &vulkan, VkShaderModule vertShader, VkShaderModule fragShader,
								 VkDescriptorSetLayout dsLayout, VkRenderPass renderPass,
								 OUT VkPipelineLayout &outPipelineLayout, OUT VkPipeline &outPipeline);

bool CreateGraphicsPipelineVar2 (VulkanDevice &vulkan, VkShaderModule vertShader, VkShaderModule tessContShader, VkShaderModule tessEvalShader,
								 VkShaderModule fragShader, VkDescriptorSetLayout dsLayout, VkRenderPass renderPass, uint patchSize,
								 OUT VkPipelineLayout &outPipelineLayout, OUT VkPipeline &outPipeline);

bool CreateMeshPipelineVar1 (VulkanDevice &vulkan, VkShaderModule meshShader, VkShaderModule fragShader,
							 VkDescriptorSetLayout dsLayout, VkRenderPass renderPass,
							 OUT VkPipelineLayout &outPipelineLayout, OUT VkPipeline &outPipeline);

bool CreateRayTracingScene (VulkanDeviceExt &vulkan, const TestHelpers &helper, VkPipeline rtPipeline, uint numGroups,
							OUT VkBuffer &shaderBindingTable, OUT VkDeviceMemory &outMemory,
							OUT VkAccelerationStructureNV &topLevelAS, OUT VkAccelerationStructureNV &bottomLevelAS);

bool TestDebugTraceOutput (const TestHelpers &helper, ArrayView<VkShaderModule> modules, StringView referenceFile);

bool TestPerformanceOutput (const TestHelpers &helper, ArrayView<VkShaderModule> modules, ArrayView<StringView> fnNames);


inline String  _GetFuncName (StringView src)
{
	size_t	pos = src.find_last_of( "::" );

	if ( pos != StringView::npos )
		return String{ src.substr( pos+1 )};
	else
		return String{ src };
}

#define TEST_NAME	_GetFuncName( FG_FUNCTION_NAME )
