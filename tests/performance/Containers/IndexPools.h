// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "CachedIndexedPool1.h"
#include "CachedIndexedPool2.h"
#include "stl/Containers/CachedIndexedPool.h"

#include "ChunkedIndexedPool1.h"
#include "ChunkedIndexedPool2.h"
#include "stl/Containers/ChunkedIndexedPool.h"

#include "../PerfTestCommon.h"

namespace FG
{

	template <typename ValueType,
			  typename IndexType,
			  size_t ChunkSize,
			  size_t MaxChunks = 16,
			  typename AllocatorType = UntypedAlignedAllocator,
			  typename AssignOpLock = DummyLock,
			  template <typename T> class AtomicChunkPtr = NonAtomicPtr
			 >
	using CachedIndexedPool3 = CachedIndexedPool< ValueType, IndexType, ChunkSize, MaxChunks, AllocatorType, AssignOpLock, AtomicChunkPtr >;

	
	template <typename ValueType,
			  typename IndexType,
			  size_t ChunkSize,
			  size_t MaxChunks = 16,
			  typename AllocatorType = UntypedAlignedAllocator,
			  typename AssignOpLock = DummyLock,
			  template <typename T> class AtomicChunkPtr = NonAtomicPtr
			 >
	using ChunkedIndexedPool3 = ChunkedIndexedPool< ValueType, IndexType, ChunkSize, MaxChunks, AllocatorType, AssignOpLock, AtomicChunkPtr >;
	

}	// FG