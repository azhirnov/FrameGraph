// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "framegraph/Public/LowLevel/MultiSamples.h"
#include "framegraph/Public/LowLevel/RenderStateEnums.h"
#include "framegraph/Public/LowLevel/IDs.h"

namespace FG
{

	//
	// Render State
	//
	
	struct RenderState
	{
	// types
	public:

		//
		// Color Buffer
		//
		struct ColorBuffer
		{
		// types
			template <typename T>
			struct ColorPair
			{
				T	color;
				T	alpha;

				ColorPair () : color{T::Unknown}, alpha{T::Unknown} {}
				ColorPair (T rgba) : color{rgba}, alpha{rgba} {}
				ColorPair (T rgb, T a) : color{rgb}, alpha{a} {}

				ND_ bool  operator == (const ColorPair<T> &rhs) const {
					return color == rhs.color and alpha == rhs.alpha;
				}
			};


		// variables
			ColorPair< EBlendFactor >	srcBlendFactor,
										dstBlendFactor;
			ColorPair< EBlendOp >		blendOp;
			bool4						colorMask;
			bool						blend;
		
		// methods
			ColorBuffer () :
				srcBlendFactor{ EBlendFactor::One,  EBlendFactor::One },
				dstBlendFactor{ EBlendFactor::Zero,	EBlendFactor::Zero },
				blendOp{ EBlendOp::Add, EBlendOp::Add },
				colorMask{ true },	blend{ false }
			{}

			ND_ bool  operator == (const ColorBuffer &rhs) const;
		};


		//
		// Color Buffers State
		//
		struct ColorBuffersState
		{
		// types
			using ColorBuffers_t	= FixedMap< RenderTargetID, ColorBuffer, FG_MaxColorBuffers >;

		// variables
			ColorBuffers_t		buffers;
			ELogicOp			logicOp;
			RGBA32f				blendColor;

		// methods
			ColorBuffersState () :
				logicOp{ ELogicOp::None },	blendColor{ 1.0f }
			{}

			ND_ bool  operator == (const ColorBuffersState &rhs) const;
		};
		

		//
		// Stencil Face State
		//
		struct StencilFaceState
		{
		// variables
			EStencilOp		failOp;				// stencil test failed
			EStencilOp		depthFailOp;		// depth and stencil tests are passed
			EStencilOp		passOp;				// stencil test passed and depth test failed
			ECompareOp		compareOp;
			uint			reference;
			uint			writeMask;
			uint			compareMask;

		// methods
			StencilFaceState () :
				failOp{ EStencilOp::Keep },		depthFailOp{ EStencilOp::Keep },
				passOp{ EStencilOp::Keep },		compareOp{ ECompareOp::Always },
				reference{ 0 },					writeMask{ ~0u },
				compareMask{ ~0u }
			{}

			ND_ bool  operator == (const StencilFaceState &rhs) const;
		};


		//
		// Stencil Buffer State
		//
		struct StencilBufferState
		{
		// variables
			StencilFaceState	front;
			StencilFaceState	back;
			bool				enabled;	// stencil write/test

		// methods
			StencilBufferState () :
				front{}, back{}, enabled{ false }
			{}

			ND_ bool  operator == (const StencilBufferState &rhs) const;
		};


		//
		// Depth Buffer State
		//
		struct DepthBufferState
		{
		// variables
			ECompareOp			compareOp;	// if 'test' enabled
			float2				bounds;
			bool				boundsEnabled;
			bool				write;		// depth write enabled
			bool				test;		// depth test enabled

		// methods
			DepthBufferState () :
				compareOp{ ECompareOp::LEqual },	bounds{ 0.0f, 1.0f },
				boundsEnabled{ false },
				write{ false },						test{ false }
			{}

			ND_ bool  operator == (const DepthBufferState &rhs) const;
		};


		//
		// Input Assembly State
		//
		struct InputAssemblyState
		{
		// variables
			EPrimitive		topology;
			bool			primitiveRestart;	// if 'true' then index with -1 value will restarting the assembly of primitives

		// methods
			InputAssemblyState () :
				topology{ EPrimitive::Unknown },	primitiveRestart{ false }
			{}

			ND_ bool  operator == (const InputAssemblyState &rhs) const;
		};


		//
		// Rasterization State
		//
		struct RasterizationState
		{
		// variables
			EPolygonMode	polygonMode;

			float			lineWidth;

			float			depthBiasConstFactor;
			float			depthBiasClamp;
			float			depthBiasSlopeFactor;
			bool			depthBias;

			bool			depthClamp;
			bool			rasterizerDiscard;

			ECullMode		cullMode;
			bool			frontFaceCCW;

