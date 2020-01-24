// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include <atomic>

namespace FGC
{

	//
	// Atomic Counter
	//

	template <typename T>
	struct AtomicCounter final
	{
		STATIC_ASSERT( Atomic<T>::is_always_lock_free );

	// types
	public:
		using Self		= AtomicCounter<T>;
		using Value_t	= T;


	// variables
	private:
		alignas(FG_CACHE_LINE) Atomic<T>		_value;
		

	// methods
	public:
		AtomicCounter () : _value{0} {}
		explicit AtomicCounter (const T &value) : _value{ value } {}
		explicit AtomicCounter (const Self &other) : _value{ other.Load() } {}

		void		operator ++ ()				{ Inc(); }
		void		operator -- ()				{ Dec(); }

		void		Inc ()						{ _value.fetch_add( 1, memory_order_relaxed ); }
		void		Dec ()						{ _value.fetch_sub( 1, memory_order_relaxed ); }

		ND_ const T	Load ()				const	{ return _value.load( memory_order_acquire ); }
		void		Store (const T &value)		{ _value.store( value, memory_order_release ); }
		

		ND_ bool	DecAndTest ()
		{
			const T  res = _value.fetch_sub( 1, memory_order_relaxed );
			ASSERT( res > 0 );

			if ( res == 1 ) {
				std::atomic_thread_fence( memory_order_acq_rel );
				return true;
			}
			return false;
		}


		ND_ const T	CmpExch (const T& val, const T& compare)
		{
			T	tmp = compare;
			_value.compare_exchange_strong( INOUT tmp, val, memory_order_acq_rel );
			return tmp;
		}
	};

}	// FGC
