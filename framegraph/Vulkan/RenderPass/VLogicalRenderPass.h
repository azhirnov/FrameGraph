// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/RenderPassDesc.h"
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
			mutable VkImageLayout	_layout			= VK_IMAGE_LAYOUT_UNDEFINED;	// not hashed
			HashVal					_imageHash;		// used for fast render target comparison

			ColorTarget () {}
		};

		struct DepthStencilTarget : ColorTarget
		{
			DepthStencilTarget () {}
			DepthStencilTarget (const ColorTarget &ct) : ColorTarget{ct} {}

			ND_ bool IsDefined () const		{ return imageId.IsValid(); }
		};

		using ColorTargets_t			= FixedMap< RenderTargetID, ColorTarget, FG_MaxColorBuffers >;
		using Viewports_t				= FixedArray< VkViewport, FG_MaxViewports >;
		using Scissors_t				= FixedArray< VkRect2D, FG_MaxViewports >;
		using SRPalettePerViewport_t	= FixedArray< VkShadingRatePaletteNV, FG_MaxViewports >;
		using RS						= RenderState;
		using Allocator_t				= LinearAllocator< UntypedLinearAllocator<> >;
		

	// variables
	private:
		InPlace<Allocator_t>		_allocator;

		RawFramebufferID			_framebufferId;
		RawRenderPassID				_renderPassId;
		uint						_subpassIndex			= 0;

		// TODO: DOD
		Array< IDrawTask *>			_drawTasks;				// all draw tasks created with custom allocator in FrameGraph and
															// will be released after frame execution.
		
		ColorTargets_t				_colorTargets;
		DepthStencilTarget			_depthStencilTarget;

		Viewports_t					_viewports;
		Scissors_t					_defaultScissors;
		
		RS::ColorBuffersState		_colorState;
		RS::DepthBufferState		_depthState;
		RS::StencilBufferState		_stencilState;
		RS::RasterizationState		_rasterizationState;
		RS::MultisampleState		_multisampleState;

		RectI						_area;
		//bool						_parallelExecution		= true;
		bool						_canBeMerged			= true;
		bool						_isSubmited				= false;
		
		VPipelineResourceSet		_perPassResources;

		VLocalImage const*			_shadingRateImage		= null;
		ImageLayer					_shadingRateImageLayer;
		MipmapLevel					_shadingRateImageLevel;
		SRPalettePerViewport_t		_shadingRatePalette;
		
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


		template <typename DrawTaskType, typename ...Args>
		bool AddTask (Args&& ...args)
		{
			SCOPELOCK( _rcCheck );
			auto*	ptr = _allocator->Alloc<DrawTaskType>();
			_drawTasks.push_back( PlacementNew<DrawTaskType>( ptr, this, std::forward<Args&&>(args)... ));
			return true;
		}


		bool Submit ()
		{
			SCOPELOCK( _rcCheck );
			CHECK_ERR( not _isSubmited );
			//ASSERT( _drawTasks.size() );

			_isSubmited = true;
			return true;
		}
		
		bool GetShadingRateImage (OUT VLocalImage const* &, OUT ImageViewDesc &) const;

		ND_ bool								HasShadingRateImage ()		const	{ SHAREDLOCK( _rcCheck );  return _shadingRateImage != null; }

		ND_ ArrayView< IDrawTask *>				GetDrawTasks ()				const	{ SHAREDLOCK( _rcCheck );  return _drawTasks; }
		
		ND_ ColorTargets_t const&				GetColorTargets ()			const	{ SHAREDLOCK( _rcCheck );  return _colorTargets; }
		ND_ DepthStencilTarget const&			GetDepthStencilTarget ()	const	{ SHAREDLOCK( _rcCheck );  return _depthStencilTarget; }
		
		ND_ RectI const&						GetArea ()					const	{ SHAREDLOCK( _rcCheck );  return _area; }

		ND_ bool								IsSubmited ()				const	{ SHAREDLOCK( _rcCheck );  return _isSubmited; }
		ND_ bool								IsMergingAvailable ()		const	{ SHAREDLOCK( _rcCheck );  return _canBeMerged; }
		
		ND_ RawFramebufferID					GetFramebufferID ()			const	{ SHAREDLOCK( _rcCheck );  return _framebufferId; }
		ND_ RawRenderPassID						GetRenderPassID ()			const	{ SHAREDLOCK( _rcCheck );  return _renderPassId; }
		ND_ uint								GetSubpassIndex ()			const	{ SHAREDLOCK( _rcCheck );  return _subpassIndex; }

		ND_ ArrayView<VkViewport>				GetViewports ()				const	{ SHAREDLOCK( _rcCheck );  return _viewports; }
		ND_ ArrayView<VkRect2D>					GetScissors ()				const	{ SHAREDLOCK( _rcCheck );  return _defaultScissors; }
		ND_ ArrayView<VkShadingRatePaletteNV>	GetShadingRatePalette ()	const	{ SHAREDLOCK( _rcCheck );  return _shadingRatePalette; }
		
		ND_ RS::ColorBuffersState const&		GetColorState ()			const	{ SHAREDLOCK( _rcCheck );  return _colorState; }
		ND_ RS::DepthBufferState const&			GetDepthState ()			const	{ SHAREDLOCK( _rcCheck );  return _depthState; }
		ND_ RS::StencilBufferState const&		GetStencilState ()			const	{ SHAREDLOCK( _rcCheck );  return _stencilState; }
		ND_ RS::RasterizationState const&		GetRasterizationState ()	const	{ SHAREDLOCK( _rcCheck );  return _rasterizationState; }
		ND_ RS::MultisampleState const&			GetMultisampleState ()		const	{ SHAREDLOCK( _rcCheck );  return _multisampleState; }

		ND_ VPipelineResourceSet const&			GetResources ()				const	{ SHAREDLOCK( _rcCheck );  return _perPassResources; }
	};


}	// FG
