// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/Shaders/ShaderCache.h"
#include "scene/Loader/Intermediate/VertexAttributes.h"
#include "framegraph/Shared/EnumUtils.h"
#include "stl/Stream/FileStream.h"
#include "stl/Algorithms/StringUtils.h"

#ifdef FG_STD_FILESYSTEM
#	include <filesystem>
	namespace fs = std::filesystem;
	using fpath = std::filesystem::path;
#else
#	error not supported!
#endif

namespace FG
{

/*
=================================================
	GraphicsPipelineInfo
=================================================
*/
	inline bool  ShaderCache::GraphicsPipelineInfo::operator == (const GraphicsPipelineInfo &rhs) const noexcept
	{
		return	attribs			== rhs.attribs			and
				vertexStride	== rhs.vertexStride		and
				textures		== rhs.textures			and
				detailLevel		== rhs.detailLevel		and
				constants		== rhs.constants		and
				sourceIDs		== rhs.sourceIDs;
	}

	inline size_t  ShaderCache::GraphicsPipelineInfoHash::operator () (const GraphicsPipelineInfo &x) const noexcept
	{
		return size_t(HashOf( x.attribs )   + HashOf( x.vertexStride ) +
					  HashOf( x.textures )	+ HashOf( x.detailLevel )  +
					  HashOf( x.sourceIDs )	+ HashOf( x.constants ));
	}
	
/*
=================================================
	RayTracingPipelineInfo
=================================================
*/
	inline bool  ShaderCache::RayTracingPipelineInfo::operator == (const RayTracingPipelineInfo &rhs) const noexcept
	{
		return	GraphicsPipelineInfo::operator == (rhs) and
				shaders == rhs.shaders;
	}

	inline size_t  ShaderCache::RayTracingPipelineInfoHash::operator () (const RayTracingPipelineInfo &x) const noexcept
	{
		return	size_t(HashVal{GraphicsPipelineInfoHash{}( x )} + HashOf( x.shaders ));
	}

/*
=================================================
	ComputePipelineInfo
=================================================
*/
	inline bool  ShaderCache::ComputePipelineInfo::operator == (const ComputePipelineInfo &rhs) const noexcept
	{
		return	sourceIDs == rhs.sourceIDs;
	}

