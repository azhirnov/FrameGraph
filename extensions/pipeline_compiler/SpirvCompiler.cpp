// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SpirvCompiler.h"
#include "PrivateDefines.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Algorithms/StringParser.h"
#include "framegraph/Shared/EnumUtils.h"

// glslang includes
#include "glslang/glslang/Include/revision.h"
#include "glslang/glslang/Public/ShaderLang.h"
#include "glslang/glslang/OSDependent/osinclude.h"
#include "glslang/glslang/MachineIndependent/localintermediate.h"
#include "glslang/glslang/Include/intermediate.h"
#include "glslang/SPIRV/doc.h"
#include "glslang/SPIRV/disassemble.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/SPIRV/GLSL.std.450.h"

// SPIRV-Tools includes
#include "spirv-tools/optimizer.hpp"
#include "spirv-tools/libspirv.h"


namespace FG
{
	using namespace glslang;


	struct SpirvCompiler::GLSLangResult
	{
		TProgram				prog;
		UniquePtr< TShader >	shader;
	};
	

/*
=================================================
	constructor
=================================================
*/
	SpirvCompiler::SpirvCompiler ()
	{
		glslang::InitializeProcess();

		_GenerateResources( OUT _builtinResource );
	}
	
/*
=================================================
	destructor
=================================================
*/
	SpirvCompiler::~SpirvCompiler ()
	{
		glslang::FinalizeProcess();
	}
	
/*
=================================================
	SetCompilationFlags
=================================================
*/
	bool SpirvCompiler::SetCompilationFlags (EShaderCompilationFlags flags)
	{
		_compilerFlags = flags;
		return true;
	}
	
/*
=================================================
	Compile
=================================================
*/
	bool SpirvCompiler::Compile (EShader shaderType, EShaderLangFormat srcShaderFmt, EShaderLangFormat dstShaderFmt,
								StringView entry, StringView source,
								OUT PipelineDescription::Shader &outShader, OUT ShaderReflection &outReflection, OUT String &log)
	{
		using SpirvShaderData = PipelineDescription::SharedShaderPtr<Array<uint>>;

		log.clear();
		COMP_CHECK_ERR( (dstShaderFmt & EShaderLangFormat::_StorageFormatMask) == EShaderLangFormat::SPIRV );

		_currentStage = EShaderStages_FromShader( shaderType );

		GLSLangResult	glslang_data;
		COMP_CHECK_ERR( _ParseGLSL( shaderType, srcShaderFmt, dstShaderFmt, entry, source, OUT glslang_data, INOUT log ));

		Array<uint>		spirv;
		COMP_CHECK_ERR( _CompileSPIRV( glslang_data, OUT spirv, INOUT log ));

		COMP_CHECK_ERR( _BuildReflection( glslang_data, OUT outReflection ));

		outShader.specConstants	= outReflection.specConstants;
		outShader.AddShaderData( dstShaderFmt, entry, std::move(spirv) );
		return true;
	}
	
/*
=================================================
	ConvertShaderType
=================================================
*/
	ND_ static EShLanguage  ConvertShaderType (EShader shaderType)
	{
		ENABLE_ENUM_CHECKS();
		switch ( shaderType )
		{
			case EShader::Vertex :			return EShLangVertex;
			case EShader::TessControl :		return EShLangTessControl;
			case EShader::TessEvaluation :	return EShLangTessEvaluation;
			case EShader::Geometry :		return EShLangGeometry;
			case EShader::Fragment :		return EShLangFragment;
			case EShader::Compute :			return EShLangCompute;
			case EShader::MeshTask :		return EShLangTaskNV;
			case EShader::Mesh :			return EShLangMeshNV;
			case EShader::RayGen :			return EShLangRayGenNV;
			case EShader::RayAnyHit :		return EShLangAnyHitNV;
			case EShader::RayClosestHit :	return EShLangClosestHitNV;
			case EShader::RayMiss :			return EShLangMissNV;
			case EShader::RayIntersection:	return EShLangIntersectNV;
			case EShader::RayCallable :		return EShLangCallableNV;
			case EShader::Unknown :
			case EShader::_Count :			break;	// to shutup warnings
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown shader type", EShLangCount );
	}

/*
=================================================
	EShaderLangFormat_GetVersion
=================================================
*/
	ND_ static int  EShaderLangFormat_GetVersion (EShaderLangFormat fmt)
	{
		return int(fmt & EShaderLangFormat::_VersionMask) >> uint(EShaderLangFormat::_VersionOffset);
	}

/*
=================================================
	_ParseGLSL
=================================================
*/
	bool SpirvCompiler::_ParseGLSL (EShader shaderType, EShaderLangFormat srcShaderFmt, EShaderLangFormat dstShaderFmt,
									StringView entry, StringView source, OUT GLSLangResult &glslangData, OUT String &log)
	{
		EShClient					client			= EShClientOpenGL;
		EshTargetClientVersion		client_version	= EShTargetOpenGL_450;

		EShTargetLanguage			target			= EShTargetNone;
		EShTargetLanguageVersion	target_version	= EShTargetLanguageVersion(0);
		
		int							version			= 0;
		int			 				sh_version		= 450;		// TODO
		EProfile					sh_profile		= ENoProfile;
		EShSource					sh_source;

		_spirvTraget	= SPV_ENV_UNIVERSAL_1_0;
		_targetVulkan	= false;
		
		switch ( srcShaderFmt & EShaderLangFormat::_ApiMask )
		{
			case EShaderLangFormat::OpenGL :
				sh_source		= EShSourceGlsl;
				version			= EShaderLangFormat_GetVersion( srcShaderFmt );
				sh_profile		= version >= 330 ? ECoreProfile : ENoProfile;
				break;

			case EShaderLangFormat::OpenGLES :
				sh_source		= EShSourceGlsl;
				version			= EShaderLangFormat_GetVersion( srcShaderFmt );
				sh_profile		= EEsProfile;
				break;

			case EShaderLangFormat::DirectX :
				sh_source		= EShSourceHlsl;
				version			= EShaderLangFormat_GetVersion( srcShaderFmt );
				sh_profile		= ENoProfile;	// TODO
				break;

			case EShaderLangFormat::Vulkan :
				sh_source		= EShSourceGlsl;
				version			= EShaderLangFormat_GetVersion( srcShaderFmt );
				sh_profile		= ECoreProfile;
				break;

			default :
				COMP_RETURN_ERR( "unsupported shader format" );
		}
		
		switch ( dstShaderFmt & EShaderLangFormat::_ApiMask )
		{
			case EShaderLangFormat::Vulkan :
			{
				version			= EShaderLangFormat_GetVersion( dstShaderFmt );
				client			= EShClientVulkan;
				client_version	= (version == 110 ? EShTargetVulkan_1_1 : EShTargetVulkan_1_0);
				target			= EShTargetSpv;
				target_version	= (version == 110 ? EShTargetSpv_1_2 : EShTargetSpv_1_0);
				_spirvTraget	= (version == 110 ? SPV_ENV_VULKAN_1_1 : SPV_ENV_VULKAN_1_0);
				_targetVulkan	= true;
				break;
			}

			case EShaderLangFormat::OpenGL :
				if ( (dstShaderFmt & EShaderLangFormat::_FormatMask) == EShaderLangFormat::SPIRV )
				{
					target			= EShTargetSpv;
					target_version	= EShTargetSpv_1_0;
					_spirvTraget	= SPV_ENV_OPENGL_4_5;
				}
				break;
		}


		EShMessages			messages	= EShMsgDefault;
		EShLanguage			stage		= ConvertShaderType( shaderType );
		auto&				shader		= glslangData.shader;
		String				temp_src	= source.data();
		const char *		sources[]	= { temp_src.data() };
		//ShaderIncluder	includer	{ baseFolder };
		const bool			auto_map	= EnumEq( _compilerFlags, EShaderCompilationFlags::AutoMapLocations );

		shader.reset( new TShader( stage ));
		shader->setStrings( sources, int(CountOf(sources)) );
		shader->setEntryPoint( entry.data() );
		shader->setEnvInput( sh_source, stage, client, version );
		shader->setEnvClient( client, client_version );
		shader->setEnvTarget( target, target_version );
		
		shader->setAutoMapLocations( auto_map );
		shader->setAutoMapBindings( auto_map );

		if ( not shader->parse( &_builtinResource, sh_version, sh_profile, false, true, messages/*, includer*/ ) )
		{
			log += shader->getInfoLog();
			_OnCompilationFailed( {source}, INOUT log );
			return false;
		}
		
		glslangData.prog.addShader( shader.get() );

		if ( not glslangData.prog.link( messages ) )
		{
			log += glslangData.prog.getInfoLog();
			_OnCompilationFailed( {source}, INOUT log );
			return false;
		}

		if ( auto_map and not glslangData.prog.mapIO() )
		{
			log += "mapIO - failed";
			return false;
		}

		return true;
	}

/*
=================================================
	_CompileSPIRV
=================================================
*/
	bool SpirvCompiler::_CompileSPIRV (const GLSLangResult &glslangData, OUT Array<uint> &spirv, OUT String &log) const
	{
		const TIntermediate* intermediate = glslangData.prog.getIntermediate( glslangData.shader->getStage() );
		COMP_CHECK_ERR( intermediate );

		SpvOptions				spv_options;
		spv::SpvBuildLogger		logger;

		spv_options.generateDebugInfo	= false;
		spv_options.disableOptimizer	= not EnumEq( _compilerFlags, EShaderCompilationFlags::Optimize );
		spv_options.optimizeSize		= EnumEq( _compilerFlags, EShaderCompilationFlags::OptimizeSize );
		
		GlslangToSpv( *intermediate, OUT spirv, &logger, &spv_options );
		log += logger.getAllMessages();

		if ( EnumEq( _compilerFlags, EShaderCompilationFlags::StrongOptimization ) )
			CHECK_ERR( _OptimizeSPIRV( INOUT spirv, OUT log ));

		return true;
	}
	
/*
=================================================
	DisassempleSPIRV
=================================================
*/
	inline bool DisassempleSPIRV (spv_target_env targetEnv, const Array<uint> &spirv, OUT String &result)
	{
		result.clear();

		spv_context	ctx = spvContextCreate( targetEnv );
		CHECK_ERR( ctx != null );

		spv_text		text		= null;
		spv_diagnostic	diagnostic	= null;

		if ( spvBinaryToText( ctx, spirv.data(), spirv.size(), 0, &text, &diagnostic ) == SPV_SUCCESS )
		{
			result = String{ text->str, text->length };
		}
		
		spvTextDestroy( text );
		spvDiagnosticDestroy( diagnostic );
		spvContextDestroy( ctx );

		return true;
	}

/*
=================================================
	_OptimizeSPIRV
=================================================
*/
	bool SpirvCompiler::_OptimizeSPIRV (INOUT Array<uint> &spirv, INOUT String &log) const
	{
		spv_target_env	target_env = BitCast<spv_target_env>( _spirvTraget );

		spvtools::Optimizer	optimizer{ target_env };
		optimizer.SetMessageConsumer(
			[&log] (spv_message_level_t level, const char *source, const spv_position_t &position, const char *message) {
				switch (level)
				{
				case SPV_MSG_FATAL:
				case SPV_MSG_INTERNAL_ERROR:
				case SPV_MSG_ERROR:
					log << "error: ";
					break;
				case SPV_MSG_WARNING:
					log << "warning: ";
					break;
				case SPV_MSG_INFO:
				case SPV_MSG_DEBUG:
					log << "info: ";
					break;
				}

				if (source)
					log << source << ":";
				
				log << ToString(position.line) << ":" << ToString(position.column) << ":" << ToString(position.index) << ":";
				if (message)
					log << " " << message;
			});

		optimizer.RegisterLegalizationPasses();
		optimizer.RegisterSizePasses();
		optimizer.RegisterPerformancePasses();

		optimizer.RegisterPass(spvtools::CreateCompactIdsPass());
		optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());;
		optimizer.RegisterPass(spvtools::CreateRemoveDuplicatesPass());
		optimizer.RegisterPass(spvtools::CreateCFGCleanupPass());

		//Array<uint>		temp = spirv;
		optimizer.Run(spirv.data(), spirv.size(), &spirv);

		//String	origin, optimized;
		//DisassempleSPIRV( target_env, temp, OUT origin );
		//DisassempleSPIRV( target_env, spirv, OUT optimized );

		return true;
	}
	
/*
=================================================
	ReadLine
=================================================
*/
	inline bool ReadLine (StringView log, INOUT size_t &pos, OUT StringView &line)
	{
		size_t	begin = pos;

		line = Default;

		// find new line
		for (; pos < log.size(); ++pos)
		{
			const char	c = log[pos];

			if ( c == '/r' or c == '/n' )
				break;
		}

		if ( pos == begin )
			return false;

		line = log.substr( begin, pos - begin );

		// skip empty lines
		for (; pos < log.size(); ++pos)
		{
			const char	c = log[pos];

			if ( c != '/r' and c != '/n' )
				break;
		}
		return true;
	}
	
/*
=================================================
	ParseGLSLError
=================================================
*/
	struct GLSLErrorInfo
	{
		StringView	description;
		StringView	fileName;
		uint		sourceIndex;
		size_t		line;
		bool		isError;

