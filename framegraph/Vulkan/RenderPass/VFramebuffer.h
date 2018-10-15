// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VLogicalRenderPass.h"

namespace FG
{

	//
	// Vulkan Framebuffer
	//

	class VFramebuffer final : public std::enable_shared_from_this<VFramebuffer>
	{
		friend class VRenderPassCache;

	// types
	private:
		using Attachments_t	= FixedArray< Pair<VImagePtr, ImageViewDesc>, FG_MaxColorBuffers+1 >;

		
	// variables
	private:
		HashVal				_hash;
		VkFramebuffer		_framebufferId;

		VRenderPassPtr		_renderPass;

		uint2				_dimension;
		ImageLayer			_layers;
		Attachments_t		_attachments;

		
	// methods
	private:
		bool _Setup (ArrayView<Pair<VImagePtr, ImageViewDesc>> attachments, const VRenderPassPtr &rp, uint2 dim, uint layers);
		bool _Create (const VDevice &dev);
		void _Destroy (const VDevice &dev);
		

	public:
		VFramebuffer ();
		~VFramebuffer ();

		ND_ bool HasImage (const VImagePtr &img) const;

		ND_ bool operator == (const VFramebuffer &rhs) const;

		ND_ VkFramebuffer	GetFramebufferID ()	const	{ return _framebufferId; }

		ND_ uint2 const&	Dimension ()		const	{ return _dimension; }
		ND_ uint			Layers ()			const	{ return _layers.Get(); }
	};


}	// FG