		// methods
			RasterizationState () :
				polygonMode{ EPolygonMode::Fill },	lineWidth{ 1.0f },
				depthBiasConstFactor{ 0.0f },		depthBiasClamp{ 0.0f },
				depthBiasSlopeFactor{ 0.0f },		depthBias{ false },
				depthClamp{ false },				rasterizerDiscard{ false },
				cullMode{ ECullMode::None },		frontFaceCCW{ true }
			{}

			ND_ bool  operator == (const RasterizationState &rhs) const;
		};


		//
		// Multisample State
		//
		struct MultisampleState
		{
		// types
			using SampleMask	= StaticArray< uint, 4 >;

		// variables
			SampleMask			sampleMask;
			MultiSamples		samples;

			float				minSampleShading;
			bool				sampleShading;

			bool				alphaToCoverage;
			bool				alphaToOne;

		// methods
			MultisampleState () :
				sampleMask{},				samples{ 1 },
				minSampleShading{},			sampleShading{ false },
				alphaToCoverage{ false },	alphaToOne{ false }
			{}

			ND_ bool  operator == (const MultisampleState &rhs) const;
		};


	// variables
	public:
		ColorBuffersState		color;
		DepthBufferState		depth;
		StencilBufferState		stencil;
		InputAssemblyState		inputAssembly;
		RasterizationState		rasterization;
		MultisampleState		multisample;


	// methods
	public:
		RenderState () {}

		ND_ bool  operator == (const RenderState &rhs) const;

		// color
		RenderState&  AddColorBuffer (const RenderTargetID &id, EBlendFactor srcBlendFactor, EBlendFactor dstBlendFactor, EBlendOp blendOp, bool4 colorMask = bool4{true});
		RenderState&  AddColorBuffer (const RenderTargetID &id, EBlendFactor srcBlendFactorColor, EBlendFactor srcBlendFactorAlpha,
									  EBlendFactor dstBlendFactorColor, EBlendFactor dstBlendFactorAlpha,
									  EBlendOp blendOpColor, EBlendOp blendOpAlpha, bool4 colorMask = bool4{true});
		RenderState&  AddColorBuffer (const RenderTargetID &id, bool4 colorMask = bool4{true}); // without blending
		RenderState&  SetLogicOp (ELogicOp value);
		RenderState&  SetBlendColor (const RGBA32f &value);

		// depth
		RenderState&  SetDepthTestEnabled (bool value);
		RenderState&  SetDepthWriteEnabled (bool value);
		RenderState&  SetDepthCompareOp (ECompareOp value);
		RenderState&  SetDepthBounds (float min, float max);
		RenderState&  SetDepthBoundsEnabled (bool value);

		// stencil
		RenderState&  SetStencilTestEnabled (bool value);
		RenderState&  SetStencilFrontFaceFailOp (EStencilOp value);
		RenderState&  SetStencilFrontFaceDepthFailOp (EStencilOp value);
		RenderState&  SetStencilFrontFacePassOp (EStencilOp value);
		RenderState&  SetStencilFrontFaceCompareOp (ECompareOp value);
		RenderState&  SetStencilFrontFaceReference (uint value);
		RenderState&  SetStencilFrontFaceWriteMask (uint value);
		RenderState&  SetStencilFrontFaceCompareMask (uint value);
		RenderState&  SetStencilBackFaceFailOp (EStencilOp value);
		RenderState&  SetStencilBackFaceDepthFailOp (EStencilOp value);
		RenderState&  SetStencilBackFacePassOp (EStencilOp value);
		RenderState&  SetStencilBackFaceCompareOp (ECompareOp value);
		RenderState&  SetStencilBackFaceReference (uint value);
		RenderState&  SetStencilBackFaceWriteMask (uint value);
		RenderState&  SetStencilBackFaceCompareMask (uint value);
		
		// input assembly
		RenderState&  SetTopology (EPrimitive value);
		RenderState&  SetPrimitiveRestartEnabled (bool value);

		// rasterization
		RenderState&  SetPolygonMode (EPolygonMode value);
		RenderState&  SetLineWidth (float value);
		RenderState&  SetDepthBiasConstFactor (float value);
		RenderState&  SetDepthBiasClamp (float value);
		RenderState&  SetDepthBiasSlopeFactor (float value);
		RenderState&  SetDepthBias (float constFactor, float clamp, float slopeFactor);
		RenderState&  SetDepthBiasEnabled (bool value);
		RenderState&  SetDepthClampEnabled (bool value);
		RenderState&  SetRasterizerDiscard (bool value);
		RenderState&  SetCullMode (ECullMode value);
		RenderState&  SetFrontFaceCCW (bool value);
		
