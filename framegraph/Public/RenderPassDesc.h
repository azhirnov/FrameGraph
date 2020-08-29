// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/RenderState.h"
#include "framegraph/Public/ImageDesc.h"
#include "framegraph/Public/PipelineResources.h"

namespace FG
{

	//
	// Render Pass Description
	//
	struct RenderPassDesc
	{
	// types
		using ClearValue_t			= Union< NullUnion, RGBA32f, RGBA32u, RGBA32i, DepthStencil >;
		using ShadingRatePalette_t	= FixedArray< EShadingRatePalette, uint(EShadingRatePalette::_Count) >;
		
		struct RT
		{
			RawImageID					image;		// may be image module in initial state (created by CreateRenderTarget or other)
			Optional< ImageViewDesc >	desc;		// may be used to specialize level, layer, different format, layout, ...
			ClearValue_t				clearValue;	// default is black color
			EAttachmentLoadOp			loadOp		= EAttachmentLoadOp::Load;
			EAttachmentStoreOp			storeOp		= EAttachmentStoreOp::Store;
		};

		struct Viewport
		{
			RectF					rect;
			float					minDepth	= 0.0f;
			float					maxDepth	= 1.0f;
			ShadingRatePalette_t	palette;
		};

		using Targets_t		= StaticArray< RT, FG_MaxColorBuffers+1 >;
		using Viewports_t	= FixedArray< Viewport, FG_MaxViewports >;
		using RS			= RenderState;
		
		struct ShaderDebugMode
		{
			EShaderDebugMode	mode		= Default;
			EShaderStages		stages		= Default;
			short2				fragCoord	{ std::numeric_limits<short>::min() };
			RawImageID			dstImage;	// for timemap
		};


	// variables
		RS::ColorBuffersState		colorState;
		RS::DepthBufferState		depthState;
		RS::StencilBufferState		stencilState;
		RS::RasterizationState		rasterizationState;
		RS::MultisampleState		multisampleState;

		struct {
			RawImageID					image;
			ImageLayer					layer;
			MipmapLevel					mipmap;
			// TODO: coarse sample order
		}							shadingRate;

		Targets_t					renderTargets;
		Viewports_t					viewports;
		RectI						area;
		
		PipelineResourceSet			perPassResources;	// this resources will be added for all draw tasks

		ShaderDebugMode				debugMode;

		//bool						useSecondaryCmdbuf	= false;	// CPU optimization

		//bool						parallelExecution	= true;		// (optimization) if 'false' all draw and compute tasks will be executed in initial order
		//bool						canBeMerged			= true;		// (optimization) g-buffer render passes can be merged, but don't merge conditional passes
		// TODO: push constants, specialization constants


	// methods
		RenderPassDesc () {}

		explicit RenderPassDesc (const RectI &rect) : area{rect}
		{
			ASSERT( area.IsValid() );
		}

		explicit RenderPassDesc (const int2 &size) : RenderPassDesc{ RectI{int2(), size} }
		{}


		// render target
		RenderPassDesc&  AddTarget (RenderTargetID id, RawImageID image);
		RenderPassDesc&  AddTarget (RenderTargetID id, RawImageID image, EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp);
		
		template <typename ClearVal>
		RenderPassDesc&  AddTarget (RenderTargetID id, RawImageID image, const ClearVal &clearValue, EAttachmentStoreOp storeOp);
		RenderPassDesc&  AddTarget (RenderTargetID id, RawImageID image, const ImageViewDesc &desc, EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp);

		template <typename ClearVal>
		RenderPassDesc&  AddTarget (RenderTargetID id, RawImageID image, const ImageViewDesc &desc, const ClearVal &clearValue, EAttachmentStoreOp storeOp);


		// viewport
		template <typename T>
		RenderPassDesc&  AddViewport (const Rectangle<T> &rect, float minDepth = 0.0f, float maxDepth = 1.0f, ArrayView<EShadingRatePalette> palette = Default);
		
