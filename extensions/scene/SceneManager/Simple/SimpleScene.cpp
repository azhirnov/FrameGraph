// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/SceneManager/Simple/SimpleScene.h"
#include "scene/SceneManager/IImageCache.h"
#include "scene/Renderer/ScenePreRender.h"
#include "scene/Renderer/RenderQueue.h"
#include "scene/Renderer/IRenderTechnique.h"
#include "scene/Loader/Intermediate/IntermMesh.h"
#include "framegraph/Shared/EnumUtils.h"

namespace FG
{
namespace {
	static constexpr uint		max_instance_count = 1 << 10;

	static const StringView		shader_source = R"#(

	vec3  RotateVec (const vec4 qleft, const vec3 vright)
	{
		vec3  uv  = cross( qleft.xyz, vright );
		vec3  uuv = cross( qleft.xyz, uv );
		return vright + (uv * 2.0f * qleft.w) + (uuv * 2.0f);
	}

	vec3  Transform (const vec3 point, const vec3 position, const vec4 orientation, const float scale)
	{
		return RotateVec( orientation, point * scale ) + position;
	}

	vec4  ToNonLinear (const vec4 color)
	{
		 return vec4(pow( color.rgb, vec3(2.2f) ), color.a ); 
	}

#ifdef VERTEX_SHADER
	struct ObjectTransform
	{
		vec4	orientation;
		vec3	position;
		float	scale;
	};

	layout(set=0, binding=0, std140) uniform PerInstanceUB {
		ObjectTransform		transforms[1 << 11];	// [max_instance_count]
	} perInstance;

	layout(push_constant, std140) uniform VSPushConst {
		layout(offset=0) int	instanceID;
		layout(offset=4) int	materialID;
	};

	vec3 GetWorldPosition ()
	{
		vec3	pos   = perInstance.transforms[instanceID].position;
		float	scale = perInstance.transforms[instanceID].scale;
		vec4	quat  = perInstance.transforms[instanceID].orientation;
		//return RotateVec( quat, at_Position.xyz ) * scale + pos;
		return Transform( at_Position.xyz, pos, quat, scale );
	}

	vec2 GetTextureCoordinate0 ()  { return at_TextureUV; }
#endif	// VERTEX_SHADER


#ifdef FRAGMENT_SHADER
	/*layout(set=0, binding=1, std140) uniform MaterialsUB {
		vec4	someData;
	} mtr;*/

# ifdef ALBEDO_MAP
	layout(set=0, binding=2) uniform sampler2D  un_AlbedoTex;
	vec4  SampleAlbedoLinear (const vec2 texcoord)    { return texture( un_AlbedoTex, texcoord ); }
	vec4  SampleAlbedoNonlinear (const vec2 texcoord) { return ToNonLinear(texture( un_AlbedoTex, texcoord )); }
# endif

# ifdef SPECULAR_MAP
	layout(set=0, binding=3) uniform sampler2D  un_SpecularTex;
	float SampleSpecular (const vec2 texcoord)  { return texture( un_SpecularTex, texcoord ).r; }
# endif

# ifdef ROUGHTNESS_MAP
	layout(set=0, binding=4) uniform sampler2D  un_RoughtnessTex;
	float SampleRoughtness (const vec2 texcoord)  { return texture( un_RoughtnessTex, texcoord ).r; }
# endif

# ifdef METALLIC_MAP
	layout(set=0, binding=5) uniform sampler2D  un_MetallicTex;
	float SampleMetallic (const vec2 texcoord)  { return texture( un_MetallicTex, texcoord ).r; }
# endif
#endif	// FRAGMENT_SHADER
)#";
}

/*
=================================================
	constructor
=================================================
*/
	SimpleScene::SimpleScene ()
	{
	}
	
