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
		using IntermImageWeak		= WeakPtr< IntermImage >;
		using ImageDataCache_t		= HashMap< String, IntermImageWeak >;
		using ImageHandleCache_t	= HashMap< const void*, Pair<IntermImageWeak, ImageID> >;


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

		bool  GetImageData (const String &filename, OUT IntermImagePtr &) override;
		bool  AddImageData (const String &filename, const IntermImagePtr &) override;
		
		bool  CreateImage (const FGThreadPtr &, const IntermImagePtr &, bool genMipmaps, OUT RawImageID &) override;
		bool  GetImageHandle (const IntermImagePtr &, OUT RawImageID &) override;
		bool  AddImageHandle (const IntermImagePtr &, ImageID &&) override;
	};


}	// FG
