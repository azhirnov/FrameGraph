// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Common.h"
#include "stl/Containers/StructView.h"
#include "framegraph/Public/VertexInputState.h"

namespace FG
{

	struct VertexAttributeName
	{
		using Name_t = StaticString<32>;

		static constexpr Name_t		Position	{"at_Position"};
		static constexpr Name_t		Position_1	{"at_Position_1"};
		
		static constexpr Name_t		Normal		{"at_Normal"};
		static constexpr Name_t		BiNormal	{"at_BiNormal"};
		static constexpr Name_t		Tangent		{"at_Tangent"};
		
		static constexpr Name_t		ObjectID	{"at_ObjectID"};
		static constexpr Name_t		MaterialID	{"at_MaterialID"};

		static constexpr Name_t		BoneWeights	{"at_BoneWeights"};

		static constexpr Name_t		LightmapUV	{"at_LightmapUV"};
		static constexpr Name_t		TextureUVs[] = { "at_TextureUV", "at_TextureUV_1", "at_TextureUV_2", "at_TextureUV_3" };

		static Name_t  GetName (const VertexID &);
	};


	struct EVertexAttribute
	{
		static constexpr VertexID	Position	{VertexAttributeName::Position};
		static constexpr VertexID	Position_1	{VertexAttributeName::Position_1};	// for morphing animation
		
		static constexpr VertexID	Normal		{VertexAttributeName::Normal};
		static constexpr VertexID	BiNormal	{VertexAttributeName::BiNormal};
		static constexpr VertexID	Tangent		{VertexAttributeName::Tangent};
		
		static constexpr VertexID	ObjectID	{VertexAttributeName::ObjectID};
		static constexpr VertexID	MaterialID	{VertexAttributeName::MaterialID};

		static constexpr VertexID	BoneWeights	{VertexAttributeName::BoneWeights};	// for sceletal animation

		static constexpr VertexID	LightmapUV	{VertexAttributeName::LightmapUV};
		static constexpr VertexID	TextureUVs[] = { VertexID{VertexAttributeName::TextureUVs[0]},
													 VertexID{VertexAttributeName::TextureUVs[1]},
													 VertexID{VertexAttributeName::TextureUVs[2]},
													 VertexID{VertexAttributeName::TextureUVs[3]} };
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
		VertexInputState	_vertInput;


	// methods
	public:
		explicit VertexAttributes (const VertexInputState &vertexInput, const VertexBufferID &bufferId = Default);

		template <typename T>
		ND_ StructView<T>			GetData (const VertexID &id, const void* vertexData, size_t vertexCount, BytesU stride) const;

		ND_ VertexInputState const&	GetVertexInput ()	const	{ return _vertInput; }

		ND_ HashVal	CalcHash () const;

		ND_ bool	operator == (const VertexAttributes &rhs) const;
	};
	
	
/*
=================================================
	GetData
=================================================
*/
	template <typename T>
	inline StructView<T>  VertexAttributes::GetData (const VertexID &id, const void* vertexData, size_t vertexCount, BytesU stride) const
	{
		auto	iter = _vertInput.Vertices().find( id );
		if ( iter == _vertInput.Vertices().end() )
			return Default;

		ASSERT( iter->second.type == VertexDesc<T>::attrib );

		return StructView<T>{ vertexData + iter->second.offset, vertexCount, uint(stride) };
	}


}	// FG
