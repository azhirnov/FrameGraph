// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ShaderCompiler.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Algorithms/StringParser.h"
#include "glsl_trace/include/ShaderTrace.h"

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

//#include <sstream>
#include "glslang/External/SPIRV-Tools/include/spirv-tools/libspirv.h"

using namespace glslang;


ShaderCompiler::ShaderCompiler ()
{
	glslang::InitializeProcess();
	_tempBuf.reserve( 1024 );
}

ShaderCompiler::~ShaderCompiler ()
{
	for (auto& sh : _debuggableShaders) {
		delete sh.second;
	}

	glslang::FinalizeProcess();
}

/*
=================================================
	Compile
=================================================
*/
bool ShaderCompiler::Compile  (OUT VkShaderModule &		shaderModule,
							   const VulkanDevice &		vulkan,
							   ArrayView<const char *>	source,
							   EShLanguage				shaderType,
							   uint						dbgBufferSetIndex,
							   EShTargetLanguageVersion	spvVersion)
{
	Array<const char *>		shader_src;
	const bool				debuggable	= dbgBufferSetIndex != ~0u;
	UniquePtr<ShaderTrace>	debug_info	{ debuggable ? new ShaderTrace{} : null };
	const FGC::String		header		= "#version 460 core\n"s <<
										  "#extension GL_ARB_separate_shader_objects : require\n" <<
										  "#extension GL_ARB_shading_language_420pack : require\n";
	
	shader_src.push_back( header.data() );
	shader_src.insert( shader_src.end(), source.begin(), source.end() );

	if ( not _Compile( OUT _tempBuf, OUT debug_info.get(), dbgBufferSetIndex, shader_src, shaderType, spvVersion ) )
		return false;

	VkShaderModuleCreateInfo	info = {};
	info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.flags		= 0;
	info.pCode		= _tempBuf.data();
	info.codeSize	= size_t(ArraySizeOf( _tempBuf ));

	VK_CHECK( vulkan.vkCreateShaderModule( vulkan.GetVkDevice(), &info, null, OUT &shaderModule ));

	if ( debuggable ) {
		_debuggableShaders.insert_or_assign( shaderModule, debug_info.release() );
	}
	return true;
}

/*
=================================================
	_Compile
=================================================
*/
bool ShaderCompiler::_Compile (OUT Array<uint>&			spirvData,
							   OUT ShaderTrace*			dbgInfo,
							   uint						dbgBufferSetIndex,
							   ArrayView<const char *>	source,
							   EShLanguage				shaderType,
							   EShTargetLanguageVersion	spvVersion)
{
	EShMessages				messages		= EShMsgDefault;
	TProgram				program;
	TShader					shader			{ shaderType };
	EshTargetClientVersion	client_version	= EShTargetVulkan_1_1;
	TBuiltInResource		builtin_res		= DefaultTBuiltInResource;

	shader.setStrings( source.data(), int(source.size()) );
	shader.setEntryPoint( "main" );
	shader.setEnvInput( EShSourceGlsl, shaderType, EShClientVulkan, 110 );
	shader.setEnvClient( EShClientVulkan, client_version );
	shader.setEnvTarget( EshTargetSpv, spvVersion );

	if ( not shader.parse( &builtin_res, 460, ECoreProfile, false, true, messages ) )
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

	TIntermediate*	intermediate = program.getIntermediate( shader.getStage() );
	if ( not intermediate )
		return false;

	if ( dbgInfo )
	{
		CHECK_ERR( dbgInfo->InsertTraceRecording( INOUT *intermediate, dbgBufferSetIndex ));
		
		dbgInfo->SetSource( source.data(), null, source.size() );
	}

	SpvOptions				spv_options;
	spv::SpvBuildLogger		logger;

	spv_options.generateDebugInfo	= false;
	spv_options.disableOptimizer	= true;
	spv_options.optimizeSize		= false;
	spv_options.validate			= true;
		
	spirvData.clear();
	GlslangToSpv( *intermediate, OUT spirvData, &logger, &spv_options );

	if ( spirvData.empty() )
		return false;

	// disassemble
	/*{
		spv_context	ctx = spvContextCreate( SPV_ENV_VULKAN_1_1 );
		CHECK_ERR( ctx != null );

		spv_text		text		= null;
		spv_diagnostic	diagnostic	= null;
		std::string		disasm;

		if ( spvBinaryToText( ctx, spirvData.data(), spirvData.size(), 0, &text, &diagnostic ) == SPV_SUCCESS )
		{
			disasm = std::string{ text->str, text->length };
		}
		
		spvTextDestroy( text );
		spvDiagnosticDestroy( diagnostic );
		spvContextDestroy( ctx );
	}

	// to string
	{
		FGC::String	temp = "{";
		for (auto& val : spirvData) {
			temp << "0x" << ToString<16>( val ) << ", ";
		}

		temp.pop_back();
		temp.pop_back();
		temp << "};\n";
		temp << "\n";
	}*/

	return true;
}

/*
=================================================
	ParseDebugOutput
=================================================
*/
bool ShaderCompiler::GetDebugOutput (VkShaderModule shaderModule, const void *ptr, BytesU maxSize, OUT Array<FGC::String> &result) const
{
	auto	iter = _debuggableShaders.find( shaderModule );
	CHECK_ERR( iter != _debuggableShaders.end() );

	return iter->second->ParseShaderTrace( ptr, uint64_t(maxSize), OUT result );
}

