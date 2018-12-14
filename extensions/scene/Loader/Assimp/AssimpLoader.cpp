// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef FG_ENABLE_ASSIMP

#include "scene/Loader/Assimp/AssimpLoader.h"
#include "framegraph/Shared/EnumUtils.h"

#include "assimp/Importer.hpp"
#include "assimp/PostProcess.h"
#include "assimp/Scene.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"

namespace FG
{

	struct SceneData
	{
		Array<IntermediateMaterialPtr>	materials;
		Array<IntermediateMeshPtr>		meshes;
		Array<IntermediateLightPtr>		lights;
		IntermediateScene::SceneNode	root;
	};


/*
=================================================
	constructor
=================================================
*/
	AssimpLoader::AssimpLoader () :
		_importerPtr{ new Assimp::Importer{} }
	{
	}

/*
=================================================
	destructor
=================================================
*/
	AssimpLoader::~AssimpLoader ()
	{
	}
	
/*
=================================================
	ConvertMatrix
=================================================
*/
	ND_ static Transform  ConvertMatrix (const aiMatrix4x4 &mat)
	{
		return Transform{ transpose( BitCast<mat4x4>(mat) )};
	}
	
/*
=================================================
	ConvertWrapMode
=================================================
*/
	ND_ static EAddressMode  ConvertWrapMode (aiTextureMapMode mode)
	{
		ENABLE_ENUM_CHECKS();
		switch ( mode )
		{
			case aiTextureMapMode_Wrap :	return EAddressMode::Repeat;
			case aiTextureMapMode_Mirror :	return EAddressMode::MirrorRepeat;
			case aiTextureMapMode_Clamp :	return EAddressMode::ClampToEdge;
			case aiTextureMapMode_Decal :	return EAddressMode::ClampToBorder;	// TODO: add transparent border color
			#ifndef SWIG
			case _aiTextureMapMode_Force32Bit : break;
			#endif
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown wrap mode", EAddressMode::Repeat );
	}
	
/*
=================================================
	ConvertMapping
=================================================
*/
	using ETextureMapping = IntermediateMaterial::ETextureMapping;

	ND_ static ETextureMapping  ConvertMapping (aiTextureMapping mapping)
	{
		ENABLE_ENUM_CHECKS();
		switch ( mapping )
		{
			case aiTextureMapping_UV :			return ETextureMapping::UV;
			case aiTextureMapping_SPHERE :		return ETextureMapping::Sphere;
			case aiTextureMapping_CYLINDER :	return ETextureMapping::Cylinder;
			case aiTextureMapping_BOX :			return ETextureMapping::Box;
			case aiTextureMapping_PLANE :		return ETextureMapping::Plane;
			case aiTextureMapping_OTHER :		break;
			#ifndef SWIG
			case _aiTextureMapping_Force32Bit :	break;
			#endif
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown texture mapping" );
	};

/*
=================================================
	LoadMaterial
=================================================
*/
	static bool LoadMaterial (const aiMaterial *src, OUT IntermediateMaterialPtr &dst)
	{
		IntermediateMaterial::Settings	mtr;

		aiColor4D	value;
		uint		max_size = 1;

		aiString	mtr_name;
		aiGetMaterialString( src, AI_MATKEY_NAME, &mtr_name );
		mtr.name = mtr_name.C_Str();


		if ( aiGetMaterialColor( src, AI_MATKEY_COLOR_DIFFUSE, &value ) == aiReturn_SUCCESS )
			mtr.diffuseColor = BitCast<float4>( value );

		if ( aiGetMaterialColor( src, AI_MATKEY_COLOR_SPECULAR, &value ) == aiReturn_SUCCESS )
			mtr.specularColor = BitCast<float4>( value );

		if ( aiGetMaterialColor( src, AI_MATKEY_COLOR_AMBIENT, &value ) == aiReturn_SUCCESS )
			mtr.ambientColor = BitCast<float4>( value );
	
		if ( aiGetMaterialColor( src, AI_MATKEY_COLOR_EMISSIVE, &value ) == aiReturn_SUCCESS )
			mtr.emissiveColor = BitCast<float4>( value );
	
		aiGetMaterialFloatArray( src, AI_MATKEY_OPACITY, &mtr.opacity, &max_size );
		aiGetMaterialFloatArray( src, AI_MATKEY_SHININESS, &mtr.shininess, &max_size );
		aiGetMaterialFloatArray( src, AI_MATKEY_SHININESS_STRENGTH, &mtr.shininessStrength, &max_size );

		int twosided = 0;
		aiGetMaterialInteger( src, AI_MATKEY_TWOSIDED, &twosided );
	
		StaticArray< Optional<IntermediateMaterial::MtrTexture>*, aiTextureType_UNKNOWN >	textures =
		{
			null, &mtr.diffuseMap, &mtr.specularMap, &mtr.ambientMap, &mtr.emissiveMap, &mtr.heightMap,
			&mtr.normalsMap, &mtr.shininessMap, &mtr.opacityMap, &mtr.displacementMap, &mtr.lightMap, &mtr.reflectionMap
		};

		for (uint i = 0; i < aiTextureType_UNKNOWN; ++i)
		{
			auto tex = textures[i];

			if ( not tex )
				continue;
		
			aiString			tex_name;
			aiTextureMapMode	map_mode[3]	= { aiTextureMapMode_Wrap, aiTextureMapMode_Wrap, aiTextureMapMode_Wrap };
			aiTextureMapping	mapping		= aiTextureMapping_UV;
			uint				uv_index	= 0;

			if ( src->GetTexture( aiTextureType(i), 0, &tex_name, &mapping, &uv_index, null, null, map_mode ) == AI_SUCCESS )
			{
				*tex = IntermediateMaterial::MtrTexture{};
				auto&	mtr_tex = tex->value();

				mtr_tex.name			= tex_name.C_Str();
				mtr_tex.addressModeU	= ConvertWrapMode( map_mode[0] );
				mtr_tex.addressModeV	= ConvertWrapMode( map_mode[1] );
				mtr_tex.addressModeW	= ConvertWrapMode( map_mode[2] );
				mtr_tex.mapping			= ConvertMapping( mapping );
				mtr_tex.filter			= EFilter::Linear;
				mtr_tex.uvIndex			= uv_index;
				mtr_tex.image			= MakeShared<IntermediateImage>( StringView{tex_name.C_Str()} );
			}
		}

		dst = MakeShared<IntermediateMaterial>( std::move(mtr) );
		return true;
	}

/*
=================================================
	CreateVertexAttribs
=================================================
*/
	static bool CreateVertexAttribs (const aiMesh *mesh, OUT BytesU &stride, OUT VertexAttributesPtr &attribs)
	{
		const StaticArray<EVertexType, 5>	float_vert_types = {
			EVertexType::Unknown, EVertexType::Float, EVertexType::Float2, EVertexType::Float3, EVertexType::Float4
		};

		VertexInputState	vertex_input;
		BytesU				offset;

		vertex_input.Bind( Default, 1_b );

		const auto	AddVertex = [&vertex_input, &offset] (const VertexID &id, EVertexType type)
		{
			vertex_input.Add( id, type, offset );
			offset += EVertexType_SizeOf( type );
		};
		
		if ( mesh->HasPositions() )
			AddVertex( EVertexAttribute::Position, EVertexType::Float3 );

		if ( mesh->HasNormals() )
			AddVertex( EVertexAttribute::Normal, EVertexType::Float3 );

		if ( mesh->mTangents and mesh->mNumVertices )
			AddVertex( EVertexAttribute::Tangent, EVertexType::Float3 );
		
		if ( mesh->mBitangents and mesh->mNumVertices )
			AddVertex( EVertexAttribute::BiNormal, EVertexType::Float3 );

		for (uint t = 0; t < CountOf(EVertexAttribute::TextureUVs); ++t)
		{
			if ( mesh->HasTextureCoords(t) )
				AddVertex( EVertexAttribute::TextureUVs[t], float_vert_types[mesh->mNumUVComponents[t]] );
		}

		if ( mesh->HasBones() )
			AddVertex( EVertexAttribute::BoneWeights, EVertexType::Float );

		stride  = offset;
		attribs = MakeShared<VertexAttributes>( vertex_input );
		return true;
	}

/*
=================================================
	LoadMesh
=================================================
*/
	static bool LoadMesh (const aiMesh *src, OUT IntermediateMeshPtr &dst)
	{
		CHECK_ERR( src->mPrimitiveTypes == aiPrimitiveType_TRIANGLE );
		CHECK_ERR( not src->HasBones() );

		BytesU				vert_stride;
		VertexAttributesPtr	attribs;
		CHECK_ERR( CreateVertexAttribs( src, OUT vert_stride, OUT attribs ));

		Array<uint8_t>		vertices;
		Array<uint8_t>		indices;

		const auto			CopyAttrib = [&vertices, attribs, src, vert_stride] (const auto &data, const VertexID &id, uint vertIdx)
										{
											using	T		= std::remove_const_t< std::remove_reference_t< decltype(data) >>;
											auto	at_data	= attribs->GetData<T>( id, vertices.data(), src->mNumVertices, vert_stride );
											MemCopy( const_cast<T &>(at_data[vertIdx]), data );
										};

		vertices.resize( src->mNumVertices * size_t(vert_stride) );
		indices.resize( src->mNumFaces * 3 * sizeof(uint) );

		for (uint i = 0; i < src->mNumFaces; ++i)
		{
			const aiFace *	face = &src->mFaces[i];
			ASSERT( face->mNumIndices == 3 );

			for (uint j = 0; j < 3; ++j) {
				Cast<uint>(indices.data()) [i*3 + j] = face->mIndices[j];
			}
		}

		for (uint i = 0; i < src->mNumVertices; ++i)
		{
			if ( src->HasPositions() )
				CopyAttrib( float3{src->mVertices[i].x, src->mVertices[i].y, src->mVertices[i].z}, EVertexAttribute::Position, i );

			if ( src->HasNormals() )
				CopyAttrib( float3{src->mNormals[i].x, src->mNormals[i].y, src->mNormals[i].z}, EVertexAttribute::Normal, i );

			if ( src->mTangents )
				CopyAttrib( float3{src->mTangents[i].x, src->mTangents[i].y, src->mTangents[i].z}, EVertexAttribute::Tangent, i );

			if ( src->mBitangents )
				CopyAttrib( float3{src->mBitangents[i].x, src->mBitangents[i].y, src->mBitangents[i].z}, EVertexAttribute::BiNormal, i );
		
			for (uint t = 0; t < CountOf(EVertexAttribute::TextureUVs); ++t)
			{
				if ( not src->HasTextureCoords(t) )
					continue;

				if ( src->mNumUVComponents[t] == 2 )
					CopyAttrib( float2{src->mTextureCoords[t][i].x, src->mTextureCoords[t][i].y}, EVertexAttribute::TextureUVs[t], i );
				else
				//if ( src->mNumUVComponents[t] == 3 )
					CopyAttrib( float3{src->mTextureCoords[t][i].x, src->mTextureCoords[t][i].y, src->mTextureCoords[t][i].z}, EVertexAttribute::TextureUVs[t], i );
			}
		}
	
		dst = MakeShared<IntermediateMesh>( std::move(vertices), attribs, std::move(indices), EIndex::UInt );
		return true;
	}

/*
=================================================
	LoadLight
=================================================
*/
	static bool LoadLight (const aiLight *src, OUT IntermediateLightPtr &dst)
	{
		using ELightType = IntermediateLight::ELightType;

		IntermediateLight::Settings		light;
		
		light.position		= BitCast<vec3>( src->mPosition );
		light.direction		= normalize( BitCast<vec3>( src->mDirection ));
		light.upDirection	= normalize( BitCast<vec3>( src->mUp ));

		light.attenuation	= vec3{ src->mAttenuationConstant, src->mAttenuationLinear, src->mAttenuationQuadratic };
		light.diffuseColor	= BitCast<vec3>( src->mColorDiffuse );
		light.specularColor	= BitCast<vec3>( src->mColorSpecular );
		light.ambientColor	= BitCast<vec3>( src->mColorAmbient );

		light.coneAngleInnerOuter = vec2{ src->mAngleInnerCone, src->mAngleOuterCone };

		switch ( src->mType )
		{
			case aiLightSource_DIRECTIONAL :	light.type = ELightType::Directional;	break;
			case aiLightSource_POINT :			light.type = ELightType::Point;			break;
			case aiLightSource_SPOT :			light.type = ELightType::Spot;			break;
			case aiLightSource_AMBIENT :		light.type = ELightType::Ambient;		break;
			case aiLightSource_AREA :			light.type = ELightType::Area;			break;
			default :							ASSERT( !"unknown light type" );		break;
		}

		dst = MakeShared<IntermediateLight>( std::move(light) );
		return true;
	}

/*
=================================================
	LoadMaterials
=================================================
*/
	static bool LoadMaterials (const aiScene *scene, OUT Array<IntermediateMaterialPtr> &outMaterials)
	{
		outMaterials.resize( scene->mNumMaterials );

		for (uint i = 0; i < scene->mNumMaterials; ++i)
		{
			CHECK_ERR( LoadMaterial( scene->mMaterials[i], OUT outMaterials[i] ));
		}
		return true;
	}

/*
=================================================
	LoadMeshes
=================================================
*/
	static bool LoadMeshes (const aiScene *scene, OUT Array<IntermediateMeshPtr> &outMeshes)
	{
		outMeshes.resize( scene->mNumMeshes );

		for (uint i = 0; i < scene->mNumMeshes; ++i)
		{
			CHECK_ERR( LoadMesh( scene->mMeshes[i], OUT outMeshes[i] ));
		}
		return true;
	}

/*
=================================================
	LoadLights
=================================================
*/
	static bool LoadLights (const aiScene *scene, OUT Array<IntermediateLightPtr> &outLights)
	{
		outLights.resize( scene->mNumLights );

		for (uint i = 0; i < scene->mNumLights; ++i)
		{
			CHECK_ERR( LoadLight( scene->mLights[i], OUT outLights[i] ));
		}
		return true;
	}

/*
=================================================
	RecursiveLoadHierarchy
=================================================
*/
	static bool RecursiveLoadHierarchy (const aiScene *aiScene, const aiNode *node, const SceneData &scene, INOUT IntermediateScene::SceneNode &parent)
	{
		IntermediateScene::SceneNode	snode;
		snode.localTransform	= ConvertMatrix( node->mTransformation );
		snode.name				= node->mName.C_Str();

		for (uint i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh const*	ai_mesh	= aiScene->mMeshes[ node->mMeshes[i] ];
			CHECK( not ai_mesh->HasBones() );

			IntermediateScene::MeshNode		mnode;
			mnode.mesh		= scene.meshes[ node->mMeshes[i] ];
			mnode.material	= scene.materials[ ai_mesh->mMaterialIndex ];

			snode.data.push_back( mnode );
		}
		
		for (uint i = 0; i < node->mNumChildren; ++i)
		{
			CHECK_ERR( RecursiveLoadHierarchy( aiScene, node->mChildren[i], scene, INOUT snode ));
		}

		parent.nodes.push_back( snode );
		return true;
	}
	
/*
=================================================
	LoadHierarchy
=================================================
*/
	static bool LoadHierarchy (const aiScene *aiScene, INOUT SceneData &scene)
	{
		scene.root.localTransform	= ConvertMatrix( aiScene->mRootNode->mTransformation );
		scene.root.name				= aiScene->mRootNode->mName.C_Str();
		
		for (uint i = 0; i < aiScene->mRootNode->mNumChildren; ++i)
		{
			CHECK_ERR( RecursiveLoadHierarchy( aiScene, aiScene->mRootNode->mChildren[i], scene, INOUT scene.root ));
		}
		return true;
	}

/*
=================================================
	Load
=================================================
*/
	IntermediateScenePtr  AssimpLoader::Load (const Config &config, StringView filename)
	{
		const uint sceneLoadFlags = 0
									| aiProcess_CalcTangentSpace
									| aiProcess_Triangulate
									| (config.smoothNormals ? aiProcess_GenSmoothNormals : aiProcess_GenNormals)
									| aiProcess_RemoveRedundantMaterials
									| aiProcess_GenUVCoords 
									| aiProcess_TransformUVCoords
									| (config.splitLargeMeshes ? aiProcess_SplitLargeMeshes : 0)
									| (config.optimize ?
										(aiProcess_JoinIdenticalVertices | aiProcess_FindInstances |
										 aiProcess_OptimizeMeshes | aiProcess_ImproveCacheLocality /*| aiProcess_OptimizeGraph*/) :
										0);

		_importerPtr->SetPropertyInteger( AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, config.maxTrianglesPerMesh );
		_importerPtr->SetPropertyInteger( AI_CONFIG_PP_SLM_VERTEX_LIMIT, config.maxVerticesPerMesh );

		const aiScene *	scene	= _importerPtr->ReadFile( filename.data(), sceneLoadFlags );
		const char*		errStr	= _importerPtr->GetErrorString();
		CHECK_ERR( scene );
		
		SceneData	scene_data;
		CHECK_ERR( LoadMaterials( scene, OUT scene_data.materials ));
		CHECK_ERR( LoadMeshes( scene, OUT scene_data.meshes ));
		//CHECK_ERR( LoadAnimations( scene, OUT scene_data.animations ));
		CHECK_ERR( LoadLights( scene, OUT scene_data.lights ));
		CHECK_ERR( LoadHierarchy( scene, INOUT scene_data ));
		
		return MakeShared<IntermediateScene>( std::move(scene_data.materials),
											  std::move(scene_data.meshes),
											  std::move(scene_data.lights),
											  std::move(scene_data.root) );
	}


}	// FG

#endif	// FG_ENABLE_ASSIMP
