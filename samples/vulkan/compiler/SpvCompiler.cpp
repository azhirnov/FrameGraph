// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SpvCompiler.h"

// glslang includes
#include "glslang/glslang/Include/revision.h"
#include "glslang/glslang/OSDependent/osinclude.h"
#include "glslang/glslang/MachineIndependent/localintermediate.h"
#include "glslang/glslang/Include/intermediate.h"
#include "glslang/SPIRV/doc.h"
#include "glslang/SPIRV/disassemble.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/SPIRV/GLSL.std.450.h"
#include "glslang/StandAlone/ResourceLimits.cpp"

using namespace glslang;


SpvCompiler::SpvCompiler ()
{
	glslang::InitializeProcess();
	_tempBuf.reserve( 1024 );
}

SpvCompiler::~SpvCompiler ()
{
	glslang::FinalizeProcess();
}


bool SpvCompiler::Compile (OUT VkShaderModule &		shaderModule,
						   const VulkanDevice &		vulkan,
						   const char *				source,
						   const char *				entry,
						   EShLanguage				shaderType,
						   EShTargetLanguageVersion	spvVersion,
						   bool						autoMapLocations) const
{
	if ( not Compile( OUT _tempBuf, source, entry, shaderType, spvVersion, autoMapLocations ) )
		return false;

	VkShaderModuleCreateInfo	info = {};
	info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.flags		= 0;
	info.pCode		= _tempBuf.data();
	info.codeSize	= size_t(ArraySizeOf( _tempBuf ));

	VK_CHECK( vulkan.vkCreateShaderModule( vulkan.GetVkDevice(), &info, null, OUT &shaderModule ));
	return true;
}


bool SpvCompiler::Compile (OUT Array<uint>&			spirvData,
						   const char *				source,
						   const char *				entry,
						   EShLanguage				shaderType,
						   EShTargetLanguageVersion	spvVersion,
						   bool						autoMapLocations) const
{

	EShMessages					messages		= EShMsgDefault;
	TProgram					program;
	TShader						shader			{ shaderType };
	EshTargetClientVersion		client_version	= EShTargetVulkan_1_1;
	TBuiltInResource			builtin_res		= DefaultTBuiltInResource;

	shader.setStrings( &source, 1 );
	shader.setEntryPoint( entry );
    shader.setEnvInput( EShSourceGlsl, shaderType, EShClientVulkan, 110 );
	shader.setEnvClient( EShClientVulkan, client_version );
	shader.setEnvTarget( EshTargetSpv, spvVersion );
		
	shader.setAutoMapLocations( autoMapLocations );
	shader.setAutoMapBindings( autoMapLocations );

	if ( not shader.parse( &builtin_res, 450, ENoProfile, false, true, messages ) )
	{
		FG_LOGI( shader.getInfoLog() );
		return false;
	}
		
	program.addShader( &shader );

	if ( not program.link( messages ) )
	{
		FG_LOGI( program.getInfoLog() );
		return false;
	}

	if ( autoMapLocations and not program.mapIO() )
	{
		FG_LOGI( "mapIO - failed" );
		return false;
	}

	const TIntermediate* intermediate = program.getIntermediate( shader.getStage() );
	if ( not intermediate )
		return false;

	SpvOptions				spv_options;
	spv::SpvBuildLogger		logger;

	spv_options.generateDebugInfo	= true;
	spv_options.disableOptimizer	= true;
	spv_options.optimizeSize		= false;
		
	spirvData.clear();
	GlslangToSpv( *intermediate, OUT spirvData, &logger, &spv_options );

	if ( spirvData.empty() )
		return false;

	return true;
}
