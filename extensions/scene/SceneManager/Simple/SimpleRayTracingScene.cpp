// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/SceneManager/Simple/SimpleRayTracingScene.h"
#include "scene/SceneManager/IImageCache.h"
#include "scene/Renderer/ScenePreRender.h"
#include "scene/Renderer/RenderQueue.h"
#include "scene/Renderer/IRenderTechnique.h"
#include "scene/Loader/Intermediate/IntermMesh.h"
#include "framegraph/Shared/EnumUtils.h"

namespace FG
{
namespace {
	static constexpr GeometryID		SceneIDs[]		= { GeometryID{"OpaqueScene"}, GeometryID{"TranslucentScene"} };
	static constexpr InstanceID		InstanceIDs[]	= { InstanceID{"OpaqueSceneInstance"}, InstanceID{"TranslucentSceneInstance"} };
}

/*
=================================================
	constructor
=================================================
*/
	SimpleRayTracingScene::SimpleRayTracingScene ()
	{
		STATIC_ASSERT( CountOf(SceneIDs) == CountOf(InstanceIDs) );
		STATIC_ASSERT( CountOf(SceneIDs) == CountOf(decltype(_enabledInstances){}) );
	}
	
/*
=================================================
	destructor
=================================================
*/
	SimpleRayTracingScene::~SimpleRayTracingScene ()
	{
	}

/*
=================================================
	Create
=================================================
*/
	bool SimpleRayTracingScene::Create (const CommandBuffer &cmdbuf, const IntermScenePtr &scene,
										const ImageCachePtr &imageCache, const Transform &initialTransform)
	{
		CHECK_ERR( cmdbuf and scene and imageCache );

		Destroy( cmdbuf->GetFrameGraph() );

		CHECK_ERR( _CreateMaterials( cmdbuf, scene, imageCache ));
		CHECK_ERR( _CreateGeometries( cmdbuf, scene, initialTransform ));
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void SimpleRayTracingScene::Destroy (const FrameGraph &fg)
	{
		CHECK_ERR( fg, void());

		fg->ReleaseResource( _rtScene );
		fg->ReleaseResource( _shaderTable );
		fg->ReleaseResource( _materialsBuffer );
		fg->ReleaseResource( _resources );

		for (auto& buf : _primitivesBuffers) {
			fg->ReleaseResource( buf );
		}
		for (auto& buf : _attribsBuffers) {
			fg->ReleaseResource( buf );
		}

		_primitivesBuffers.clear();
		_attribsBuffers.clear();
		_albedoMaps.clear();
		_normalMaps.clear();
		_specularMaps.clear();

		_enabledInstances = Default;
	}
	
/*
=================================================
	Build
=================================================
*/
	bool SimpleRayTracingScene::Build (const CommandBuffer &cmdbuf, const RenderTechniquePtr &renTech)
	{
		CHECK_ERR( renTech and cmdbuf );

		CHECK_ERR( _UpdateShaderTable( cmdbuf, renTech ));
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
	bool SimpleRayTracingScene::_CreateMaterials (const CommandBuffer &cmdbuf, const IntermScenePtr &scene, const ImageCachePtr &imageCache)
	{
		using Texture = IntermMaterial::MtrTexture;

		uint								albedo_maps_offset = 0;
		HashMap< IntermImagePtr, uint >		albedo_maps;
		HashMap< IntermImagePtr, uint >		specular_maps;
		HashMap< IntermImagePtr, uint >		normal_maps;
		Array< Material >					dst_materials;	dst_materials.resize( scene->GetMaterials().size() );

		// add default (dummy) images
		{
			RawImageID	img;

			if ( imageCache->GetDefaultImage( "white", OUT img ))
			{
				_albedoMaps.push_back( img );
				++albedo_maps_offset;
			}
		}

		for (auto& src : scene->GetMaterials())
		{
			auto&	dst		 = dst_materials[ src.second ];
			auto&	settings = src.first->GetSettings();
			
			if ( auto* albedo_map = UnionGetIf<Texture>( &settings.albedo ))
				dst.albedoMap = albedo_maps.insert({ albedo_map->image, uint(albedo_maps.size()) + albedo_maps_offset }).first->second;

			if ( auto* albedo_Color = UnionGetIf<RGBA32f>( &settings.albedo ))
				dst.albedoColor = {albedo_Color->r, albedo_Color->g, albedo_Color->b};
			
			if ( auto* specular_map = UnionGetIf<Texture>( &settings.specular ))
				dst.specularMap = specular_maps.insert({ specular_map->image, uint(specular_maps.size()) }).first->second;
			
			if ( auto* specular_color = UnionGetIf<RGBA32f>( &settings.specular ))
				dst.specularColor = {specular_color->r, specular_color->g, specular_color->b};

			if ( auto* normal_map = UnionGetIf<Texture>( &settings.normalMap ))
				dst.normalMap = normal_maps.insert({ normal_map->image, uint(normal_maps.size()) }).first->second;

			if ( auto* roughtness = UnionGetIf<float>( &settings.roughtness ))
				dst.roughtness = *roughtness;

			if ( auto* metallic = UnionGetIf<float>( &settings.metallic ))
				dst.metallic = *metallic;

			if ( auto* refraction = UnionGetIf<float>( &settings.refraction ))
				dst.indexOfRefraction = *refraction;

			if ( auto* optical_depth = UnionGetIf<float>( &settings.opticalDepth ))
				dst.opticalDepth = *optical_depth;
		}

		_materialsBuffer = cmdbuf->GetFrameGraph()->CreateBuffer( BufferDesc{ ArraySizeOf(dst_materials), EBufferUsage::Storage | EBufferUsage::TransferDst }, Default, "Materials" );
		CHECK_ERR( _materialsBuffer );

		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _materialsBuffer ).AddData( dst_materials ));

		_albedoMaps.resize( albedo_maps.size() + albedo_maps_offset );
		_normalMaps.resize( normal_maps.size() );
		_specularMaps.reserve( specular_maps.size() );

		for (auto& map : albedo_maps) {
			CHECK_ERR( imageCache->CreateImage( cmdbuf, map.first, true, OUT _albedoMaps[map.second] ));
		}
		for (auto& map : normal_maps) {
			CHECK_ERR( imageCache->CreateImage( cmdbuf, map.first, true, OUT _normalMaps[map.second] ));
		}
		for (auto& map : specular_maps) {
			CHECK_ERR( imageCache->CreateImage( cmdbuf, map.first, true, OUT _specularMaps[map.second] ));
		}

		return true;
	}

/*
=================================================
	_CreateGeometries
=================================================
*/
	bool SimpleRayTracingScene::_CreateGeometries (const CommandBuffer &cmdbuf, const IntermScenePtr &scene, const Transform &initialTransform)
	{
		FrameGraph	fg = cmdbuf->GetFrameGraph();
		MeshData	opaque_data;
		MeshData	translucent_data;
		
		MeshMap_t	mesh_map;
		mesh_map.insert_or_assign( SceneIDs[0], &opaque_data );
		mesh_map.insert_or_assign( SceneIDs[1], &translucent_data );

		_primitivesBuffers.resize( mesh_map.size() );
		_attribsBuffers.resize( mesh_map.size() );

		CHECK_ERR( _ConvertHierarchy( scene, initialTransform, OUT mesh_map ));


		RTGeometryID	opaque;
		RTGeometryID	translucent;

		Task	t_build_opaque		= _CreateGeometry( cmdbuf, 0, opaque_data, OUT opaque );
		Task	t_build_translucent	= _CreateGeometry( cmdbuf, 1, translucent_data, OUT translucent );


		_rtScene = fg->CreateRayTracingScene( RayTracingSceneDesc{ uint(mesh_map.size()) });
		CHECK_ERR( _rtScene );

		BuildRayTracingScene	build_tlas;
		build_tlas.SetTarget( _rtScene );
		build_tlas.SetHitShadersPerInstance( 2 );

		BuildRayTracingScene::Instance	opaque_instance;
		if ( opaque ) {
			opaque_instance.SetID( InstanceIDs[0] ).SetGeometry( opaque )
							.SetInstanceIndex( 0 ).AddFlags( ERayTracingInstanceFlags::TriangleCullDisable );
			build_tlas.Add( opaque_instance );
			_enabledInstances[0] = true;
		}

		BuildRayTracingScene::Instance	translucent_instance;
		if ( translucent ) {
			translucent_instance.SetID( InstanceIDs[1] ).SetGeometry( translucent )
								.SetInstanceIndex( 1 ).AddFlags( ERayTracingInstanceFlags::TriangleCullDisable );
			build_tlas.Add( translucent_instance );
			_enabledInstances[1] = true;
		}

		Task	t_build_tlas = cmdbuf->AddTask( build_tlas.DependsOn( t_build_opaque, t_build_translucent ));
		
		fg->ReleaseResource( opaque );
		fg->ReleaseResource( translucent );
		return true;
	}
	
/*
=================================================
	_CreateGeometry
=================================================
*/
	Task  SimpleRayTracingScene::_CreateGeometry (const CommandBuffer &cmdbuf, uint index, const MeshData &meshData, OUT RTGeometryID &geom)
	{
		FrameGraph	fg = cmdbuf->GetFrameGraph();

		_primitivesBuffers[index]= fg->CreateBuffer( BufferDesc{ Max( 256_b, ArraySizeOf( meshData.primitives )),
													EBufferUsage::Storage | EBufferUsage::TransferDst }, Default, "Primitives" );
		_attribsBuffers[index]	 = fg->CreateBuffer( BufferDesc{ Max( 256_b, ArraySizeOf( meshData.attribs )),
													EBufferUsage::Storage | EBufferUsage::TransferDst }, Default, "Attribs" );
		CHECK_ERR( _primitivesBuffers[index] and _attribsBuffers[index] );


		if ( meshData.primitives.empty() )
			return null;

		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _primitivesBuffers[index] ).AddData( meshData.primitives ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _attribsBuffers[index] ).AddData( meshData.attribs ));

		RayTracingGeometryDesc::Triangles	mesh_info;
		mesh_info.SetID( SceneIDs[index] )
				 .SetVertices< decltype(meshData.vertices[index]) >( meshData.vertices.size() )
				 .SetIndices< decltype(meshData.indices[index]) >( meshData.indices.size() )
				 .AddFlags( ERayTracingGeometryFlags::Opaque );
		
		geom = fg->CreateRayTracingGeometry( RayTracingGeometryDesc{{ mesh_info }} );
		CHECK_ERR( geom );
		

		BuildRayTracingGeometry::Triangles	mesh;
		mesh.SetID( SceneIDs[index] );
		mesh.SetVertexArray( meshData.vertices );
		mesh.SetIndexArray( meshData.indices );

		return cmdbuf->AddTask( BuildRayTracingGeometry{}.SetTarget( geom ).Add( mesh ));
	}

