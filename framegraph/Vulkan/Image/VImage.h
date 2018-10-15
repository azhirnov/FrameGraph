// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Shared/ImageDataRange.h"
#include "framegraph/Public/LowLevel/ImageResource.h"
#include "framegraph/Shared/ImageViewHashMap.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Image
	//

	class VImage final : public ImageResource
	{
		friend class VFrameGraph;
		friend class VImageUnitTest;

	// types
	public:
		using ImageRange	= ImageDataRange;

		struct ImageState
		{
		// variables
			EResourceState		state;
			VkImageLayout		layout;
			VkImageAspectFlags	aspect;
			ImageRange			range;
			Task				task;
			
		// methods
			ImageState () {}

			ImageState (EResourceState state, VkImageLayout layout, const ImageRange &range, VkImageAspectFlags aspect, Task task) :
				state{state}, layout{layout}, aspect{aspect}, range{range}, task{task} {}
		};

	private:
		using SubRange	= ImageRange::SubRange_t;

		struct ImageBarrier
		{
		// variables
			SubRange				range;
			VkImageLayout			layout		= VK_IMAGE_LAYOUT_MAX_ENUM;
			VkPipelineStageFlags	stages		= 0;
			VkAccessFlags			access		= 0;
			ExeOrderIndex			index		= ExeOrderIndex::Initial;
			bool					isReadable : 1;
			bool					isWritable : 1;

		// methods
			ImageBarrier () : isReadable{false}, isWritable{false} {}
		};

		using ImageViewMap_t	= ImageViewHashMap< VkImageView >;
		using BarrierArray_t	= Array< ImageBarrier >;
		using DebugName_t		= StaticString<64>;


	// variables
	private:
		VkImage						_imageId;
		mutable ImageViewMap_t		_imageViewMap;
		VMemoryHandle				_memoryId;
		
		VkImageAspectFlags			_aspectMask;
		VkImageLayout				_currentLayout;

		BarrierArray_t				_pendingBarriers;
		BarrierArray_t				_readWriteBarriers;

		VFrameGraphWeak				_frameGraph;
		DebugName_t					_debugName;


	// methods
	private:
			bool			_CreateImage (const Description_t &desc, EMemoryTypeExt memType, VMemoryManager &alloc);
		ND_ VkImageView		_CreateImageView (const ImageViewDesc &desc) const;
			void			_Destroy ();
		
		ND_ static BarrierArray_t::iterator	_FindFirstBarrier (BarrierArray_t &arr, const SubRange &range);
			static void						_ReplaceBarrier (BarrierArray_t &arr, BarrierArray_t::iterator iter, const ImageBarrier &barrier);


	public:
		explicit VImage (const VFrameGraphWeak &fg);
		~VImage ();

		void AddPendingState (const ImageState &);
		void CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger = null);
		
		void SetDebugName (StringView name) FG_OVERRIDE;
		bool MakeImmutable (bool immutable) FG_OVERRIDE;

		ND_ ImagePtr		GetReal (Task, EResourceState)				FG_OVERRIDE		{ return shared_from_this(); }
		ND_ VkImage			GetImageID ()								const			{ return _imageId; }
		ND_ VMemoryHandle	GetMemory ()								const			{ return _memoryId; }
		ND_ VkImageView		GetImageView (const ImageViewDesc &desc)	const;
		ND_ StringView		GetDebugName ()								const			{ return _debugName; }
		ND_ VImagePtr		GetSelfShared ()											{ return std::static_pointer_cast<VImage>(shared_from_this()); }
	};
	

/*
=================================================
	GetImageView
=================================================
*/
	forceinline VkImageView  VImage::GetImageView (const ImageViewDesc &desc) const
	{
		auto	view = _imageViewMap.Find( desc );
				
		if ( view != VK_NULL_HANDLE )
			return view;

		return _CreateImageView( desc );
	}
	
/*
=================================================
	Cast
=================================================
*/
	template <>
	ND_ forceinline VImagePtr  Cast<VImage, ImageResource> (const ImagePtr &other)
	{
#		ifdef FG_DEBUG
			ASSERT( not other->IsLogical() );	// for logical resource use GetReal()
			return std::dynamic_pointer_cast<VImage>( other );
#		else
			return std::static_pointer_cast<VImage>( other );
#		endif
	}

}	// FG