	inline size_t  ShaderCache::ComputePipelineInfoHash::operator () (const ComputePipelineInfo &x) const noexcept
	{
		return size_t(HashOf( x.sourceIDs ));
	}
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	ShaderCache::ShaderCache ()
	{
		_sources.reserve( 128 );
	}
	
/*
=================================================
	destructor
=================================================
*/
	ShaderCache::~ShaderCache ()
	{
		ASSERT( _gpplnCache.empty() );
		ASSERT( _mpplnCache.empty() );
		ASSERT( _rtpplnCache.empty() );
		ASSERT( _cpplnCache.empty() );
	}

/*
=================================================
	Load
=================================================
*/
	bool ShaderCache::Load (const FGThreadPtr &fg, StringView path)
	{
		Clear();

		CHECK_ERR( fs::exists( path ));

		_frameGraph		= fg;
		_filePath		= path;
		_sharedSource	= CacheFileSource( "Utils/math.glsl" );

		return true;
	}
	
/*
=================================================
	Clear
=================================================
*/
	void ShaderCache::Clear ()
	{
		if ( _frameGraph )
		{
			for (auto& ppln : _gpplnCache) {
				_frameGraph->ReleaseResource( ppln.second );
			}
			for (auto& ppln : _mpplnCache) {
				_frameGraph->ReleaseResource( ppln.second );
			}
			for (auto& ppln : _rtpplnCache) {
				_frameGraph->ReleaseResource( ppln.second );
			}
			for (auto& ppln : _cpplnCache) {
				_frameGraph->ReleaseResource( ppln.second );
			}
		}

		_gpplnCache.clear();
		_mpplnCache.clear();
		_rtpplnCache.clear();
		_cpplnCache.clear();

		_frameGraph = null;
		_filePath.clear();
		_sourceCache.clear();
		_sources.clear();
	}

/*
=================================================
	CacheFileSource
=================================================
*/
	ShaderCache::ShaderSourceID  ShaderCache::CacheFileSource (StringView filename)
	{
		fpath	path{ _filePath };
		path.append( filename );

		FileRStream		file{ path };
		CHECK_ERR( file.IsOpen() );

		String	str;
		CHECK_ERR( file.Read( size_t(file.Size()), OUT str ));

		return _CacheShaderSource( std::move(str) );
	}
	
/*
=================================================
	BuildShaderDefines
=================================================
*/
	ND_ static String  BuildShaderDefines (const ShaderCache::GraphicsPipelineInfo &info)
	{
		String	result;

		for (ETextureType t = ETextureType(1); t <= info.textures; t = ETextureType(uint(t) << 1))
		{
			if ( not EnumEq( info.textures, t ) )
				continue;

			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case ETextureType::Albedo :		result << "#define ALBEDO_MAP\n";		break;
				case ETextureType::Normal :		result << "#define NORMAL_MAP\n";		break;
				case ETextureType::Height :		result << "#define HEIGHT_MAP\n";		break;
				case ETextureType::Reflection :	result << "#define REFLECTION_MAP\n";	break;
				case ETextureType::Specular :	result << "#define SPECULAR_MAP\n";		break;
				case ETextureType::Roughtness :	result << "#define ROUGHTNESS_MAP\n";	break;
				case ETextureType::Metallic :	result << "#define METALLIC_MAP\n";		break;
				case ETextureType::Unknown :
				default :						CHECK(!"unknown texture type");			break;
			}
			DISABLE_ENUM_CHECKS();
		}

		for (auto& const_pair : info.constants)
		{
			result << "#define " << StringView{const_pair.first} << " " << ToString( const_pair.second ) << '\n';
		}

		result << "#define DETAIL_LEVEL " << ToString( uint(info.detailLevel) ) << "\n";

		return result;
	}

/*
=================================================
	GetPipeline
=================================================
*/
	bool ShaderCache::GetPipeline (GraphicsPipelineInfo &info, OUT RawGPipelineID &outPipeline)
	{
		outPipeline = Default;

		info.sourceIDs.push_back( _sharedSource );
		std::sort( info.sourceIDs.begin(), info.sourceIDs.end() );
		std::sort( info.constants.begin(), info.constants.end() );

		auto	iter = _gpplnCache.find( info );

		if ( iter != _gpplnCache.end() )
		{
			outPipeline = iter->second.Get();
			return true;
		}

		const String	defines		= BuildShaderDefines( info ); //+ "void dbg_EnableTraceRecording (bool b) {}\n\n";
		String			fs_source	= "#define FRAGMENT_SHADER\n" + defines;
		String			vs_source	= "#define VERTEX_SHADER\n" + defines + _CacheVertexAttribs( info );

		for (auto& id : info.sourceIDs)
		{
			StringView	src = _GetCachedSource( id );
			vs_source << src;
			fs_source << src;
		}

		GraphicsPipelineDesc	desc;
		desc.AddShader( EShader::Vertex,   EShaderLangFormat::VKSL_110, "main", std::move(vs_source) );
		desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110, "main", std::move(fs_source) );

		GPipelineID	pipeline = _frameGraph->CreatePipeline( desc );
		if ( not pipeline )
			return false;

		outPipeline = pipeline.Get();
		_gpplnCache.insert_or_assign( info, std::move(pipeline) );

		return true;
	}
	
/*
=================================================
	GetPipeline
=================================================
*/
	bool ShaderCache::GetPipeline (GraphicsPipelineInfo &, OUT RawMPipelineID &)
	{
		return false;
	}
	
