// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Public/RenderState.h"

namespace FG
{

/*
=================================================
	ColorBuffer::operator ==
=================================================
*/
	bool  RenderState::ColorBuffer::operator == (const ColorBuffer &rhs) const
	{
		return	blend == rhs.blend	and
				(blend ?
					(srcBlendFactor	== rhs.srcBlendFactor	and
					 dstBlendFactor	== rhs.dstBlendFactor	and
					 blendOp		== rhs.blendOp			and
					 All( colorMask	== rhs.colorMask )) : true);
	}

/*
=================================================
	ColorBuffersState::operator ==
=================================================
*/
	bool  RenderState::ColorBuffersState::operator == (const ColorBuffersState &rhs) const
	{
		return	buffers		== rhs.buffers	and
				logicOp		== rhs.logicOp	and
				blendColor	== rhs.blendColor;
	}
	
/*
=================================================
	StencilFaceState::operator ==
=================================================
*/
	bool  RenderState::StencilFaceState::operator == (const StencilFaceState &rhs) const
	{
		return	failOp		== rhs.failOp		and
				depthFailOp	== rhs.depthFailOp	and
				passOp		== rhs.passOp		and
				compareOp	== rhs.compareOp	and
				reference	== rhs.reference	and
				writeMask	== rhs.writeMask	and
				compareMask	== rhs.compareMask;
	}
	
/*
=================================================
	StencilBufferState::operator ==
=================================================
*/
	bool  RenderState::StencilBufferState::operator == (const StencilBufferState &rhs) const
	{
		return	enabled		== rhs.enabled	and
				(enabled ? 
					(front	== rhs.front	and
					 back	== rhs.back)	: true);
	}
	
/*
=================================================
	DepthBufferState::operator ==
=================================================
*/
	bool  RenderState::DepthBufferState::operator == (const DepthBufferState &rhs) const
	{
		return	compareOp		== rhs.compareOp		and
				boundsEnabled	== rhs.boundsEnabled	and
				(boundsEnabled ? All(Equals( bounds, rhs.bounds )) : true)	and
				write			== rhs.write			and
				test			== rhs.test;
	}
	
/*
=================================================
	InputAssemblyState::operator ==
=================================================
*/
	bool  RenderState::InputAssemblyState::operator == (const InputAssemblyState &rhs) const
	{
		return	topology			== rhs.topology		and
				primitiveRestart	== rhs.primitiveRestart;
	}
	
/*
=================================================
	RasterizationState::operator ==
=================================================
*/
	bool  RenderState::RasterizationState::operator == (const RasterizationState &rhs) const
	{
		return	polygonMode					==	rhs.polygonMode				and
				Equals(	lineWidth,				rhs.lineWidth )				and
				Equals(	depthBiasConstFactor,	rhs.depthBiasConstFactor )	and
				Equals(	depthBiasClamp,			rhs.depthBiasClamp )		and
				Equals(	depthBiasSlopeFactor,	rhs.depthBiasSlopeFactor )	and
				depthBias					==	rhs.depthBias				and
				depthClamp					==	rhs.depthClamp				and
				rasterizerDiscard			==	rhs.rasterizerDiscard		and
				cullMode					==	rhs.cullMode				and
				frontFaceCCW				==	rhs.frontFaceCCW;
	}

/*
=================================================
	MultisampleState::operator ==
=================================================
*/
	bool  RenderState::MultisampleState::operator == (const MultisampleState &rhs) const
	{
		return	sampleMask				==	rhs.sampleMask			and
				samples					==	rhs.samples				and
				Equals(	minSampleShading,	rhs.minSampleShading )	and
				sampleShading			==	rhs.sampleShading		and
				alphaToCoverage			==	rhs.alphaToCoverage		and
				alphaToOne				==	rhs.alphaToOne;
	}

/*
=================================================
	operator ==
=================================================
*/
	bool  RenderState::operator == (const RenderState &rhs) const
	{
		return	color			== rhs.color			and
				depth			== rhs.depth			and
				stencil			== rhs.stencil			and
				inputAssembly	== rhs.inputAssembly	and
				rasterization	== rhs.rasterization	and
				multisample		== rhs.multisample;
	}

}	// FG


