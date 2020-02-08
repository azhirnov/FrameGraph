// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/Loader/IImageLoader.h"
#include "scene/Loader/Intermediate/IntermScene.h"

#ifdef FG_STD_FILESYSTEM
#	include <filesystem>
	namespace FS = std::filesystem;
#else
#	error not supported!
#endif

namespace FG
{

/*
=================================================
	FindImage2
=================================================
*/
namespace {
	bool  FindImage2 (StringView name, ArrayView<StringView> directories, OUT String &result)
	{
		// check default directory
		{
			FS::path	img_path {};

			img_path.append( name );

			if ( FS::exists( img_path ) )
			{
				result = img_path.string();
				return true;
			}
		}

		// check directories
		for (auto& dir : directories)
		{
			FS::path	img_path {dir};
			
			img_path.append( name );

			if ( FS::exists( img_path ) )
			{
				result = img_path.string();
				return true;
			}
		}
		return false;
	}
}
/*
=================================================
	_FindImage
=================================================
*/
	bool  IImageLoader::_FindImage (StringView name, ArrayView<StringView> directories, OUT String &result)
	{
		if ( FindImage2( name, directories, OUT result ) )
			return true;
		
		if ( FindImage2( FS::path{name}.filename().string(), directories, OUT result ) )
			return true;

		RETURN_ERR( "image file not found!" );
	}

/*
=================================================
	Load
=================================================
*/
	bool  IImageLoader::Load (const IntermMaterialPtr &material, ArrayView<StringView> directories, const ImageCachePtr &imgCache)
	{
		using Texture = IntermMaterial::MtrTexture;

		CHECK_ERR( material );

		auto&	settings = material->EditSettings();

		if ( auto* albedo = UnionGetIf<Texture>( &settings.albedo ))
			CHECK_ERR( LoadImage( albedo->image, directories, imgCache ));
		
		if ( auto* specular = UnionGetIf<Texture>( &settings.specular ))
			CHECK_ERR( LoadImage( specular->image, directories, imgCache ));
		
		if ( auto* ambient = UnionGetIf<Texture>( &settings.ambient ))
			CHECK_ERR( LoadImage( ambient->image, directories, imgCache ));
		
		if ( auto* emissive = UnionGetIf<Texture>( &settings.emissive ))
			CHECK_ERR( LoadImage( emissive->image, directories, imgCache ));
		
		if ( auto* height_map = UnionGetIf<Texture>( &settings.heightMap ))
			CHECK_ERR( LoadImage( height_map->image, directories, imgCache ));
		
		if ( auto* normal_map = UnionGetIf<Texture>( &settings.normalMap ))
			CHECK_ERR( LoadImage( normal_map->image, directories, imgCache ));
		
		if ( auto* shininess = UnionGetIf<Texture>( &settings.shininess ))
			CHECK_ERR( LoadImage( shininess->image, directories, imgCache ));
		
		if ( auto* opacity = UnionGetIf<Texture>( &settings.opacity ))
			CHECK_ERR( LoadImage( opacity->image, directories, imgCache ));
		
		if ( auto* displacement_map = UnionGetIf<Texture>( &settings.displacementMap ))
			CHECK_ERR( LoadImage( displacement_map->image, directories, imgCache ));
		
		if ( auto* light_map = UnionGetIf<Texture>( &settings.lightMap ))
			CHECK_ERR( LoadImage( light_map->image, directories, imgCache ));
		
		if ( auto* reflection_map = UnionGetIf<Texture>( &settings.reflectionMap ))
			CHECK_ERR( LoadImage( reflection_map->image, directories, imgCache ));
		
		if ( auto* roughtness = UnionGetIf<Texture>( &settings.roughtness ))
			CHECK_ERR( LoadImage( roughtness->image, directories, imgCache ));
		
		if ( auto* metallic = UnionGetIf<Texture>( &settings.metallic ))
			CHECK_ERR( LoadImage( metallic->image, directories, imgCache ));
		
		if ( auto* subsurface = UnionGetIf<Texture>( &settings.subsurface ))
			CHECK_ERR( LoadImage( subsurface->image, directories, imgCache ));
		
		if ( auto* ambient_occlusion = UnionGetIf<Texture>( &settings.ambientOcclusion ))
			CHECK_ERR( LoadImage( ambient_occlusion->image, directories, imgCache ));
		
		if ( auto* refraction = UnionGetIf<Texture>( &settings.refraction ))
			CHECK_ERR( LoadImage( refraction->image, directories, imgCache ));

		return true;
	}
	
/*
=================================================
	Load
=================================================
*/
	bool  IImageLoader::Load (ArrayView<IntermMaterialPtr> materials, ArrayView<StringView> directories, const ImageCachePtr &imgCache)
	{
		for (auto& mtr : materials)
		{
			CHECK_ERR( Load( mtr, directories, imgCache ));
		}
		return true;
	}
	
/*
=================================================
	Load
=================================================
*/
	bool  IImageLoader::Load (const IntermScenePtr &scene, ArrayView<StringView> directories, const ImageCachePtr &imgCache)
	{
		CHECK_ERR( scene );

		for (auto& mtr : scene->GetMaterials())
		{
			CHECK_ERR( Load( mtr.first, directories, imgCache ));
		}
		return true;
	}


}	// FG
