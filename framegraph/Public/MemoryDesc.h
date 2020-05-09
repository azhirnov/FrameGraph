// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/ResourceEnums.h"
#include "framegraph/Public/IDs.h"
#include "framegraph/Public/VulkanTypes.h"

namespace FG
{

	//
	// Memory Description
	//

	struct MemoryDesc
	{
	// types
		using MemRequirements_t = Union< NullUnion, VulkanMemRequirements >;

	// variables
		EMemoryType			type	= EMemoryType::Default;
		MemPoolID			poolId;
		MemRequirements_t	req;

	// methods
		MemoryDesc () {}
		MemoryDesc (EMemoryType type) : type{type} {}
		MemoryDesc (EMemoryType type, MemPoolID poolId) : type{type}, poolId{poolId} {}
	};


}	// FG
