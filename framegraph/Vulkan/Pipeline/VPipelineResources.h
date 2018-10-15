// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/PipelineResources.h"
#include "VBuffer.h"
#include "VImage.h"
#include "VDescriptorSetLayout.h"

namespace FG
{

	//
	// Pipeline Resources
	//

	class VPipelineResources final : public PipelineResources
	{
		friend class VPipelineCache;

	// types
	public:
		static constexpr uint	INVALID_BINDING = ~0u;

		struct Buffer
		{
			uint				binding		= INVALID_BINDING;
			EResourceState		state		= Default;
			BufferPtr			buffer;
			VkDeviceSize		offset;
			VkDeviceSize		size;
		};

		struct Image
		{
			uint				binding		= INVALID_BINDING;
			EResourceState		state		= Default;
			ImagePtr			image;
			ImageViewDesc		desc;
			VkImageLayout		layout		= VK_IMAGE_LAYOUT_GENERAL;
		};

		struct Texture final : Image
		{
			VSamplerPtr			sampler;
		};

		struct Subpass final : Image
		{
			uint				attachmentIndex		= ~0u;
		};

		struct Sampler
		{
			uint				binding		= INVALID_BINDING;
			VSamplerPtr			sampler;
		};
		

		struct UpdateDescriptors
		{
		// variables
			static constexpr uint									maxResources = 64;
			FixedArray< VkDescriptorBufferInfo, maxResources >		buffers;
			FixedArray< VkDescriptorImageInfo, maxResources >		images;
			FixedArray< VkBufferView, maxResources >				bufferViews;
			FixedArray< VkWriteDescriptorSet, maxResources >		descriptors;

		// methods
			UpdateDescriptors () {}

			void AddBuffer (const Buffer &buf, VkBuffer id, VkDescriptorSet ds);
			void AddImage (const Image &img, VkImageView id, VkDescriptorSet ds);
			void AddTexture (const Texture &tex, VkImageView img, VkSampler samp, VkDescriptorSet ds);
			void AddSampler (const Sampler &samp, VkSampler id, VkDescriptorSet ds);
		};


	private:
#		if FG_IS_VIRTUAL
		using Base_t		= PipelineResources;
#		else
		using Base_t		= VPipelineResources;
#		endif

		using Resource_t	= Union< std::monostate, Buffer, Image, Texture, Sampler, Subpass >;
		using ResourceSet_t	= Array< Resource_t >;


	// variables
	private:
		VkDescriptorSet				_descriptorSet;
		VDescriptorSetLayoutPtr		_layout;
		IDescriptorPoolPtr			_descriptorPool;
		ResourceSet_t				_resources;
		bool						_hasLogical;


	// methods
	private:
		template <typename T>  ND_ T const&  _GetRes (const UniformID &id) const;

		ND_ bool _IsCreated () const;

		bool _Create (const IDescriptorPoolPtr &pool, VkDescriptorSet descriptorSet);
		void _Destroy ();


	public:
		explicit VPipelineResources (const VDescriptorSetLayoutPtr &layout);
		~VPipelineResources ();

		Base_t&	BindImage (const UniformID &id, const ImagePtr &image)								FG_OVERRIDE;
		Base_t&	BindImage (const UniformID &id, const ImagePtr &image, const ImageViewDesc &desc)	FG_OVERRIDE;

		Base_t&	BindTexture (const UniformID &id, const ImagePtr &image, const SamplerPtr &sampler)								FG_OVERRIDE;
		Base_t&	BindTexture (const UniformID &id, const ImagePtr &image, const SamplerPtr &sampler, const ImageViewDesc &desc)	FG_OVERRIDE;

		Base_t&	BindSampler (const UniformID &id, const SamplerPtr &sampler) FG_OVERRIDE;

		Base_t&	BindBuffer (const UniformID &id, const BufferPtr &buffer)								FG_OVERRIDE;
		Base_t&	BindBuffer (const UniformID &id, const BufferPtr &buffer, BytesU offset, BytesU size)	FG_OVERRIDE;

		ND_ DescriptorSetLayoutPtr  GetLayout ()			const FG_OVERRIDE	{ return Cast<DescriptorSetLayout>( _layout ); }

		ND_ VkDescriptorSet			GetDescriptorSetID ()	const				{ return _descriptorSet; }

		ND_ bool					HasLogicalResources ()	const				{ return _hasLogical; }

		ND_ ArrayView<Resource_t>	GetUniforms ()			const				{ return _resources; }
	};


}	// FG
