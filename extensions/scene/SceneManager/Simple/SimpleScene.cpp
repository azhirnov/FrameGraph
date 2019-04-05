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
	bool SimpleScene::Create (const CommandBuffer &cmdbuf, const IntermScenePtr &scene, const ImageCachePtr &imageCache, const Transform &initialTransform)
	{
		CHECK_ERR( cmdbuf and scene and imageCache );

		Destroy( cmdbuf->GetFrameGraph() );

		CHECK_ERR( _ConvertMeshes( cmdbuf, scene ));
		CHECK_ERR( _ConvertMaterials( cmdbuf, scene, imageCache ));
		CHECK_ERR( _ConvertHierarchy( scene, initialTransform ));
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void SimpleScene::Destroy (const FrameGraph &fg)
	{
		CHECK_ERR( fg, void());

		_instances.clear();
		_modelLODs.clear();
		_models.clear();
		_meshes.clear();
		_materials.clear();
		_vertexAttribs.clear();

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
	bool SimpleScene::Build (const CommandBuffer &, const RenderTechniquePtr &renTech)
	{
		CHECK_ERR( renTech );

		auto	fg = renTech->GetFrameGraph();

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
		bool		debug		= false;

		for (size_t inst_idx = 0; inst_idx < _instances.size(); ++inst_idx)
		{
			auto&	inst = _instances[ inst_idx ];
			
			/*AABB	bbox = inst.boundingBox;
			bbox.Move( camera_pos );

			// frustum culling
			if ( not camera.frustum.IsVisible( bbox ) )
				continue;

			// calc detail level
			auto	sphere	= bbox.ToOuterSphere();
			float	dist	= Max( 0.0f, length( sphere.center ) - sphere.radius ) * inv_range;
			auto	detail	= EDetailLevel(Max( uint(camera.detailRange.min), uint(mix( float(camera.detailRange.min), float(camera.detailRange.max), dist ) + 0.5f) ));

			if ( detail > camera.detailRange.max )
				continue;	// detail level is too small*/

			auto	detail	= EDetailLevel::High;

			for (uint i = inst.index; i < inst.lastIndex; ++i)
			{
				auto&	lod			= _modelLODs[i];
				uint	model_idx	= lod.levels[uint(detail)];

				if ( model_idx == UMax					or
					 not camera.layers[uint(lod.layer)] or
					 not _models[ model_idx ].pipeline )
					continue;

				auto&	model	= _models[ model_idx ];
				auto&	mesh	= _meshes[ model.meshID ];
				//auto&	mtr		= _materials[ model.materialID ];

				DrawIndexed		draw_task;
				draw_task.pipeline		= model.pipeline;
				draw_task.vertexInput	= _vertexAttribs[mesh.attribsIndex]->GetVertexInput();
				draw_task.vertexInput.Bind( Default, _vertexStride, 0 );

				draw_task.AddBuffer( Default, _vertexBuffer )
						 .SetIndexBuffer( _indexBuffer, 0_b, _indexType )
						 .SetTopology( mesh.topology ).SetCullMode( mesh.cullMode )
						 .Draw( mesh.indexCount, 1, mesh.firstIndex, mesh.vertexOffset )
						 .AddResources( DescriptorSetID{"PerObject"}, &model.resources )
						 .AddPushConstant( PushConstantID{"VSPushConst"}, uint2{model.materialID, uint(inst_idx)} );

				if ( debug )
					draw_task.EnableDebugTrace( EShaderStages::Vertex );

				queue.Draw( lod.layer, draw_task );
			}
		}
	}
	
/*
=================================================
	_UpdatePerObjectUniforms
=================================================
*/
	bool SimpleScene::_UpdatePerObjectUniforms (const FrameGraph &fg)
	{
		CHECK_ERR( _instances.size() <= max_instance_count );

		const BytesU	size = SizeOf<Transform> * max_instance_count;

		if ( _perInstanceUB and size > fg->GetDescription( _perInstanceUB ).size )
		{
			fg->ReleaseResource( INOUT _perInstanceUB );
		}

		if ( not _perInstanceUB )
		{
			_perInstanceUB = fg->CreateBuffer( BufferDesc{ size, EBufferUsage::Uniform }, MemoryDesc{ EMemoryType::HostWrite }, "PerInstanceUB" );
			CHECK_ERR( _perInstanceUB );
		}
		
		BytesU	mapped_size = size;
		void*	mapped_ptr	= null;
		CHECK_ERR( fg->MapBufferRange( _perInstanceUB, 0_b, INOUT mapped_size, OUT mapped_ptr ));

		for (size_t i = 0; i < _instances.size(); ++i)
		{
			auto&	tr = _instances[i].transform;
			memcpy( mapped_ptr + SizeOf<Transform> * i, &tr, sizeof(tr) );
		}

		return true;
	}
	
/*
=================================================
	_BuildModels
=================================================
*/
	bool SimpleScene::_BuildModels (const FrameGraph &fg, const RenderTechniquePtr &renTech)
	{
		auto		source_id = renTech->GetShaderBuilder()->CacheFileSource( "Scene/simple_scene.glsl" );

		SamplerID	sampler = fg->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::Repeat )
															  .SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear ));
		CHECK_ERR( sampler );

		for (auto& model : _models)
		{
			auto&	mesh = _meshes[ model.meshID ];
			auto&	mtr  = _materials[ model.materialID ];

			CHECK_ERR( mesh.attribsIndex != UMax );

			ShaderCache::GraphicsPipelineInfo	info;
			info.attribs		= _vertexAttribs[mesh.attribsIndex];
			info.textures		= mtr.textureBits;
			info.detailLevel	= EDetailLevel::High;
			info.sourceIDs.push_back( source_id );
			info.constants.emplace_back( "MAX_INSTANCE_COUNT", max_instance_count );

			if ( not renTech->GetPipeline( model.layer, info, OUT model.pipeline ) )
				continue;

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
	bool SimpleScene::_ConvertMeshes (const CommandBuffer &cmdbuf, const IntermScenePtr &scene)
	{
		HashMap< VertexAttributesPtr, size_t >	attribs;

		FrameGraph	fg = cmdbuf->GetFrameGraph();

		BytesU	vert_stride, idx_stride;
		for (auto& src : scene->GetMeshes())
		{
			vert_stride	= Max( vert_stride, src.first->GetVertexStride() );
			idx_stride	= Max( idx_stride,  EIndex_SizeOf(src.first->GetIndexType()) );
		}

		BytesU	vert_size, idx_size;
		for (auto& src : scene->GetMeshes())
		{
			vert_size = AlignToLarger( vert_size + ArraySizeOf(src.first->GetVertices()), vert_stride );
			idx_size  = AlignToLarger( idx_size  + ArraySizeOf(src.first->GetIndices()),  idx_stride  );

			src.first->CalcAABB();
			attribs.insert({ src.first->GetAttribs(), attribs.size() });
		}

		_vertexBuffer	= fg->CreateBuffer( BufferDesc{ vert_size, EBufferUsage::Vertex | EBufferUsage::TransferDst });
		_indexBuffer	= fg->CreateBuffer( BufferDesc{ idx_size,  EBufferUsage::Index  | EBufferUsage::TransferDst });
		_indexType		= EIndex::UInt;

		vert_size = idx_size = 0_b;
		_meshes.resize( scene->GetMeshes().size() );

		Task	last_task;
		for (auto& src : scene->GetMeshes())
		{
			Mesh&	dst = _meshes[ src.second ];

			dst.boundingBox		= src.first->GetAABB().value();
			dst.attribsIndex	= uint(attribs.find( src.first->GetAttribs() )->second);
			dst.topology		= src.first->GetTopology();
			dst.vertexOffset	= uint(vert_size / vert_stride);
			dst.firstIndex		= uint(idx_size / idx_stride);
			dst.indexCount		= uint(ArraySizeOf(src.first->GetIndices()) / idx_stride);

			last_task = cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _vertexBuffer ).AddData( src.first->GetVertices(), vert_size ).DependsOn( last_task ));
			last_task = cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _indexBuffer ).AddData( src.first->GetIndices(), idx_size ).DependsOn( last_task ));
			
			vert_size = AlignToLarger( vert_size + ArraySizeOf(src.first->GetVertices()), vert_stride );
			idx_size  = AlignToLarger( idx_size  + ArraySizeOf(src.first->GetIndices()),  idx_stride  );

			if ( src.first == scene->GetMeshes().begin()->first )
				_boundingBox = dst.boundingBox;
			else
				_boundingBox.Add( dst.boundingBox );

			CHECK( src.first->GetIndexType() == _indexType );
		}

		_vertexStride = vert_stride;
		_vertexAttribs.resize( attribs.size() );

		for (auto& src : attribs) {
			_vertexAttribs[src.second] = src.first;
		}

		return true;
	}
	
