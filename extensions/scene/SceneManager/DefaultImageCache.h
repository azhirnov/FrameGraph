// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/SceneManager/IImageCache.h"

namespace FG
{

	//
	// Default Image Cache
	//

	class DefaultImageCache final : public IImageCache
	{
	// types
	private:
		using IntermediateImageWeak	= WeakPtr< IntermediateImage >;
		using ImageDataCache_t		= HashMap< String, IntermediateImageWeak >;
		using ImageHandleCache_t	= HashMap< const void*, Pair<IntermediateImageWeak, ImageID> >;


	// variables
	private:
		ImageDataCache_t		_dataCache;
		ImageHandleCache_t		_handleCache;
		Array<ImageID>			_readyToDelete;


	// methods
	public:
		DefaultImageCache ();
		
		void  Destroy (const FGThreadPtr &) override;
		void  ReleaseUnused (const FGThreadPtr &) override;

		bool  GetImageData (const String &filename, OUT IntermediateImagePtr &) override;
		bool  AddImageData (const String &filename, const IntermediateImagePtr &) override;
		
		bool  CreateImage (const FGThreadPtr &, const IntermediateImagePtr &, OUT RawImageID &) override;
		bool  GetImageHandle (const IntermediateImagePtr &, OUT RawImageID &) override;
		bool  AddImageHandle (const IntermediateImagePtr &, ImageID &&) override;
	};


}	// FG
