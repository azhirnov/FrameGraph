// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
namespace {

	struct AttribHash {
		size_t  operator () (const VertexAttributesPtr &value) const {
			return size_t(value->CalcHash());
		}
	};

	struct AttribEq {
		bool  operator () (const VertexAttributesPtr &lhs, const VertexAttributesPtr &rhs) const {
			return *lhs == *rhs;
		}
	};

	using VertexAttribsSet_t = std::unordered_set< VertexAttributesPtr, AttribHash, AttribEq >;


	struct SceneData
	{
		Array<IntermMaterialPtr>	materials;
		Array<IntermMeshPtr>		meshes;
		VertexAttribsSet_t			attribsCache;
		Array<IntermLightPtr>		lights;
		IntermScene::SceneNode		root;
	};

/*
=================================================
	AssimpInit
=================================================
*/
	static void AssimpInit ()
	{
		static bool	isAssimpInit = false;

		if ( isAssimpInit )
			return;

		isAssimpInit = true;

		// Create Logger
		Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;
		Assimp::DefaultLogger::create( "", severity, aiDefaultLogStream_STDOUT );
		//Assimp::DefaultLogger::create( "assimp_log.txt", severity, aiDefaultLogStream_FILE );
	}
	
/*
=================================================
	ConvertMatrix
=================================================
*/
	ND_ static Transform  ConvertMatrix (const aiMatrix4x4 &src)
	{
		const mat4x4	dst {
			src[0][0], src[0][1], src[0][2], src[0][3],
			src[1][0], src[1][1], src[1][2], src[1][3],
			src[2][0], src[2][1], src[2][2], src[2][3],
			src[3][0], src[3][1], src[3][2], src[3][3]
		};
		return Transform{ transpose( dst )};
	}
	
/*
=================================================
	ConvertVec
=================================================
*/
	ND_ static vec3  ConvertVec (const aiVector3D &src)
	{
		return vec3{ src.x, src.y, src.z };
	}

	ND_ static vec3  ConvertVec (const aiColor3D &src)
	{
		return vec3{ src.r, src.g, src.b };
	}

/*
=================================================
	ConvertColor
=================================================
*/
	ND_ static RGBA32f  ConvertColor (const aiColor4D &src)
	{
		return RGBA32f{ src.r, src.g, src.b, src.a };
	}

/*
=================================================
	ConvertWrapMode
=================================================
*/
	ND_ static EAddressMode  ConvertWrapMode (aiTextureMapMode mode)
	{
		BEGIN_ENUM_CHECKS();
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
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown wrap mode", EAddressMode::Repeat );
	}
	
/*
=================================================
	ConvertMapping
=================================================
*/
	using ETextureMapping = IntermMaterial::ETextureMapping;