namespace std
{
	using namespace FG;

/*
=================================================
	hash::operator (ColorBuffer)
=================================================
*/
	size_t  hash< RenderState::ColorBuffer >::operator () (const RenderState::ColorBuffer &value) const
	{
	#if FG_FAST_HASH
		return size_t(HashOf( AddressOf(value), sizeof(value) ));
	#else
		HashVal	result;
		result << HashOf( value.blend );
		result << HashOf( value.srcBlendFactor.color );
		result << HashOf( value.srcBlendFactor.alpha );
		result << HashOf( value.dstBlendFactor.color );
		result << HashOf( value.dstBlendFactor.alpha );
		result << HashOf( value.blendOp.color );
		result << HashOf( value.blendOp.alpha );
		result << HashOf( value.colorMask );
		return size_t(result);
	#endif
	}

/*
=================================================
	hash::operator (ColorBuffer)
=================================================
*/
	size_t  hash< RenderState::ColorBuffersState >::operator () (const RenderState::ColorBuffersState &value) const
	{
	#if FG_FAST_HASH
		return size_t(HashOf( AddressOf(value), sizeof(value) ));
	#else
		HashVal	result;
		result << HashOf( value.buffers );
		result << HashOf( value.blendColor );
		result << HashOf( value.logicOp );
		return size_t(result);
	#endif
	}

/*
=================================================
	hash::operator (DepthBufferState)
=================================================
*/
	size_t  hash< RenderState::DepthBufferState >::operator () (const RenderState::DepthBufferState &value) const
	{
	#if FG_FAST_HASH
		return size_t(HashOf( AddressOf(value), sizeof(value) ));
	#else
		HashVal	result;
		result << HashOf( value.compareOp );
		result << HashOf( value.boundsEnabled );
		result << HashOf( value.bounds );
		result << HashOf( value.test );
		result << HashOf( value.write );
		return size_t(result);
	#endif
	}

/*
=================================================
	hash::operator (StencilFaceState)
=================================================
*/
	size_t  hash< RenderState::StencilFaceState >::operator () (const RenderState::StencilFaceState &value) const
	{
	#if FG_FAST_HASH
		return size_t(HashOf( AddressOf(value), sizeof(value) ));
	#else
		HashVal	result;
		result << HashOf( value.failOp );
		result << HashOf( value.depthFailOp );
		result << HashOf( value.passOp );
		result << HashOf( value.compareOp );
		result << HashOf( value.reference );
		result << HashOf( value.writeMask );
		result << HashOf( value.compareMask );
		return size_t(result);
	#endif
	}
	
/*
=================================================
	hash::operator (StencilBufferState)
=================================================
*/
	size_t  hash< RenderState::StencilBufferState >::operator () (const RenderState::StencilBufferState &value) const
	{
		HashVal	result;
		if ( value.enabled )
		{
		#if FG_FAST_HASH
			return size_t(HashOf( AddressOf(value), sizeof(value) ));
		#else
			result << HashOf( value.front );
			result << HashOf( value.back );
		#endif
		}
		return size_t(result);
	}
	
/*
=================================================
	hash::operator (InputAssemblyState)
=================================================
*/
	size_t  hash< RenderState::InputAssemblyState >::operator () (const RenderState::InputAssemblyState &value) const
	{
	#if FG_FAST_HASH
		return size_t(HashOf( AddressOf(value), sizeof(value) ));
	#else
		HashVal	result;
		result << HashOf( value.topology );
		result << HashOf( value.primitiveRestart );
		return size_t(result);
	#endif
	}
	
/*
=================================================
	hash::operator (RasterizationState)
=================================================
*/
	size_t  hash< RenderState::RasterizationState >::operator () (const RenderState::RasterizationState &value) const
	{
	#if FG_FAST_HASH
		return size_t(HashOf( AddressOf(value), sizeof(value) ));
	#else
		HashVal	result;
		result << HashOf( value.polygonMode );
		result << HashOf( value.lineWidth );
		result << HashOf( value.depthBiasConstFactor );
		result << HashOf( value.depthBiasClamp );
		result << HashOf( value.depthBiasSlopeFactor );
		result << HashOf( value.depthBias );
		result << HashOf( value.depthClamp );
		result << HashOf( value.rasterizerDiscard );
		result << HashOf( value.cullMode );
		result << HashOf( value.frontFaceCCW );
		return size_t(result);
	#endif
	}
	
/*
=================================================
	hash::operator (MultisampleState)
=================================================
*/
	size_t  hash< RenderState::MultisampleState >::operator () (const RenderState::MultisampleState &value) const
	{
	#if FG_FAST_HASH
		return size_t(HashOf( AddressOf(value), sizeof(value) ));
	#else
		HashVal	result;
		result << HashOf( value.sampleMask );
		result << HashOf( value.samples );
		result << HashOf( value.minSampleShading );
		result << HashOf( value.sampleShading );
		result << HashOf( value.alphaToCoverage );
		result << HashOf( value.alphaToOne );
		return size_t(result);
	#endif
	}
	
/*
=================================================
	hash::operator (RenderState)
=================================================
*/
	size_t  hash< RenderState >::operator () (const RenderState &value) const
	{
	#if FG_FAST_HASH
		return size_t(HashOf( AddressOf(value), sizeof(value) ));
	#else
		HashVal	result;
		result << HashOf( value.color );
		result << HashOf( value.depth );
		result << HashOf( value.stencil );
		result << HashOf( value.inputAssembly );
		result << HashOf( value.rasterization );
		result << HashOf( value.multisample );
		return size_t(result);
	#endif
	}


}	// std
