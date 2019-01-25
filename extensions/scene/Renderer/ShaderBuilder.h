// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Renderer/Enums.h"

namespace FG
{

	//
	// Shader Builder
	//

	class ShaderBuilder
	{
	// types
	public:
		enum class ShaderSourceID : uint { Unknown = ~0u };

	private:
		using ShaderSourceCache_t	= HashMap< String, ShaderSourceID >;
		using ShaderSources_t		= Array< StringView >;


	// variables
	private:
		ShaderSourceCache_t		_sourceCache;
		ShaderSources_t			_sources;

		String					_tempString;


	// methods
	public:
		ShaderBuilder ();

		ND_ StringView  GetCachedSource (ShaderSourceID id) const;

		ND_ ShaderSourceID  CacheShaderSource (StringView src);
		ND_ ShaderSourceID  CacheShaderSource (String &&src);

		ND_ ShaderSourceID  CacheVertexAttribs (const VertexInputState &);
		ND_ ShaderSourceID  CacheMeshBuffer (const VertexInputState &, uint binding, uint set, uint vertCount, BytesU stride);
	};


}	// FG
