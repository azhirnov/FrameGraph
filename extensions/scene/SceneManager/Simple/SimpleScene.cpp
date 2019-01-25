// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/SceneManager/Simple/SimpleScene.h"
#include "scene/SceneManager/IImageCache.h"
#include "scene/Renderer/ScenePreRender.h"
#include "scene/Renderer/RenderQueue.h"
#include "scene/Renderer/IRenderTechnique.h"
#include "scene/Loader/Intermediate/IntermediateMesh.h"
#include "framegraph/Shared/EnumUtils.h"

namespace FG
{
namespace {
	static constexpr uint		max_instance_count = 1 << 11;

	static const StringView		shader_source = R"#(
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

	vec3 RotateVec (const vec4 qleft, const vec3 vright)
	{
		vec3	uv, uuv, qvec = qleft.xyz;
		uv	= cross( qvec, vright );
		uuv	= cross( qvec, uv );
		uv	*= 2.0f * qleft.w;
		uuv	*= 2.0f;
		return vright + uv + uuv;
	}

	vec3 GetWorldPosition ()
	{
		vec3	pos   = perInstance.transforms[instanceID].position;
		float	scale = perInstance.transforms[instanceID].scale;
		vec4	quat  = perInstance.transforms[instanceID].orientation;
		return RotateVec( quat, at_Position ) * scale + pos;
	}

	vec2 GetTextureCoordinate0 ()  { return at_TextureUV; }
#endif	// VERTEX_SHADER


#ifdef FRAGMENT_SHADER
	/*layout(set=0, binding=1, std140) uniform MaterialsUB {
		vec4	someData;
	} mtr;*/

# ifdef ALBEDO_MAP
	layout(set=0, binding=2) uniform sampler2D  un_AlbedoTex;
	vec4  SampleAlbedo (const vec2 texcoord)  { return texture( un_AlbedoTex, texcoord ); }
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
	bool SimpleScene::Create (const FGThreadPtr &fg, const IntermediateScenePtr &scene, const ImageCachePtr &imageCache, const Transform &initialTransform)
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
		const float	inv_range	= 1.0f / camera.visibilityRange[1];
		const int	first_layer	= BitScanForward( camera.layers.to_ulong() );
		const int	last_layer	= BitScanReverse( camera.layers.to_ulong() );

		for (auto& inst : _instances)
		{
			// frustum culling
			if ( not camera.frustum.IsVisible( inst.boundingBox ) )
				continue;

			// calc detail level
			auto	sphere	= inst.boundingBox.ToOuterSphere();
			float	dist	= Max( 0.0f, distance( camera.camera.transform.position, sphere.center ) - sphere.radius ) * inv_range;
			auto	detail	= EDetailLevel(Max( uint(camera.detailRange.min), uint(mix( float(camera.detailRange.min), float(camera.detailRange.max), dist ) + 0.5f) ));

			if ( detail > camera.detailRange.max )
				continue;	// detail level is too small

			auto&	model_layers = _modelLODs[ inst.index ][ uint(detail) ];
			
			for (int layer_idx = first_layer; layer_idx <= last_layer; ++layer_idx)
			{
				if ( not camera.layers[layer_idx] )
					continue;

				auto&	models_range = model_layers[ layer_idx ];

				for (uint i = 0; i < models_range.y; ++i)
				{
					auto&	model	= _models[ models_range.x + i ];
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

					queue.AddRenderObj( ERenderLayer(layer_idx), draw_task );
				}
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
	bool SimpleScene::_ConvertMeshes (const FGThreadPtr &fg, const IntermediateScenePtr &scene)
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
	bool SimpleScene::_ConvertMaterials (const FGThreadPtr &fg, const IntermediateScenePtr &scene, const ImageCachePtr &imageCache)
	{
		using MtrTexture = IntermediateMaterial::MtrTexture;

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
	bool SimpleScene::_CreateMesh (const Transform &transform, const IntermediateScenePtr &scene, const IntermediateScene::MeshNode &node)
	{
		Instance		inst;
		inst.index			= uint(_modelLODs.size());
		inst.transform		= transform;
		inst.boundingBox	= Default;	// TODO
		_instances.push_back( inst );
		
		LayerBits		layers			= node.material->GetRenderLayers();
		DetailLevels_t	model_lod;
		RenderLayers_t	model_layers;
		const uint		mesh_index		= uint(scene->GetIndexOfMesh( node.mesh ));
		const uint		material_index	= uint(scene->GetIndexOfMaterial( node.material ));

		for (size_t i = 0; i < model_layers.size(); ++i)
		{
			if ( not layers[i] )
			{
				model_layers[i] = {0, 0};
				continue;
			}
			
			Model	model;
			model.meshID	 = mesh_index;
			model.materialID = material_index;
			model_layers[i]  = {uint32_t(_models.size()), 1};
			model.layer		 = ERenderLayer(i);
			_models.push_back( std::move(model) );
		}
		
		for (auto& lod : model_lod) {
			lod = model_layers;
		}

		_modelLODs.push_back( model_lod );
		return true;
	}

/*
=================================================
	_ConvertHierarchy
=================================================
*/
	bool SimpleScene::_ConvertHierarchy (const IntermediateScenePtr &scene, const Transform &initialTransform)
	{
		_instances.reserve( 128 );

		Array<Pair< IntermediateScene::SceneNode const*, Transform >>	stack;
		stack.emplace_back( &scene->GetRoot(), initialTransform );

		for (; not stack.empty();)
		{
			const auto	node		= stack.back();
			const auto	transform	= node.second + node.first->localTransform;

			for (auto& data : node.first->data)
			{
				CHECK_ERR( Visit( data,
								  [&] (const IntermediateScene::MeshNode &m)	{ return _CreateMesh( transform, scene, m ); },
								  [] (const std::monostate &)					{ return false; }
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