		template <typename T>
		RenderPassDesc&  AddViewport (const Vec<T,2> &size, float minDepth = 0.0f, float maxDepth = 1.0f, ArrayView<EShadingRatePalette> palette = Default);
		

		// color
		RenderPassDesc&  AddColorBuffer (RenderTargetID id, EBlendFactor srcBlendFactor, EBlendFactor dstBlendFactor, EBlendOp blendOp, bool4 colorMask = bool4{true});
		RenderPassDesc&  AddColorBuffer (RenderTargetID id, EBlendFactor srcBlendFactorColor, EBlendFactor srcBlendFactorAlpha,
										 EBlendFactor dstBlendFactorColor, EBlendFactor dstBlendFactorAlpha,
										 EBlendOp blendOpColor, EBlendOp blendOpAlpha, bool4 colorMask = bool4{true});
		RenderPassDesc&  AddColorBuffer (RenderTargetID id, bool4 colorMask = bool4{true}); // without blending
		RenderPassDesc&  SetLogicOp (ELogicOp value);
		RenderPassDesc&  SetBlendColor (const RGBA32f &value);

		// depth
		RenderPassDesc&  SetDepthTestEnabled (bool value);
		RenderPassDesc&  SetDepthWriteEnabled (bool value);
		RenderPassDesc&  SetDepthCompareOp (ECompareOp value);
		RenderPassDesc&  SetDepthBounds (float min, float max);
		RenderPassDesc&  SetDepthBoundsEnabled (bool value);

		// stencil
		RenderPassDesc&  SetStencilTestEnabled (bool value);
		RenderPassDesc&  SetStencilFrontFaceFailOp (EStencilOp value);
		RenderPassDesc&  SetStencilFrontFaceDepthFailOp (EStencilOp value);
		RenderPassDesc&  SetStencilFrontFacePassOp (EStencilOp value);
		RenderPassDesc&  SetStencilFrontFaceCompareOp (ECompareOp value);
		RenderPassDesc&  SetStencilFrontFaceReference (uint value);
		RenderPassDesc&  SetStencilFrontFaceWriteMask (uint value);
		RenderPassDesc&  SetStencilFrontFaceCompareMask (uint value);
		RenderPassDesc&  SetStencilBackFaceFailOp (EStencilOp value);
		RenderPassDesc&  SetStencilBackFaceDepthFailOp (EStencilOp value);
		RenderPassDesc&  SetStencilBackFacePassOp (EStencilOp value);
		RenderPassDesc&  SetStencilBackFaceCompareOp (ECompareOp value);
		RenderPassDesc&  SetStencilBackFaceReference (uint value);
		RenderPassDesc&  SetStencilBackFaceWriteMask (uint value);
		RenderPassDesc&  SetStencilBackFaceCompareMask (uint value);

		// rasterization
		RenderPassDesc&  SetPolygonMode (EPolygonMode value);
		RenderPassDesc&  SetLineWidth (float value);
		RenderPassDesc&  SetDepthBiasConstFactor (float value);
		RenderPassDesc&  SetDepthBiasClamp (float value);
		RenderPassDesc&  SetDepthBiasSlopeFactor (float value);
		RenderPassDesc&  SetDepthBias (float constFactor, float clamp, float slopeFactor);
		RenderPassDesc&  SetDepthBiasEnabled (bool value);
		RenderPassDesc&  SetDepthClampEnabled (bool value);
		RenderPassDesc&  SetRasterizerDiscard (bool value);
		RenderPassDesc&  SetCullMode (ECullMode value);
		RenderPassDesc&  SetFrontFaceCCW (bool value);
		
		// multisample
		RenderPassDesc&  SetSampleMask (ArrayView<uint> value);
		RenderPassDesc&  SetMultiSamples (MultiSamples value);
		RenderPassDesc&  SetMultiSamples (MultiSamples value, ArrayView<uint> mask);
		RenderPassDesc&  SetMinSampleShading (float value);
		RenderPassDesc&  SetSampleShadingEnabled (bool value);
		RenderPassDesc&  SetAlphaToCoverageEnabled (bool value);
		RenderPassDesc&  SetAlphaToOneEnabled (bool value);

