// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SphericalCube.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{
namespace {

	static constexpr bool	UseQuads = true;

	ND_ inline uint  CalcVertCount (uint lod)
	{
		return (lod+2) * (lod+2) * 6;
	}
	
	ND_ inline uint  CalcIndexCount (uint lod)
	{
		return 6*(UseQuads ? 4 : 6) * (lod+1) * (lod+1);
	}
}
//-----------------------------------------------------------------------------


/*
=================================================
	destructor
=================================================
*/
	SphericalCube::~SphericalCube ()
	{
		CHECK( not _vertexBuffer );
		CHECK( not _indexBuffer );
	}

/*
=================================================
	Create
----
	lod = 0 -- cube, only 8 vertices, 6*6 indices
	lod = x -- has {(x+1)*(x+2)*4 + x*x*2} vertices, (6*6*(x+1)^2) indices
=================================================
*/
	bool  SphericalCube::Create (const CommandBuffer &cmdbuf, uint minLod, uint maxLod)
	{
		CHECK_ERR( minLod <= maxLod );
		CHECK_ERR( maxLod < 32 );
		CHECK_ERR( cmdbuf );

		FrameGraph	fg	= cmdbuf->GetFrameGraph();
		Task		last_task;

		Destroy( fg );
		_minLod = minLod;	_maxLod = maxLod;

		// calculate total memory size
		uint64_t	vert_count	= 0;
		uint64_t	index_count	= 0;

		for (uint lod = minLod; lod <= maxLod; ++lod)
		{
			vert_count  += CalcVertCount( lod );
			index_count += CalcIndexCount( lod );
		}

		// create resources
		_vertexBuffer	= fg->CreateBuffer( BufferDesc{ SizeOf<Vertex> * vert_count, EBufferUsage::Vertex | EBufferUsage::Transfer | EBufferUsage::Storage },
											Default, "SphericalCube.Vertices" );
		_indexBuffer	= fg->CreateBuffer( BufferDesc{ SizeOf<uint> * index_count, EBufferUsage::Index | EBufferUsage::Transfer },
											Default, "SphericalCube.Indices" );
		CHECK_ERR( _vertexBuffer and _indexBuffer );
		
		vert_count = index_count = 0;
		BytesU	vert_offset;
		BytesU	index_offset;


		for (uint lod = minLod; lod <= maxLod; ++lod)
		{
			const BytesU	vert_size	= SizeOf<Vertex> * CalcVertCount( lod );
			const BytesU	idx_size	= SizeOf<uint> * CalcIndexCount( lod );
			RawBufferID		staging_vb,	staging_ib;
			BytesU			vb_offset,	ib_offset;
			Vertex *		vb_mapped	= null;
			uint *			ib_mapped	= null;

			CHECK_ERR( cmdbuf->AllocBuffer( vert_size, SizeOf<Vertex>, OUT staging_vb, OUT vb_offset, OUT vb_mapped ));
			CHECK_ERR( cmdbuf->AllocBuffer( idx_size, SizeOf<uint>, OUT staging_ib, OUT ib_offset, OUT ib_mapped ));
			
			last_task = cmdbuf->AddTask( CopyBuffer{}.From( staging_vb ).To( _vertexBuffer ).AddRegion( vb_offset, vert_offset, vert_size ).DependsOn( last_task ));
			last_task = cmdbuf->AddTask( CopyBuffer{}.From( staging_ib ).To( _indexBuffer ).AddRegion( ib_offset, index_offset, idx_size ).DependsOn( last_task ));

			vert_offset  += vert_size;
			index_offset += idx_size;

			// for cube sides
			const uint	vcount	= lod + 2;
			const uint	icount	= lod + 1;
			uint		vert_i	= 0;
			uint		index_i	= 0;

			// for top/bottom faces
			for (uint face = 0; face < 6; ++face)
			{
				//FG_LOGI( "Lod: "s << ToString(lod) << ", Face: " << ToString(face) );
				
				// generate indices
				for (uint y = 0; y < icount; ++y)
				for (uint x = 0; x < icount; ++x)
				{
					const uint	indices[4] = { vert_i + (x+0) + (y+0)*vcount, vert_i + (x+1) + (y+0)*vcount,
											   vert_i + (x+0) + (y+1)*vcount, vert_i + (x+1) + (y+1)*vcount };
					
					if constexpr( UseQuads )
					{
						ib_mapped[index_i++] = indices[0];	ib_mapped[index_i++] = indices[1];
						ib_mapped[index_i++] = indices[3];	ib_mapped[index_i++] = indices[2];
					}
					else
					{
						if ( (x < icount/2 and y < icount/2) or (x >= icount/2 and y >= icount/2) )
						{
							ib_mapped[index_i++] = indices[0];	ib_mapped[index_i++] = indices[1];	ib_mapped[index_i++] = indices[3];
							ib_mapped[index_i++] = indices[0];	ib_mapped[index_i++] = indices[3];	ib_mapped[index_i++] = indices[2];
						}
						else
						if ( (x >= icount/2 and y < icount/2) or (x < icount/2 and y >= icount/2) )
						{
							ib_mapped[index_i++] = indices[0];	ib_mapped[index_i++] = indices[1];	ib_mapped[index_i++] = indices[2];
							ib_mapped[index_i++] = indices[2];	ib_mapped[index_i++] = indices[1];	ib_mapped[index_i++] = indices[3];
						}
					}

					//FG_LOGI( "Indices: "s << ToString(ib_mapped[index_i-6]) << ", " << ToString(ib_mapped[index_i-5]) << ", "
					//				<< ToString(ib_mapped[index_i-4]) << ", " << ToString(ib_mapped[index_i-3]) << ", "
					//				<< ToString(ib_mapped[index_i-2]) << ", " << ToString(ib_mapped[index_i-1]) );
				}

				// generate vertices
				for (uint y = 0; y < vcount; ++y)
				for (uint x = 0; x < vcount; ++x)
				{
					double2		ncoord	= double2{ double(x)/(vcount-1), double(y)/(vcount-1) } * 2.0 - 1.0;
					
					vb_mapped[vert_i++] = Vertex{ float3(ForwardProjection( ncoord, ECubeFace(face) )),
												  float3(ForwardTexProjection( ncoord, ECubeFace(face) )) };
					
					//FG_LOGI( "Vertex["s << ToString(vert_i-1) << "] = " << ToString(vb_mapped[vert_i-1].position)
					//			<< "; TC: " << ToString(vb_mapped[vert_i-1].texcoord) );
				}
			}

			//FG_LOGI( "--------------------------------\n\n" );
			
			ASSERT( SizeOf<Vertex> * vert_i == vert_size );
			ASSERT( SizeOf<uint> * index_i == idx_size );
		}

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  SphericalCube::Destroy (const FrameGraph &fg)
	{
		fg->ReleaseResource( _vertexBuffer );
		fg->ReleaseResource( _indexBuffer );
	}
	
/*
=================================================
	GetAttribs
=================================================
*/
	VertexInputState  SphericalCube::GetAttribs ()
	{
		VertexInputState	vert_input;
		vert_input.Bind( Default, SizeOf<Vertex> );
		vert_input.Add( VertexID{"at_Position"}, &Vertex::position );
		vert_input.Add( VertexID{"at_TextureUV"}, &Vertex::texcoord );
		return vert_input;
	}
	
/*
=================================================
	Draw
=================================================
*/
	DrawIndexed  SphericalCube::Draw (uint lod) const
	{
		CHECK( lod >= _minLod and lod <= _maxLod );
		lod = Min( lod, _maxLod );

		BytesU	vb_offset, ib_offset;

		for (uint i = _minLod; i < lod; ++i)
		{
			vb_offset += SizeOf<Vertex> * CalcVertCount( i );
			ib_offset += SizeOf<uint> * CalcIndexCount( i );
		}

		DrawIndexed		task;
		task.SetVertexInput( GetAttribs() );
		task.AddBuffer( Default, _vertexBuffer, vb_offset );
		task.SetIndexBuffer( _indexBuffer, ib_offset, EIndex::UInt );
		task.Draw( CalcIndexCount( lod ) );
		task.SetTopology( EPrimitive::TriangleList );
		task.SetFrontFaceCCW( false );

		return task;
	}
	
/*
=================================================
	RayCast
----
	from http://paulbourke.net/geometry/circlesphere/index.html#linesphere
=================================================
*/
	bool  SphericalCube::RayCast (const float3 &center, float radius, const float3 &begin, const float3 &end, OUT float3 &outIntersection) const
	{
		float	a = Square(end.x - begin.x) + Square(end.y - begin.y) + Square(end.z - begin.z);

		float	b = 2.0f * ( (end.x - begin.x)*(begin.x - center.x) + (end.y - begin.y)*(begin.y - center.y) + (end.z - begin.z)*(begin.z - center.z) );

		float	c = Square(center.x) + Square(center.y) + Square(center.z) + Square(begin.x) + Square(begin.y) + Square(begin.z) -
					2.0f * ( center.x*begin.x + center.y*begin.y + center.z*begin.z ) - Square(radius);

		float	i = b * b - 4 * a * c;
		
		// no intersection
		if ( i < 0.0f )
			return false;
		
		// one intersection
		if ( i == 0.0f )
		{
			float	mu	= -b / (2.0f * a);
			
			if ( (mu < 0.0001f) & (mu > 1.0f) )
				return false;

			outIntersection = begin + mu * (end - begin);
			return true;
		}
		
		// two intersections
		float	mu1			= (-b + Sqrt( Square(b) - 4.0f * a * c )) / (2.0f * a);
		bool	mu1_valid	= (mu1 > -0.0001f) & (mu1 < 1.0001f);
		float	mu2			= (-b - Sqrt( Square(b) - 4.0f * a * c )) / (2.0f * a);
		bool	mu2_valid	= (mu2 > -0.0001f) & (mu2 < 1.0001f);
		float	mu;
		
		if ( mu1_valid & mu2_valid )
			mu = Min( mu1, mu2 );
		else
		if ( mu1_valid )
			mu = mu1;
		else
		if ( mu2_valid )
			mu = mu2;
		else
			return false;

		outIntersection = begin + mu * (end - begin);
		return true;
	}
	
/*
=================================================
	GetVertexBuffer
=================================================
*/
	bool  SphericalCube::GetVertexBuffer (uint lod, uint face, OUT RawBufferID &id, OUT BytesU &offset, OUT BytesU &size, OUT uint2 &vertCount) const
	{
		CHECK_ERR( lod >= _minLod and lod <= _maxLod );
		CHECK_ERR( face < 6 );
		
		offset = 0_b;
		for (uint i = _minLod; i < lod; ++i) {
			offset += SizeOf<Vertex> * CalcVertCount( i );
		}

		vertCount = uint2{ lod+2 };

		BytesU	face_size = SizeOf<Vertex> * CalcVertCount( lod );

		size	= face_size / 6;
		offset += (face_size * face) / 6;
		id		= _vertexBuffer;

		return true;
	}


}	// FG
