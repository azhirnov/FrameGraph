// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VRenderPass.h"
#include "VFramebuffer.h"

namespace FG
{

	//
	// Vulkan Render Pass Cache
	//

	class VRenderPassCache final
	{
	// types
	private:
		struct RenderPassItem
		{
			VRenderPassPtr	ptr;

			RenderPassItem () {}
			explicit RenderPassItem (const VRenderPassPtr &p) : ptr{p} {}
			
			ND_ bool  operator == (const RenderPassItem &rhs) const	{ return *ptr == *rhs.ptr; }
		};

		struct RenderPassHash {
			ND_ size_t  operator () (const RenderPassItem &value) const noexcept	{ return size_t(value.ptr->_hash); }
		};


		struct RenderPassAttachmentItem
		{
			VRenderPassPtr	ptr;

			RenderPassAttachmentItem () {}
			explicit RenderPassAttachmentItem (const VRenderPassPtr &p) : ptr{p} {}
			
			ND_ bool  operator == (const RenderPassAttachmentItem &rhs) const;
		};

		struct RenderPassAttachmentHash {
			ND_ size_t  operator () (const RenderPassAttachmentItem &value) const noexcept	{ return size_t(value.ptr->_attachmentHash); }
		};


		struct FramebufferItem
		{
			VFramebufferPtr		ptr;

			FramebufferItem () {}
			explicit FramebufferItem (const VFramebufferPtr &p) : ptr{p} {}

			ND_ bool  operator == (const FramebufferItem &rhs) const	{ return *ptr == *rhs.ptr; }
		};

		struct FramebufferHash {
			ND_ size_t  operator () (const FramebufferItem &value) const noexcept	{ return size_t(value.ptr->_hash); }
		};

		using RenderPassMap_t	= HashSet< RenderPassItem, RenderPassHash >;
		using FramebufferMap_t	= HashSet< FramebufferItem, FramebufferHash >;
		using FragmentOutput	= GraphicsPipelineDesc::FragmentOutput;


	// variables
	private:
		VDevice const&		_device;

		RenderPassMap_t		_renderPassCache;
		FramebufferMap_t	_framebufferCache;


	// methods
	public:
		explicit VRenderPassCache (const VDevice &dev);
		~VRenderPassCache ();

		bool Initialize ();
		void Deinitialize ();

		bool CreateRenderPasses (ArrayView<VLogicalRenderPass*> logicalPasses, ArrayView<FragmentOutput> fragOutput);

	private:
		ND_ VRenderPassPtr   _CreateRenderPass (ArrayView<VLogicalRenderPass*> logicalPasses, ArrayView<FragmentOutput> fragOutput);
		ND_ VFramebufferPtr  _CreateFramebuffer (ArrayView<VLogicalRenderPass*> logicalPasses, const VRenderPassPtr &renderPass);
	};

}	// FG