/*
=================================================
	_ConvertHierarchy
=================================================
*/
	bool SimpleRayTracingScene::_ConvertHierarchy (const IntermScenePtr &scene, const Transform &initialTransform, OUT MeshMap_t &outMeshMap) const
	{
		uint	object_id = 0;

		Array<Pair< IntermScene::SceneNode const*, Transform >>	stack;
		stack.emplace_back( &scene->GetRoot(), initialTransform );

		for (; not stack.empty();)
		{
			const auto	node		= stack.back();
			const auto	transform	= node.second + node.first->localTransform;

			for (auto& data : node.first->data)
			{
				CHECK_ERR( Visit( data,
								  [&] (const IntermScene::ModelData &m)	{ return _CreateMesh( scene, m, transform, object_id++, INOUT outMeshMap ); },
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
	_ChooseMaterialType
----
	opaque and translucent materials has different closest-hit shaders
=================================================
*/
	SimpleRayTracingScene::MeshData*  SimpleRayTracingScene::_ChooseMaterialType (const IntermMaterialPtr &material, MeshMap_t &meshMap) const
	{
		if ( auto* optical_depth = UnionGetIf<float>( &material->GetSettings().opticalDepth ); optical_depth and *optical_depth > 0.0f )
			return meshMap[ SceneIDs[1] ];
		else
			return meshMap[ SceneIDs[0] ];
	}

/*
=================================================
	_CreateMesh
=================================================
*/
	bool SimpleRayTracingScene::_CreateMesh (const IntermScenePtr &scene, const IntermScene::ModelData &model,
											 const Transform &transform, uint objectID, INOUT MeshMap_t &meshMap) const
	{
		if ( model.levels.empty() )
			return true;

		IntermMeshPtr		mesh		= model.levels.front().first;
		IntermMaterialPtr	material	= model.levels.front().second;
		MeshData *			mesh_data	= _ChooseMaterialType( material, meshMap );
		const uint			mat_id		= scene->GetIndexOfMaterial( material );
		const uint			idx_offset	= uint(mesh_data->vertices.size());
		StructView<vec3>	positions	= mesh->GetData<vec3>( EVertexAttribute::Position );
		StructView<vec3>	normals		= mesh->GetData<vec3>( EVertexAttribute::Normal );
		StructView<vec2>	texcoords0	= mesh->GetData<vec2>( EVertexAttribute::TextureUVs[0] );
		StructView<vec2>	texcoords1	= mesh->GetData<vec2>( EVertexAttribute::TextureUVs[1] );

		for (size_t i = 0; i < positions.size(); ++i)
		{
			mesh_data->vertices.push_back( transform.ToGlobalPosition( positions[i] ));

			VertexAttrib	attr;

			if ( i < normals.size() )		attr.normal		= normalize( transform.ToGlobalVector( normals[i] ));
			if ( i < texcoords0.size() )	attr.texcoord0	= texcoords0[i];
			if ( i < texcoords1.size() )	attr.texcoord1	= texcoords1[i];

			mesh_data->attribs.push_back( attr );
		}

		switch ( mesh->GetIndexType() )
		{
			case EIndex::UShort :
			{
				size_t	count	= mesh->GetIndices().size() / sizeof(uint16_t);
				auto*	ptr		= Cast<uint16_t>( mesh->GetIndices().data() );

				for (size_t i = 0; i < count; ++i)
				{
					mesh_data->indices.push_back( idx_offset + ptr[i] );

					if ( i % 3 == 0 )
						mesh_data->primitives.push_back(Primitive{ uvec3{ptr[i], ptr[i+1], ptr[i+2]} + idx_offset, mat_id, objectID });
				}
				break;
			}
			case EIndex::UInt :
			{
				size_t	count	= mesh->GetIndices().size() / sizeof(uint);
				auto*	ptr		= Cast<uint>( mesh->GetIndices().data() );

				for (size_t i = 0; i < count; ++i)
				{
					mesh_data->indices.push_back( idx_offset + ptr[i] );
					
					if ( i % 3 == 0 )
						mesh_data->primitives.push_back(Primitive{ uvec3{ptr[i], ptr[i+1], ptr[i+2]} + idx_offset, mat_id, objectID });
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
	bool SimpleRayTracingScene::_UpdateShaderTable (const CommandBuffer &cmdbuf, const RenderTechniquePtr &renTech)
	{
		FrameGraph	fg = cmdbuf->GetFrameGraph();

		if ( not _shaderTable )
		{
			_shaderTable = fg->CreateRayTracingShaderTable();
			CHECK_ERR( _shaderTable );
		}

		UpdateRayTracingShaderTable  update_st;
		update_st.SetScene( _rtScene );
		update_st.SetTarget( _shaderTable );
		update_st.SetMaxRecursionDepth( 16 );
		update_st.SetRayGenShader( RTShaderID{"Main"} );
		update_st.AddMissShader( RTShaderID{"PrimaryMiss"}, 0 );
		update_st.AddMissShader( RTShaderID{"ShadowMiss"}, 1 );

		if ( _enabledInstances[0] ) {
			update_st.AddHitShader( InstanceIDs[0], 0, RTShaderID{"OpaquePrimaryHit"} );
			update_st.AddHitShader( InstanceIDs[0], 1, RTShaderID{"OpaqueShadowHit"} );
		}
		if ( _enabledInstances[1] ) {
			update_st.AddHitShader( InstanceIDs[1], 0, RTShaderID{"TranslucentPrimaryHit"} );
			update_st.AddHitShader( InstanceIDs[1], 1, RTShaderID{"TranslucentPrimaryHit"} );
		}

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
			info.shaders.emplace_back( RTShaderID{"OpaquePrimaryHit"}, EShader::RayClosestHit );
			info.shaders.emplace_back( RTShaderID{"OpaqueShadowHit"}, EShader::RayClosestHit );
			info.shaders.emplace_back( RTShaderID{"TranslucentPrimaryHit"}, EShader::RayClosestHit );
		}
		
		auto	source_id = renTech->GetShaderBuilder()->CacheFileSource( "Scene/simple_raytracing.glsl" );
		info.sourceIDs.push_back( source_id );

		CHECK_ERR( renTech->GetPipeline( ERenderLayer::RayTracing, info, OUT update_st.pipeline ));

		cmdbuf->AddTask( update_st );
		

		// set pipeline resources
		{
			CHECK_ERR( fg->InitPipelineResources( update_st.pipeline, DescriptorSetID{"PerObject"}, OUT _resources ));

			RawSamplerID	sampler = fg->CreateSampler( SamplerDesc{}.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )
																	.SetAddressMode( EAddressMode::Repeat ).SetAnisotropy( 16.0f )).Release();
			CHECK_ERR( sampler );

			_resources.BindBuffers( UniformID{"PrimitivesSSB"}, _primitivesBuffers );
			_resources.BindBuffers( UniformID{"VertexAttribsSSB"}, _attribsBuffers );
			_resources.BindBuffer( UniformID{"MaterialsSSB"}, _materialsBuffer );
			_resources.BindRayTracingScene( UniformID{"un_RtScene"}, _rtScene );
			_resources.BindTextures( UniformID{"un_AlbedoMaps"}, _albedoMaps, sampler );
			//_resources.BindTextures( UniformID{"un_NormalMaps"}, _normalMaps, sampler );
			//_resources.BindTextures( UniformID{"un_SpecularMaps"}, _specularMaps, sampler );

			CHECK_ERR( fg->CachePipelineResources( INOUT _resources ));
		}
		return true;
	}


}	// FG
