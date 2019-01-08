// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

#include <shared_mutex>

#ifdef FG_ENABLE_RACE_CONDITION_CHECK

#include <atomic>
#include <mutex>
#include <thread>

namespace FG
{

	//
	// Race Condition Check
	//

	struct RaceConditionCheck
	{
	// variables
	private:
		mutable std::atomic<size_t>		_state  { 0 };


	// methods
	public:
		RaceConditionCheck () {}

		ND_ bool  Lock () const
		{
			const size_t	id		= size_t(HashOf( std::this_thread::get_id() ));
			size_t			curr	= _state.load( memory_order_acquire );
			
			if ( curr == id )
				return true;	// recursive lock

			curr	= 0;
			bool	locked	= _state.compare_exchange_strong( INOUT curr, id, memory_order_release, memory_order_relaxed );
			CHECK_ERR( curr == 0 );		// locked by another thread - race condition detected!
			CHECK_ERR( locked );
			return true;
		}

		void  Unlock () const
		{
			_state.store( 0, memory_order_release );
		}
	};



	//
	// Read/Write Race Condition Check
	//

	struct RWRaceConditionCheck
	{
	// variables
	private:
		mutable std::recursive_mutex	_lockWrite;
		mutable std::atomic<int>		_readCounter { 0 };


	// methods
	public:
		RWRaceConditionCheck () {}


		ND_ bool  LockExclusive ()
		{
			bool	locked = _lockWrite.try_lock();
			CHECK_ERR( locked );	// locked by another thread - race condition detected!

			int		expected = _readCounter.load( memory_order_acquire );
			CHECK_ERR( expected <= 0 );	// has read lock(s) - race condition detected!

			_readCounter.compare_exchange_strong( INOUT expected, expected-1, memory_order_release, memory_order_relaxed );
			CHECK_ERR( expected <= 0 );	// has read lock(s) - race condition detected!
			return true;
		}

		void  UnlockExclusive ()
		{
			_readCounter.fetch_add( 1, memory_order_release );
			_lockWrite.unlock();
		}


		ND_ bool  LockShared () const
		{
			int		expected	= 0;
			bool	locked		= false;
			do {
				locked = _readCounter.compare_exchange_weak( INOUT expected, expected+1, memory_order_release, memory_order_relaxed );
			
				// if has exclusive lock in current thread
				if ( expected < 0 and _lockWrite.try_lock() )
				{
					_lockWrite.unlock();
					return false;	// don't call 'UnlockShared'
				}
				CHECK_ERR( expected >= 0 );	// has write lock(s) - race condition detected!
			}
			while ( not locked );

			return true;
		}

		void  UnlockShared () const
		{
			_readCounter.fetch_sub( 1, memory_order_release );
		}
	};

}	// FG

namespace std
{
	template <>
	struct unique_lock< FG::RaceConditionCheck >
	{
	private:
		FG::RaceConditionCheck &	_lock;
		bool						_locked	= false;

	public:
		explicit unique_lock (FG::RaceConditionCheck &ref) : _lock{ref}
		{
			_locked = _lock.Lock();
		}

		unique_lock (const unique_lock &) = delete;
		unique_lock (unique_lock &&) = delete;

		~unique_lock ()
		{
			if ( _locked )
				_lock.Unlock();
		}
	};

	template <>
	struct unique_lock< const FG::RaceConditionCheck > :
		unique_lock< FG::RaceConditionCheck >
	{
		explicit unique_lock (const FG::RaceConditionCheck &ref) :
			unique_lock< FG::RaceConditionCheck >{ const_cast<FG::RaceConditionCheck &>(ref) }
		{}
	};


	template <>
	struct unique_lock< FG::RWRaceConditionCheck >
	{
	private:
		FG::RWRaceConditionCheck &	_lock;
		bool						_locked	= false;

	public:
		explicit unique_lock (FG::RWRaceConditionCheck &ref) : _lock{ref}
		{
			_locked = _lock.LockExclusive();
		}

		unique_lock (const unique_lock &) = delete;
		unique_lock (unique_lock &&) = delete;

		~unique_lock ()
		{
			if ( _locked )
				_lock.UnlockExclusive();
		}
	};

	template <>
	struct unique_lock< const FG::RWRaceConditionCheck > :
		unique_lock< FG::RWRaceConditionCheck >
	{
		explicit unique_lock (const FG::RWRaceConditionCheck &ref) :
			unique_lock< FG::RWRaceConditionCheck >{ const_cast<FG::RWRaceConditionCheck &>(ref) }
		{}
	};


	template <>
	struct shared_lock< FG::RWRaceConditionCheck >
	{
	private:
		FG::RWRaceConditionCheck &	_lock;
		bool						_locked	= false;

	public:
		explicit shared_lock (FG::RWRaceConditionCheck &ref) : _lock{ref}
		{
			_locked = _lock.LockShared();
		}

		shared_lock (const shared_lock &) = delete;
		shared_lock (shared_lock &&) = delete;

		~shared_lock ()
		{
			if ( _locked )
				_lock.UnlockShared();
		}
	};

	template <>
	struct shared_lock< const FG::RWRaceConditionCheck > :
		shared_lock< FG::RWRaceConditionCheck >
	{
		explicit shared_lock (const FG::RWRaceConditionCheck &ref) :
			shared_lock< FG::RWRaceConditionCheck >{ const_cast<FG::RWRaceConditionCheck &>(ref) }
		{}
	};

}	// std


#else

namespace FG
{

	//
	// Race Condition Check
	//
	struct RaceConditionCheck
	{
		void lock () const		{}
		void unlock () const	{}
	};
	

	//
	// Read/Write Race Condition Check
	//
	struct RWRaceConditionCheck
	{
		void lock () const			{}
		void unlock () const		{}

		void lock_shared () const	{}
		void unlock_shared () const	{}
	};

}	// FG

#endif	// FG_ENABLE_RACE_CONDITION_CHECK

