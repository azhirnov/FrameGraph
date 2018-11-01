// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Shared/LocalResourceID.h"
#include "stl/ThreadSafe/AtomicCounter.h"

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
		std::atomic<uint>									_instanceId	= 0;

		EState												_state		= EState::Initial;


	// methods
	public:
		ResourceBase ()
		{}

		~ResourceBase ()
		{
			ASSERT( _state == EState::Initial );
		}
		
		void MarkForDelete ()
		{
			ASSERT( IsCreated() );
			ASSERT( GetRefCount() == 0 );

			_state = EState::ReadyToDelete;
		}

		bool Commit ()
		{
			if ( _state == EState::CreatedInThread ) {
				_state = EState::Created;
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

		ND_ EState		GetState ()			const	{ return _state; }
		ND_ bool		IsCreated ()		const	{ return _state == EState::CreatedInThread or _state == EState::Created; }
		ND_ bool		IsDestroyed ()		const	{ return _state == EState::Initial; }

		ND_ uint		GetInstanceID ()	const	{ return _instanceId.load( std::memory_order_acquire ); }
		ND_ int			GetRefCount ()		const	{ return _refCounter.Load(); }


	protected:
		void _OnCreate ()
		{
			_state = EState::CreatedInThread;
		}

		void _OnDestroy ()
		{
			ASSERT( not IsDestroyed() );
			//ASSERT( GetRefCount() == 0 );

			_state = EState::Initial;
			_instanceId.fetch_add( 1, std::memory_order_release );
		}

		void _Replace (INOUT ResourceBase &&other)
		{
			ASSERT( IsDestroyed() );
			ASSERT( GetRefCount() == 0 );

			// warning: don't swap _instanceId !

			int	tmp = other._refCounter.Load();
			other._refCounter.Store( 0 );
			_refCounter.Store( tmp );

			std::swap( _state, other._state );
		}
	};


}	// FG

namespace std
{

	template <typename T1, typename T2>
	inline enable_if_t<is_base_of_v<FG::ResourceBase, T1> and is_base_of_v<FG::ResourceBase, T2>, void> swap (T1 &lhs, T2 &rhs)
	{
		lhs.Replace( move(rhs) );
	}

}	// std