/*
=================================================
	Create
=================================================
*/
	bool SimpleScene::Create (const FGThreadPtr &fg, const IntermScenePtr &scene, const ImageCachePtr &imageCache, const Transform &initialTransform)
	{
		CHECK_ERR( scene );
		CHECK_ERR( _ConvertMeshes( fg, scene ));
		CHECK_ERR( _ConvertMaterials( fg, scene, imageCache ));
		CHECK_ERR( _ConvertHierarchy( scene, initialTransform ));
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void SimpleScene::Destroy (const FGThreadPtr &fg)
	{
		fg->ReleaseResource( _vertexBuffer );
		fg->ReleaseResource( _indexBuffer );
		fg->ReleaseResource( _perInstanceUB );
		fg->ReleaseResource( _materialsUB );
	}

/*
=================================================
	Build
=================================================
*/
	bool SimpleScene::Build (const FGThreadPtr &fg, const RenderTechniquePtr &renTech)
	{
		CHECK_ERR( _UpdatePerObjectUniforms( fg ));
		CHECK_ERR( _BuildModels( fg, renTech ));
		return true;
	}
	
/*
=================================================
	PreDraw
=================================================
*/
	void SimpleScene::PreDraw (const CameraInfo &, ScenePreRender &preRender) const
	{
		preRender.AddScene( shared_from_this() );
	}

/*
=================================================
	Draw
=================================================
*/
	void SimpleScene::Draw (RenderQueue &queue) const
	{
		const auto&	camera		= queue.GetCamera();
		const auto&	camera_pos	= camera.camera.transform.position;
		const float	inv_range	= 1.0f / camera.visibilityRange[1];
		const int	first_layer	= BitScanForward( camera.layers.to_ulong() );
		const int	last_layer	= BitScanReverse( camera.layers.to_ulong() );

		for (auto& inst : _instances)
		{
			AABB	bbox = inst.boundingBox;
			bbox.Move( camera_pos );

			// frustum culling
			if ( not camera.frustum.IsVisible( bbox ) )
				continue;

			// calc detail level
			auto	sphere	= bbox.ToOuterSphere();
			float	dist	= Max( 0.0f, length( sphere.center ) - sphere.radius ) * inv_range;
			auto	detail	= EDetailLevel(Max( uint(camera.detailRange.min), uint(mix( float(camera.detailRange.min), float(camera.detailRange.max), dist ) + 0.5f) ));

			if ( detail > camera.detailRange.max )
				continue;	// detail level is too small

			for (uint i = inst.index; i < inst.lastIndex; ++i)
			{
				auto&	lod			= _modelLODs[i];
				uint	model_idx	= lod.levels[uint(detail)];

				if ( model_idx == UMax					or
					 not camera.layers[uint(lod.layer)] )
					continue;

				auto&	model	= _models[ model_idx ];
				auto&	mesh	= _meshes[ model.meshID ];
				//auto&	mtr		= _materials[ model.materialID ];

				DrawIndexed		draw_task;
				draw_task.pipeline = model.pipeline;
				draw_task.AddBuffer( Default, _vertexBuffer )
							.SetIndexBuffer( _indexBuffer, 0_b, _indexType )
							.SetVertexInput( _vertexAttribs[mesh.attribsIndex].first )
							.SetTopology( mesh.topology ).SetCullMode( mesh.cullMode )
							.Draw( mesh.indexCount, 1, mesh.firstIndex, mesh.vertexOffset )
							.AddResources( DescriptorSetID{"PerObject"}, &model.resources )
							.AddPushConstant( PushConstantID{"VSPushConst"}, uint2{model.materialID, inst.index} );

				queue.Draw( lod.layer, draw_task );
			}
		}
	}
	
/*
=================================================
	_UpdatePerObjectUniforms
=================================================
*/
	bool SimpleScene::_UpdatePerObjectUniforms (const FGThreadPtr &fg)
	{
		CHECK_ERR( _instances.size() <= max_instance_count );

		const BytesU	size = (SizeOf<mat4x4>) * max_instance_count;

		if ( not _perInstanceUB )
		{
			_perInstanceUB = fg->CreateBuffer( BufferDesc{ size, EBufferUsage::Uniform }, MemoryDesc{ EMemoryType::HostWrite }, "PerInstanceUB" );
			CHECK_ERR( _perInstanceUB );
		}
		
		BytesU	mapped_size = size;
		void*	mapped_ptr	= null;
		CHECK_ERR( fg->MapBufferRange( _perInstanceUB, 0_b, INOUT mapped_size, OUT mapped_ptr ));

		for (auto& inst : _instances)
		{
			memcpy( mapped_ptr + SizeOf<Transform> * inst.index, &inst.transform, sizeof(inst.transform) );
		}

		return true;
	}
	
/*
=================================================
	_BuildModels
=================================================
*/
	bool SimpleScene::_BuildModels (const FGThreadPtr &fg, const RenderTechniquePtr &renTech)
	{
		ShaderBuilder*	builder = renTech->GetShaderBuilder();

		for (auto& attr : _vertexAttribs) {
			attr.second = builder->CacheVertexAttribs( attr.first );
		}

		SamplerID	sampler = fg->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::Repeat )
															  .SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear ));
		CHECK_ERR( sampler );

		for (auto& model : _models)
		{
			auto&	mesh = _meshes[ model.meshID ];
			auto&	mtr  = _materials[ model.materialID ];

			CHECK_ERR( mesh.attribsIndex != UMax );

			IRenderTechnique::PipelineInfo	info;
			info.textures			= mtr.textureBits;
			info.layer				= model.layer;
			info.detailLevel		= EDetailLevel::High;
			info.sourceIDs.push_back( _vertexAttribs[mesh.attribsIndex].second );
			info.sourceIDs.push_back( builder->CacheShaderSource( shader_source ));

			CHECK_ERR( renTech->GetPipeline( info, OUT model.pipeline ));
			CHECK_ERR( fg->InitPipelineResources( model.pipeline, DescriptorSetID{"PerObject"}, OUT model.resources ));

			if ( mtr.albedoTex )
				model.resources.BindTexture( UniformID{"un_AlbedoTex"}, mtr.albedoTex, sampler );

			if ( mtr.specularTex )
				model.resources.BindTexture( UniformID{"un_SpecularTex"}, mtr.specularTex, sampler );

			if ( mtr.roughtnessTex )
				model.resources.BindTexture( UniformID{"un_RoughtnessTex"}, mtr.roughtnessTex, sampler );

			if ( mtr.metallicTex )
				model.resources.BindTexture( UniformID{"un_MetallicTex"}, mtr.metallicTex, sampler );

			model.resources.BindBuffer( UniformID{"PerInstanceUB"}, _perInstanceUB );
			//model.resources.BindBuffer( UniformID{"MaterialsUB"}, _materialsUB );
		}

		FG_UNUSED( sampler.Release() );
		return true;
	}

