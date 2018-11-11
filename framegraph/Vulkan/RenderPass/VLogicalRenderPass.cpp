// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VLogicalRenderPass.h"
#include "VResourceManagerThread.h"
#include "VEnumCast.h"

namespace FG
{
	
/*
=================================================
	constructor
=================================================
*/
	inline void ConvertClearValue (const RenderPassDesc::ClearValue_t &cv, OUT VkClearValue &result)
	{
		Visit( cv,
			[&result] (const RGBA32f &src)		{ MemCopy( result.color.float32, src ); },
			[&result] (const RGBA32u &src)		{ MemCopy( result.color.uint32, src ); },
			[&result] (const RGBA32i &src)		{ MemCopy( result.color.int32, src ); },
			[&result] (const DepthStencil &src)	{ result.depthStencil = {src.depth, src.stencil}; },
			[] (const auto&)					{ ASSERT(false); }
		);
	}
//-----------------------------------------------------------------------------



/*
=================================================
	Create
=================================================
*/
	bool VLogicalRenderPass::Create (VResourceManagerThread &rm, const RenderPassDesc &rp)
	{
		SCOPELOCK( _rcCheck );

		_area				= rp.area;
		//_parallelExecution= rp.parallelExecution;
		_canBeMerged		= rp.canBeMerged;

		for (const auto& src : rp.renderTargets)
		{
			ColorTarget		dst;
			VImage const*	image = rm.GetResource( src.second.image );

			dst.imageId		= src.second.image;
			dst.imagePtr	= rm.GetState( src.second.image );
			dst.desc		= src.second.desc.has_value() ? *src.second.desc : ImageViewDesc{image->Description()};
			dst.samples		= VEnumCast( image->Description().samples );
			dst.loadOp		= VEnumCast( src.second.loadOp );
			dst.storeOp		= VEnumCast( src.second.storeOp );
			dst.state		= EResourceState::Unknown;

			dst.desc.Validate( image->Description() );
			dst._imageHash	= HashOf( dst.imageId ) + HashOf( dst.desc );

			ConvertClearValue( src.second.clearValue, OUT dst.clearValue );

			// validate image view description
			if ( dst.desc.format == EPixelFormat::Unknown )
				dst.desc.format = image->Description().format;

			// add resource state flags
			if ( src.second.loadOp  == EAttachmentLoadOp::Clear )		dst.state |= EResourceState::ClearBefore;
			if ( src.second.loadOp  == EAttachmentLoadOp::Invalidate )	dst.state |= EResourceState::InvalidateBefore;
			if ( src.second.storeOp == EAttachmentStoreOp::Invalidate )	dst.state |= EResourceState::InvalidateAfter;


			// add color or depth-stencil render target
			if ( EPixelFormat_HasDepthOrStencil( dst.desc.format ) )
			{
				ASSERT( src.first == RenderTargetID()			or
					    src.first == RenderTargetID("depth")	or
						src.first == RenderTargetID("depthStencil") );
				
				dst.state |= EResourceState::DepthStencilAttachmentReadWrite;	// TODO: support other layouts

				_depthStencilTarget = DepthStencilTarget{ dst };
			}
			else
			{
				dst.state |= EResourceState::ColorAttachmentReadWrite;		// TODO: remove 'Read' state if blending disabled or 'loadOp' is 'Clear'

				_colorTargets.insert({ src.first, dst });
			}
		}

		// create viewports and default scissors
		for (auto& src : rp.viewports)
		{
			ASSERT( src.rect.left >= float(rp.area.left) and src.rect.right  <= float(rp.area.right)  );
			ASSERT( src.rect.top  >= float(rp.area.top)  and src.rect.bottom <= float(rp.area.bottom) );

			VkViewport		dst;
			dst.x			= src.rect.left;
			dst.y			= src.rect.top;
			dst.width		= src.rect.Width();
			dst.height		= src.rect.Height();
			dst.minDepth	= src.minDepth;
			dst.maxDepth	= src.maxDepth;
			_viewports.push_back( dst );

			VkRect2D		rect;
			rect.offset.x		= RoundToInt( src.rect.left );
			rect.offset.y		= RoundToInt( src.rect.top );
			rect.extent.width	= RoundToInt( src.rect.Width() );
			rect.extent.height	= RoundToInt( src.rect.Height() );
			_defaultScissors.push_back( rect );
		}

		// create default viewport
		if ( rp.viewports.empty() )
		{
			VkViewport		dst;
			dst.x			= float(rp.area.left);
			dst.y			= float(rp.area.top);
			dst.width		= float(rp.area.Width());
			dst.height		= float(rp.area.Height());
			dst.minDepth	= 0.0f;
			dst.maxDepth	= 1.0f;
			_viewports.push_back( dst );

			VkRect2D		rect;
			rect.offset.x		= rp.area.left;
			rect.offset.y		= rp.area.top;
			rect.extent.width	= rp.area.Width();
			rect.extent.height	= rp.area.Height();
			_defaultScissors.push_back( rect );
		}
		
		_OnCreate();
		return true;
	}
	
/*
=================================================
	destructor
=================================================
*/
	VLogicalRenderPass::~VLogicalRenderPass ()
	{
		SCOPELOCK( _rcCheck );

		for (auto& task : _drawTasks)
		{
			task->~IDrawTask();
		}
		_drawTasks.clear();
	}


}	// FG
