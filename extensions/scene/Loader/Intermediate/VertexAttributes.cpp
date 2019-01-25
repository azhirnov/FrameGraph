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


}	// FG
