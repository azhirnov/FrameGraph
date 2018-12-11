// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/VertexAttributes.h"

namespace FG
{

	//
	// Intermediate Mesh
	//

	class IntermediateMesh final : public std::enable_shared_from_this<IntermediateMesh>
	{
	// variables
	private:
		Array<uint8_t>			_vertices;
		VertexAttributesPtr		_attribs;

		Array<uint8_t>			_indices;
		EIndex					_indexType	= EIndex::UInt;


	// methods
	public:
		IntermediateMesh () {}
		
		IntermediateMesh (Array<uint8_t> &&vertices, const VertexAttributesPtr &attribs,
						  Array<uint8_t> &&indices, EIndex indexType);
	};
	
	using IntermediateMeshPtr = SharedPtr< IntermediateMesh >;

	
	
/*
=================================================
	constructor
=================================================
*/
	inline IntermediateMesh::IntermediateMesh (Array<uint8_t> &&vertices, const VertexAttributesPtr &attribs,
											   Array<uint8_t> &&indices, EIndex indexType) :
		_vertices{ std::move(vertices) }, _attribs{ attribs },
		_indices{ std::move(indices) }, _indexType{ indexType }
	{}

}	// FG
