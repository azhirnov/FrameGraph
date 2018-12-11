// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Common.h"
#include "stl/Containers/StructView.h"
#include "framegraph/Public/VertexInputState.h"

namespace FG
{

	struct EVertexAttribute
	{
		static constexpr VertexID	Position	{"at_Position"};
		static constexpr VertexID	Position_1	{"at_Position_1"};	// for morphing animation
		
		static constexpr VertexID	Normal		{"at_Normal"};
		static constexpr VertexID	BiNormal	{"at_BiNormal"};
		static constexpr VertexID	Tangent		{"at_Tangent"};
		
		static constexpr VertexID	ObjectID	{"at_ObjectID"};
		static constexpr VertexID	MaterialID	{"at_MaterialID"};

		static constexpr VertexID	BoneWeights	{"at_BoneWeights"};	// for sceletal animation

		static constexpr VertexID	LightmapUV	{"at_LightmapUV"};
		static constexpr VertexID	TextureUVs[] = { VertexID{"at_TextureUV"},   VertexID{"at_TextureUV_1"},
													 VertexID{"at_TextureUV_2"}, VertexID{"at_TextureUV_3"} };
	};



	//
	// Vertex Attributes
	//

	class VertexAttributes final : public std::enable_shared_from_this<VertexAttributes>
	{
	// types
	private:
		using Attribs_t = VertexInputState::Vertices_t;


	// variables
	private:
		Attribs_t		_attribs;


	// methods
	public:
		explicit VertexAttributes (const VertexInputState &vertexInput, const VertexBufferID &bufferId = Default);

		template <typename T>
		ND_ StructView<T>  GetData (const VertexID &id, const void* vertexData, size_t vertexCount, BytesU stride) const;
	};

	using VertexAttributesPtr = SharedPtr< VertexAttributes >;
	

	
/*
=================================================
	constructor
=================================================
*/
	inline VertexAttributes::VertexAttributes (const VertexInputState &vertexInput, const VertexBufferID &bufferId)
	{
		uint	binding_idx = 0;
		{
			auto	iter = vertexInput.BufferBindings().find( bufferId );
			if ( iter != vertexInput.BufferBindings().end() )
				binding_idx = iter->second.index;
		}

		for (auto& vert : vertexInput.Vertices())
		{
			if ( vert.second.bindingIndex == binding_idx )
			{
				_attribs.insert({ vert.first, vert.second });
			}
		}

		ASSERT( not _attribs.empty() );
	}
	
/*
=================================================
	GetData
=================================================
*/
	template <typename T>
	inline StructView<T>  VertexAttributes::GetData (const VertexID &id, const void* vertexData, size_t vertexCount, BytesU stride) const
	{
		auto	iter = _attribs.find( id );
		if ( iter == _attribs.end() )
			return Default;

		ASSERT( iter->second.type == VertexDesc<T>::attrib );

		return StructView<T>{ vertexData + iter->second.offset, vertexCount, uint(stride) };
	}


}	// FG
