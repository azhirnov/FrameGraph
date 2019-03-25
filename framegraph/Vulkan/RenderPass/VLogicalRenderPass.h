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
	// types
	public:
		struct ColorTarget
		{
			uint					index			= Default;
			RawImageID				imageId;
			VLocalImage const*		imagePtr		= null;
			ImageViewDesc			desc;
			VkSampleCountFlagBits	samples			= VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
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
		
		using VkClearValues_t			= StaticArray< VkClearValue, FG_MaxColorBuffers+1 >;
		using ColorTargets_t			= FixedArray< ColorTarget, FG_MaxColorBuffers >;
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
		VkClearValues_t				_clearValues;

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


	// methods
	public:
		VLogicalRenderPass () {}
		VLogicalRenderPass (VLogicalRenderPass &&) = delete;
		~VLogicalRenderPass ();

		bool Create (VCommandBuffer &, const RenderPassDesc &);
		void Destroy (VResourceManager &);


		template <typename DrawTaskType, typename ...Args>
		bool AddTask (Args&& ...args)
		{
			auto*	ptr = _allocator->Alloc<DrawTaskType>();
			_drawTasks.push_back( PlacementNew<DrawTaskType>( ptr, *this, std::forward<Args&&>(args)... ));
			return true;
		}


		bool Submit ()
		{
			CHECK_ERR( not _isSubmited );
			//ASSERT( _drawTasks.size() );

			_isSubmited = true;
			return true;
		}

		void _SetRenderPass (RawRenderPassID rp, uint subpass, RawFramebufferID fb, uint depthIndex)
		{
			_framebufferId	= fb;
			_renderPassId	= rp;
			_subpassIndex	= subpass;
			
			_clearValues[_depthStencilTarget.index]	= _clearValues[depthIndex];
			_depthStencilTarget.index				= depthIndex;
		}
		
		bool GetShadingRateImage (OUT VLocalImage const* &, OUT ImageViewDesc &) const;

		ND_ bool								HasShadingRateImage ()		const	{ return _shadingRateImage != null; }

		ND_ ArrayView< IDrawTask *>				GetDrawTasks ()				const	{ return _drawTasks; }
		
		ND_ ColorTargets_t const&				GetColorTargets ()			const	{ return _colorTargets; }
		ND_ DepthStencilTarget const&			GetDepthStencilTarget ()	const	{ return _depthStencilTarget; }
		ND_ ArrayView< VkClearValue >			GetClearValues ()			const	{ return _clearValues; }
		
		ND_ RectI const&						GetArea ()					const	{ return _area; }

		ND_ bool								IsSubmited ()				const	{ return _isSubmited; }
		ND_ bool								IsMergingAvailable ()		const	{ return _canBeMerged; }
		
		ND_ RawFramebufferID					GetFramebufferID ()			const	{ return _framebufferId; }
		ND_ RawRenderPassID						GetRenderPassID ()			const	{ return _renderPassId; }
		ND_ uint								GetSubpassIndex ()			const	{ return _subpassIndex; }

		ND_ ArrayView<VkViewport>				GetViewports ()				const	{ return _viewports; }
		ND_ ArrayView<VkRect2D>					GetScissors ()				const	{ return _defaultScissors; }
		ND_ ArrayView<VkShadingRatePaletteNV>	GetShadingRatePalette ()	const	{ return _shadingRatePalette; }
		
		ND_ RS::ColorBuffersState const&		GetColorState ()			const	{ return _colorState; }
		ND_ RS::DepthBufferState const&			GetDepthState ()			const	{ return _depthState; }
		ND_ RS::StencilBufferState const&		GetStencilState ()			const	{ return _stencilState; }
		ND_ RS::RasterizationState const&		GetRasterizationState ()	const	{ return _rasterizationState; }
		ND_ RS::MultisampleState const&			GetMultisampleState ()		const	{ return _multisampleState; }

		ND_ VPipelineResourceSet const&			GetResources ()				const	{ return _perPassResources; }
	};


}	// FG
