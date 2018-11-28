// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphDrawTask.h"
#include "framegraph/Public/EResourceState.h"
#include "VDrawTask.h"

namespace FG
{

	//
	// Vulkan Logical Render Pass
	//

	class VLogicalRenderPass final
	{
		friend class VRenderPassCache;

	// types
	public:
		struct ColorTarget
		{
			RawImageID				imageId;
			VLocalImage const*		imagePtr		= null;
			ImageViewDesc			desc;
			VkSampleCountFlagBits	samples			= VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
			VkClearValue			clearValue;
			VkAttachmentLoadOp		loadOp			= VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
			VkAttachmentStoreOp		storeOp			= VK_ATTACHMENT_STORE_OP_MAX_ENUM;
			EResourceState			state			= Default;
			HashVal					_imageHash;		// used for fast render target comparison

			ColorTarget () {}
		};

		struct DepthStencilTarget : ColorTarget
		{
			DepthStencilTarget () {}
			DepthStencilTarget (const ColorTarget &ct) : ColorTarget{ct} {}

			ND_ bool IsDefined () const		{ return imageId.IsValid(); }
		};

		using ColorTargets_t	= FixedMap< RenderTargetID, ColorTarget, FG_MaxColorBuffers >;
		using Viewports_t		= FixedArray< VkViewport, FG_MaxViewports >;
		using Scissors_t		= FixedArray< VkRect2D, FG_MaxViewports >;
		

	// variables
	private:
		RawFramebufferID			_framebufferId;
		RawRenderPassID				_renderPassId;
		uint						_subpassIndex		= 0;

		// TODO: DOD
		Array< IDrawTask *>			_drawTasks;			// all draw tasks created with custom allocator in FrameGraph and
														// will be released after frame execution.
		
		ColorTargets_t				_colorTargets;
		DepthStencilTarget			_depthStencilTarget;

		Viewports_t					_viewports;
		Scissors_t					_defaultScissors;

		RectI						_area;
		//bool						_parallelExecution	= true;
		bool						_canBeMerged		= true;
		bool						_isSubmited			= false;
		
		RWRaceConditionCheck		_rcCheck;


	// methods
	private:
		void _SetRenderPass (const RawRenderPassID &rp, uint subpass, const RawFramebufferID &fb)
		{
			_framebufferId	= fb;
			_renderPassId	= rp;
			_subpassIndex	= subpass;
		}


	public:
		VLogicalRenderPass () {}
		VLogicalRenderPass (VLogicalRenderPass &&) = delete;
		~VLogicalRenderPass ();

		bool Create (VResourceManagerThread &rm, const RenderPassDesc &rp);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);


		bool AddTask (IDrawTask *task)
		{
			SCOPELOCK( _rcCheck );
			_drawTasks.push_back( std::move(task) );
			return true;
		}


		bool Submit ()
		{
			SCOPELOCK( _rcCheck );
			CHECK_ERR( not _isSubmited );

			_isSubmited = true;
			return true;
		}


		ND_ ArrayView< IDrawTask *>		GetDrawTasks ()				const	{ SHAREDLOCK( _rcCheck );  return _drawTasks; }
		
		ND_ ColorTargets_t const&		GetColorTargets ()			const	{ SHAREDLOCK( _rcCheck );  return _colorTargets; }
		ND_ DepthStencilTarget const&	GetDepthStencilTarget ()	const	{ SHAREDLOCK( _rcCheck );  return _depthStencilTarget; }
		
		ND_ RectI const&				GetArea ()					const	{ SHAREDLOCK( _rcCheck );  return _area; }

		ND_ bool						IsSubmited ()				const	{ SHAREDLOCK( _rcCheck );  return _isSubmited; }

		ND_ bool						IsMergingAvailable ()		const	{ SHAREDLOCK( _rcCheck );  return _canBeMerged; }
		
		ND_ RawFramebufferID			GetFramebufferID ()			const	{ return _framebufferId; }
		ND_ RawRenderPassID				GetRenderPassID ()			const	{ return _renderPassId; }
		ND_ uint						GetSubpassIndex ()			const	{ return _subpassIndex; }

		ND_ ArrayView<VkViewport>		GetViewports ()				const	{ return _viewports; }
		ND_ ArrayView<VkRect2D>			GetScissors ()				const	{ return _defaultScissors; }
	};


}	// FG
