// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/MultiSamples.h"
#include "framegraph/Public/RenderStateEnums.h"
#include "framegraph/Public/IDs.h"

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
				reference{ 0 },					writeMask{ UMax },
				compareMask{ UMax }
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

			bool			frontFaceCCW;
			ECullMode		cullMode;

		// methods
			RasterizationState () :
				polygonMode{ EPolygonMode::Fill },	lineWidth{ 1.0f },
				depthBiasConstFactor{ 0.0f },		depthBiasClamp{ 0.0f },
				depthBiasSlopeFactor{ 0.0f },		depthBias{ false },
				depthClamp{ false },				rasterizerDiscard{ false },
				frontFaceCCW{ true },				cullMode{ ECullMode::None }
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
		ND_ bool  operator != (const RenderState &rhs) const	{ return not (*this == rhs); }
	};
	
	
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