/*
=================================================
	_ConvertMaterials
=================================================
*/
	bool SimpleScene::_ConvertMaterials (const CommandBuffer &cmdbuf, const IntermScenePtr &scene, const ImageCachePtr &imageCache)
	{
		using MtrTexture = IntermMaterial::MtrTexture;

		_materials.resize( scene->GetMaterials().size() );

		for (auto& src : scene->GetMaterials())
		{
			CHECK_ERR( src.first->GetRenderLayers() != Default );

			auto&		settings = src.first->GetSettings();
			Material&	dst		 = _materials[ src.second ];

			dst.dataID = uint(_materials.size());

			if ( auto* tex = UnionGetIf<MtrTexture>( &settings.albedo ) )
			{
				CHECK_ERR( imageCache->CreateImage( cmdbuf, tex->image, true, OUT dst.albedoTex ));
				dst.textureBits |= ETextureType::Albedo;
			}

			if ( auto* tex = UnionGetIf<MtrTexture>( &settings.specular ) )
			{
				CHECK_ERR( imageCache->CreateImage( cmdbuf, tex->image, true, OUT dst.specularTex ));
				dst.textureBits |= ETextureType::Specular;
			}

			if ( auto* tex = UnionGetIf<MtrTexture>( &settings.roughtness ) )
			{
				CHECK_ERR( imageCache->CreateImage( cmdbuf, tex->image, true, OUT dst.roughtnessTex ));
				dst.textureBits |= ETextureType::Roughtness;
			}

			if ( auto* tex = UnionGetIf<MtrTexture>( &settings.metallic ) )
			{
				CHECK_ERR( imageCache->CreateImage( cmdbuf, tex->image, true, OUT dst.metallicTex ));
				dst.textureBits |= ETextureType::Metallic;
			}
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
				model.meshID	 = scene->GetIndexOfMesh( modelData.levels[j].first );
				model.materialID = scene->GetIndexOfMaterial( modelData.levels[j].second );
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


}	// FG