/*
=================================================
	GetPipeline
=================================================
*/
	bool ShaderCache::GetPipeline (RayTracingPipelineInfo &info, OUT RawRTPipelineID &outPipeline)
	{
		outPipeline = Default;
		
		info.sourceIDs.push_back( _sharedSource );
		std::sort( info.sourceIDs.begin(), info.sourceIDs.end() );
		std::sort( info.constants.begin(), info.constants.end() );
		std::sort( info.shaders.begin(), info.shaders.end() );

		auto	iter = _rtpplnCache.find( info );

		if ( iter != _rtpplnCache.end() )
		{
			outPipeline = iter->second.Get();
			return true;
		}

		String		header = "#version 460\n"
							 "#extension GL_NV_ray_tracing : require\n"
							 "#extension GL_EXT_nonuniform_qualifier : require\n" +
							 BuildShaderDefines( info ) +
							 _CacheRayTracingVertexBuffer( info );

		RayTracingPipelineDesc	desc;
		EShaderLangFormat		mode = Default; // EShaderLangFormat::EnableDebugTrace;

		for (auto& sh : info.shaders)
		{
			String	source = header;
			source << "#define RAYSHADER_" << sh.first.GetName() << "\n";
			
			for (auto& id : info.sourceIDs) {
				source << _GetCachedSource( id );
			}
			desc.AddShader( sh.first, sh.second, EShaderLangFormat::VKSL_110 | mode, "main", std::move(source) );
		}

		RTPipelineID	pipeline = _frameGraph->CreatePipeline( desc );
		if ( not pipeline )
			return false;

		outPipeline = pipeline.Get();
		_rtpplnCache.insert_or_assign( info, std::move(pipeline) );

		return true;
	}
	
/*
=================================================
	GetPipeline
=================================================
*/
	bool ShaderCache::GetPipeline (ComputePipelineInfo &, OUT RawCPipelineID &)
	{
		return false;
	}
	
/*
=================================================
	_GetCachedSource
=================================================
*/
	StringView  ShaderCache::_GetCachedSource (ShaderSourceID id) const
	{
		if ( size_t(id) < _sources.size() )
			return _sources[ size_t(id) ];

		RETURN_ERR( "doesn't exists", "" );
	}

/*
=================================================
	_CacheShaderSource
=================================================
*/
	ShaderCache::ShaderSourceID  ShaderCache::_CacheShaderSource (StringView src)
	{
		auto[iter, inserted] = _sourceCache.insert({ String(src), ShaderSourceID(_sources.size()) });

		if ( inserted )
			_sources.push_back( iter->first );
		
		return iter->second;
	}
	
/*
=================================================
	_CacheShaderSource
=================================================
*/
	ShaderCache::ShaderSourceID  ShaderCache::_CacheShaderSource (String &&src)
	{
		auto[iter, inserted] = _sourceCache.insert({ std::move(src), ShaderSourceID(_sources.size()) });

		if ( inserted )
			_sources.push_back( iter->first );
		
		return iter->second;
	}

/*
=================================================
	_CacheVertexAttribs
=================================================
*/
	String  ShaderCache::_CacheVertexAttribs (const GraphicsPipelineInfo &info)
	{
		if ( not info.attribs )
			return {};

		uint	index	= 0;
		String	str;

		str << "#ifdef VERTEX_SHADER\n";

		for (auto& attr : info.attribs->GetVertexInput().Vertices())
		{
			ASSERT( attr.second.index == UMax );

			str << "layout(location=" << ToString( index++ ) << ") in ";

			const EVertexType	type		= attr.second.ToDstType();
			const EVertexType	scalar_type	= (type & EVertexType::_TypeMask);
			const EVertexType	vec_size	= (type & EVertexType::_VecMask);
			const char			size_suffix	= char('0' + (int(vec_size) >> int(EVertexType::_VecOffset)));
			const bool			is_vec		= vec_size != EVertexType::_Vec1;
			const StringView	name		= VertexAttributeName::GetName( attr.first );

			switch ( scalar_type )
			{
				case EVertexType::_Int :	str << (is_vec ? "ivec"s + size_suffix : "int");	break;
				case EVertexType::_UInt :	str << (is_vec ? "uvec"s + size_suffix : "uint");	break;
				case EVertexType::_Float :	str << (is_vec ? "vec"s + size_suffix  : "float");	break;
				case EVertexType::_Double :	str << (is_vec ? "dvec"s + size_suffix : "double");	break;
				default :					RETURN_ERR( "unsupported vertex type" );
			}

			str << "  " << name << ";\n";
			str << "#define ATTRIB" << name.substr( 2 ) << ' ' << size_suffix << '\n';
		}
		str << "#endif	// VERTEX_SHADER\n";

		return str;
	}
	