		GLSLErrorInfo () : sourceIndex{0}, line{UMax}, isError{false} {}
	};
	
	static bool ParseGLSLError (StringView line, OUT GLSLErrorInfo &info)
	{
		const StringView	c_error		= "error";
		const StringView	c_warning	= "warning";

		size_t				pos = 0;

		const auto			ReadToken	= [&line, &pos] (OUT bool &isNumber)
		{{
							isNumber= true;
			const size_t	start	= pos;

			for (; pos < line.length() and line[pos] != ':'; ++pos) {
				isNumber &= (line[pos] >= '0' and line[pos] <= '9');
			}
			return line.substr( start, pos - start );
		}};

		const auto			SkipSeparator = [&line, &pos] ()
		{{
			if ( line[pos] == ':' and line[pos+1] == ' ' )
				pos += 2;
			else if ( line[pos] == ':' )
				pos += 1;
			else
				return false;

			return true;
		}};

		
		// parse error/warning/...
		if ( StartsWithIC( line, c_error ))
		{
			pos			+= c_error.length();
			info.isError = true;
		}
		else
		if ( StartsWithIC( line, c_warning ))
		{
			pos			+= c_warning.length();
			info.isError = false;
		}
		else
			return false;

		if ( not SkipSeparator() )
			return false;


		// parse source index or header name
		{
			bool				is_number;
			const StringView	src		= ReadToken( OUT is_number );

			if ( not SkipSeparator() )
				return false;

			if ( not is_number )
				info.fileName = src;
			else
				info.sourceIndex = std::stoi( String(src) );
		}


		// parse line number
		{
			bool				is_number;
			const StringView	src		= ReadToken( OUT is_number );

			if ( not SkipSeparator() or not is_number )
				return false;

			info.line = std::stoi( String(src) );
		}

		info.description = line.substr( pos );
		return true;
	}

/*
=================================================
	_OnCompilationFailed
=================================================
*/
	bool SpirvCompiler::_OnCompilationFailed (ArrayView<StringView> source, INOUT String &log) const
	{
		// glslang errors format:
		// pattern: <error/warning>: <number>:<line>: <description>
		// pattern: <error/warning>: <file>:<line>: <description>
		
		StringView		line;
		uint			line_number = 0;
		size_t			prev_line	= UMax;
		size_t			pos			= 0;
		String			str;		str.reserve( log.length() );
		Array<size_t>	num_lines;	num_lines.resize( source.size() );

		for (; ReadLine( log, INOUT pos, OUT line ); ++line_number)
		{
			GLSLErrorInfo	error_info;
			bool			added = false;

			if ( ParseGLSLError( line, OUT error_info ))
			{
				// unite error in same source lines
				if ( prev_line == error_info.line )
				{
					str << line << "\n";
					continue;
				}

				prev_line = error_info.line;

				if ( error_info.fileName.empty() )
				{
					// search in sources
					StringView	cur_source	= error_info.sourceIndex < source.size() ? source[ error_info.sourceIndex ] : "";
					size_t		lines_count	= error_info.sourceIndex < num_lines.size() ? num_lines[ error_info.sourceIndex ] : 0;

					if ( lines_count == 0 and error_info.sourceIndex < num_lines.size() )
					{
						lines_count = StringParser::CalculateNumberOfLines( cur_source ) + 1;
						num_lines[ error_info.sourceIndex ] = lines_count;
					}
					
					CHECK( error_info.line < lines_count );

					size_t	line_pos = 0;
					CHECK( StringParser::MoveToLine( cur_source, INOUT line_pos, error_info.line-1 ));

					StringView	line_str;
					StringParser::ReadLineToEnd( cur_source, INOUT line_pos, OUT line_str );

					str << "in source (" << ToString(error_info.sourceIndex) << ": " << ToString(error_info.line) << "):\n\"" << line_str << "\"\n" << line << "\n";
					added = true;
				}
				else
				{
					// search in header
					/*StringView	src;
					if ( includer.GetHeaderSource( error_info.fileName, OUT src ))
					{
						const size_t	lines_count = StringParser::CalculateNumberOfLines( src ) + 1;
						const size_t	local_line	= error_info.line;
						size_t			line_pos	= 0;
						StringView		line_str;

						CHECK( local_line < lines_count );
						CHECK( StringParser::MoveToLine( src, INOUT line_pos, local_line-1 ));
						
						StringParser::ReadLineToEnd( src, INOUT line_pos, OUT line_str );

						str << "in source (" << error_info.fileName << ": " << ToString(local_line) << "):\n\"" << line_str << "\"\n" << line << "\n";
						added = true;
					}*/
				}
			}
			
			if ( not added )
			{
				str << DEBUG_ONLY( "<unknown> " << ) line << "\n";
			}
		}

		std::swap( log, str );
		return true;
	}
	
/*
=================================================
	_BuildReflection 
=================================================
*/
	bool SpirvCompiler::_BuildReflection (const GLSLangResult &glslangData, OUT ShaderReflection &result)
	{
		_intermediate = glslangData.prog.getIntermediate( glslangData.shader->getStage() );
		COMP_CHECK_ERR( _intermediate );

		// deserialize shader
		TIntermNode*	root	= _intermediate->getTreeRoot();
		
		COMP_CHECK_ERR( _ProcessExternalObjects( null, root, OUT result ));
		COMP_CHECK_ERR( _ProcessShaderInfo( INOUT result ));

		_intermediate = null;
		return true;
	}
	
/*
=================================================
	_ProcessExternalObjects
=================================================
*/
	bool SpirvCompiler::_ProcessExternalObjects (TIntermNode*, TIntermNode* node, INOUT ShaderReflection &result) const
	{
		TIntermAggregate* aggr = node->getAsAggregate();
		
		if ( not aggr )
			return true;

		switch ( aggr->getOp() )
		{
			// continue deserializing
			case TOperator::EOpSequence :
			{
				for (auto& seq : aggr->getSequence())
				{
					COMP_CHECK_ERR( _ProcessExternalObjects( aggr, seq, INOUT result ));
				}
				return true;
			}

			// uniforms, buffers, ...
			case TOperator::EOpLinkerObjects :
			{
				for (auto& seq : aggr->getSequence())
				{
					COMP_CHECK_ERR( _DeserializeExternalObjects( seq, INOUT result ));
				}
				return true;
			}
		}
		return true;
	}
	
/*
=================================================
	_ToBindingIndex
=================================================
*/
	BindingIndex  SpirvCompiler::_ToBindingIndex (uint index) const
	{
		if ( _targetVulkan )
			return BindingIndex{ UMax, index };
		else
			return BindingIndex{ index, UMax };
	}
	
/*
=================================================
	GetDesciptorSet
=================================================
*/
	ND_ static PipelineDescription::DescriptorSet&  GetDesciptorSet (uint dsIndex, INOUT SpirvCompiler::ShaderReflection &reflection)
	{
		for (auto& ds : reflection.layout.descriptorSets)
		{
			if ( ds.bindingIndex == dsIndex )
				return ds;
		}

		PipelineDescription::DescriptorSet	ds;
		ds.bindingIndex	= dsIndex;
		ds.id			= DescriptorSetID( ToString(dsIndex) );
		ds.uniforms		= MakeShared< PipelineDescription::UniformMap_t >();

		reflection.layout.descriptorSets.push_back( std::move(ds) );

		return reflection.layout.descriptorSets.back();
	}
	
/*
=================================================
	_ExtractImageType
=================================================
*/
	EImage  SpirvCompiler::_ExtractImageType (const TType &type) const
	{
		if ( type.getBasicType() == TBasicType::EbtSampler and not type.isSubpass() )
		{
			TSampler const&	samp = type.getSampler();
			
			ENABLE_ENUM_CHECKS();
			switch ( samp.dim )
			{
				case TSamplerDim::Esd1D :
				{
					if ( samp.isShadow() and samp.isArrayed() )		return EImage::Tex1DArray;		else
					if ( samp.isShadow() )							return EImage::Tex1D;			else
					if ( samp.isArrayed() )							return EImage::Tex1DArray;		else
																	return EImage::Tex1D;
				}
				case TSamplerDim::Esd2D :
				{
					if ( samp.isShadow() and samp.isArrayed() )			return EImage::Tex2DArray;		else
					if ( samp.isShadow() )								return EImage::Tex2D;			else
					if ( samp.isMultiSample() and samp.isArrayed() )	return EImage::Tex2DMSArray;	else
					if ( samp.isArrayed() )								return EImage::Tex2DArray;		else
					if ( samp.isMultiSample() )							return EImage::Tex2DMS;			else
																		return EImage::Tex2D;
				}
				case TSamplerDim::Esd3D :
				{
					return EImage::Tex3D;
				}
				case TSamplerDim::EsdCube :
				{
					if ( samp.isShadow() )		return EImage::TexCube;			else
					if ( samp.isArrayed() )		return EImage::TexCubeArray;	else
												return EImage::TexCube;
					break;
				}
				case TSamplerDim::EsdBuffer :
				{
					return EImage::Buffer;
				}
				case TSamplerDim::EsdNone :		// to shutup warnings
				case TSamplerDim::EsdRect :
				case TSamplerDim::EsdSubpass :
				case TSamplerDim::EsdNumDims :
				default :
					COMP_RETURN_ERR( "unknown sampler dimension type!" );
			}
			DISABLE_ENUM_CHECKS();
		}
		COMP_RETURN_ERR( "type is not image/sampler type!" );
	}
	
/*
=================================================
	_ExtractImageFormat
=================================================
*/
	EPixelFormat  SpirvCompiler::_ExtractImageFormat (uint format) const
	{
		ENABLE_ENUM_CHECKS();
		switch ( BitCast<TLayoutFormat>(format) )
		{
			case TLayoutFormat::ElfNone :			return EPixelFormat::Unknown;
			case TLayoutFormat::ElfRgba32f :		return EPixelFormat::RGBA32F;
			case TLayoutFormat::ElfRgba16f :		return EPixelFormat::RGBA16F;
			case TLayoutFormat::ElfR32f :			return EPixelFormat::R32F;
			case TLayoutFormat::ElfRgba8 :			return EPixelFormat::RGBA8_UNorm;
			case TLayoutFormat::ElfRgba8Snorm :		return EPixelFormat::RGBA8_SNorm;
			case TLayoutFormat::ElfRg32f :			return EPixelFormat::RG32F;
			case TLayoutFormat::ElfRg16f :			return EPixelFormat::RG16F;
			case TLayoutFormat::ElfR11fG11fB10f :	return EPixelFormat::RGB_11_11_10F;
			case TLayoutFormat::ElfR16f :			return EPixelFormat::R16F;
			case TLayoutFormat::ElfRgba16 :			return EPixelFormat::RGBA16_UNorm;
			case TLayoutFormat::ElfRgb10A2 :		return EPixelFormat::RGB10_A2_UNorm;
			case TLayoutFormat::ElfRg16 :			return EPixelFormat::RG16_UNorm;
			case TLayoutFormat::ElfRg8 :			return EPixelFormat::RG8_UNorm;
			case TLayoutFormat::ElfR16 :			return EPixelFormat::R16_UNorm;
			case TLayoutFormat::ElfR8 :				return EPixelFormat::R8_UNorm;
			case TLayoutFormat::ElfRgba16Snorm :	return EPixelFormat::RGBA16_SNorm;
			case TLayoutFormat::ElfRg16Snorm :		return EPixelFormat::RG16_SNorm;
			case TLayoutFormat::ElfRg8Snorm :		return EPixelFormat::RG8_SNorm;
			case TLayoutFormat::ElfR16Snorm :		return EPixelFormat::R16_SNorm;
			case TLayoutFormat::ElfR8Snorm :		return EPixelFormat::R8_SNorm;
			case TLayoutFormat::ElfRgba32i :		return EPixelFormat::RGBA32I;
			case TLayoutFormat::ElfRgba16i :		return EPixelFormat::RGBA16I;
			case TLayoutFormat::ElfRgba8i :			return EPixelFormat::RGBA8I;
			case TLayoutFormat::ElfR32i :			return EPixelFormat::R32I;
			case TLayoutFormat::ElfRg32i :			return EPixelFormat::RG32I;
			case TLayoutFormat::ElfRg16i :			return EPixelFormat::RG16I;
			case TLayoutFormat::ElfRg8i :			return EPixelFormat::RG8I;
			case TLayoutFormat::ElfR16i :			return EPixelFormat::R16I;
			case TLayoutFormat::ElfR8i :			return EPixelFormat::R8I;
			case TLayoutFormat::ElfRgba32ui :		return EPixelFormat::RGBA32U;
			case TLayoutFormat::ElfRgba16ui :		return EPixelFormat::RGBA16U;
			case TLayoutFormat::ElfRgba8ui :		return EPixelFormat::RGBA8U;
			case TLayoutFormat::ElfR32ui :			return EPixelFormat::R32U;
			case TLayoutFormat::ElfRg32ui :			return EPixelFormat::RG32U;
			case TLayoutFormat::ElfRg16ui :			return EPixelFormat::RG16U;
			case TLayoutFormat::ElfRgb10a2ui :		return EPixelFormat::RGB10_A2U;
			case TLayoutFormat::ElfRg8ui :			return EPixelFormat::RG8U;
			case TLayoutFormat::ElfR16ui :			return EPixelFormat::R16U;
			case TLayoutFormat::ElfR8ui :			return EPixelFormat::R8U;
			case TLayoutFormat::ElfEsFloatGuard :
			case TLayoutFormat::ElfFloatGuard :
			case TLayoutFormat::ElfEsIntGuard :
			case TLayoutFormat::ElfIntGuard :
			case TLayoutFormat::ElfEsUintGuard :
			case TLayoutFormat::ElfCount :			break;	// to shutup warnings
		}
		DISABLE_ENUM_CHECKS();
		COMP_RETURN_ERR( "Unsupported image format!" );
	}
	
/*
=================================================
	ExtractShaderAccessType
=================================================
*/
	ND_ static EResourceState  ExtractShaderAccessType (const TQualifier &q, EShaderCompilationFlags flags)
	{
		if ( q.coherent or
			 q.volatil	or
			 q.restrict )
		{
			return EResourceState::ShaderReadWrite;
		}

		if ( q.readonly )
			return EResourceState::ShaderRead;

		if ( q.writeonly )
			return EResourceState::ShaderWrite | (EnumEq( flags, EShaderCompilationFlags::AlwaysWriteDiscard ) ? EResourceState::InvalidateBefore : EResourceState::Unknown);

		// defualt:
		return EResourceState::ShaderReadWrite;
	}

/*
=================================================
	ExtractNodeName
=================================================
*/
	ND_ static String  ExtractNodeName (TIntermNode *node)
	{
		CHECK_ERR( node and node->getAsSymbolNode() );
		
		String				name	= node->getAsSymbolNode()->getName().c_str();
		const StringView	prefix	= "anon@";

		if ( not name.compare( 0, prefix.size(), prefix ))
			name.clear();

		return name;
	}
	
/*
=================================================
	Extract***ID
=================================================
*/
	ND_ static UniformID  ExtractUniformID (TIntermNode *node)
	{
		return UniformID( ExtractNodeName( node ));
	}
	
