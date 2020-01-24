// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/IntermImage.h"

namespace FG
{

	//
	// Image Cache interface
	//

	class IImageCache : public std::enable_shared_from_this<IImageCache>
	{
	// interface
	public:
		virtual void  Destroy (const FrameGraph &) = 0;
		virtual void  ReleaseUnused (const FrameGraph &) = 0;

		virtual bool  GetImageData (const String &filename, OUT IntermImagePtr &) = 0;
		virtual bool  AddImageData (const String &filename, const IntermImagePtr &) = 0;

		virtual bool  CreateImage (const CommandBuffer &, const IntermImagePtr &, bool genMipmaps, OUT RawImageID &) = 0;
		virtual bool  GetImageHandle (const IntermImagePtr &, OUT RawImageID &) = 0;
		virtual bool  AddImageHandle (const IntermImagePtr &, ImageID &&) = 0;

		virtual bool  GetDefaultImage (StringView name, OUT RawImageID &) = 0;
	};


}	// FG
