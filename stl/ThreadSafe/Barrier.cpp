// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/ThreadSafe/Barrier.h"
#include "stl/Memory/MemUtils.h"

#if (FG_BARRIER_MODE == 0)

# ifdef COMPILER_MSVC
#	pragma warning (push, 0)
#	include <Windows.h>
#	pragma warning (pop)
# else
#	include <Windows.h>
# endif

namespace FG
{

/*
=================================================
	ArraySizeOf
=================================================
*/
	Barrier::Barrier (uint numThreads)
	{
		auto*	barrier_ptr = PlacementNew<SYNCHRONIZATION_BARRIER>( _data );

		CHECK( InitializeSynchronizationBarrier( barrier_ptr, numThreads, -1 ));
	}
	
/*
=================================================
	ArraySizeOf
=================================================
*/
	Barrier::~Barrier ()
	{
		CHECK( DeleteSynchronizationBarrier( BitCast<SYNCHRONIZATION_BARRIER *>( &_data[0] ) ));
	}
	
/*
=================================================
	wait
=================================================
*/
	void Barrier::wait ()
	{
		EnterSynchronizationBarrier( BitCast<SYNCHRONIZATION_BARRIER *>( &_data[0] ), SYNCHRONIZATION_BARRIER_FLAGS_NO_DELETE );
	}
	
}	// FG
#endif	// FG_BARRIER_MODE == 0
