// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/CompileTime/TypeTraits.h"
#include <typeindex>

namespace FGC
{

	//
	// Reference to Any type
	//

	class AnyTypeRef
	{
	// types
	private:
		template <typename T>
		using NonAnyTypeRef = DisableIf< IsSameTypes< T, AnyTypeRef >, int >;


	// variables
	private:
		std::type_index		_typeId	= typeid(void);
		void *				_ref	= null;


	// methods
	public:
		AnyTypeRef () {}
		AnyTypeRef (AnyTypeRef &&) = default;
		AnyTypeRef (const AnyTypeRef &) = default;

		template <typename T>	AnyTypeRef (T &value, NonAnyTypeRef<T> = 0) : _typeId{ typeid(T) }, _ref{ std::addressof(value) } {}

		template <typename T>	ND_ bool  Is ()				const	{ return _typeId == typeid(T); }
		template <typename T>	ND_ bool  Is (const T &)	const	{ return _typeId == typeid(T); }

		template <typename T>	ND_ T *   GetIf ()			const	{ return Is<T>() ? static_cast<T*>( _ref ) : null; }
		template <typename T>	ND_ T &   As ()				const	{ ASSERT( Is<T>() );  return *static_cast<T*>( _ref ); }

		ND_ StringView  GetTypeName ()	const	{ return _typeId.name(); }
	};


}	// FGC