/*
=================================================
	_CacheMeshBuffer
=================================================
*
	ShaderCache::ShaderSourceID  ShaderCache::_CacheMeshBuffer (const VertexInputState &, uint binding, uint set, uint vertCount)
	{
		// TODO
	}
	*/
	
/*
=================================================
	_CacheRayTracingVertexBuffer
=================================================
*/
	String  ShaderCache::_CacheRayTracingVertexBuffer (const GraphicsPipelineInfo &info)
	{
		if ( not info.attribs )
			return {};

		struct Attrib
		{
			StaticString<64>	str;
			BytesU				offset;
			BytesU				size;
		};
		using AttribArray_t	= FixedArray< Attrib, FG_MaxVertexAttribs >;
		
		String			str, temp;
		AttribArray_t	sorted;

		for (auto& attr : info.attribs->GetVertexInput().Vertices())
		{
			ASSERT( attr.second.index == UMax );
			
			const EVertexType	type		= attr.second.ToDstType();
			const EVertexType	scalar_type	= (type & EVertexType::_TypeMask);
			const EVertexType	vec_size	= (type & EVertexType::_VecMask);
			const char			size_suffix	= char('0' + (int(vec_size) >> int(EVertexType::_VecOffset)));
			const bool			is_vec		= vec_size != EVertexType::_Vec1;
			const StringView	name		= VertexAttributeName::GetName( attr.first );

			temp.clear();
			temp << '\t';

			switch ( scalar_type )
			{
				case EVertexType::_Int :	temp << (is_vec ? "ivec"s + size_suffix : "int");	break;
				case EVertexType::_UInt :	temp << (is_vec ? "uvec"s + size_suffix : "uint");	break;
				case EVertexType::_Float :	temp << (is_vec ? "vec"s + size_suffix  : "float");	break;
				case EVertexType::_Double :	temp << (is_vec ? "dvec"s + size_suffix : "double");	break;
				default :					RETURN_ERR( "unsupported vertex type" );
			}
			temp << "  " << name << ";\n";

			sorted.push_back(Attrib{ temp.data(), BytesU{attr.second.offset}, EVertexType_SizeOf( type ) });
			str << "#define ATTRIB" << name.substr( 2 ) << ' ' << size_suffix << '\n';
		}

		std::sort( sorted.begin(), sorted.end(),
				   [] (auto& lhs, auto& rhs) { return lhs.offset < rhs.offset; });

		str << "struct VertexAttrib\n{\n";

		for (size_t i = 0, j = 0; i < sorted.size(); ++i)
		{
			StringView	attr	= sorted[i].str;
			BytesU		off		= sorted[i].offset;
			BytesU		size	= sorted[i].size;
			BytesU		next	= i+1 < sorted.size() ? BytesU{sorted[i+1].offset} : info.vertexStride;
			auto		padding	= int64_t(next - (off + size));

			// TODO: check align

			str << attr;

			for (; padding > 0; padding -= 4) {
				str << "\tfloat  _padding" << ToString( j++ ) << ";\n";
			}
		}

		str << "};\n";
		return str;
	}


}	// FG
