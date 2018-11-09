// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/ImageDesc.h"
#include "framegraph/Shared/ResourceBase.h"
#include "VRenderPass.h"

namespace FG
{

	//
	// Vulkan Framebuffer
	//

	class VFramebuffer final : public ResourceBase
	{
		friend class VRenderPassCache;

	// types
	private:
		using Attachments_t	= FixedArray< Pair<ImageID, ImageViewDesc>, FG_MaxColorBuffers+1 >;

		
	// variables
	private:
		HashVal				_hash;
		VkFramebuffer		_framebuffer;
		RenderPassID		_renderPassId;

		uint2				_dimension;
		ImageLayer			_layers;
		//Attachments_t		_attachments;

		
	// methods
	public:
		VFramebuffer ();
		~VFramebuffer ();
		
		bool Initialize (ArrayView<Pair<RawImageID, ImageViewDesc>> attachments, RawRenderPassID rp, uint2 dim, uint layers);
		bool Create (const VResourceManagerThread &);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		void Replace (INOUT VFramebuffer &&other);

		//ND_ bool HasImage (const VImagePtr &img) const;

		ND_ bool operator == (const VFramebuffer &rhs) const;

		ND_ VkFramebuffer		Handle ()			const	{ return _framebuffer; }
		ND_ RawRenderPassID		GetRenderPassID ()	const	{ return _renderPassId.Get(); }
		ND_ uint2 const&		Dimension ()		const	{ return _dimension; }
		ND_ uint				Layers ()			const	{ return _layers.Get(); }
		ND_ HashVal				GetHash ()			const	{ return _hash; }
	};


}	// FG


namespace std
{
	template <>
	struct hash< FG::VFramebuffer > {
		ND_ size_t  operator () (const FG::VFramebuffer &value) const noexcept {
			return size_t(value.GetHash());
		}
	};

}	// std