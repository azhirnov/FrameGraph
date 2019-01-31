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
	Destroy
=================================================
*/
	void  DefaultImageCache::Destroy (const FGThreadPtr &fg)
	{
		CHECK_ERR( fg, void());

		ReleaseUnused( fg );

		for (auto& item : _handleCache) {
			fg->ReleaseResource( INOUT item.second.second );
		}

		_dataCache.clear();
		_handleCache.clear();
	}
	
/*
=================================================
	ReleaseUnused
=================================================
*/
	void  DefaultImageCache::ReleaseUnused (const FGThreadPtr &fg)
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
	bool  DefaultImageCache::CreateImage (const FGThreadPtr &fg, const IntermImagePtr &image, OUT RawImageID &outHandle)
	{
		CHECK_ERR( image );

		if ( GetImageHandle( image, OUT outHandle ))
			return true;
		
		auto&	levels	= image->GetData();
		CHECK_ERR( levels.size() and levels.front().size() );

		auto&	layer	= levels.front().front();

		ImageDesc	desc;
		desc.imageType		= levels.size() > 1 ? EImage::Tex2DArray : EImage::Tex2D;
		desc.dimension		= layer.dimension;
		desc.format			= layer.format;
		desc.arrayLayers	= ImageLayer{ uint(levels.size()) };
		desc.maxLevel		= MipmapLevel{ uint(levels.size()) };
		desc.usage			= EImageUsage::Sampled | EImageUsage::Transfer;

		ImageID	id;
		CHECK_ERR( id = fg->CreateImage( desc ));
		outHandle = id.Get();

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

			FG_UNUSED( fg->AddTask( task ));
		}
		
		image->MakeImmutable();
		image->ReleaseData();

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


}	// FG