		RenderPassDesc&  SetShadingRateImage (RawImageID image, ImageLayer layer = Default, MipmapLevel level = Default);
		
		RenderPassDesc&  AddResources (const DescriptorSetID &id, const PipelineResources *res);

		// debugging
		RenderPassDesc&  EnableShaderTrace (EShaderStages stages);
		RenderPassDesc&  EnableFragmentShaderTrace (int x, int y);
		RenderPassDesc&  EnableShaderTimemap (RawImageID target, EShaderStages stages = EShaderStages::Fragment);
	};


	


	inline RenderPassDesc&  RenderPassDesc::AddTarget (RenderTargetID id, RawImageID image)
	{
		return AddTarget( id, image, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store );
	}

	inline RenderPassDesc&  RenderPassDesc::AddTarget (RenderTargetID id, RawImageID image, EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp)
	{
		ASSERT( image );
		ASSERT( loadOp != EAttachmentLoadOp::Clear );	// clear value is not defined
		renderTargets[ uint(id) ] = RT{ image, {}, ClearValue_t{}, loadOp, storeOp };
		return *this;
	}
		
	template <typename ClearVal>
	inline RenderPassDesc&  RenderPassDesc::AddTarget (RenderTargetID id, RawImageID image, const ClearVal &clearValue, EAttachmentStoreOp storeOp)
	{
		ASSERT( image );
		renderTargets[ uint(id) ] = RT{ image, {}, clearValue, EAttachmentLoadOp::Clear, storeOp };
		return *this;
	}
		
	inline RenderPassDesc&  RenderPassDesc::AddTarget (RenderTargetID id, RawImageID image, const ImageViewDesc &desc, EAttachmentLoadOp loadOp, EAttachmentStoreOp storeOp)
	{
		ASSERT( image );
		ASSERT( loadOp != EAttachmentLoadOp::Clear );	// clear value is not defined
		renderTargets[ uint(id) ] = RT{ image, desc, ClearValue_t{}, loadOp, storeOp };
		return *this;
	}

	template <typename ClearVal>
	inline RenderPassDesc&  RenderPassDesc::AddTarget (RenderTargetID id, RawImageID image, const ImageViewDesc &desc, const ClearVal &clearValue, EAttachmentStoreOp storeOp)
	{
		ASSERT( image );
		renderTargets[ uint(id) ] = RT{ image, desc, clearValue, EAttachmentLoadOp::Clear, storeOp };
		return *this;
	}

	template <typename T>
	inline RenderPassDesc&  RenderPassDesc::AddViewport (const Rectangle<T> &rect, float minDepth, float maxDepth, ArrayView<EShadingRatePalette> palette)
	{
		ASSERT( rect.IsValid() );
		viewports.push_back({ RectF{rect}, minDepth, maxDepth, palette });
		return *this;
	}

	template <typename T>
	inline RenderPassDesc&  RenderPassDesc::AddViewport (const Vec<T,2> &size, float minDepth, float maxDepth, ArrayView<EShadingRatePalette> palette)
	{
		viewports.push_back({ RectF{float2(), float2(size)}, minDepth, maxDepth, palette });
		return *this;
	}

/*
=================================================
	color states
=================================================
*/
	inline RenderPassDesc&  RenderPassDesc::AddColorBuffer (RenderTargetID id, EBlendFactor srcBlendFactor, EBlendFactor dstBlendFactor, EBlendOp blendOp, bool4 colorMask)
	{
		return AddColorBuffer( id, srcBlendFactor, srcBlendFactor, dstBlendFactor, dstBlendFactor, blendOp, blendOp, colorMask );
	}

