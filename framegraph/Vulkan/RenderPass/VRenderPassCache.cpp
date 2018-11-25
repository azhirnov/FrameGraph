// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRenderPassCache.h"
#include "VResourceManagerThread.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VRenderPassCache::VRenderPassCache ()
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VRenderPassCache::~VRenderPassCache ()
	{
	}

/*
=================================================
	CreateRenderPasses
=================================================
*/
	bool VRenderPassCache::CreateRenderPasses (VResourceManagerThread &resMngr, ArrayView<VLogicalRenderPass*> logicalPasses, ArrayView<FragmentOutput> fragOutput)
	{
		RawRenderPassID		rp_id = resMngr.CreateRenderPass( logicalPasses, fragOutput, "", true );
		CHECK_ERR( rp_id );

		RawFramebufferID	fb_id = _CreateFramebuffer( resMngr, logicalPasses, rp_id );
		CHECK_ERR( fb_id );

		uint	subpass = 0;
		for (auto& pass : logicalPasses)
		{
			pass->_SetRenderPass( rp_id, subpass++, fb_id );
		}
		return true;
	}

/*
=================================================
	_CreateFramebuffer
=================================================
*/
	RawFramebufferID  VRenderPassCache::_CreateFramebuffer (VResourceManagerThread &resMngr, ArrayView<VLogicalRenderPass*> logicalPasses,
															RawRenderPassID renderPassId)
	{
		using Attachment_t = FixedArray< Pair<RawImageID, ImageViewDesc>, FG_MaxColorBuffers+1 >;

		VRenderPass const*	render_pass		= resMngr.GetResource( renderPassId );
		Attachment_t		render_targets;	render_targets.resize( render_pass->GetCreateInfo().attachmentCount );
		Optional<RectI>		total_area;

		for (const auto& lrp : logicalPasses)
		{
			// merge rendering areas
			if ( total_area.has_value() )
				total_area->Merge( lrp->GetArea() );
			else
				total_area = lrp->GetArea();


			if ( lrp->GetDepthStencilTarget().IsDefined() )
			{
				auto&	src = lrp->GetDepthStencilTarget();
				auto&	dst = render_targets[0];

				// compare attachments
				CHECK_ERR( not dst.first or (dst.first == src.imageId and dst.second == src.desc) );

				dst.first	= src.imageId;
				dst.second	= src.desc;
			}

			for (const auto& rt : lrp->GetColorTargets())
			{
				uint	index;
				CHECK_ERR( render_pass->GetColorAttachmentIndex( rt.first, OUT index ));
				
				auto&	dst = render_targets[index];
				
				// compare attachments
				CHECK_ERR( not dst.first or (dst.first == rt.second.imageId and dst.second == rt.second.desc) );

				dst.first	= rt.second.imageId;
				dst.second	= rt.second.desc;
			}
		}

		return resMngr.CreateFramebuffer( render_targets, renderPassId, uint2(total_area->Size()), 1, "", true );
	}


}	// FG
