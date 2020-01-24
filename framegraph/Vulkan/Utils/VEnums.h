// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/ResourceEnums.h"

namespace FG
{

	// TODO: remove?
	enum class EMemoryTypeExt : uint
	{
		HostRead		= uint(EMemoryType::HostRead),
		HostWrite		= uint(EMemoryType::HostWrite),
		Dedicated		= uint(EMemoryType::Dedicated),
		AllowAliasing	= uint(EMemoryType::AllowAliasing),
		Sparse			= uint(EMemoryType::Sparse),
		_Offset			= uint(EMemoryType::_Last)-1,

		LocalInGPU		= _Offset << 1,
		HostCoherent	= _Offset << 2,
		HostCached		= _Offset << 3,
		ForBuffer		= _Offset << 4,
		ForImage		= _Offset << 5,
		Virtual			= _Offset << 6,
		_Last,
		
		All				= ((_Last-1) << 1) - 1,
		HostVisible		= HostRead | HostWrite,
	};
	FG_BIT_OPERATORS( EMemoryTypeExt );

	

	enum class ExeOrderIndex : uint
	{
		Initial		= 0,
		First		= 1,
		Final		= 0x80000000,
		Unknown		= ~0u,
	};

	forceinline ExeOrderIndex&  operator ++ (ExeOrderIndex &value)	{ return (value = BitCast<ExeOrderIndex>( BitCast<uint>( value ) + 1 )); }


	enum class EQueueFamily : uint
	{
		_Count		= 31,

		External	= VK_QUEUE_FAMILY_EXTERNAL,
		Foreign		= VK_QUEUE_FAMILY_FOREIGN_EXT,
		Ignored		= VK_QUEUE_FAMILY_IGNORED,
		Unknown		= Ignored,
	};


	enum class EQueueFamilyMask : uint
	{
		All			= ~0u,
		Unknown		= 0,
	};
	FG_BIT_OPERATORS( EQueueFamilyMask );



	forceinline EQueueFamilyMask&  operator |= (EQueueFamilyMask &lhs, EQueueFamily rhs)
	{
		ASSERT( uint(rhs) < 32 );
		return lhs = BitCast<EQueueFamilyMask>( uint(lhs) | (1u << (uint(rhs) & 31)) );
	}

	forceinline EQueueFamilyMask   operator |  (EQueueFamilyMask lhs, EQueueFamily rhs)
	{
		ASSERT( uint(rhs) < 32 );
		return BitCast<EQueueFamilyMask>( uint(lhs) | (1u << (uint(rhs) & 31)) );
	}


}	// FG
