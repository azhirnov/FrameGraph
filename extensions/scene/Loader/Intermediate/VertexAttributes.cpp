// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/Loader/Intermediate/VertexAttributes.h"

namespace FG
{

/*
=================================================
	GetName
=================================================
*/
	VertexAttributeName::Name_t  VertexAttributeName::GetName (const VertexID &id)
	{
#		if FG_OPTIMIZE_IDS
		{
			using N = VertexAttributeName;
			using V = EVertexAttribute;

			static HashMap< VertexID, Name_t >	name_map = {
				{ V::Position,		N::Position },
				{ V::Position_1,	N::Position_1 },
				{ V::Normal,		N::Normal },
				{ V::BiNormal,		N::BiNormal },
				{ V::Tangent,		N::Tangent },
				{ V::ObjectID,		N::ObjectID },
				{ V::MaterialID,	N::MaterialID },
				{ V::BoneWeights,	N::BoneWeights },
				{ V::LightmapUV,	N::LightmapUV },
				{ V::TextureUVs[0],	N::TextureUVs[0] },
				{ V::TextureUVs[1],	N::TextureUVs[1] },
				{ V::TextureUVs[2],	N::TextureUVs[2] },
				{ V::TextureUVs[3],	N::TextureUVs[3] }
			};

			CHECK( name_map.size() == 13 );
			STATIC_ASSERT( VertexID::IsOptimized() );

			auto	iter = name_map.find( id );
			CHECK_ERR( iter != name_map.end() );

			return iter->second;
		}
#		else
		{
			return id.GetName();
		}
#		endif
	}
	
	
/*
=================================================
	constructor
=================================================
*/
	VertexAttributes::VertexAttributes (const VertexInputState &vertexInput, const VertexBufferID &bufferId)
	{
		uint	binding_idx = 0;
		{
			auto	iter = vertexInput.BufferBindings().find( bufferId );
			if ( iter != vertexInput.BufferBindings().end() )
				binding_idx = iter->second.index;
		}

		_vertInput.Bind( Default, 0_b );

		for (auto& vert : vertexInput.Vertices())
		{
			if ( vert.second.bufferBinding == binding_idx )
			{
				_vertInput.Add( vert.first, vert.second.type, BytesU{vert.second.offset} );
			}
		}

		ASSERT( not _vertInput.Vertices().empty() );
	}

/*
=================================================
	CalcHash
=================================================
*/
	HashVal  VertexAttributes::CalcHash () const
	{
		HashVal	result;

		for (auto& attr : _vertInput.Vertices()) {
			result << (HashOf( attr.first ) + HashOf( attr.second.type ) + HashOf( attr.second.offset ));
		}
		return result;
	}
	
/*
=================================================
	operator ==
=================================================
*/
	bool  VertexAttributes::operator == (const VertexAttributes &rhs) const
	{
		auto&	lattr = this->GetVertexInput().Vertices();
		auto&	rattr = rhs.GetVertexInput().Vertices();

		if ( lattr.size() != rattr.size() )
			return false;

		for (auto& at : lattr)
		{
			auto	iter = rattr.find( at.first );
			if ( iter == rattr.end() )
				return false;

			if ( at.second.offset != iter->second.offset or
					at.second.type   != iter->second.type   )
				return false;
		}
		return true;
	}


}	// FG
