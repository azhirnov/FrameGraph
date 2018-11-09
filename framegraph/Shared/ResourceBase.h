// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Shared/LocalResourceID.h"
#include "stl/ThreadSafe/AtomicCounter.h"
#include "stl/ThreadSafe/RaceConditionCheck.h"

namespace FG
{

	//
	// Resource Base class
	//

	class ResourceBase
	{
	// types
	public:
		enum class EState : uint
		{
			Initial			= 0,
			CreatedInThread,		// in
			Created,				// created and commited
			ReadyToDelete,
		};


	// variables
	private:
		// reference counter may be used for cached resources like samples, pipeline layout and other
		alignas(FG_CACHE_LINE) mutable AtomicCounter<int>	_refCounter;

		// instance counter used to detect deprecated handles
		std::atomic<uint>		_instanceId	= 0;

		std::atomic<EState>		_state		= EState::Initial;

	protected:
		RWRaceConditionCheck	_rcCheck;


	// methods
	public:
		ResourceBase ()
		{}

		~ResourceBase ()
		{
			ASSERT( GetState() == EState::Initial );
		}

		// must be externally synchronized!
		bool Commit ()
		{
			if ( GetState() == EState::CreatedInThread ) {
				_state.store( EState::Created, memory_order_release );
				return true;
			}
			return false;
		}

		void AddRef () const
		{
			++_refCounter;
		}

		ND_ bool ReleaseRef () const
		{
			return _refCounter.DecAndTest();
		}
		
		ND_ EState		GetState ()			const	{ return _state.load( memory_order_acquire ); }
		ND_ bool		IsCreated ()		const	{ auto state = GetState();  return state == EState::CreatedInThread or state == EState::Created; }
		ND_ bool		IsDestroyed ()		const	{ return GetState() == EState::Initial; }

		ND_ uint		GetInstanceID ()	const	{ return _instanceId.load( memory_order_acquire ); }
		ND_ int			GetRefCount ()		const	{ return _refCounter.Load(); }


	protected:
		void _OnCreate ()
		{
			_state.store( EState::CreatedInThread, memory_order_release );
		}

		void _OnDestroy ()
		{
			ASSERT( not IsDestroyed() );
			//ASSERT( GetRefCount() == 0 );

			_state.store( EState::Initial, memory_order_release );
			_instanceId.fetch_add( 1, memory_order_release );
		}
	};


}	// FG
