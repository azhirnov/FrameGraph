// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "extensions/pipeline_compiler/PipelineCppSerializer.h"
#include "extensions/pipeline_compiler/VPipelineCompiler.h"
#include "framegraph/Shared/EnumUtils.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"
using namespace FG;


/*
=================================================
	AddShaderSource
=================================================
*/
static bool AddShaderSource (OUT PipelineDescription::Shader &shaderData, const EShader type, StringView src)
{
	StringView	keyword;

	switch ( type )
	{
		case EShader::Vertex :			keyword = "SH_VERTEX";				break;
		case EShader::TessControl :		keyword = "SH_TESS_CONTROL";		break;
		case EShader::TessEvaluation :	keyword = "SH_TESS_EVALUATION";		break;
		case EShader::Geometry :		keyword = "SH_GEOMETRY";			break;
		case EShader::Fragment :		keyword = "SH_FRAGMENT";			break;
		case EShader::Compute :			keyword = "SH_COMPUTE";				break;
		case EShader::MeshTask :		keyword = "SH_MESH_TASK";			break;
		case EShader::Mesh :			keyword = "SH_MESH";				break;
		default :						return false;
	}

	if ( not HasSubString( src, keyword ) )
		return false;

	enum EModeBits {
		Mode_Comment			= 1 << 0,
		Mode_CommentMultiline	= 1 << 1,
		Mode_AnyComment			= Mode_Comment | Mode_CommentMultiline,
		Mode_Macro				= 1 << 2,
		Mode_OneLineFlags		= Mode_Comment | Mode_Macro,
	};

	String	temp;			temp.reserve( src.length() + 20 );
	size_t	insertion_pos	= 0;
	size_t	first_include	= ~size_t(0);
	uint	mode			= 0;
	bool	keyword_found	= false;

	// insert shader type macro after the last '#extension' with respect to comments
	for (size_t pos = 0; pos < src.length(); ++pos)
	{
		const char	c = src[pos];
		const char	n = pos+1 < src.length() ? src[pos+1] : '\0';

		// skip spaces and tabs
		if ( c == ' ' or c == '\t' )
			continue;

		// next line
		if ( c == '\n' or c == '\r' )
		{
			mode &= ~Mode_OneLineFlags;
			continue;
		}

		// end of multiline comment
		if ( c == '*' and n == '/' and (mode & Mode_CommentMultiline) ) {
			mode &= ~Mode_CommentMultiline;
			continue;
		}

		if ( mode & Mode_AnyComment )
			continue;

		if ( c == '/' and n == '/' ) {
			mode |= Mode_Comment;
			continue;
		}

		if ( c == '/' and n == '*' ) {
			mode |= Mode_CommentMultiline;
			continue;
		}

		if ( c == '#' )
		{
			mode |= Mode_Macro;
			continue;
		}

		if ( mode & Mode_Macro )
		{
			size_t	line_end = pos;

			// find line end
			for (; line_end < src.length(); ++line_end)
			{
				const char	c1 = src[line_end];
				const char	n1 = line_end+1 < src.length() ? src[line_end+1] : '\0';
				
				if ( (c == '/' and n == '/') or (c == '/' and n == '*') or c == '\n' or c == '\r' )
					break;

				++line_end;
			}
			
			StringView	line = src.substr( pos, line_end - pos );

			if ( StartsWith( line, "extension" ) )
				insertion_pos = line_end;
			else
			if ( StartsWith( line, "version" ) )
				insertion_pos = line_end;
			else
			if ( StartsWith( line, "include" ) )
				first_include = Min( first_include, pos );
			else
			if ( StartsWith( line, "if" ) or StartsWith( line, "elif" ) )
				keyword_found |= HasSubString( line, keyword );

			pos = line_end-1;
			continue;
		}
	}

	if ( not keyword_found )
		return false;

	// inserted macro has no effect on included source code
	//ASSERT( insertion_pos < first_include );

	temp << src.substr( 0, insertion_pos );
	temp << "\n#define SH_VERTEX " << ToString(1 << uint(EShader::Vertex))
		 << "\n#define SH_TESS_CONTROL " << ToString(1 << uint(EShader::TessControl))
		 << "\n#define SH_TESS_EVALUATION " << ToString(1 << uint(EShader::TessEvaluation))
		 << "\n#define SH_GEOMETRY " << ToString(1 << uint(EShader::Geometry))
		 << "\n#define SH_FRAGMENT " << ToString(1 << uint(EShader::Fragment))
		 << "\n#define SH_COMPUTE " << ToString(1 << uint(EShader::Compute))
		 << "\n#define SH_MESH_TASK " << ToString(1 << uint(EShader::MeshTask))
		 << "\n#define SH_MESH " << ToString(1 << uint(EShader::Mesh))
		 << "\n#define SHADER " << ToString( 1u << uint(type) ) << "\n";
	temp << src.substr( insertion_pos );

	shaderData.AddShaderData( EShaderLangFormat::VKSL_110, "main", std::move(temp) );
	return true;
}

