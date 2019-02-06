// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/VertexAttributes.h"
#include "scene/Utils/Math/AABB.h"

namespace FG
{

	//
	// Intermediate Mesh
	//

	class IntermMesh final : public std::enable_shared_from_this<IntermMesh>
	{
	// variables
	private:
		Array<uint8_t>			_vertices;
		VertexAttributesPtr		_attribs;
		BytesU					_vertexStride;
		EPrimitive				_topology		= Default;

		Array<uint8_t>			_indices;
		EIndex					_indexType		= Default;

		Optional<AABB>			_boundingBox;


	// methods
	public:
		IntermMesh () {}
		IntermMesh (Array<uint8_t> &&vertices, const VertexAttributesPtr &attribs,
						  BytesU vertStride, EPrimitive topology,
						  Array<uint8_t> &&indices, EIndex indexType);

		void CalcAABB ();

		ND_ ArrayView<uint8_t>		GetVertices ()		const	{ return _vertices; }
		ND_ ArrayView<uint8_t>		GetIndices ()		const	{ return _indices; }

		ND_ VertexAttributesPtr		GetAttribs ()		const	{ return _attribs; }
		ND_ size_t					GetVertexCount ()	const	{ return size_t(ArraySizeOf(_vertices) / _vertexStride); }
		ND_ BytesU					GetVertexStride ()	const	{ return _vertexStride; }
		ND_ EPrimitive				GetTopology ()		const	{ return _topology; }
		ND_ EIndex					GetIndexType ()		const	{ return _indexType; }

		ND_ Optional<AABB> const&	GetAABB ()			const	{ return _boundingBox; }
		
		template <typename T>
		ND_ StructView<T>			GetData (const VertexID &id) const;
	};
	
	using IntermMeshPtr = SharedPtr< IntermMesh >;


	template <typename T>
	inline StructView<T>  IntermMesh::GetData (const VertexID &id) const
	{
		ASSERT( _attribs );
		ASSERT( _vertexStride != 0 );
		return _attribs->GetData<T>( id, _vertices.data(), GetVertexCount(), _vertexStride );
	}

}	// FG
