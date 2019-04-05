// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Shared/LocalResourceID.h"
#include "stl/ThreadSafe/AtomicCounter.h"
#include "stl/ThreadSafe/DataRaceCheck.h"

namespace FG
{

	//
	// Resource Wrapper
	//

	template <typename ResType>
	class alignas(FG_CACHE_LINE) ResourceBase final
	{
	// types
	public:
		enum class EState : uint
		{
			Initial			= 0,
			Failed,
			Created,
		};

		using Self			= ResourceBase< ResType >;
		using Resource_t	= ResType;
		using InstanceID_t	= RawImageID::InstanceID_t;


	// variables
	private:
		// instance counter used to detect deprecated handles
		std::atomic<uint>			_instanceId	= 0;

		std::atomic<EState>			_state		= EState::Initial;

		ResType						_data;

		// reference counter may be used for cached resources like samples, pipeline layout and other
		mutable std::atomic<int>	_refCounter	= 0;

		// cached resource may be deleted if reference counter is 1 and last usage was a long ago
		//mutable std::atomic<uint>	_lastUsage	= 0;


	// methods
	public:
		ResourceBase ()
		{}

		ResourceBase (Self &&) = delete;
		ResourceBase (const Self &) = delete;

		Self& operator = (Self &&) = delete;
		Self& operator = (const Self &) = delete;

		~ResourceBase ()
		{
			ASSERT( IsDestroyed() );
		}

		void AddRef () const
		{
			_refCounter.fetch_add( 1, memory_order_relaxed );
		}

		ND_ bool ReleaseRef (int refCount) const
		{
			return _refCounter.fetch_sub( refCount, memory_order_relaxed ) == refCount;
		}
		

		ND_ bool			IsCreated ()		const	{ return _GetState() == EState::Created; }
		ND_ bool			IsDestroyed ()		const	{ return _GetState() <= EState::Failed; }

		ND_ InstanceID_t	GetInstanceID ()	const	{ return InstanceID_t(_instanceId.load( memory_order_relaxed )); }
		ND_ int				GetRefCount ()		const	{ return _refCounter.load( memory_order_relaxed ); }
		//ND_ uint			GetLastUsage ()		const	{ return _lastUsage.load( memory_order_relaxed ); }

		ND_ ResType&		Data ()						{ return _data; }
		ND_ ResType const&	Data ()				const	{ return _data; }


		ND_ bool  operator == (const Self &rhs) const	{ return _data == rhs._data; }
		ND_ bool  operator != (const Self &rhs) const	{ return not (_data == rhs._data); }


		template <typename ...Args>
		bool Create (Args&& ...args)
		{
			ASSERT( IsDestroyed() );
			ASSERT( GetRefCount() == 0 );

			bool	result = _data.Create( std::forward<Args &&>( args )... );

			// set state and flush cache
			if ( result )
				_state.store( EState::Created, memory_order_release );
			else
				_state.store( EState::Failed, memory_order_release );

			return result;
		}

		template <typename ...Args>
		void Destroy (Args&& ...args)
		{
			ASSERT( not IsDestroyed() );
			//ASSERT( GetRefCount() == 0 );

			_data.Destroy( std::forward<Args &&>( args )... );
			
			// update atomics and flush cache
			_refCounter.store( 0, memory_order_relaxed );
			_state.store( EState::Initial, memory_order_relaxed );
			_instanceId.fetch_add( 1, memory_order_release );
		}

	private:
		ND_ EState	_GetState ()	const	{ return _state.load( memory_order_relaxed ); }
	};


}	// FG


namespace std
{
	template <typename T>
	struct hash< FG::ResourceBase<T> >
	{
		ND_ size_t  operator () (const FG::ResourceBase<T> &value) const noexcept {
			return std::hash<T>{}( value.Data() );
		}
	};

}	// std
