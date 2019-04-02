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
		using DefaultImageCache_t	= HashMap< StaticString<32>, ImageID >;


	// variables
	private:
		ImageDataCache_t		_dataCache;
		ImageHandleCache_t		_handleCache;
		DefaultImageCache_t		_defaultImages;
		Array<ImageID>			_readyToDelete;


	// methods
	public:
		DefaultImageCache ();
		
		bool  Create (const FrameGraph &);
		void  Destroy (const FrameGraph &) override;

		void  ReleaseUnused (const FrameGraph &) override;

		bool  GetImageData (const String &filename, OUT IntermImagePtr &) override;
		bool  AddImageData (const String &filename, const IntermImagePtr &) override;
		
		bool  CreateImage (const FrameGraph &, const CommandBuffer &, const IntermImagePtr &, bool genMipmaps, OUT RawImageID &) override;
		bool  GetImageHandle (const IntermImagePtr &, OUT RawImageID &) override;
		bool  AddImageHandle (const IntermImagePtr &, ImageID &&) override;
		
		bool  GetDefaultImage (StringView name, OUT RawImageID &) override;
	};


}	// FG
