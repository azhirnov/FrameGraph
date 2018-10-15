// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRenderPassCache.h"
#include "VDevice.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VRenderPassCache::VRenderPassCache (const VDevice &dev) :
		_device{ dev }
	{}
	
/*
=================================================
	destructor
=================================================
*/
	VRenderPassCache::~VRenderPassCache ()
	{}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VRenderPassCache::Initialize ()
	{
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VRenderPassCache::Deinitialize ()
	{
		for (auto& rp : _renderPassCache) {
			rp.ptr->_Destroy( _device );
		}
		_renderPassCache.clear();

		for (auto& fb : _framebufferCache) {
			fb.ptr->_Destroy( _device );
		}
		_framebufferCache.clear();
	}

/*
=================================================
	CreateRenderPass
=================================================
*/
	bool VRenderPassCache::CreateRenderPasses (ArrayView<VLogicalRenderPass*> logicalPasses, ArrayView<FragmentOutput> fragOutput)
	{
		VRenderPassPtr	rp = _CreateRenderPass( logicalPasses, fragOutput );
		CHECK_ERR( rp );

		VFramebufferPtr	fb = _CreateFramebuffer( logicalPasses, rp );
		CHECK_ERR( fb );

		uint	subpass = 0;

		for (auto& pass : logicalPasses)
		{
			pass->_SetRenderPass( rp, subpass++, fb );
		}
		return true;
	}
	
/*
=================================================
	_CreateRenderPass
=================================================
*/
	VRenderPassPtr VRenderPassCache::_CreateRenderPass (ArrayView<VLogicalRenderPass*> logicalPasses, ArrayView<FragmentOutput> fragOutput)
	{
		VRenderPassPtr	rp = MakeShared<VRenderPass>();

		CHECK_ERR( rp->_Setup( logicalPasses, fragOutput ) );


		// find render pass in existing
		{
			auto	iter = _renderPassCache.find( RenderPassItem{rp} );

			if ( iter != _renderPassCache.end() )
				return iter->ptr;
		}

		// search by attachments
		// TODO...

		// add new render pass
		CHECK_ERR( rp->_Create( _device ));

		_renderPassCache.insert( RenderPassItem{rp} );
		return rp;
	}
	
/*
=================================================
	_CreateFramebuffer
=================================================
*/
	VFramebufferPtr VRenderPassCache::_CreateFramebuffer (ArrayView<VLogicalRenderPass*> logicalPasses, const VRenderPassPtr &renderPass)
	{
		FixedArray< Pair<VImagePtr, ImageViewDesc>, FG_MaxColorBuffers+1 >		render_targets;
		render_targets.resize( renderPass->GetCreateInfo().attachmentCount );

		Optional<RectI>		total_area;


		for (auto& lrp : logicalPasses)
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
				CHECK_ERR( dst.first == null or (dst.first == src.image and dst.second == src.desc) );

				dst.first	= Cast<VImage>(src.image);
				dst.second	= src.desc;
			}

			for (const auto& rt : lrp->GetColorTargets())
			{
				uint	index;
				CHECK_ERR( renderPass->GetColorAttachmentIndex( rt.first, OUT index ));
				
				auto&	dst = render_targets[index];
				
				// compare attachments
				CHECK_ERR( dst.first == null or (dst.first == rt.second.image and dst.second == rt.second.desc) );

				dst.first	= Cast<VImage>(rt.second.image);
				dst.second	= rt.second.desc;
			}
		}


		VFramebufferPtr	fb = MakeShared<VFramebuffer>();

		CHECK_ERR( fb->_Setup( render_targets, renderPass, uint2(total_area->Size()), 1 ));		// TODO: layers


		// find framebuffer in cache
		{
			auto	iter = _framebufferCache.find( FramebufferItem(fb) );

			if ( iter != _framebufferCache.end() )
				return iter->ptr;
		}

		// create new framebuffer
		CHECK_ERR( fb->_Create( _device ));

		_framebufferCache.insert( FramebufferItem(fb) );
		return fb;
	}


}	// FG
