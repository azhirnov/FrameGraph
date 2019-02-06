// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/Renderer/ShaderBuilder.h"
#include "scene/Loader/Intermediate/VertexAttributes.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	ShaderBuilder::ShaderBuilder ()
	{
		_tempString.reserve( 1 << 10 );
		_sources.reserve( 128 );
	}
	
/*
=================================================
	GetCachedSource
=================================================
*/
	StringView  ShaderBuilder::GetCachedSource (ShaderSourceID id) const
	{
		if ( size_t(id) < _sources.size() )
			return _sources[ size_t(id) ];

		RETURN_ERR( "doesn't exists", "" );
	}

/*
=================================================
	CacheShaderSource
=================================================
*/
	ShaderBuilder::ShaderSourceID  ShaderBuilder::CacheShaderSource (StringView src)
	{
		auto[iter, inserted] = _sourceCache.insert({ String(src), ShaderSourceID(_sources.size()) });

		if ( inserted )
			_sources.push_back( iter->first );
		
		return iter->second;
	}
	
/*
=================================================
	CacheShaderSource
=================================================
*/
	ShaderBuilder::ShaderSourceID  ShaderBuilder::CacheShaderSource (String &&src)
	{
		auto[iter, inserted] = _sourceCache.insert({ std::move(src), ShaderSourceID(_sources.size()) });

		if ( inserted )
			_sources.push_back( iter->first );
		
		return iter->second;
	}

/*
=================================================
	CacheVertexAttribs
=================================================
*/
	ShaderBuilder::ShaderSourceID  ShaderBuilder::CacheVertexAttribs (const VertexInputState &state)
	{
		uint	index	= 0;
		auto&	str		= _tempString;
		str.clear();

		str << "#ifdef VERTEX_SHADER\n";

		for (auto& attr : state.Vertices())
		{
			ASSERT( attr.second.index == UMax );

			str << "layout(location=" << ToString( index++ ) << ") in ";

			const EVertexType	type		= attr.second.ToDstType();
			const EVertexType	scalar_type	= (type & EVertexType::_TypeMask);
			const EVertexType	vec_size	= (type & EVertexType::_VecMask);
			const char			size_suffix	= char('0' + (int(vec_size) >> int(EVertexType::_VecOffset)));
			const bool			is_vec		= vec_size != EVertexType::_Vec1;

			switch ( scalar_type )
			{
				case EVertexType::_Int :	str << (is_vec ? "ivec"s + size_suffix : "int");	break;
				case EVertexType::_UInt :	str << (is_vec ? "uvec"s + size_suffix : "uint");	break;
				case EVertexType::_Float :	str << (is_vec ? "vec"s + size_suffix  : "float");	break;
				case EVertexType::_Double :	str << (is_vec ? "dvec"s + size_suffix : "double");	break;
				default :					RETURN_ERR( "unsupported vertex type" );
			}

			str << "  " << VertexAttributeName::GetName( attr.first ).data() << ";\n";
		}
		str << "#endif	// VERTEX_SHADER\n";

		return CacheShaderSource( std::move(str) );
	}
	
/*
=================================================
	CacheMeshBuffer
=================================================
*
	ShaderBuilder::ShaderSourceID  ShaderBuilder::CacheMeshBuffer (const VertexInputState &, uint binding, uint set, uint vertCount)
	{
		// TODO
	}
	*/

}	// FG
