// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "scene/SceneManager/DefaultImageCache.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	DefaultImageCache::DefaultImageCache ()
	{}
	
/*
=================================================
	Create
=================================================
*/
	bool  DefaultImageCache::Create (const FrameGraph &fg)
	{
		CHECK_ERR( fg );

		// create default white image
		{
			ImageID		image = fg->CreateImage( ImageDesc{ EImage::Tex2D, uint3{1,1,1}, EPixelFormat::RGBA8_UNorm, EImageUsage::Sampled | EImageUsage::Transfer },
												 Default, "Default white" );
			CHECK_ERR( image );

			// TODO
			//FG_UNUSED( fg->AddTask( ClearColorImage{}.SetImage( image ).AddRange( 0_mipmap, 1, 0_layer, 1 ).Clear(RGBA32f{ 1.0f }) ));

			_defaultImages.insert_or_assign( "white", std::move(image) );
		}

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  DefaultImageCache::Destroy (const FrameGraph &fg)
	{
		CHECK_ERR( fg, void());

		ReleaseUnused( fg );

		for (auto& item : _handleCache) {
			fg->ReleaseResource( INOUT item.second.second );
		}
		for (auto& item : _defaultImages) {
			fg->ReleaseResource( INOUT item.second );
		}

		_dataCache.clear();
		_handleCache.clear();
		_defaultImages.clear();
	}
	
/*
=================================================
	ReleaseUnused
=================================================
*/
	void  DefaultImageCache::ReleaseUnused (const FrameGraph &fg)
	{
		CHECK_ERR( fg, void());

		for (auto& id : _readyToDelete) {
			fg->ReleaseResource( INOUT id );
		}
		_readyToDelete.clear();
	}

/*
=================================================
	GetImageData
=================================================
*/
	bool  DefaultImageCache::GetImageData (const String &filename, OUT IntermImagePtr &outImage)
	{
		auto	iter = _dataCache.find( filename );
		if ( iter == _dataCache.end() )
			return false;

		auto	image = iter->second.lock();
		if ( not image ) {
			_dataCache.erase( iter );
			return false;
		}

		outImage = image;
		return true;
	}
	
/*
=================================================
	AddImageData
=================================================
*/
	bool  DefaultImageCache::AddImageData (const String &filename, const IntermImagePtr &image)
	{
		CHECK_ERR( image );

		image->MakeImmutable();
		_dataCache.insert_or_assign( filename, image );

		return true;
	}
	
/*
=================================================
	CreateImage
=================================================
*/
	bool  DefaultImageCache::CreateImage (const FrameGraph &fg, const CommandBuffer &cmdBuf, const IntermImagePtr &image, bool genMipmaps, OUT RawImageID &outHandle)
	{
		CHECK_ERR( image );

		if ( GetImageHandle( image, OUT outHandle ))
			return true;
		
		auto&	levels	= image->GetData();
		CHECK_ERR( levels.size() and levels.front().size() );

		auto&	layer		= levels.front().front();
		uint	base_level	= uint(levels.size()-1);
		uint	layer_count	= uint(levels.front().size());

		ImageDesc	desc;
		desc.imageType		= layer_count > 1 ? EImage::Tex2DArray : EImage::Tex2D;
		desc.dimension		= layer.dimension;
		desc.format			= layer.format;
		desc.arrayLayers	= ImageLayer{ layer_count };
		desc.maxLevel		= MipmapLevel{ genMipmaps ? ~0u : base_level+1 };
		desc.usage			= EImageUsage::Sampled | EImageUsage::Transfer;

		ImageID		id;
		CHECK_ERR( id = fg->CreateImage( desc ));
		outHandle = id.Get();

		Task	t_upload;
		for (size_t i = 0; i < levels.size(); ++i)
		for (size_t j = 0; j < levels[i].size(); ++j)
		{
			auto&			img = levels[i][j];
			UpdateImage		task;

			task.dstImage		= outHandle;
			task.arrayLayer		= ImageLayer{ uint(j) };
			task.mipmapLevel	= MipmapLevel{ uint(i) };
			task.aspectMask		= EImageAspect::Color;
			task.imageSize		= img.dimension;
			task.data			= img.pixels;
			task.dataRowPitch	= img.rowPitch;
			task.dataSlicePitch	= img.slicePitch;

			t_upload = cmdBuf->AddTask( task.DependsOn( t_upload ));
		}
		
		image->MakeImmutable();
		image->ReleaseData();

		if ( genMipmaps )
		{
			t_upload = cmdBuf->AddTask( GenerateMipmaps{}.SetImage( id ).SetRange( MipmapLevel{base_level}, UMax ).DependsOn( t_upload ));
		}

		_handleCache.insert_or_assign( image.operator->(), Pair<IntermImageWeak, ImageID>{ image, std::move(id) });
		return true;
	}

/*
=================================================
	GetImageHandle
=================================================
*/
	bool  DefaultImageCache::GetImageHandle (const IntermImagePtr &image, OUT RawImageID &outHandle)
	{
		CHECK_ERR( image );

		auto	iter = _handleCache.find( image.operator->() );
		if ( iter == _handleCache.end() )
			return false;

		auto	ptr = iter->second.first.lock();
		if ( not ptr )
		{
			_readyToDelete.push_back( std::move(iter->second.second) );
			_handleCache.erase( iter );
			return false;
		}

		outHandle = iter->second.second.Get();
		return true;
	}
	
/*
=================================================
	AddImageHandle
=================================================
*/
	bool  DefaultImageCache::AddImageHandle (const IntermImagePtr &image, ImageID &&handle)
	{
		CHECK_ERR( image );

		image->MakeImmutable();
		image->ReleaseData();		// TODO: keep?

		_handleCache.insert_or_assign( image.operator->(), Pair<IntermImageWeak, ImageID>{ image, std::move(handle) });
		return true;
	}
	
/*
=================================================
	GetDefaultImage
=================================================
*/
	bool  DefaultImageCache::GetDefaultImage (StringView name, OUT RawImageID &outHandle)
	{
		auto	iter = _defaultImages.find( name );
		if ( iter != _defaultImages.end() )
		{
			outHandle = iter->second;
			return true;
		}

		return false;
	}


}	// FG
