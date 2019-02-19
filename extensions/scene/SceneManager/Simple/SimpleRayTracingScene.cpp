// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/SceneManager/Simple/SimpleRayTracingScene.h"
#include "scene/SceneManager/IImageCache.h"
#include "scene/Renderer/ScenePreRender.h"
#include "scene/Renderer/RenderQueue.h"
#include "scene/Renderer/IRenderTechnique.h"
#include "scene/Loader/Intermediate/IntermMesh.h"
#include "framegraph/Shared/EnumUtils.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	SimpleRayTracingScene::SimpleRayTracingScene ()
	{
	}
	
/*
=================================================
	Create
=================================================
*/
	bool SimpleRayTracingScene::Create (const FGThreadPtr &fg, const IntermScenePtr &scene, const ImageCachePtr &imageCache, const Transform &initialTransform)
	{
		CHECK_ERR( fg and scene and imageCache );

		Destroy( fg );
		CHECK_ERR( _CreateMaterials( fg, scene, imageCache ));
		CHECK_ERR( _CreateGeometry( fg, scene, initialTransform ));
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void SimpleRayTracingScene::Destroy (const FGThreadPtr &fg)
	{
		fg->ReleaseResource( _rtScene );
		fg->ReleaseResource( _shaderTable );
		fg->ReleaseResource( _attribsBuffer );
		fg->ReleaseResource( _primitivesBuffer );
		fg->ReleaseResource( _materialsBuffer );
		fg->ReleaseResource( _resources );

		_albedoMaps.clear();
		_normalMaps.clear();
		_specularMaps.clear();
	}
	
/*
=================================================
	Build
=================================================
*/
	bool SimpleRayTracingScene::Build (const FGThreadPtr &fg, const RenderTechniquePtr &renTech)
	{
		CHECK_ERR( _UpdateShaderTable( fg, renTech ));
		return true;
	}
	
/*
=================================================
	PreDraw
=================================================
*/
	void SimpleRayTracingScene::PreDraw (const CameraInfo &, ScenePreRender &preRender) const
	{
		preRender.AddScene( shared_from_this() );
	}
	
/*
=================================================
	Draw
=================================================
*/
	void SimpleRayTracingScene::Draw (RenderQueue &queue) const
	{
		const auto&		camera	= queue.GetCamera();
		const uint2		size	= uint2{ uint(camera.viewportSize.x + 0.5f), uint(camera.viewportSize.y + 0.5f) };

		TraceRays	trace_rays;
		trace_rays.AddResources( DescriptorSetID{"PerObject"}, &_resources );
		trace_rays.SetGroupCount( size.x, size.y );
		trace_rays.SetShaderTable( _shaderTable );

		queue.Compute( ERenderLayer::RayTracing, trace_rays );
	}
	
/*
=================================================
	_CreateMaterials
=================================================
*/
	bool SimpleRayTracingScene::_CreateMaterials (const FGThreadPtr &fg, const IntermScenePtr &scene, const ImageCachePtr &imageCache)
	{
		using Texture = IntermMaterial::MtrTexture;

		HashMap< IntermImagePtr, uint >		albedo_maps;
		HashMap< IntermImagePtr, uint >		specular_maps;
		HashMap< IntermImagePtr, uint >		normal_maps;
		Array< Material >					dst_materials;	dst_materials.resize( scene->GetMaterials().size() );

		for (auto& src : scene->GetMaterials())
		{
			auto&	dst		 = dst_materials[ src.second ];
			auto&	settings = src.first->GetSettings();
			
			if ( auto* albedo_map = UnionGetIf<Texture>( &settings.albedo ))
				dst.albedoMap = albedo_maps.insert({ albedo_map->image, uint(albedo_maps.size()) }).first->second;

			if ( auto* albedo_Color = UnionGetIf<RGBA32f>( &settings.albedo ))
				dst.albedoColor = {albedo_Color->r, albedo_Color->g, albedo_Color->b};
			
			if ( auto* specular_map = UnionGetIf<Texture>( &settings.specular ))
				dst.specularMap = albedo_maps.insert({ specular_map->image, uint(specular_maps.size()) }).first->second;
			
			if ( auto* specular_color = UnionGetIf<RGBA32f>( &settings.specular ))
				dst.specularColor = {specular_color->r, specular_color->g, specular_color->b};

			if ( auto* normal_map = UnionGetIf<Texture>( &settings.normalsMap ))
				dst.normalMap = normal_maps.insert({ normal_map->image, uint(normal_maps.size()) }).first->second;

			if ( auto* roughtness = UnionGetIf<float>( &settings.roughtness ))
				dst.roughtness = *roughtness;

			if ( auto* metallic = UnionGetIf<float>( &settings.metallic ))
				dst.metallic = *metallic;
		}

		_materialsBuffer = fg->CreateBuffer( BufferDesc{ ArraySizeOf(dst_materials), EBufferUsage::Storage | EBufferUsage::TransferDst }, Default, "Materials" );
		CHECK_ERR( _materialsBuffer );

		Task	t_upload = fg->AddTask( UpdateBuffer{}.SetBuffer( _materialsBuffer ).AddData( dst_materials ));
		FG_UNUSED( t_upload );

		_albedoMaps.resize( albedo_maps.size() );
		_normalMaps.resize( normal_maps.size() );

		for (auto& map : albedo_maps) {
			CHECK_ERR( imageCache->CreateImage( fg, map.first, true, OUT _albedoMaps[map.second] ));
		}
		for (auto& map : normal_maps) {
			CHECK_ERR( imageCache->CreateImage( fg, map.first, true, OUT _normalMaps[map.second] ));
		}

		return true;
	}

/*
=================================================
	_CreateGeometry
=================================================
*/
	bool SimpleRayTracingScene::_CreateGeometry (const FGThreadPtr &fg, const IntermScenePtr &scene, const Transform &initialTransform)
	{
		MeshData	mesh_data;
		mesh_data.vertices.reserve( 1 << 10 );
		mesh_data.indices.reserve( 1 << 10 );
		mesh_data.primitives.reserve( 1 << 10 );
		mesh_data.attribs.reserve( 1 << 10 );

		CHECK_ERR( _ConvertHierarchy( scene, initialTransform, OUT mesh_data ));
		
		_primitivesBuffer = fg->CreateBuffer( BufferDesc{ ArraySizeOf(mesh_data.primitives), EBufferUsage::Storage | EBufferUsage::TransferDst }, Default, "Primitives" );
		CHECK_ERR( _primitivesBuffer );

		_attribsBuffer = fg->CreateBuffer( BufferDesc{ ArraySizeOf(mesh_data.attribs), EBufferUsage::Storage | EBufferUsage::TransferDst }, Default, "Attribs" );
		CHECK_ERR( _attribsBuffer );

		Task	t_upload1 = fg->AddTask( UpdateBuffer{}.SetBuffer( _primitivesBuffer ).AddData( mesh_data.primitives ));
		Task	t_upload2 = fg->AddTask( UpdateBuffer{}.SetBuffer( _attribsBuffer ).AddData( mesh_data.attribs ));
		FG_UNUSED( t_upload1, t_upload2 );
		
		RayTracingGeometryDesc::Triangles	mesh_info;
		mesh_info.SetID( GeometryID{"Scene"} )
				 .SetVertices< decltype(mesh_data.vertices[0]) >( mesh_data.vertices.size() )
				 .SetIndices< decltype(mesh_data.indices[0]) >( mesh_data.indices.size() )
				 .AddFlags( ERayTracingGeometryFlags::Opaque );

		RTGeometryID	geom = fg->CreateRayTracingGeometry( RayTracingGeometryDesc{{ mesh_info }} );
		CHECK_ERR( geom );

		BuildRayTracingGeometry::Triangles	mesh;
		mesh.SetID( GeometryID{"Scene"} );
		mesh.SetVertexArray( mesh_data.vertices );
		mesh.SetIndexArray( mesh_data.indices );

		Task	t_build_blas = fg->AddTask( BuildRayTracingGeometry{}.SetTarget( geom ).Add( mesh ));


		_rtScene = fg->CreateRayTracingScene( RayTracingSceneDesc{ 1 });
		CHECK_ERR( _rtScene );

		BuildRayTracingScene::Instance	instance;
		instance.SetID( InstanceID{"Static"} ).SetGeometry( geom );

		Task	t_build_tlas = fg->AddTask( BuildRayTracingScene{}.SetTarget( _rtScene ).Add( instance )
												.SetHitShadersPerInstance( 2 ).DependsOn( t_build_blas ));
		
		fg->ReleaseResource( geom );
		return true;
	}
	
/*
=================================================
	_ConvertHierarchy
=================================================
*/
	bool SimpleRayTracingScene::_ConvertHierarchy (const IntermScenePtr &scene, const Transform &initialTransform, OUT MeshData &outMeshData) const
	{
		Array<Pair< IntermScene::SceneNode const*, Transform >>	stack;
		stack.emplace_back( &scene->GetRoot(), initialTransform );

		for (; not stack.empty();)
		{
			const auto	node		= stack.back();
			const auto	transform	= node.second + node.first->localTransform;

			for (auto& data : node.first->data)
			{
				CHECK_ERR( Visit( data,
								  [&] (const IntermScene::ModelData &m)	{ return _CreateMesh( scene, m, transform, INOUT outMeshData ); },
								  [] (const NullUnion &)				{ return false; }
							));
			}
			
			stack.pop_back();
			for (auto& n : node.first->nodes) {
				stack.emplace_back( &n, transform );
			}
		}

		return true;
	}
	
/*
=================================================
	_CreateMesh
=================================================
*/
	bool SimpleRayTracingScene::_CreateMesh (const IntermScenePtr &scene, const IntermScene::ModelData &model,
											 const Transform &transform, INOUT MeshData &meshData) const
	{
		if ( model.levels.empty() )
			return true;

		IntermMeshPtr		mesh		= model.levels.front().first;
		IntermMaterialPtr	material	= model.levels.front().second;
		const uint			mat_id		= scene->GetIndexOfMaterial( material );
		const uint			idx_offset	= uint(meshData.vertices.size());
		StructView<vec3>	positions	= mesh->GetData<vec3>( EVertexAttribute::Position );
		StructView<vec3>	normals		= mesh->GetData<vec3>( EVertexAttribute::Normal );
		StructView<vec2>	texcoords0	= mesh->GetData<vec2>( EVertexAttribute::TextureUVs[0] );
		StructView<vec2>	texcoords1	= mesh->GetData<vec2>( EVertexAttribute::TextureUVs[1] );

		for (size_t i = 0; i < positions.size(); ++i)
		{
			meshData.vertices.push_back( transform.ToGlobalPosition( positions[i] ));

			VertexAttrib	attr;

			if ( i < normals.size() )		attr.normal		= normalize( transform.ToGlobalVector( normals[i] ));
			if ( i < texcoords0.size() )	attr.texcoord0	= texcoords0[i];
			if ( i < texcoords1.size() )	attr.texcoord1	= texcoords1[i];

			meshData.attribs.push_back( attr );
		}

		switch ( mesh->GetIndexType() )
		{
			case EIndex::UShort :
			{
				size_t	count	= mesh->GetIndices().size() / sizeof(uint16_t);
				auto*	ptr		= Cast<uint16_t>( mesh->GetIndices().data() );

				for (size_t i = 0; i < count; ++i)
				{
					meshData.indices.push_back( idx_offset + ptr[i] );

					if ( i % 3 == 0 )
						meshData.primitives.push_back(Primitive{ uvec3{ptr[i], ptr[i+1], ptr[i+2]} + idx_offset, mat_id });
				}
				break;
			}
			case EIndex::UInt :
			{
				size_t	count	= mesh->GetIndices().size() / sizeof(uint);
				auto*	ptr		= Cast<uint>( mesh->GetIndices().data() );

				for (size_t i = 0; i < count; ++i)
				{
					meshData.indices.push_back( idx_offset + ptr[i] );
					
					if ( i % 3 == 0 )
						meshData.primitives.push_back(Primitive{ uvec3{ptr[i], ptr[i+1], ptr[i+2]} + idx_offset, mat_id });
				}
				break;
			}
			default : RETURN_ERR( "not supported" );
		}

		return true;
	}
	
/*
=================================================
	_UpdateShaderTable
=================================================
*/
	bool SimpleRayTracingScene::_UpdateShaderTable (const FGThreadPtr &fg, const RenderTechniquePtr &renTech)
	{
		if ( not _shaderTable )
		{
			_shaderTable = fg->CreateRayTracingShaderTable();
			CHECK_ERR( _shaderTable );
		}

		UpdateRayTracingShaderTable  update_st;
		update_st.SetScene( _rtScene );
		update_st.SetTarget( _shaderTable );
		update_st.SetMaxRecursionDepth( 1 );
		update_st.SetRayGenShader( RTShaderID{"Main"} );
		update_st.AddMissShader( RTShaderID{"PrimaryMiss"}, 0 );
		update_st.AddMissShader( RTShaderID{"ShadowMiss"}, 1 );
		update_st.AddHitShader( InstanceID{"Static"}, GeometryID{"Scene"}, 0, RTShaderID{"PrimaryHit"} );
		update_st.AddHitShader( InstanceID{"Static"}, GeometryID{"Scene"}, 1, RTShaderID{"ShadowHit"} );

		ShaderCache::RayTracingPipelineInfo	info;
		info.textures		= ETextureType::Albedo;
		info.detailLevel	= EDetailLevel::High;
		info.vertexStride	= SizeOf<VertexAttrib>;

		// setup vertex attribs
		{
			VertexInputState	state;
			state.Bind( Default, 0_b );
			state.Add( EVertexAttribute::Normal, &VertexAttrib::normal );
			state.Add( EVertexAttribute::TextureUVs[0], &VertexAttrib::texcoord0 );
			state.Add( EVertexAttribute::TextureUVs[1], &VertexAttrib::texcoord1 );

			info.attribs = MakeShared<VertexAttributes>( state );
		}

		// setup shaders
		{
			info.shaders.emplace_back( RTShaderID{"Main"}, EShader::RayGen );
			info.shaders.emplace_back( RTShaderID{"PrimaryMiss"}, EShader::RayMiss );
			info.shaders.emplace_back( RTShaderID{"ShadowMiss"}, EShader::RayMiss );
			info.shaders.emplace_back( RTShaderID{"PrimaryHit"}, EShader::RayClosestHit );
			info.shaders.emplace_back( RTShaderID{"ShadowHit"}, EShader::RayClosestHit );
		}
		
		auto	source_id = renTech->GetShaderBuilder()->CacheFileSource( "Scene/simple_raytracing.glsl" );
		info.sourceIDs.push_back( source_id );

		CHECK_ERR( renTech->GetPipeline( ERenderLayer::RayTracing, info, OUT update_st.pipeline ));

		FG_UNUSED( fg->AddTask( update_st ));
		

		// set pipeline resources
		{
			CHECK_ERR( fg->InitPipelineResources( update_st.pipeline, DescriptorSetID{"PerObject"}, OUT _resources ));

			SamplerID	sampler = fg->CreateSampler( SamplerDesc{}.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )
																	.SetAddressMode( EAddressMode::Repeat ).SetAnisotropy( 16.0f ));
			CHECK_ERR( sampler );

			_resources.BindBuffer( UniformID{"PrimitivesSSB"}, _primitivesBuffer );
			_resources.BindBuffer( UniformID{"VertexAttribsSSB"}, _attribsBuffer );
			_resources.BindBuffer( UniformID{"MaterialsSSB"}, _materialsBuffer );
			_resources.BindRayTracingScene( UniformID{"un_RtScene"}, _rtScene );
			_resources.BindTextures( UniformID{"un_AlbedoMaps"}, _albedoMaps, sampler );
			//_resources.BindTextures( UniformID{"un_NormalMaps"}, _normalMaps, sampler );

			CHECK_ERR( fg->CachePipelineResources( INOUT _resources ));

			fg->ReleaseResource( sampler );
		}
		return true;
	}


}	// FG
