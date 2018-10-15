// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFramebuffer.h"
#include "VDevice.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VFramebuffer::VFramebuffer () :
		_hash{ 0 },
		_framebufferId{ VK_NULL_HANDLE }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VFramebuffer::~VFramebuffer ()
	{
		CHECK( not _framebufferId );
	}
	
/*
=================================================
	HasImage
=================================================
*/
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
		if ( _hash != rhs._hash )
			return false;

		return	All( _dimension	== rhs._dimension )	and
				_layers			== rhs._layers		and
				_attachments	== rhs._attachments;
	}

/*
=================================================
	_Setup
=================================================
*/
	bool VFramebuffer::_Setup (ArrayView<Pair<VImagePtr, ImageViewDesc>> attachments, const VRenderPassPtr &rp, uint2 dim, uint layers)
	{
		CHECK_ERR( not _framebufferId );
		CHECK_ERR( not attachments.empty() );
		CHECK_ERR( _attachments.empty() );

		_attachments	= attachments;
		_renderPass		= rp;
		_dimension		= dim;
		_layers			= ImageLayer(layers);

		// calc hash
		_hash =  HashOf( _attachments );
		_hash << HashOf( _renderPass );
		_hash << HashOf( _dimension );
		_hash << HashOf( _layers );

		return true;
	}

/*
=================================================
	_Create
=================================================
*/
	bool VFramebuffer::_Create (const VDevice &dev)
	{
		CHECK_ERR( not _framebufferId );
		CHECK_ERR( _renderPass );

		FixedArray< VkImageView, FG_MaxColorBuffers+1 >		image_views;

		for (auto& rt : _attachments) {
			image_views.push_back( rt.first->GetImageView( rt.second ));
		}


		// create framebuffer
		VkFramebufferCreateInfo		fb_info	= {};
		
		fb_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass		= _renderPass->GetRenderPassID();
		fb_info.attachmentCount	= uint(image_views.size());
		fb_info.pAttachments	= image_views.data();
		fb_info.width			= _dimension.x;
		fb_info.height			= _dimension.y;
		fb_info.layers			= _layers.Get();
		
		VK_CHECK( dev.vkCreateFramebuffer( dev.GetVkDevice(), &fb_info, null, OUT &_framebufferId ) );
		return true;
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void VFramebuffer::_Destroy (const VDevice &dev)
	{
		if ( _framebufferId )
		{
			dev.vkDestroyFramebuffer( dev.GetVkDevice(), _framebufferId, null );
			_framebufferId = VK_NULL_HANDLE;
		}

		_hash		= Default;
		_renderPass	= null;
		_attachments.clear();
	}


}	// FG