		// multisample
		RenderState&  SetSampleMask (ArrayView<uint> value);
		RenderState&  SetMultiSamples (MultiSamples value);
		RenderState&  SetMultiSamples (MultiSamples value, ArrayView<uint> mask);
		RenderState&  SetMinSampleShading (float value);
		RenderState&  SetSampleShadingEnabled (bool value);
		RenderState&  SetAlphaToCoverageEnabled (bool value);
		RenderState&  SetAlphaToOneEnabled (bool value);
	};
	
	
/*
=================================================
	color states
=================================================
*/
	inline RenderState&  RenderState::AddColorBuffer (const RenderTargetID &id, EBlendFactor srcBlendFactor, EBlendFactor dstBlendFactor, EBlendOp blendOp, bool4 colorMask)
	{
		return AddColorBuffer( id, srcBlendFactor, srcBlendFactor, dstBlendFactor, dstBlendFactor, blendOp, blendOp, colorMask );
	}

	inline RenderState&  RenderState::AddColorBuffer (const RenderTargetID &id, EBlendFactor srcBlendFactorColor, EBlendFactor srcBlendFactorAlpha,
													  EBlendFactor dstBlendFactorColor, EBlendFactor dstBlendFactorAlpha,
													  EBlendOp blendOpColor, EBlendOp blendOpAlpha, bool4 colorMask)
	{
		ColorBuffer	cb;
		cb.blend			= true;
		cb.srcBlendFactor	= { srcBlendFactorColor, srcBlendFactorAlpha };
		cb.dstBlendFactor	= { dstBlendFactorColor, dstBlendFactorAlpha };
		cb.blendOp			= { blendOpColor, blendOpAlpha };
		cb.colorMask		= colorMask;

		color.buffers.insert({ id, std::move(cb) });
		return *this;
	}

	inline RenderState&  RenderState::AddColorBuffer (const RenderTargetID &id, bool4 colorMask)
	{
		ColorBuffer	cb;
		cb.colorMask = colorMask;
		
		color.buffers.insert({ id, std::move(cb) });
		return *this;
	}

