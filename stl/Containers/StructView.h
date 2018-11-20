// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/Math/Bytes.h"
#include "stl/Containers/ArrayView.h"

namespace FG
{

	//
	// Structure View
	//

	template <typename T>
	struct StructView
	{
	// types
	public:
		using Self	= StructView< T >;
		// TODO: iterator

	private:
		static constexpr uint	DBG_VIEW_COUNT = 400;

		struct _IViewer
		{
			virtual ~_IViewer () {}
		};
		

		template <typename St, size_t Padding>
		struct _ViewerWithPaddingUnaligned final : _IViewer
		{
		// types
			#pragma pack (push, 1)
			struct Element {
				T			value;
				uint8_t		_padding [Padding];
			};
			#pragma pack (pop)
			using ElementsPtr_t = Element const (*) [DBG_VIEW_COUNT];
			
		// variables
			ElementsPtr_t const		elements;
			
		// methods
			explicit _ViewerWithPaddingUnaligned (const void *ptr) : elements{ BitCast<ElementsPtr_t>(ptr) }
			{ STATIC_ASSERT( sizeof(Element) == sizeof(St) ); }
		};
		

		template <typename St, size_t Padding>
		struct _ViewerWithPadding final : _IViewer
		{
		// types
			struct Element {
				T			value;
				uint8_t		_padding [Padding];
			};
			using ElementsPtr_t = Element const (*) [DBG_VIEW_COUNT];
			
		// variables
			ElementsPtr_t const		elements;
			
		// methods
			explicit _ViewerWithPadding (const void *ptr) : elements{ BitCast<ElementsPtr_t>(ptr) }
			{ STATIC_ASSERT( sizeof(Element) == sizeof(St) ); }
		};


		template <typename St>
		struct _ViewerImpl final : _IViewer
		{
		// types
			using ElementsPtr_t = T const (*) [DBG_VIEW_COUNT];
			
		// variables
			ElementsPtr_t const		elements;
			
		// methods
			explicit _ViewerImpl (const void *ptr) : elements{ BitCast<ElementsPtr_t>(ptr) } {}
		};


	// variables
	private:
		void const *	_array		= null;
		size_t			_count		= 0;
		uint			_stride		= 0;

		DEBUG_ONLY(
			_IViewer*	_dbgView	= null;	
		)


	// methods
	public:
		StructView ()
		{}

		StructView (ArrayView<T> arr) :
			_array{ arr.data() }, _count{ arr.size() }, _stride{ sizeof(T) }
		{
			DEBUG_ONLY( _dbgView = _CreateView<T, sizeof(T)>( _array ));
		}

		StructView (Self &&other) :
			_array{other._array}, _count{other._count}, _stride{other._stride}
		{
			DEBUG_ONLY( std::swap( _dbgView, other._dbgView ));
		}
/*
		template <typename Class>
		StructView (ArrayView<Class> arr, T (Class::*member)) :
			_array{ arr.data() + OffsetOf(member) }, _count{ arr.size() }, _stride{ sizeof(Class) }
		{
			DEBUG_ONLY( _dbgView = _CreateView<Class, sizeof(Class)>( _array ));
		}
		*/
		StructView (const void *ptr, size_t count, uint stride) :
			_array{ptr}, _count{count}, _stride{stride}
		{}

		~StructView ()
		{
			DEBUG_ONLY( delete _dbgView );
		}


		ND_ size_t		size ()					const	{ return _count; }
		ND_ bool		empty ()				const	{ return _count == 0; }
		ND_ T const &	operator [] (size_t i)	const	{ ASSERT(i < _count);  return *static_cast<T const *>(_array + BytesU(i * _stride)); }

		ND_ T const&	front ()				const	{ return operator[] (0); }
		ND_ T const&	back ()					const	{ return operator[] (_count-1); }


		ND_ bool  operator == (StructView<T> rhs) const
		{
			if ( size() != rhs.size() )
				return false;

			for (size_t i = 0; i < size(); ++i) {
				if ( not ((*this)[i] == rhs[i]) )
					return false;
			}
			return true;
		}

		ND_ StructView<T>  section (size_t first, size_t count) const
		{
			return first < size() ?
                    StructView<T>{ _array + BytesU(first * _stride), Min(size() - first, count) } :
					StructView<T>{};
		}


	private:
		template <typename Class, size_t Stride>
		ND_ static _IViewer*  _CreateView (const void *ptr)
		{
			STATIC_ASSERT( Stride >= sizeof(T) );
			const size_t	padding = Stride - sizeof(T);

			if constexpr ( padding == 0 )
				return new _ViewerImpl< Class >{ ptr };
			else
			if constexpr ( padding % alignof(T) == 0 )
				return new _ViewerWithPadding< Class, padding >{ ptr };
			else
				return new _ViewerWithPaddingUnaligned< Class, padding >{ ptr };
		}
	};


}	// FG
