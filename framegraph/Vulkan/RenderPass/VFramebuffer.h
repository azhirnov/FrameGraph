// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
	// types
	private:
		using Attachments_t	= FixedArray< Pair<RawImageID, ImageViewDesc>, FG_MaxColorBuffers+1 >;

		
	// variables
	private:
		HashVal					_hash;
		VkFramebuffer			_framebuffer;
		RawRenderPassID			_renderPassId;		// strong ref

		uint2					_dimension;
		ImageLayer				_layers;
		Attachments_t			_attachments;		// weak ref
		
		DebugName_t				_debugName;
		
		RWDataRaceCheck			_drCheck;

		
	// methods
	public:
		VFramebuffer ();
		VFramebuffer (VFramebuffer &&) = delete;
		VFramebuffer (const VFramebuffer &) = delete;
		VFramebuffer (ArrayView<Pair<RawImageID, ImageViewDesc>> attachments, RawRenderPassID rp, uint2 dim, uint layers);
		~VFramebuffer ();
		
		bool Create (VResourceManager &, StringView dbgName);
		void Destroy (VResourceManager &);
		
		ND_ bool IsAllResourcesAlive (const VResourceManager &) const;

		ND_ bool operator == (const VFramebuffer &rhs) const;

		ND_ VkFramebuffer		Handle ()			const	{ SHAREDLOCK( _drCheck );  return _framebuffer; }
		ND_ RawRenderPassID		GetRenderPassID ()	const	{ SHAREDLOCK( _drCheck );  return _renderPassId; }
		ND_ uint2 const&		Dimension ()		const	{ SHAREDLOCK( _drCheck );  return _dimension; }
		ND_ uint				Layers ()			const	{ SHAREDLOCK( _drCheck );  return _layers.Get(); }
		ND_ HashVal				GetHash ()			const	{ SHAREDLOCK( _drCheck );  return _hash; }
	};


}	// FG


namespace std
{
	template <>
	struct hash< FG::VFramebuffer > {
		ND_ size_t  operator () (const FG::VFramebuffer &value) const {
			return size_t(value.GetHash());
		}
	};

}	// std