	inline RenderPassDesc&  RenderPassDesc::AddColorBuffer (RenderTargetID id, EBlendFactor srcBlendFactorColor, EBlendFactor srcBlendFactorAlpha,
															EBlendFactor dstBlendFactorColor, EBlendFactor dstBlendFactorAlpha,
															EBlendOp blendOpColor, EBlendOp blendOpAlpha, bool4 colorMask)
	{
		RS::ColorBuffer	cb;
		cb.blend			= true;
		cb.srcBlendFactor	= { srcBlendFactorColor, srcBlendFactorAlpha };
		cb.dstBlendFactor	= { dstBlendFactorColor, dstBlendFactorAlpha };
		cb.blendOp			= { blendOpColor, blendOpAlpha };
		cb.colorMask		= colorMask;

		colorState.buffers[ uint(id) ] = std::move(cb);
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::AddColorBuffer (RenderTargetID id, bool4 colorMask)
	{
		RS::ColorBuffer	cb;
		cb.colorMask = colorMask;
		
		colorState.buffers[ uint(id) ] = std::move(cb);
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetLogicOp (ELogicOp value)
	{
		colorState.logicOp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetBlendColor (const RGBA32f &value)
	{
		colorState.blendColor = value;
		return *this;
	}
	
/*
=================================================
	depth states
=================================================
*/
	inline RenderPassDesc&  RenderPassDesc::SetDepthTestEnabled (bool value)
	{
		depthState.test = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetDepthWriteEnabled (bool value)
	{
		depthState.write = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetDepthCompareOp (ECompareOp value)
	{
		depthState.compareOp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetDepthBounds (float min, float max)
	{
		depthState.bounds			= { min, max };
		depthState.boundsEnabled	= true;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetDepthBoundsEnabled (bool value)
	{
		depthState.boundsEnabled = value;
		return *this;
	}
	
/*
=================================================
	stencil states
=================================================
*/
	inline RenderPassDesc&  RenderPassDesc::SetStencilTestEnabled (bool value)
	{
		stencilState.enabled = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilFrontFaceFailOp (EStencilOp value)
	{
		stencilState.front.failOp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilFrontFaceDepthFailOp (EStencilOp value)
	{
		stencilState.front.depthFailOp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilFrontFacePassOp (EStencilOp value)
	{
		stencilState.front.passOp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilFrontFaceCompareOp (ECompareOp value)
	{
		stencilState.front.compareOp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilFrontFaceReference (uint value)
	{
		stencilState.front.reference = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilFrontFaceWriteMask (uint value)
	{
		stencilState.front.writeMask = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilFrontFaceCompareMask (uint value)
	{
		stencilState.front.compareMask = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilBackFaceFailOp (EStencilOp value)
	{
		stencilState.back.failOp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilBackFaceDepthFailOp (EStencilOp value)
	{
		stencilState.back.depthFailOp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilBackFacePassOp (EStencilOp value)
	{
		stencilState.back.passOp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilBackFaceCompareOp (ECompareOp value)
	{
		stencilState.back.compareOp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilBackFaceReference (uint value)
	{
		stencilState.back.reference = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilBackFaceWriteMask (uint value)
	{
		stencilState.back.writeMask = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetStencilBackFaceCompareMask (uint value)
	{
		stencilState.back.compareMask = value;
		return *this;
	}
	
/*
=================================================
	rasterization states
=================================================
*/
	inline RenderPassDesc&  RenderPassDesc::SetPolygonMode (EPolygonMode value)
	{
		rasterizationState.polygonMode = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetLineWidth (float value)
	{
		rasterizationState.lineWidth = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetDepthBiasConstFactor (float value)
	{
		rasterizationState.depthBiasConstFactor	= value;
		rasterizationState.depthBias			= true;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetDepthBiasClamp (float value)
	{
		rasterizationState.depthBiasClamp	= value;
		rasterizationState.depthBias		= true;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetDepthBiasSlopeFactor (float value)
	{
		rasterizationState.depthBiasSlopeFactor = value;
		rasterizationState.depthBias			= true;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetDepthBias (float constFactor, float clamp, float slopeFactor)
	{
		rasterizationState.depthBiasConstFactor	= constFactor;
		rasterizationState.depthBiasClamp		= clamp;
		rasterizationState.depthBiasSlopeFactor	= slopeFactor;
		rasterizationState.depthBias			= true;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetDepthBiasEnabled (bool value)
	{
		rasterizationState.depthBias = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetDepthClampEnabled (bool value)
	{
		rasterizationState.depthClamp = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetRasterizerDiscard (bool value)
	{
		rasterizationState.rasterizerDiscard = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetCullMode (ECullMode value)
	{
		rasterizationState.cullMode = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetFrontFaceCCW (bool value)
	{
		rasterizationState.frontFaceCCW = value;
		return *this;
	}
		
/*
=================================================
	multisample states
=================================================
*/
	inline RenderPassDesc&  RenderPassDesc::SetSampleMask (ArrayView<uint> value)
	{
		for (size_t i = 0; i < multisampleState.sampleMask.size(); ++i)
		{
			multisampleState.sampleMask[i] = (i < value.size() ? value[i] : 0u);
		}
		return *this;
	}
	
	inline RenderPassDesc&  RenderPassDesc::SetMultiSamples (MultiSamples value, ArrayView<uint> mask)
	{
		ASSERT( mask.size() == ((value.Get() + 31) / 32) );
		return SetMultiSamples( value ).SetSampleMask( mask );
	}

	inline RenderPassDesc&  RenderPassDesc::SetMultiSamples (MultiSamples value)
	{
		multisampleState.samples = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetMinSampleShading (float value)
	{
		multisampleState.minSampleShading = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetSampleShadingEnabled (bool value)
	{
		multisampleState.sampleShading = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetAlphaToCoverageEnabled (bool value)
	{
		multisampleState.alphaToCoverage = value;
		return *this;
	}

	inline RenderPassDesc&  RenderPassDesc::SetAlphaToOneEnabled (bool value)
	{
		multisampleState.alphaToOne = value;
		return *this;
	}

/*
=================================================
	SetShadingRateImage
=================================================
*/
	inline RenderPassDesc&  RenderPassDesc::SetShadingRateImage (RawImageID image, ImageLayer layer, MipmapLevel level)
	{
		ASSERT( image );
		shadingRate.image	= image;
		shadingRate.layer	= layer;
		shadingRate.mipmap	= level;
		return *this;
	}
	
/*
=================================================
	AddResources
=================================================
*/
	inline RenderPassDesc&  RenderPassDesc::AddResources (const DescriptorSetID &id, const PipelineResources *res)
	{
		ASSERT( id.IsDefined() and res );
		perPassResources.insert({ id, res });
		return *this;
	}
	
/*
=================================================
	EnableShaderTrace
----
	enable shader trace recording for all draw calls in the render pass
=================================================
*/
	inline RenderPassDesc&  RenderPassDesc::EnableShaderTrace (EShaderStages stages)
	{
		debugMode.mode		 = EShaderDebugMode::Trace;
		debugMode.stages	|= stages;
		debugMode.dstImage	 = Default;
		return *this;
	}
	
/*
=================================================
	EnableFragmentShaderTrace
----
	enable shader trace recording for specified pixel coordinate
	for all draw calls in the render pass
=================================================
*/
	inline RenderPassDesc&  RenderPassDesc::EnableFragmentShaderTrace (int x, int y)
	{
		debugMode.mode		 = EShaderDebugMode::Trace;
		debugMode.stages	|= EShaderStages::Fragment;
		debugMode.fragCoord	 = { int16_t(x), int16_t(y) };
		debugMode.dstImage	 = Default;
		return *this;
	}
	
/*
=================================================
	EnableShaderTimemap
----
	create timemap for all draw calls in the render pass.
	'target' must be RGBA UNorm/Float texture with CollorAttachment usage.
	'target' can be one of render targets, drawing order is sequential (no data races).
=================================================
*/
	inline RenderPassDesc&  RenderPassDesc::EnableShaderTimemap (RawImageID target, EShaderStages stages)
	{
		ASSERT( target );
		debugMode.mode		 = EShaderDebugMode::Timemap;
		debugMode.stages	|= stages;
		debugMode.fragCoord	 = short2{ std::numeric_limits<short>::min() };
		debugMode.dstImage	 = target;
		return *this;
	}

}	// FG
