// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{
	class ICommandBuffer;



	//
	// Command Buffer pointer
	//

	struct CommandBuffer
	{
	// types
	public:
		class Batch
		{
			friend struct CommandBuffer;
			
		protected:
			std::atomic<int>	_counter;

			virtual void Release () = 0;
		};


	// variables
	private:
		ICommandBuffer *	_cmdBuf		= null;
		Batch *				_batch		= null;


	// methods
	public:
		CommandBuffer ()
		{}
		
		CommandBuffer (std::nullptr_t)
		{}


		template <typename T1, typename T2>
		explicit CommandBuffer (T1 *cb, T2 *batch) :
			_cmdBuf{ static_cast<ICommandBuffer *>(cb) },
			_batch{ static_cast<Batch *>(batch) }
		{
			STATIC_ASSERT( std::is_base_of< ICommandBuffer, T1 >::value );
			STATIC_ASSERT( std::is_base_of< Batch, T2 >::value );

			_IncRef();
		}


		CommandBuffer (const CommandBuffer &other) :
			_cmdBuf{other._cmdBuf}, _batch{other._batch}
		{
			_IncRef();
		}
		

		CommandBuffer (CommandBuffer &&other) :
			_cmdBuf{other._cmdBuf}, _batch{other._batch}
		{
			other._cmdBuf = null;
			other._batch  = null;
		}


		~CommandBuffer ()
		{
			_DecRef();
		}


		CommandBuffer&  operator = (const CommandBuffer &rhs)
		{
			_DecRef();
			_cmdBuf	= rhs._cmdBuf;
			_batch	= rhs._batch;
			_IncRef();
			return *this;
		}


		CommandBuffer&  operator = (CommandBuffer &&rhs)
		{
			_DecRef();
			_cmdBuf	= rhs._cmdBuf;
			_batch	= rhs._batch;

			rhs._cmdBuf	= null;
			rhs._batch	= null;
			return *this;
		}

		CommandBuffer&  operator = (std::nullptr_t)
		{
			_DecRef();
			_cmdBuf	= null;
			_batch	= null;
			return *this;
		}


		ND_ ICommandBuffer*  operator -> () const		{ ASSERT( _cmdBuf );  return _cmdBuf; }

		ND_ explicit operator bool () const				{ return _batch != null; }

		ND_ ICommandBuffer*	GetCommandBuffer () const	{ return _cmdBuf; }
		ND_ Batch *			GetBatch ()			const	{ return _batch; }


	private:
		void _IncRef ()
		{
			if ( _batch )
				_batch->_counter.fetch_add( 1, memory_order_relaxed );
		}

		void _DecRef ()
		{
			if ( _batch )
			{
				auto	res = _batch->_counter.fetch_sub( 1, memory_order_relaxed );
				ASSERT( res > 0 );

				if ( res == 1 ) {
					std::atomic_thread_fence( std::memory_order_acquire );
					_batch->Release();
				}
			}
		}
	};


}	// FG
