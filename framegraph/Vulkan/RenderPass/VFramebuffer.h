// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/ImageDesc.h"
#include "VRenderPass.h"

namespace FG
{

	//
	// Vulkan Framebuffer
	//

	class VFramebuffer final
	{
		friend class VRenderPassCache;

	// types
	private:
		using Attachments_t	= FixedArray< Pair<RawImageID, ImageViewDesc>, FG_MaxColorBuffers+1 >;

		
	// variables
	private:
		HashVal					_hash;
		VkFramebuffer			_framebuffer;
		RawRenderPassID			_renderPassId;

		uint2					_dimension;
		ImageLayer				_layers;
		Attachments_t			_attachments;
		
		DebugName_t				_debugName;
		
		RWRaceConditionCheck	_rcCheck;

		
	// methods
	public:
		VFramebuffer ();
		VFramebuffer (VFramebuffer &&) = default;
		VFramebuffer (ArrayView<Pair<RawImageID, ImageViewDesc>> attachments, RawRenderPassID rp, uint2 dim, uint layers);
		~VFramebuffer ();
		
		bool Create (VResourceManagerThread &, StringView dbgName);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		//ND_ bool HasImage (const VImagePtr &img) const;

		ND_ bool operator == (const VFramebuffer &rhs) const;

		ND_ VkFramebuffer		Handle ()			const	{ SHAREDLOCK( _rcCheck );  return _framebuffer; }
		ND_ RawRenderPassID		GetRenderPassID ()	const	{ SHAREDLOCK( _rcCheck );  return _renderPassId; }
		ND_ uint2 const&		Dimension ()		const	{ SHAREDLOCK( _rcCheck );  return _dimension; }
		ND_ uint				Layers ()			const	{ SHAREDLOCK( _rcCheck );  return _layers.Get(); }
		ND_ HashVal				GetHash ()			const	{ SHAREDLOCK( _rcCheck );  return _hash; }
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