	inline RenderState&  RenderState::SetLogicOp (ELogicOp value)
	{
		color.logicOp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetBlendColor (const RGBA32f &value)
	{
		color.blendColor = value;
		return *this;
	}
	
/*
=================================================
	depth states
=================================================
*/
	inline RenderState&  RenderState::SetDepthTestEnabled (bool value)
	{
		depth.test = value;
		return *this;
	}

	inline RenderState&  RenderState::SetDepthWriteEnabled (bool value)
	{
		depth.write = value;
		return *this;
	}

	inline RenderState&  RenderState::SetDepthCompareOp (ECompareOp value)
	{
		depth.compareOp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetDepthBounds (float min, float max)
	{
		depth.bounds		= { min, max };
		depth.boundsEnabled	= true;
		return *this;
	}

	inline RenderState&  RenderState::SetDepthBoundsEnabled (bool value)
	{
		depth.boundsEnabled = value;
		return *this;
	}
	
/*
=================================================
	stencil states
=================================================
*/
	inline RenderState&  RenderState::SetStencilTestEnabled (bool value)
	{
		stencil.enabled = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilFrontFaceFailOp (EStencilOp value)
	{
		stencil.front.failOp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilFrontFaceDepthFailOp (EStencilOp value)
	{
		stencil.front.depthFailOp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilFrontFacePassOp (EStencilOp value)
	{
		stencil.front.passOp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilFrontFaceCompareOp (ECompareOp value)
	{
		stencil.front.compareOp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilFrontFaceReference (uint value)
	{
		stencil.front.reference = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilFrontFaceWriteMask (uint value)
	{
		stencil.front.writeMask = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilFrontFaceCompareMask (uint value)
	{
		stencil.front.compareMask = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilBackFaceFailOp (EStencilOp value)
	{
		stencil.back.failOp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilBackFaceDepthFailOp (EStencilOp value)
	{
		stencil.back.depthFailOp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilBackFacePassOp (EStencilOp value)
	{
		stencil.back.passOp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilBackFaceCompareOp (ECompareOp value)
	{
		stencil.back.compareOp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilBackFaceReference (uint value)
	{
		stencil.back.reference = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilBackFaceWriteMask (uint value)
	{
		stencil.back.writeMask = value;
		return *this;
	}

	inline RenderState&  RenderState::SetStencilBackFaceCompareMask (uint value)
	{
		stencil.back.compareMask = value;
		return *this;
	}
		
/*
=================================================
	input assembly states
=================================================
*/
	inline RenderState&  RenderState::SetTopology (EPrimitive value)
	{
		inputAssembly.topology = value;
		return *this;
	}

	inline RenderState&  RenderState::SetPrimitiveRestartEnabled (bool value)
	{
		inputAssembly.primitiveRestart = value;
		return *this;
	}
	
/*
=================================================
	rasterization states
=================================================
*/
	inline RenderState&  RenderState::SetPolygonMode (EPolygonMode value)
	{
		rasterization.polygonMode = value;
		return *this;
	}

	inline RenderState&  RenderState::SetLineWidth (float value)
	{
		rasterization.lineWidth = value;
		return *this;
	}

	inline RenderState&  RenderState::SetDepthBiasConstFactor (float value)
	{
		rasterization.depthBiasConstFactor	= value;
		return *this;
	}

	inline RenderState&  RenderState::SetDepthBiasClamp (float value)
	{
		rasterization.depthBiasClamp	= value;
		//rasterization.depthClamp		= true;	// TODO ???
		return *this;
	}

	inline RenderState&  RenderState::SetDepthBiasSlopeFactor (float value)
	{
		rasterization.depthBiasSlopeFactor = value;
		return *this;
	}

	inline RenderState&  RenderState::SetDepthBias (float constFactor, float clamp, float slopeFactor)
	{
		rasterization.depthBiasConstFactor	= constFactor;
		rasterization.depthBiasClamp		= clamp;
		rasterization.depthBiasSlopeFactor	= slopeFactor;
		//rasterization.depthBias			= true;	// TODO ???
		return *this;
	}

	inline RenderState&  RenderState::SetDepthBiasEnabled (bool value)
	{
		rasterization.depthBias = value;
		return *this;
	}

	inline RenderState&  RenderState::SetDepthClampEnabled (bool value)
	{
		rasterization.depthClamp = value;
		return *this;
	}

	inline RenderState&  RenderState::SetRasterizerDiscard (bool value)
	{
		rasterization.rasterizerDiscard = value;
		return *this;
	}

	inline RenderState&  RenderState::SetCullMode (ECullMode value)
	{
		rasterization.cullMode = value;
		return *this;
	}

	inline RenderState&  RenderState::SetFrontFaceCCW (bool value)
	{
		rasterization.frontFaceCCW = value;
		return *this;
	}
		
/*
=================================================
	multisample states
=================================================
*/
	inline RenderState&  RenderState::SetSampleMask (ArrayView<uint> value)
	{
		for (size_t i = 0; i < multisample.sampleMask.size(); ++i)
		{
			multisample.sampleMask[i] = (i < value.size() ? value[i] : 0u);
		}
		return *this;
	}
	
	inline RenderState&  RenderState::SetMultiSamples (MultiSamples value, ArrayView<uint> mask)
	{
		ASSERT( mask.size() == ((value.Get() + 31) / 32) );
		return SetMultiSamples( value ).SetSampleMask( mask );
	}

	inline RenderState&  RenderState::SetMultiSamples (MultiSamples value)
	{
		multisample.samples = value;
		return *this;
	}

	inline RenderState&  RenderState::SetMinSampleShading (float value)
	{
		multisample.minSampleShading = value;
		return *this;
	}

	inline RenderState&  RenderState::SetSampleShadingEnabled (bool value)
	{
		multisample.sampleShading = value;
		return *this;
	}

	inline RenderState&  RenderState::SetAlphaToCoverageEnabled (bool value)
	{
		multisample.alphaToCoverage = value;
		return *this;
	}

	inline RenderState&  RenderState::SetAlphaToOneEnabled (bool value)
	{
		multisample.alphaToOne = value;
		return *this;
	}

}	// FG


namespace std
{
	template <>
	struct hash< FG::RenderState::ColorBuffer > {
		ND_ size_t  operator () (const FG::RenderState::ColorBuffer &value) const noexcept;
	};

	template <>
	struct hash< FG::RenderState::ColorBuffersState > {
		ND_ size_t  operator () (const FG::RenderState::ColorBuffersState &value) const noexcept;
	};

	template <>
	struct hash< FG::RenderState::DepthBufferState > {
		ND_ size_t  operator () (const FG::RenderState::DepthBufferState &value) const noexcept;
	};

	template <>
	struct hash< FG::RenderState::StencilFaceState > {
		ND_ size_t  operator () (const FG::RenderState::StencilFaceState &value) const noexcept;
	};

	template <>
	struct hash< FG::RenderState::StencilBufferState > {
		ND_ size_t  operator () (const FG::RenderState::StencilBufferState &value) const noexcept;
	};

	template <>
	struct hash< FG::RenderState::InputAssemblyState > {
		ND_ size_t  operator () (const FG::RenderState::InputAssemblyState &value) const noexcept;
	};

	template <>
	struct hash< FG::RenderState::RasterizationState > {
		ND_ size_t  operator () (const FG::RenderState::RasterizationState &value) const noexcept;
	};

	template <>
	struct hash< FG::RenderState::MultisampleState > {
		ND_ size_t  operator () (const FG::RenderState::MultisampleState &value) const noexcept;
	};

	template <>
	struct hash< FG::RenderState > {
		ND_ size_t  operator () (const FG::RenderState &value) const noexcept;
	};

}	// std

