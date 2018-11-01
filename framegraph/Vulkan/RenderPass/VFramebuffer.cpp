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
		if ( _hash != rhs._hash )
			return false;

		return	All( _dimension	== rhs._dimension )	and
				_layers			== rhs._layers		and
				//_attachments	== rhs._attachments	and
				_renderPassId	== rhs._renderPassId;
	}

/*
=================================================
	Initialize
=================================================
*/
	bool VFramebuffer::Initialize (ArrayView<Pair<RawImageID, ImageViewDesc>> attachments, RawRenderPassID rp, uint2 dim, uint layers)
	{
		CHECK_ERR( not _framebuffer );
		CHECK_ERR( not attachments.empty() );
		//CHECK_ERR( _attachments.empty() );

		//_attachments	= attachments;
		_renderPassId	= RenderPassID{ rp };
		_dimension		= dim;
		_layers			= ImageLayer(layers);

		// calc hash
		//_hash =  HashOf( _attachments );
		_hash << HashOf( _renderPassId );
		_hash << HashOf( _dimension );
		_hash << HashOf( _layers );

		return true;
	}

/*
=================================================
	Create
=================================================
*/
	bool VFramebuffer::Create (const VResourceManagerThread &rm)
	{
		CHECK_ERR( not _framebuffer );
		CHECK_ERR( _renderPassId );

		FixedArray< VkImageView, FG_MaxColorBuffers+1 >		image_views;
		VDevice const &										dev = rm.GetDevice();

		//for (auto& rt : _attachments) {
		//	image_views.push_back( rm.GetResource( rt.first.Get() )->GetView( rt.second ));
		//}

		// create framebuffer
		VkFramebufferCreateInfo		fb_info	= {};
		
		fb_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass		= rm.GetResource( _renderPassId.Get() )->Handle();
		fb_info.attachmentCount	= uint(image_views.size());
		fb_info.pAttachments	= image_views.data();
		fb_info.width			= _dimension.x;
		fb_info.height			= _dimension.y;
		fb_info.layers			= _layers.Get();
		
		VK_CHECK( dev.vkCreateFramebuffer( dev.GetVkDevice(), &fb_info, null, OUT &_framebuffer ));
		
		_OnCreate();
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VFramebuffer::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		if ( _framebuffer ) {
			readyToDelete.emplace_back( VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, uint64_t(_framebuffer) );
		}

		if ( _renderPassId ) {
			unassignIDs.emplace_back( _renderPassId.Release() );
		}

		//for (auto& att : _attachments) {
		//	unassignIDs.emplace_back( att.first.Release() );
		//}

		_framebuffer	= VK_NULL_HANDLE;
		_hash			= Default;
		_renderPassId	= Default;
		//_attachments.clear();
		
		_OnDestroy();
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VFramebuffer::Replace (INOUT VFramebuffer &&other)
	{
		_hash			= other._hash;
		_framebuffer	= other._framebuffer;
		_renderPassId	= std::move( other._renderPassId );
		_dimension		= other._dimension;
		_layers			= other._layers;
		//_attachments	= std::move( other._attachments );

		other._framebuffer	= VK_NULL_HANDLE;
		other._renderPassId	= Default;
		other._hash			= Default;
		//other._attachments.clear();
		
		ResourceBase::_Replace( std::move(other) );
	}


}	// FG