	ND_ static VertexID  ExtractVertexID (TIntermNode *node)
	{
		return VertexID( ExtractNodeName( node ));
	}

	ND_ static RenderTargetID  ExtractRenderTargetID (TIntermNode *node)
	{
		return RenderTargetID( ExtractNodeName( node ));
	}

	ND_ static SpecializationID  ExtractSpecializationID (TIntermNode *node)
	{
		return SpecializationID( ExtractNodeName( node ));
	}

/*
=================================================
	ExtractBufferUniformID
=================================================
*/
	ND_ static UniformID  ExtractBufferUniformID (const TType &type)
	{
		return UniformID( type.getTypeName().c_str() );
	}

/*
=================================================
	_ExtractVertexType
=================================================
*/
	EVertexType  SpirvCompiler::_ExtractVertexType (const TType &type) const
	{
		EVertexType		result = EVertexType(0);

		COMP_CHECK_ERR( not type.isArray() );
		
		ENABLE_ENUM_CHECKS();
		switch ( type.getBasicType() )
		{
			case TBasicType::EbtFloat :		result |= EVertexType::_Float;	break;
			case TBasicType::EbtDouble :	result |= EVertexType::_Double;	break;
			case TBasicType::EbtFloat16 :	result |= EVertexType::_Half;	break;
			case TBasicType::EbtInt8 :		result |= EVertexType::_Byte;	break;
			case TBasicType::EbtUint8 :		result |= EVertexType::_UByte;	break;
			case TBasicType::EbtInt16 :		result |= EVertexType::_Short;	break;
			case TBasicType::EbtUint16 :	result |= EVertexType::_UShort;	break;
			case TBasicType::EbtInt :		result |= EVertexType::_Int;	break;
			case TBasicType::EbtUint :		result |= EVertexType::_UInt;	break;
			case TBasicType::EbtInt64 :		result |= EVertexType::_Long;	break;
			case TBasicType::EbtUint64 :	result |= EVertexType::_ULong;	break;
			//case TBasicType::EbtBool :	result |= EVertexType::_Bool;	break;
			case TBasicType::EbtVoid :		// to shutup warnings
			case TBasicType::EbtBool :
			case TBasicType::EbtAtomicUint :
			case TBasicType::EbtSampler :
			case TBasicType::EbtStruct :
			case TBasicType::EbtBlock :
			#ifdef NV_EXTENSIONS
			case TBasicType::EbtAccStructNV :
			#endif
			case TBasicType::EbtString :
			case TBasicType::EbtNumTypes :
			default :						COMP_RETURN_ERR( "unsupported basic type!" );
		}
		DISABLE_ENUM_CHECKS();

		if ( type.isScalarOrVec1() )
			return result;

		if ( type.isVector() )
		{
			switch ( type.getVectorSize() )
			{
				case 1 :	result |= EVertexType::_Vec1;	break;
				case 2 :	result |= EVertexType::_Vec2;	break;
				case 3 :	result |= EVertexType::_Vec3;	break;
				case 4 :	result |= EVertexType::_Vec4;	break;
				default :	COMP_RETURN_ERR( "unsupported vector size!" );
			}
			return result;
		}

		if ( type.isMatrix() )
		{
			COMP_RETURN_ERR( "not supported, yet" );
		}

		COMP_RETURN_ERR( "unknown vertex type" );
	}
	
/*
=================================================
	_ExtractFragmentOutputType
=================================================
*/
	EFragOutput  SpirvCompiler::_ExtractFragmentOutputType (const TType &type) const
	{
		COMP_CHECK_ERR( type.getVectorSize() == 4 );

		switch ( type.getBasicType() )
		{
			case TBasicType::EbtFloat :		return EFragOutput::Float4;
			case TBasicType::EbtInt :		return EFragOutput::Int4;
			case TBasicType::EbtUint :		return EFragOutput::UInt4;
		}
		COMP_RETURN_ERR( "unsupported fragment output" );
	}
	
/*
=================================================
	_CalculateStructSize
----
	based on TParseContext::fixBlockUniformOffsets
=================================================
*/
	bool SpirvCompiler::_CalculateStructSize (const TType &bufferType, OUT BytesU &staticSize, OUT BytesU &arrayStride, OUT BytesU &minOffset) const
	{
		staticSize = arrayStride = 0_b;
		minOffset = ~0_b;

		COMP_CHECK_ERR( bufferType.isStruct() );
		COMP_CHECK_ERR( bufferType.getQualifier().isUniformOrBuffer() or bufferType.getQualifier().layoutPushConstant );
		COMP_CHECK_ERR( bufferType.getQualifier().layoutPacking == ElpStd140 or
						bufferType.getQualifier().layoutPacking == ElpStd430 );

		int		member_size		= 0;
		int		offset			= 0;
		auto&	struct_fields	= *bufferType.getStruct();

		for (size_t member = 0; member < struct_fields.size(); ++member)
		{
			const TType&		member_type			= *struct_fields[member].type;
			const TQualifier&	member_qualifier	= member_type.getQualifier();
			TLayoutMatrix		sub_matrix_layout	= member_qualifier.layoutMatrix;

			int dummy_stride;
			int member_alignment = _intermediate->getBaseAlignment( member_type, OUT member_size, OUT dummy_stride,
																	bufferType.getQualifier().layoutPacking,
																	sub_matrix_layout != ElmNone ? sub_matrix_layout == ElmRowMajor
																		: bufferType.getQualifier().layoutMatrix == ElmRowMajor );

			if ( member_qualifier.hasOffset() )
			{
				ASSERT( IsMultipleOfPow2( member_qualifier.layoutOffset, member_alignment ));

				if ( _intermediate->getSpv().spv == 0 )
				{
					ASSERT( member_qualifier.layoutOffset >= offset );

					offset = std::max( offset, member_qualifier.layoutOffset );
				}
				else {
					offset = member_qualifier.layoutOffset;
				}
			}

			if ( member_qualifier.hasAlign() )
				member_alignment = std::max( member_alignment, member_qualifier.layoutAlign );

			RoundToPow2( offset, member_alignment );
			//member_type.getQualifier().layoutOffset = offset;

			minOffset = Min( minOffset, uint(offset) );

			offset += member_size;

			// for last member
			if ( member+1 == struct_fields.size() and member_type.isUnsizedArray() )
			{
				ASSERT( member_size == 0 );

				arrayStride = BytesU(dummy_stride);
			}
		}
		staticSize = BytesU(offset);
		return true;
	}

/*
=================================================
	_DeserializeExternalObjects
=================================================
*/
	bool SpirvCompiler::_DeserializeExternalObjects (TIntermNode* node, INOUT ShaderReflection &result) const
	{
		TIntermTyped*	tnode			= node->getAsTyped();
		auto const&		type			= tnode->getType();
		auto const&		qual			= type.getQualifier();
		const bool		is_dynamic		= EnumEq( _compilerFlags, EShaderCompilationFlags::AlwaysBufferDynamicOffset );

		auto&			descriptor_set	= GetDesciptorSet( qual.hasSet() ? uint(qual.layoutSet) : 0, result );
		auto&			uniforms		= const_cast<PipelineDescription::UniformMap_t &>( *descriptor_set.uniforms );

		if ( type.getBasicType() == TBasicType::EbtSampler )
		{
			// texture
			if ( type.getSampler().isCombined() )
			{
				PipelineDescription::Texture	tex;
				tex.textureType	= _ExtractImageType( type );
				tex.state		= EResourceState::ShaderSample | EResourceState_FromShaders( _currentStage );

				PipelineDescription::Uniform	un;
				un.index		= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.stageFlags	= _currentStage;
				un.data			= std::move(tex);

				uniforms.insert({ ExtractUniformID( node ), std::move(un) });
				return true;
			}
			
			// image
			if ( type.getSampler().isImage() )
			{
				PipelineDescription::Image		image;
				image.imageType	= _ExtractImageType( type );
				image.format	= _ExtractImageFormat( qual.layoutFormat );
				image.state		= ExtractShaderAccessType( qual, _compilerFlags ) | EResourceState_FromShaders( _currentStage );
				
				PipelineDescription::Uniform	un;
				un.index		= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.stageFlags	= _currentStage;
				un.data			= std::move(image);

				uniforms.insert({ ExtractUniformID( node ), std::move(un) });
				return true;
			}
			
			// subpass
			if ( type.getSampler().isSubpass() )
			{
				PipelineDescription::SubpassInput	subpass;
				subpass.attachmentIndex	= qual.hasAttachment() ? uint(qual.layoutAttachment) : UMax;
				subpass.isMultisample	= false;	// TODO
				subpass.state			= EResourceState::InputAttachment | EResourceState_FromShaders( _currentStage );
				
				PipelineDescription::Uniform	un;
				un.index		= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.stageFlags	= _currentStage;
				un.data			= std::move(subpass);

				uniforms.insert({ ExtractUniformID( node ), std::move(un) });
				return true;
			}
			
			// sampler
			if ( qual.storage == TStorageQualifier::EvqUniform )
			{
				PipelineDescription::Uniform	un;
				un.index		= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.stageFlags	= _currentStage;
				un.data			= PipelineDescription::Sampler{};

				uniforms.insert({ ExtractUniformID( node ), std::move(un) });
				return true;
			}
		}
		
		// push constants
		if ( qual.layoutPushConstant )
		{
			BytesU	size, stride, offset;
			COMP_CHECK_ERR( _CalculateStructSize( type, OUT size, OUT stride, OUT offset ));

			result.layout.pushConstants.insert({ PushConstantID(type.getTypeName().c_str()), {_currentStage, offset, size} });
			return true;
		}
		
		// uniform buffer or storage buffer
		if ( type.getBasicType() == TBasicType::EbtBlock	and
			 (qual.storage == TStorageQualifier::EvqUniform	or qual.storage == TStorageQualifier::EvqBuffer) )
		{
			COMP_CHECK_ERR( type.isStruct() );

			if ( qual.layoutShaderRecordNV )
				return true;
			
			// uniform block
			if ( qual.storage == TStorageQualifier::EvqUniform )
			{
				PipelineDescription::UniformBuffer	ubuf;
				ubuf.state = EResourceState::UniformRead | EResourceState_FromShaders( _currentStage ) |
							 (is_dynamic ? EResourceState::_BufferDynamicOffset : EResourceState::Unknown);
				ubuf.dynamicOffsetIndex = is_dynamic ? 0 : PipelineDescription::STATIC_OFFSET;

				BytesU	stride, offset;
				COMP_CHECK_ERR( _CalculateStructSize( type, OUT ubuf.size, OUT stride, OUT offset ));
				
				PipelineDescription::Uniform	un;
				un.index		= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.stageFlags	= _currentStage;
				un.data			= std::move(ubuf);

				uniforms.insert({ ExtractBufferUniformID( type ), std::move(un) });
				return true;
			}

			// storage block
			if ( qual.storage == TStorageQualifier::EvqBuffer )
			{
				PipelineDescription::StorageBuffer	sbuf;
				sbuf.state = ExtractShaderAccessType( qual, _compilerFlags ) | EResourceState_FromShaders( _currentStage ) |
							 (is_dynamic ? EResourceState::_BufferDynamicOffset : EResourceState::Unknown);
				sbuf.dynamicOffsetIndex = is_dynamic ? 0 : PipelineDescription::STATIC_OFFSET;
			
				BytesU	offset;
				COMP_CHECK_ERR( _CalculateStructSize( type, OUT sbuf.staticSize, OUT sbuf.arrayStride, OUT offset ));
				
				PipelineDescription::Uniform	un;
				un.index		= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
				un.stageFlags	= _currentStage;
				un.data			= std::move(sbuf);

				uniforms.insert({ ExtractBufferUniformID( type ), std::move(un) });
				return true;
			}
		}
		
		// acceleration structure
		#ifdef NV_EXTENSIONS
		if ( type.getBasicType() == TBasicType::EbtAccStructNV )
		{
			PipelineDescription::RayTracingScene	rt_scene;
			rt_scene.state = EResourceState::_RayTracingShader | EResourceState::ShaderRead;
			
			PipelineDescription::Uniform	un;
			un.index		= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : UMax );
			un.stageFlags	= _currentStage;
			un.data			= std::move(rt_scene);

			uniforms.insert({ ExtractUniformID( node ), std::move(un) });
			return true;
		}