	ND_ static ETextureMapping  ConvertMapping (aiTextureMapping mapping)
	{
		BEGIN_ENUM_CHECKS();
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
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown texture mapping" );
	};

/*
=================================================
	LoadMaterial
=================================================
*/
	static bool LoadMaterial (const aiMaterial *src, OUT IntermMaterialPtr &dst)
	{
		IntermMaterial::Settings	mtr;

		aiColor4D	color;
		float		fvalue		= 0.0f;
		uint		max_size	= 1;

		aiString	mtr_name;
		aiGetMaterialString( src, AI_MATKEY_NAME, OUT &mtr_name );
		mtr.name = mtr_name.C_Str();

		if ( aiGetMaterialColor( src, AI_MATKEY_COLOR_DIFFUSE, OUT &color ) == aiReturn_SUCCESS )
			mtr.albedo = ConvertColor( color );

		if ( aiGetMaterialColor( src, AI_MATKEY_COLOR_SPECULAR, OUT &color ) == aiReturn_SUCCESS )
			mtr.specular = ConvertColor( color );

		if ( aiGetMaterialColor( src, AI_MATKEY_COLOR_AMBIENT, OUT &color ) == aiReturn_SUCCESS )
			mtr.ambient = ConvertColor( color );
	
		if ( aiGetMaterialColor( src, AI_MATKEY_COLOR_EMISSIVE, OUT &color ) == aiReturn_SUCCESS )
			mtr.emissive = ConvertColor( color );
	
		if ( aiGetMaterialFloatArray( src, AI_MATKEY_OPACITY, OUT &fvalue, &max_size ) == aiReturn_SUCCESS and fvalue < 1.0f )
			mtr.opacity = fvalue;

		if ( aiGetMaterialFloatArray( src, AI_MATKEY_SHININESS, OUT &fvalue, &max_size ) == aiReturn_SUCCESS )
			mtr.shininess = fvalue;

		aiGetMaterialFloatArray( src, AI_MATKEY_SHININESS_STRENGTH, OUT &mtr.shininessStrength, &max_size );

		int		twosided = 1;
		aiGetMaterialInteger( src, AI_MATKEY_TWOSIDED, OUT &twosided );
		mtr.cullMode = twosided ? ECullMode::None : ECullMode::Back;
	
		StaticArray< IntermMaterial::Parameter*, aiTextureType_UNKNOWN >	textures =
		{
			null, &mtr.albedo, &mtr.specular, &mtr.ambient, &mtr.emissive, &mtr.heightMap, &mtr.normalMap,
			&mtr.shininess, &mtr.opacity, &mtr.displacementMap, &mtr.lightMap, &mtr.reflectionMap
		};

		for (uint i = 0; i < aiTextureType_UNKNOWN; ++i)
		{
			auto tex = textures[i];
			if ( not tex ) continue;
		
			aiString			tex_name;
			aiTextureMapMode	map_mode[3]	= { aiTextureMapMode_Wrap, aiTextureMapMode_Wrap, aiTextureMapMode_Wrap };
			aiTextureMapping	mapping		= aiTextureMapping_UV;
			uint				uv_index	= 0;

			if ( src->GetTexture( aiTextureType(i), 0, OUT &tex_name, OUT &mapping, OUT &uv_index, null, null, OUT map_mode ) == AI_SUCCESS )
			{
				IntermMaterial::MtrTexture	mtr_tex;
				mtr_tex.name			= tex_name.C_Str();
				mtr_tex.addressModeU	= ConvertWrapMode( map_mode[0] );
				mtr_tex.addressModeV	= ConvertWrapMode( map_mode[1] );
				mtr_tex.addressModeW	= ConvertWrapMode( map_mode[2] );
				mtr_tex.mapping			= ConvertMapping( mapping );
				mtr_tex.filter			= EFilter::Linear;
				mtr_tex.uvIndex			= uv_index;
				mtr_tex.image			= MakeShared<IntermImage>( StringView{tex_name.C_Str()} );

				*tex = std::move(mtr_tex);
			}
		}

		LayerBits	layers;

		//if ( mtr.opacity.index() )
		//	layers[uint(ERenderLayer::Translucent)] = true;
		//else {
			layers[uint(ERenderLayer::Opaque_1)]  = true;
		//	layers[uint(ERenderLayer::DepthOnly)] = true;
		//	layers[uint(ERenderLayer::Shadow)]    = true;
		//}

		//if ( mtr.emissive.index() )
		//	layers[uint(ERenderLayer::Emission)] = true;

		dst = MakeShared<IntermMaterial>( std::move(mtr), layers );
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
	static bool LoadMesh (const aiMesh *src, OUT IntermMeshPtr &dst, INOUT VertexAttribsSet_t &attribsCache)
	{
		CHECK_ERR( src->mPrimitiveTypes == aiPrimitiveType_TRIANGLE );
		CHECK_ERR( not src->HasBones() );

		BytesU				vert_stride;
		VertexAttributesPtr	attribs;
		CHECK_ERR( CreateVertexAttribs( src, OUT vert_stride, OUT attribs ));

		attribs = *attribsCache.insert( attribs ).first;

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
	
		EPrimitive	topology = Default;
		switch ( src->mPrimitiveTypes )
		{
			case aiPrimitiveType_POINT :		topology = EPrimitive::Point;			break;
			case aiPrimitiveType_LINE :			topology = EPrimitive::LineList;		break;
			case aiPrimitiveType_TRIANGLE :		topology = EPrimitive::TriangleList;	break;
			default :							RETURN_ERR( "unsupported type" );
		}

		dst = MakeShared<IntermMesh>( std::move(vertices), attribs, vert_stride, topology, std::move(indices), EIndex::UInt );
		return true;
	}

/*
=================================================
	LoadLight
=================================================
*/
	static bool LoadLight (const aiLight *src, OUT IntermLightPtr &dst)
	{
		using ELightType = IntermLight::ELightType;

		IntermLight::Settings		light;
		
		light.position		= ConvertVec( src->mPosition );
		light.direction		= normalize( ConvertVec( src->mDirection ));
		light.upDirection	= normalize( ConvertVec( src->mUp ));

		light.attenuation	= vec3{ src->mAttenuationConstant, src->mAttenuationLinear, src->mAttenuationQuadratic };
		light.diffuseColor	= ConvertVec( src->mColorDiffuse );
		light.specularColor	= ConvertVec( src->mColorSpecular );
		light.ambientColor	= ConvertVec( src->mColorAmbient );

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

		dst = MakeShared<IntermLight>( std::move(light) );
		return true;
	}

/*
=================================================
	LoadMaterials
=================================================
*/
	static bool LoadMaterials (const aiScene *scene, OUT Array<IntermMaterialPtr> &outMaterials)
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
	static bool LoadMeshes (const aiScene *scene, OUT Array<IntermMeshPtr> &outMeshes, INOUT VertexAttribsSet_t &attribsCache)
	{
		outMeshes.resize( scene->mNumMeshes );

		for (uint i = 0; i < scene->mNumMeshes; ++i)
		{
			CHECK_ERR( LoadMesh( scene->mMeshes[i], OUT outMeshes[i], INOUT attribsCache ));
		}
		return true;
	}

/*
=================================================
	LoadLights
=================================================
*/
	static bool LoadLights (const aiScene *scene, OUT Array<IntermLightPtr> &outLights)
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
	static bool RecursiveLoadHierarchy (const aiScene *aiScene, const aiNode *node, const SceneData &scene, INOUT IntermScene::SceneNode &parent)
	{
		IntermScene::SceneNode	snode;
		snode.localTransform	= ConvertMatrix( node->mTransformation );
		snode.name				= node->mName.C_Str();

		for (uint i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh const*	ai_mesh	= aiScene->mMeshes[ node->mMeshes[i] ];
			CHECK( not ai_mesh->HasBones() );

			IntermScene::ModelData	mnode;

			for (auto& level : mnode.levels)
			{
				level.first		= scene.meshes[ node->mMeshes[i] ];
				level.second	= scene.materials[ ai_mesh->mMaterialIndex ];
			}

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
		
		CHECK_ERR( RecursiveLoadHierarchy( aiScene, aiScene->mRootNode, scene, INOUT scene.root ));
		return true;
	}
}

/*
=================================================
	constructor
=================================================
*/
	AssimpLoader::AssimpLoader () :
		_importerPtr{ new Assimp::Importer{} }
	{
		AssimpInit();
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
	Load
=================================================
*/
	IntermScenePtr  AssimpLoader::Load (const Config &config, NtStringView filename)
	{
		const uint sceneLoadFlags = 0
									| (config.calculateTBN ? aiProcess_CalcTangentSpace : 0)
									| aiProcess_Triangulate
									| (config.smoothNormals ? aiProcess_GenSmoothNormals : aiProcess_GenNormals)
									| aiProcess_RemoveRedundantMaterials
									| aiProcess_GenUVCoords 
									| aiProcess_TransformUVCoords
									| (config.splitLargeMeshes ? aiProcess_SplitLargeMeshes : 0)
									| (not config.optimize ? 0 :
										(aiProcess_JoinIdenticalVertices | aiProcess_FindInstances |
										 aiProcess_OptimizeMeshes | aiProcess_ImproveCacheLocality /*| aiProcess_OptimizeGraph*/));

		_importerPtr->SetPropertyInteger( AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, config.maxTrianglesPerMesh );
		_importerPtr->SetPropertyInteger( AI_CONFIG_PP_SLM_VERTEX_LIMIT, config.maxVerticesPerMesh );

		const aiScene *	scene	= _importerPtr->ReadFile( filename.c_str(), sceneLoadFlags );
		const char*		errStr	= _importerPtr->GetErrorString();
		CHECK_ERR( scene );
		FG_UNUSED( errStr );
		
		SceneData	scene_data;
		CHECK_ERR( LoadMaterials( scene, OUT scene_data.materials ));
		CHECK_ERR( LoadMeshes( scene, OUT scene_data.meshes, INOUT scene_data.attribsCache ));
		//CHECK_ERR( LoadAnimations( scene, OUT scene_data.animations ));
		CHECK_ERR( LoadLights( scene, OUT scene_data.lights ));
		CHECK_ERR( LoadHierarchy( scene, INOUT scene_data ));
		
		return MakeShared<IntermScene>( scene_data.materials, scene_data.meshes,
										std::move(scene_data.lights), std::move(scene_data.root) );
	}


}	// FG

#endif	// FG_ENABLE_ASSIMP
