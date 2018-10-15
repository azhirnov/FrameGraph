// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphDrawTask.h"
#include "framegraph/Shared/LogicalRenderPass.h"
#include "VDrawTask.h"
#include "VRenderPass.h"

namespace FG
{

	//
	// Vulkan Logical Render Pass
	//

	class VLogicalRenderPass final : public LogicalRenderPass
	{
		friend class VRenderPassCache;

	// types
	public:
		struct ColorTarget
		{
			ImagePtr			image;
			ImageViewDesc		desc;
			VkClearValue		clearValue;
			VkAttachmentLoadOp	loadOp;
			VkAttachmentStoreOp	storeOp;
			EResourceState		state;
			HashVal				_imageHash;		// used for fast render target comparison

			ColorTarget () {}
		};

		struct DepthStencilTarget : ColorTarget
		{
			DepthStencilTarget () {}
			DepthStencilTarget (const ColorTarget &ct) : ColorTarget{ct} {}

			ND_ bool IsDefined () const		{ return image != null; }
		};

		using ColorTargets_t	= FixedMap< RenderTargetID, ColorTarget, FG_MaxColorBuffers >;
		using Viewports_t		= FixedArray< VkViewport, FG_MaxViewports >;
		using Scissors_t		= FixedArray< VkRect2D, FG_MaxViewports >;
		

	// variables
	private:
		VFramebufferPtr				_framebuffer;
		VRenderPassPtr				_renderPass;
		uint						_subpassIndex;

		Array< IDrawTask *>			_drawTasks;			// all draw tasks created with custom allocator in FrameGraph and
														// will be released after frame execution.
		
		ColorTargets_t				_colorTargets;
		DepthStencilTarget			_depthStencilTarget;

		Viewports_t					_viewports;
		Scissors_t					_defaultScissors;

		RectI						_area;
		bool						_parallelExecution	= true;
		bool						_canBeMerged		= true;
		bool						_isSubmited			= false;


	// methods
	private:
		void _SetRenderPass (const VRenderPassPtr &rp, uint subpass, const VFramebufferPtr &fb)
		{
			_framebuffer	= fb;
			_renderPass		= rp;
			_subpassIndex	= subpass;
		}


	public:
		explicit VLogicalRenderPass (const RenderPassDesc &rp);
		~VLogicalRenderPass ();


		bool AddTask (IDrawTask *task)
		{
			_drawTasks.push_back( std::move(task) );
			return true;
		}


		bool Submit ()
		{
			CHECK_ERR( not _isSubmited );

			_isSubmited = true;
			return true;
		}


		ND_ ArrayView< IDrawTask *>		GetDrawTasks ()				const	{ return _drawTasks; }
		
		ND_ ColorTargets_t const&		GetColorTargets ()			const	{ return _colorTargets; }
		ND_ DepthStencilTarget const&	GetDepthStencilTarget ()	const	{ return _depthStencilTarget; }
		
		ND_ RectI const&				GetArea ()					const	{ return _area; }

		ND_ bool						IsSubmited ()				const	{ return _isSubmited; }

		ND_ bool						IsMergingAvailable ()		const	{ return _canBeMerged; }

		//ND_ bool						IsParallelDrawingSupported () const	{ return _parallelExecution; }

		ND_ VRenderPassPtr const&		GetRenderPass ()			const	{ return _renderPass; }
		ND_ uint						GetSubpassIndex ()			const	{ return _subpassIndex; }
		ND_ VFramebufferPtr const&		GetFramebuffer ()			const	{ return _framebuffer; }

		ND_ ArrayView<VkViewport>		GetViewports ()				const	{ return _viewports; }
		ND_ ArrayView<VkRect2D>			GetScissors ()				const	{ return _defaultScissors; }
	};


}	// FG
