// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VLogicalRenderPass.h"
#include "VCommandBuffer.h"
#include "VEnumCast.h"

namespace FG
{
namespace {
	static const VkShadingRatePaletteEntryNV	shadingRateDefaultEntry	= VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV;
}

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
			[&result] (const NullUnion &)		{ memset( &result, 0, sizeof(result) ); }
		);
	}
//-----------------------------------------------------------------------------



/*
=================================================
	Create
=================================================
*/
	bool VLogicalRenderPass::Create (VCommandBuffer &fgThread, const RenderPassDesc &desc)
	{
		const bool	enable_sri	= desc.shadingRate.image.IsValid();

		_allocator.Create( fgThread.GetAllocator() );
		_allocator->SetBlockSize( 4_Kb );
		
		_colorState			= desc.colorState;
		_depthState			= desc.depthState;
		_stencilState		= desc.stencilState;
		_rasterizationState	= desc.rasterizationState;
		_multisampleState	= desc.multisampleState;
		_area				= desc.area;
		
		Optional<MultiSamples>	samples;


		// copy descriptor sets
		size_t	offset_count = 0;
		for (auto& src : desc.perPassResources)
		{
			auto	offsets = src.second->GetDynamicOffsets();

			_perPassResources.resources.emplace_back( src.first, fgThread.CreateDescriptorSet( *src.second ),
													  uint(offset_count), uint(offsets.size()) );
			
			for (size_t i = 0; i < offsets.size(); ++i, ++offset_count) {
				_perPassResources.dynamicOffsets.push_back( offsets[i] );
			}
		}

		// copy render targets
		for (size_t i = 0; i < desc.renderTargets.size(); ++i)
		{
			const auto&		src = desc.renderTargets[i];
			ColorTarget		dst;

			if ( not src.image )
				continue;

			dst.imagePtr	= fgThread.ToLocal( src.image );
			CHECK_ERR( dst.imagePtr );

			if ( src.desc.has_value() ) {
				dst.desc = *src.desc;
				dst.desc.Validate( dst.imagePtr->Description() );
			}else
				dst.desc = ImageViewDesc{dst.imagePtr->Description()};

			dst.imageId		= src.image;
			dst.samples		= VEnumCast( dst.imagePtr->Description().samples );
			dst.loadOp		= VEnumCast( src.loadOp );
			dst.storeOp		= VEnumCast( src.storeOp );
			dst.state		= EResourceState::Unknown;
			dst.index		= uint(i);
			dst._imageHash	= HashOf( dst.imageId ) + HashOf( dst.desc );

			ConvertClearValue( src.clearValue, OUT _clearValues[i] );

			// validate image view description
			if ( dst.desc.format == EPixelFormat::Unknown )
				dst.desc.format = dst.imagePtr->Description().format;

			if ( samples.has_value() )
				CHECK_ERR( *samples == dst.imagePtr->Description().samples )
			else
				samples = dst.imagePtr->Description().samples;

			// add resource state flags
			if ( desc.area.left == 0 and desc.area.right  == int(dst.imagePtr->Width())  and
				 desc.area.top  == 0 and desc.area.bottom == int(dst.imagePtr->Height()) )
			{
				if ( src.loadOp  == EAttachmentLoadOp::Clear or src.loadOp  == EAttachmentLoadOp::Invalidate )
					dst.state |= EResourceState::InvalidateBefore;

				if ( src.storeOp == EAttachmentStoreOp::Invalidate )
					dst.state |= EResourceState::InvalidateAfter;
			}

			// add color or depth-stencil render target
			if ( EPixelFormat_HasDepthOrStencil( dst.desc.format ) )
			{
				ASSERT( RenderTargetID(i) == RenderTargetID::DepthStencil );
				
				dst.state |= EResourceState::DepthStencilAttachmentReadWrite;	// TODO: add support for other layouts

				_depthStencilTarget = DepthStencilTarget{ dst };
			}
			else
			{
				dst.state |= EResourceState::ColorAttachmentReadWrite;		// TODO: remove 'Read' state if blending disabled or 'loadOp' is 'Clear'

				_colorTargets.push_back( dst );
			}
		}

		// validate image samples
		if ( samples.has_value() )
		{
			if ( _multisampleState.samples != *samples )
			{
				//FG_LOGD( "Render pass attachment sample count was changed, sample mask updated" );

				for (auto& mask : _multisampleState.sampleMask) {
					mask = UMax;
				}
				_multisampleState.samples = *samples;
			}
		}

		// create viewports and default scissors
		for (auto& src : desc.viewports)
		{
			ASSERT( src.rect.left >= float(desc.area.left) and src.rect.right  <= float(desc.area.right)  );
			ASSERT( src.rect.top  >= float(desc.area.top)  and src.rect.bottom <= float(desc.area.bottom) );

			VkViewport		dst;
			dst.x			= src.rect.left;
			dst.y			= src.rect.top;
			dst.width		= src.rect.Width();
			dst.height		= src.rect.Height();
			dst.minDepth	= src.minDepth;
			dst.maxDepth	= src.maxDepth;
			_viewports.push_back( dst );

			// scissor
			VkRect2D		rect;
			rect.offset.x		= RoundToInt( src.rect.left );
			rect.offset.y		= RoundToInt( src.rect.top );
			rect.extent.width	= RoundToInt( src.rect.Width() );
			rect.extent.height	= RoundToInt( src.rect.Height() );
			_defaultScissors.push_back( rect );

			// shading rate palette
			if ( enable_sri )
			{
				VkShadingRatePaletteNV			palette = {};
				palette.shadingRatePaletteEntryCount	= Max( 1u, uint(src.palette.size()) );
				palette.pShadingRatePaletteEntries		= &shadingRateDefaultEntry;

				if ( src.palette.size() )
				{
					auto*	entries = _allocator->Alloc<VkShadingRatePaletteEntryNV>( src.palette.size() );
					palette.pShadingRatePaletteEntries = entries;

					for (uint i = 0; i < src.palette.size(); ++i) {
						entries[i] = VEnumCast( src.palette[i] );
					}
				}
				_shadingRatePalette.push_back( palette );
			}
		}

		// create default viewport
		if ( desc.viewports.empty() )
		{
			VkViewport		dst;
			dst.x			= float(desc.area.left);
			dst.y			= float(desc.area.top);
			dst.width		= float(desc.area.Width());
			dst.height		= float(desc.area.Height());
			dst.minDepth	= 0.0f;
			dst.maxDepth	= 1.0f;
			_viewports.push_back( dst );

			VkRect2D		rect;
			rect.offset.x		= desc.area.left;
			rect.offset.y		= desc.area.top;
			rect.extent.width	= desc.area.Width();
			rect.extent.height	= desc.area.Height();
			_defaultScissors.push_back( rect );
		}

		// set shading rate image
		if ( enable_sri )
		{
			_shadingRateImage		= fgThread.ToLocal( desc.shadingRate.image );
			_shadingRateImageLayer	= desc.shadingRate.layer;
			_shadingRateImageLevel	= desc.shadingRate.mipmap;
		}
		
		return true;
	}
	