/*
=================================================
	_ConvertMeshes
=================================================
*/
	bool SimpleScene::_ConvertMeshes (const FGThreadPtr &fg, const IntermScenePtr &scene)
	{
		HashMap< VertexAttributesPtr, size_t >	attribs;

		BytesU	vert_stride, idx_stride;
		for (auto& src : scene->GetMeshes())
		{
			vert_stride	= Max( vert_stride, src->GetVertexStride() );
			idx_stride	= Max( idx_stride,  EIndex_SizeOf(src->GetIndexType()) );
		}

		BytesU	vert_size, idx_size;
		for (auto& src : scene->GetMeshes())
		{
			vert_size = AlignToLarger( vert_size + ArraySizeOf(src->GetVertices()), vert_stride );
			idx_size  = AlignToLarger( idx_size  + ArraySizeOf(src->GetIndices()),  idx_stride  );

			src->CalcAABB();
			attribs.insert({ src->GetAttribs(), attribs.size() });
		}

		_vertexBuffer	= fg->CreateBuffer( BufferDesc{ vert_size, EBufferUsage::Vertex | EBufferUsage::TransferDst });
		_indexBuffer	= fg->CreateBuffer( BufferDesc{ idx_size,  EBufferUsage::Index  | EBufferUsage::TransferDst });
		_indexType		= EIndex::UInt;

		vert_size = idx_size = 0_b;
		_meshes.resize( scene->GetMeshes().size() );

		Task	last_task;
		for (size_t i = 0; i < scene->GetMeshes().size(); ++i)
		{
			auto&	src = scene->GetMeshes()[i];
			Mesh&	dst = _meshes[i];

			dst.boundingBox		= src->GetAABB().value();
			dst.attribsIndex	= uint(attribs.find( src->GetAttribs() )->second);
			dst.topology		= src->GetTopology();
			dst.vertexOffset	= uint(vert_size / vert_stride);
			dst.firstIndex		= uint(idx_size / idx_stride);
			dst.indexCount		= uint(ArraySizeOf(src->GetIndices()) / idx_stride);

			last_task = fg->AddTask( UpdateBuffer{}.SetBuffer( _vertexBuffer ).AddData( src->GetVertices(), vert_size ).DependsOn( last_task ));
			last_task = fg->AddTask( UpdateBuffer{}.SetBuffer( _indexBuffer ).AddData( src->GetIndices(), idx_size ).DependsOn( last_task ));
			
			vert_size = AlignToLarger( vert_size + ArraySizeOf(src->GetVertices()), vert_stride );
			idx_size  = AlignToLarger( idx_size  + ArraySizeOf(src->GetIndices()),  idx_stride  );

			if ( i )
				_boundingBox.Add( dst.boundingBox );
			else
				_boundingBox = dst.boundingBox;

			CHECK( src->GetIndexType() == _indexType );
		}

		_vertexAttribs.reserve( attribs.size() );
		for (auto& src : attribs)
		{
			auto&	dst = _vertexAttribs.emplace_back().first;
			dst.Bind( Default, vert_stride );

			for (auto& vert : src.first->GetVertexBinds())
			{
				dst.Add( vert.first, vert.second.type, BytesU{vert.second.offset} );
			}
		}

		return true;
	}
	