		if ( qual.storage == TStorageQualifier::EvqPayloadNV		or
			 qual.storage == TStorageQualifier::EvqPayloadInNV		or
			 qual.storage == TStorageQualifier::EvqHitAttrNV		or
			 qual.storage == TStorageQualifier::EvqCallableDataNV	or
			 qual.storage == TStorageQualifier::EvqCallableDataInNV )
		{
			return true;	// TODO
		}
		#endif

		// uniform
		if ( qual.storage == TStorageQualifier::EvqUniform )
		{
			COMP_RETURN_ERR( "uniform is not supported for Vulkan!" );
		}
		
		// shader input
		if ( qual.storage == TStorageQualifier::EvqVaryingIn )
		{
			if ( _currentStage != EShaderStages::Vertex )
				return true;	// skip

			GraphicsPipelineDesc::VertexAttrib	attrib;
			attrib.id		= ExtractVertexID( node );
			attrib.index	= (qual.hasLocation() ? uint(qual.layoutLocation) : UMax);
			attrib.type		= _ExtractVertexType( type );

			result.vertex.vertexAttribs.push_back( std::move(attrib) );
			return true;
		}
		
		// shader output
		if ( qual.storage == TStorageQualifier::EvqVaryingOut )
		{
			if ( _currentStage != EShaderStages::Fragment )
				return true;	// skip

			GraphicsPipelineDesc::FragmentOutput	frag_out;
			frag_out.id		= ExtractRenderTargetID( node );
			frag_out.index	= (qual.hasLocation() ? uint(qual.layoutLocation) : UMax);
			frag_out.type	= _ExtractFragmentOutputType( type );

			result.fragment.fragmentOutput.push_back( std::move(frag_out) );
			return true;
		}
		