/*
=================================================
	GetShadingRateImage
=================================================
*/
	bool VLogicalRenderPass::GetShadingRateImage (OUT VLocalImage const* &outImage, OUT ImageViewDesc &outDesc) const
	{
		if ( not _shadingRateImage )
			return false;

		outImage			= _shadingRateImage;

		outDesc.viewType	= EImage::Tex2D;
		outDesc.format		= EPixelFormat::R8U;
		outDesc.baseLevel	= _shadingRateImageLevel;
		outDesc.baseLayer	= _shadingRateImageLayer;
		outDesc.aspectMask	= EImageAspect::Color;

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VLogicalRenderPass::Destroy (VResourceManager &)
	{
		_drawTasks.clear();

		_allocator.Destroy();

		_shadingRateImage = null;
	}
	
/*
=================================================
	Submit
=================================================
*/
	bool VLogicalRenderPass::Submit (VCommandBuffer &fgThread,
									 ArrayView<Pair<RawImageID, EResourceState>> images,
									 ArrayView<Pair<RawBufferID, EResourceState>> buffers)
	{
		CHECK_ERR( not _isSubmited );
		//ASSERT( _drawTasks.size() );
		
		if ( images.size() )
		{
			auto*	img_ptr	= _allocator->Alloc< MutableImages_t::value_type >( images.size() );
			
			for (size_t i = 0; i < images.size(); ++i) {
				img_ptr[i] = { fgThread.ToLocal( images[i].first ), images[i].second };
			}
			_mutableImages = { img_ptr, images.size() };
		}

		if ( buffers.size() )
		{
			auto*	buf_ptr	= _allocator->Alloc< MutableBuffers_t::value_type >( buffers.size() );
			
			for (size_t i = 0; i < buffers.size(); ++i) {
				buf_ptr[i] = { fgThread.ToLocal( buffers[i].first ), buffers[i].second };
			}
			_mutableBuffers = { buf_ptr, buffers.size() };
		}

		_isSubmited = true;
		return true;
	}
	
/*
=================================================
	_SetRenderPass
=================================================
*/
	void VLogicalRenderPass::_SetRenderPass (RawRenderPassID rp, uint subpass, RawFramebufferID fb, uint depthIndex)
	{
		_framebufferId	= fb;
		_renderPassId	= rp;
		_subpassIndex	= subpass;
			
		_clearValues[depthIndex]	= _clearValues[_depthStencilTarget.index];
		_depthStencilTarget.index	= depthIndex;
	}

/*
=================================================
	destructor
=================================================
*/
	VLogicalRenderPass::~VLogicalRenderPass ()
	{
		ASSERT( _drawTasks.empty() );
	}
	
/*
=================================================
	_SetShaderDebugIndex
=================================================
*/
	void VLogicalRenderPass::_SetShaderDebugIndex (ShaderDbgIndex id)
	{
		for (auto* draw : _drawTasks)
		{
			draw->debugModeIndex = id;
		}
	}

}	// FG
