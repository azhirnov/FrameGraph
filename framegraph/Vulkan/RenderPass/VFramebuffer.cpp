// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFramebuffer.h"
#include "VDevice.h"
#include "VResourceManagerThread.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VFramebuffer::VFramebuffer () :
		_hash{ 0 },
		_framebuffer{ VK_NULL_HANDLE }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VFramebuffer::~VFramebuffer ()
	{
		CHECK( _framebuffer == VK_NULL_HANDLE );
	}
	
/*
=================================================
	HasImage
=================================================
*
	bool VFramebuffer::HasImage (const VImagePtr &img) const
	{
		ASSERT( _framebufferId );

		for (auto& att : _attachments)
		{
			if ( att.first == img )
				return true;
		}
		return false;
	}
	
/*
=================================================
	operator ==
=================================================
*/
	bool VFramebuffer::operator == (const VFramebuffer &rhs) const
	{
		SHAREDLOCK( _rcCheck );
		SHAREDLOCK( rhs._rcCheck );

		if ( _hash != rhs._hash )
			return false;

		return	All( _dimension	== rhs._dimension )	and
				_layers			== rhs._layers		and
				_attachments	== rhs._attachments	and
				_renderPassId	== rhs._renderPassId;
	}

/*
=================================================
	constructor
=================================================
*/
	VFramebuffer::VFramebuffer (ArrayView<Pair<RawImageID, ImageViewDesc>> attachments, RawRenderPassID rp, uint2 dim, uint layers)
	{
		SCOPELOCK( _rcCheck );
		ASSERT( not attachments.empty() );

		_attachments	= attachments;
		_renderPassId	= rp;
		_dimension		= dim;
		_layers			= ImageLayer(layers);

		// calc hash
		_hash =  HashOf( _attachments );
		_hash << HashOf( _renderPassId );
		_hash << HashOf( _dimension );
		_hash << HashOf( _layers );
	}

/*
=================================================
	Create
=================================================
*/
	bool VFramebuffer::Create (VResourceManagerThread &resMngr, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( not _framebuffer );
		CHECK_ERR( _renderPassId );

		FixedArray< VkImageView, FG_MaxColorBuffers+1 >		image_views;
		VDevice const &										dev = resMngr.GetDevice();

		for (auto& rt : _attachments)
		{
			VkImageView	view = resMngr.GetState( rt.first )->GetView( dev, INOUT rt.second );
			CHECK_ERR( view );

			image_views.push_back( view );
		}

		// create framebuffer
		VkFramebufferCreateInfo		fb_info	= {};
		
		fb_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass		= resMngr.GetResource( _renderPassId )->Handle();
		fb_info.attachmentCount	= uint(image_views.size());
		fb_info.pAttachments	= image_views.data();
		fb_info.width			= _dimension.x;
		fb_info.height			= _dimension.y;
		fb_info.layers			= _layers.Get();
		
		VK_CHECK( dev.vkCreateFramebuffer( dev.GetVkDevice(), &fb_info, null, OUT &_framebuffer ));
		
		_debugName = dbgName;
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VFramebuffer::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		SCOPELOCK( _rcCheck );

		if ( _framebuffer ) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_FRAMEBUFFER, uint64_t(_framebuffer) );
		}

		if ( _renderPassId ) {
			unassignIDs.emplace_back( _renderPassId );
		}

		for (auto& att : _attachments) {
			unassignIDs.emplace_back( att.first );
		}

		_framebuffer	= VK_NULL_HANDLE;
		_hash			= Default;
		_renderPassId	= Default;
		_attachments.clear();
		_debugName.clear();
	}


}	// FG