/*
=================================================
	_ConvertMaterials
=================================================
*/
	bool SimpleScene::_ConvertMaterials (const FGThreadPtr &fg, const IntermScenePtr &scene, const ImageCachePtr &imageCache)
	{
		using MtrTexture = IntermMaterial::MtrTexture;

		_materials.reserve( scene->GetMaterials().size() );

		for (auto& src : scene->GetMaterials())
		{
			CHECK_ERR( src->GetRenderLayers() != Default );

			auto&		settings = src->GetSettings();
			Material	dst;

			dst.dataID = uint(_materials.size());

			if ( auto* tex = std::get_if<MtrTexture>( &settings.albedo ) )
			{
				CHECK_ERR( imageCache->CreateImage( fg, tex->image, OUT dst.albedoTex ));
				dst.textureBits |= ETextureType::Albedo;
			}

			if ( auto* tex = std::get_if<MtrTexture>( &settings.specular ) )
			{
				CHECK_ERR( imageCache->CreateImage( fg, tex->image, OUT dst.specularTex ));
				dst.textureBits |= ETextureType::Specular;
			}

			if ( auto* tex = std::get_if<MtrTexture>( &settings.roughtness ) )
			{
				CHECK_ERR( imageCache->CreateImage( fg, tex->image, OUT dst.roughtnessTex ));
				dst.textureBits |= ETextureType::Roughtness;
			}

			if ( auto* tex = std::get_if<MtrTexture>( &settings.metallic ) )
			{
				CHECK_ERR( imageCache->CreateImage( fg, tex->image, OUT dst.metallicTex ));
				dst.textureBits |= ETextureType::Metallic;
			}

			_materials.push_back( dst );
		}

		return true;
	}
	
/*
=================================================
	_CreateMesh
=================================================
*/
	bool SimpleScene::_CreateMesh (const Transform &transform, const IntermScenePtr &scene, const IntermScene::ModelData &modelData)
	{
		Instance		inst;
		inst.index		= uint(_modelLODs.size());
		inst.transform	= transform;
		
		// find all layers
		LayerBits		layers;
		for (auto& level : modelData.levels) 
		{
			if ( level.first and level.second )
				layers |= level.second->GetRenderLayers();
		}

		Optional<AABB>	bbox;

		for (size_t i = 0; i < layers.size(); ++i)
		{
			ModelLevel	lod;
			lod.layer	= ERenderLayer(i);

			for (size_t j = 0; j < lod.levels.size(); ++j)
			{
				if ( j >= modelData.levels.size()		or
					 not modelData.levels[j].first		or
					 not modelData.levels[j].second		or
					 not modelData.levels[j].second->GetRenderLayers()[i] )
				{
					lod.levels[j] = UMax;
					continue;
				}

				lod.levels[j] = uint(_models.size());

				Model		model;
				model.meshID	 = uint(scene->GetIndexOfMesh( modelData.levels[j].first ));
				model.materialID = uint(scene->GetIndexOfMaterial( modelData.levels[j].second ));
				model.layer		 = ERenderLayer(i);
				_models.push_back( model );
				
				if ( bbox.has_value() )
					bbox->Add( _meshes[model.meshID].boundingBox );
				else
					bbox = _meshes[model.meshID].boundingBox;
			}

			_modelLODs.push_back( lod );
		}
		
		inst.boundingBox = bbox.value_or( AABB{} ).Transform( inst.transform );
		inst.lastIndex	 = uint(_modelLODs.size());
		ASSERT( inst.lastIndex > inst.index );
		
		_instances.push_back( inst );
		return true;
	}

/*
=================================================
	_ConvertHierarchy
=================================================
*/
	bool SimpleScene::_ConvertHierarchy (const IntermScenePtr &scene, const Transform &initialTransform)
	{
		_instances.reserve( 128 );

		Array<Pair< IntermScene::SceneNode const*, Transform >>	stack;
		stack.emplace_back( &scene->GetRoot(), initialTransform );

		for (; not stack.empty();)
		{
			const auto	node		= stack.back();
			const auto	transform	= node.second + node.first->localTransform;

			for (auto& data : node.first->data)
			{
				CHECK_ERR( Visit( data,
								  [&] (const IntermScene::ModelData &m)	{ return _CreateMesh( transform, scene, m ); },
								  [] (const std::monostate &)			{ return false; }
							));
			}
			
			stack.pop_back();
			for (auto& n : node.first->nodes) {
				stack.emplace_back( &n, transform );
			}
		}

		return true;
	}


}	// FG