		// skip builtin
		if ( type.isBuiltIn() )
			return true;

		// specialization constant
		if ( qual.storage == EvqConst and
			 qual.layoutSpecConstantId != TQualifier::layoutSpecConstantIdEnd )
		{
			result.specConstants.insert({ ExtractSpecializationID( node ), qual.layoutSpecConstantId });
			return true;
		}

		// global variable or global constant
		if ( qual.storage == EvqGlobal or qual.storage == EvqConst )
			return true;

		// shared variables
		if ( qual.storage == EvqShared )
			return true;

		COMP_RETURN_ERR( "unknown external type!" );
	}

/*
=================================================
	_MergeWithGeometryInputPrimitive
=================================================
*/
	void SpirvCompiler::_MergeWithGeometryInputPrimitive (INOUT GraphicsPipelineDesc::TopologyBits_t &topologyBits, uint type) const
	{
		ENABLE_ENUM_CHECKS();
		switch ( BitCast<TLayoutGeometry>(type) )
		{
			case TLayoutGeometry::ElgPoints : {
				topologyBits.set( uint(EPrimitive::Point) );
				break;
			}
			case TLayoutGeometry::ElgLines : {
				topologyBits.set( uint(EPrimitive::LineList) );
				topologyBits.set( uint(EPrimitive::LineStrip) );
				break;
			}
			case TLayoutGeometry::ElgLinesAdjacency : {
				topologyBits.set( uint(EPrimitive::LineListAdjacency) );
				topologyBits.set( uint(EPrimitive::LineStripAdjacency) );
				break;
			}
			case TLayoutGeometry::ElgTriangles : {
				topologyBits.set( uint(EPrimitive::TriangleList) );
				topologyBits.set( uint(EPrimitive::TriangleStrip) );
				topologyBits.set( uint(EPrimitive::TriangleFan) );
				break;
			}
			case TLayoutGeometry::ElgTrianglesAdjacency : {
				topologyBits.set( uint(EPrimitive::TriangleListAdjacency) );
				topologyBits.set( uint(EPrimitive::TriangleStripAdjacency) );
				break;
			}
			case TLayoutGeometry::ElgNone :
			case TLayoutGeometry::ElgLineStrip :
			case TLayoutGeometry::ElgTriangleStrip :
			case TLayoutGeometry::ElgQuads :
			case TLayoutGeometry::ElgIsolines :		break;	// to shutup warnings
		}
		DISABLE_ENUM_CHECKS();
		COMP_RETURN_ERR( "invalid geometry input primitive type!", void() );
	}
	
