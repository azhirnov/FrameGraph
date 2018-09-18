// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/Types.h"

namespace FG
{
namespace _fg_hidden_
{

	//
	// ID With String
	//

	template <size_t Size, uint32_t UID>
	struct IDWithString
	{
	// variables
	private:
		StaticString<64>	_name;

	// methods
	public:
		IDWithString () {}
		explicit IDWithString (StringView name) : _name{name} {}

		ND_ bool operator == (const IDWithString &rhs) const		{ return _name == rhs._name; }
		ND_ bool operator >  (const IDWithString &rhs) const		{ return _name >  rhs._name; }
		ND_ bool operator <  (const IDWithString &rhs) const		{ return rhs._name > _name; }
		ND_ bool operator != (const IDWithString &rhs) const		{ return not (_name == rhs._name); }
		ND_ bool operator >= (const IDWithString &rhs) const		{ return not (_name <  rhs._name); }
		ND_ bool operator <= (const IDWithString &rhs) const		{ return not (_name >  rhs._name); }

		ND_ StringView  GetName ()		const						{ return _name; }

		ND_ size_t		GetHash ()		const						{ return std::hash< decltype(_name) >()( _name ); }

		ND_ bool		IsDefined ()	const						{ return not _name.empty(); }
	};

}	// _fg_hidden_


	using UniformID			= _fg_hidden_::IDWithString< 64, 1 >;
	using PushConstantID	= _fg_hidden_::IDWithString< 64, 2 >;
	using RenderTargetID	= _fg_hidden_::IDWithString< 64, 3 >;
	using DescriptorSetID	= _fg_hidden_::IDWithString< 64, 4 >;
	using SpecializationID	= _fg_hidden_::IDWithString< 64, 5 >;
	using VertexID			= _fg_hidden_::IDWithString< 64, 6 >;
	using VertexBufferID	= _fg_hidden_::IDWithString< 64, 7 >;


}	// FG


namespace std
{

	template <size_t Size, uint32_t UID>
	struct hash< FG::_fg_hidden_::IDWithString<Size,UID > >
	{
		ND_ size_t  operator () (const FG::_fg_hidden_::IDWithString<Size,UID > &value) const noexcept
		{
			return value.GetHash();
		}
	};

}	// std