/*
=================================================
	ConvertGraphicsPipeline
=================================================
*/
static bool ConvertGraphicsPipeline (StringView pplnPath, const PipelineCppSerializer &serializer, VPipelineCompiler &compiler)
{
	FileRStream		rfile;
	CHECK_ERR( rfile.IsOpen() );

	String		source;
	CHECK_ERR( rfile.Read( size_t(rfile.Size()), OUT source ));

	std::filesystem::path	result_name{ pplnPath };
	result_name.replace_extension( "cpp" );

	GraphicsPipelineDesc		desc;
	PipelineDescription::Shader	temp;

	for (auto sh_type : {EShader::Vertex, EShader::TessControl, EShader::TessEvaluation, EShader::Geometry, EShader::Fragment})
	{
		if ( AddShaderSource( OUT temp, sh_type, source ) )
			desc._shaders.insert({ sh_type, std::move(temp) });
	}

	CHECK_ERR( compiler.Compile( INOUT desc, EShaderLangFormat::SPIRV_110 ));

	source.clear();
	CHECK_ERR( serializer.Serialize( desc, result_name.filename().string(), OUT source ));

	FileWStream		wfile{ result_name };
	CHECK_ERR( wfile.IsOpen() );
	CHECK_ERR( wfile.Write( StringView{source} ));

	return true;
}

/*
=================================================
	ConvertComputePipeline
=================================================
*/
static bool ConvertComputePipeline (StringView pplnPath, const PipelineCppSerializer &serializer, VPipelineCompiler &compiler)
{
	FileRStream		rfile;
	CHECK_ERR( rfile.IsOpen() );

	String		source;
	CHECK_ERR( rfile.Read( size_t(rfile.Size()), OUT source ));

	std::filesystem::path	result_name{ pplnPath };
	result_name.replace_extension( "cpp" );

	ComputePipelineDesc		desc;
	PipelineDescription::Shader	temp;

	if ( AddShaderSource( OUT temp, EShader::Compute, source ) )
		desc._shader = std::move(temp);

	CHECK_ERR( compiler.Compile( INOUT desc, EShaderLangFormat::SPIRV_110 ));

	source.clear();
	CHECK_ERR( serializer.Serialize( desc, result_name.filename().string(), OUT source ));

	FileWStream		wfile{ result_name };
	CHECK_ERR( wfile.IsOpen() );
	CHECK_ERR( wfile.Write( StringView{source} ));

	return true;
}

/*
=================================================
	ConvertMeshPipeline
=================================================
*/
static bool ConvertMeshPipeline (StringView pplnPath, const PipelineCppSerializer &serializer, VPipelineCompiler &compiler)
{
	FileRStream		rfile;
	CHECK_ERR( rfile.IsOpen() );

	String		source;
	CHECK_ERR( rfile.Read( size_t(rfile.Size()), OUT source ));

	std::filesystem::path	result_name{ pplnPath };
	result_name.replace_extension( "cpp" );

	MeshPipelineDesc			desc;
	PipelineDescription::Shader	temp;

	for (auto sh_type : {EShader::MeshTask, EShader::Mesh, EShader::Fragment})
	{
		if ( AddShaderSource( OUT temp, sh_type, source ) )
			desc._shaders.insert({ sh_type, std::move(temp) });
	}

	CHECK_ERR( compiler.Compile( INOUT desc, EShaderLangFormat::SPIRV_110 ));

	source.clear();
	CHECK_ERR( serializer.Serialize( desc, result_name.filename().string(), OUT source ));

	FileWStream		wfile{ result_name };
	CHECK_ERR( wfile.IsOpen() );
	CHECK_ERR( wfile.Write( StringView{source} ));

	return true;
}

/*
=================================================
	ConvertRayTracingPipeline
=================================================
*/
static bool ConvertRayTracingPipeline (StringView pplnPath, const PipelineCppSerializer &serializer, VPipelineCompiler &compiler)
{
	return false;
}

/*
=================================================
	main
=================================================
*/
int main (int argc, char** argv)
{
	CHECK_ERR( argv > 0, 1 );

	PipelineCppSerializer	serializer;
	VPipelineCompiler		compiler;

	compiler.SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations | EShaderCompilationFlags::GenerateDebugInfo );


	for (int i = 1; i < argc; ++i)
	{
		StringView	key = argv[i];
		StringView	value;

		if ( ++i < argc )
			value = argv[i];

		if ( key == "--gppln" )
		{
			CHECK_ERR( ConvertGraphicsPipeline( value, serializer, compiler ), 2 );
		}
		else
		if ( key == "--cppln" )
		{
			CHECK_ERR( ConvertComputePipeline( value, serializer, compiler ), 3 );
		}
		else
		if ( key == "--mppln" )
		{
			CHECK_ERR( ConvertMeshPipeline( value, serializer, compiler ), 4 );
		}
		else
		if ( key == "--rtppln" )
		{
			CHECK_ERR( ConvertRayTracingPipeline( value, serializer, compiler ), 5 );
		}
		else
		{
			RETURN_ERR( "unsupported command arg: "s << key, 6 );
		}
	}

	return 0;
}