/*
=================================================
	_ProcessShaderInfo
=================================================
*/
	bool SpirvCompiler::_ProcessShaderInfo (INOUT ShaderReflection &result) const
	{
		ENABLE_ENUM_CHECKS();
		switch ( _intermediate->getStage() )
		{
			case EShLangVertex :
			{
				break;
			}

			case EShLangTessControl :
			{
				result.tessellation.patchControlPoints	= _intermediate->getVertices();

				result.vertex.supportedTopology.set( uint(EPrimitive::Patch) );
				break;
			}

			case EShLangTessEvaluation :
			{
				break;
			}

			case EShLangGeometry :
			{
				_MergeWithGeometryInputPrimitive( INOUT result.vertex.supportedTopology, _intermediate->getInputPrimitive() );
				break;
			}

			case EShLangFragment :
			{
				result.fragment.earlyFragmentTests = (_intermediate->getEarlyFragmentTests() or not _intermediate->isDepthReplacing());

				break;
			}

			case EShLangCompute :
			{
				result.compute.localGroupSize.x	= _intermediate->getLocalSize(0);
				result.compute.localGroupSize.y = _intermediate->getLocalSize(1);
				result.compute.localGroupSize.z	= _intermediate->getLocalSize(2);

				result.compute.localGroupSpecialization.x = uint(_intermediate->getLocalSizeSpecId(0));
				result.compute.localGroupSpecialization.y = uint(_intermediate->getLocalSizeSpecId(1));
				result.compute.localGroupSpecialization.z = uint(_intermediate->getLocalSizeSpecId(2));
				break;
			}

			case EShLangRayGenNV :
			{
				break;
			}
			case EShLangIntersectNV :
			{
				break;
			}
			case EShLangAnyHitNV :
			{
				break;
			}
			case EShLangClosestHitNV :
			{
				break;
			}
			case EShLangMissNV :
			{
				break;
			}
			case EShLangCallableNV :
			{
				break;
			}

			case EShLangTaskNV :
			{
				result.mesh.taskGroupSize.x	= _intermediate->getLocalSize(0);
				result.mesh.taskGroupSize.y = _intermediate->getLocalSize(1);
				result.mesh.taskGroupSize.z	= _intermediate->getLocalSize(2);

				result.mesh.taskGroupSpecialization.x = uint(_intermediate->getLocalSizeSpecId(0));
				result.mesh.taskGroupSpecialization.y = uint(_intermediate->getLocalSizeSpecId(1));
				result.mesh.taskGroupSpecialization.z = uint(_intermediate->getLocalSizeSpecId(2));
				break;
			}
			case EShLangMeshNV :
			{
				result.mesh.maxVertices		= _intermediate->getVertices();
				result.mesh.maxPrimitives	= _intermediate->getPrimitives();
				result.mesh.maxIndices		= result.mesh.maxPrimitives;

				DISABLE_ENUM_CHECKS();
				switch ( _intermediate->getOutputPrimitive() )
				{
					case TLayoutGeometry::ElgPoints :
						result.mesh.topology = EPrimitive::Point;
						result.mesh.maxIndices *= 1;
						break;

					case TLayoutGeometry::ElgLines :
						result.mesh.topology = EPrimitive::LineList;
						result.mesh.maxIndices *= 2;
						break;

					case TLayoutGeometry::ElgTriangles :
						result.mesh.topology = EPrimitive::TriangleList;
						result.mesh.maxIndices *= 3;
						break;

					default :
						CHECK(false);
						break;
				}
				ENABLE_ENUM_CHECKS();

				result.mesh.meshGroupSize.x	= _intermediate->getLocalSize(0);
				result.mesh.meshGroupSize.y = _intermediate->getLocalSize(1);
				result.mesh.meshGroupSize.z	= _intermediate->getLocalSize(2);

				result.mesh.meshGroupSpecialization.x = uint(_intermediate->getLocalSizeSpecId(0));
				result.mesh.meshGroupSpecialization.y = uint(_intermediate->getLocalSizeSpecId(1));
				result.mesh.meshGroupSpecialization.z = uint(_intermediate->getLocalSizeSpecId(2));
				break;
			}

			case EShLangCount : break;
		}
		DISABLE_ENUM_CHECKS();
		return true;
	}

}	// FG
