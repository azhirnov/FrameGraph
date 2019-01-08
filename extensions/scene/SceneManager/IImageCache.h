// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Loader/Intermediate/IntermediateImage.h"

namespace FG
{

	//
	// Image Cache interface
	//

	class IImageCache : public std::enable_shared_from_this<IImageCache>
	{
	// interface
	public:
		virtual void  Destroy (const FGThreadPtr &) = 0;
		virtual void  ReleaseUnused (const FGThreadPtr &) = 0;

		virtual bool  GetImageData (const String &filename, OUT IntermediateImagePtr &) = 0;
		virtual bool  AddImageData (const String &filename, const IntermediateImagePtr &) = 0;

		virtual bool  CreateImage (const FGThreadPtr &, const IntermediateImagePtr &, OUT RawImageID &) = 0;
		virtual bool  GetImageHandle (const IntermediateImagePtr &, OUT RawImageID &) = 0;
		virtual bool  AddImageHandle (const IntermediateImagePtr &, ImageID &&) = 0;
	};


}	// FG
