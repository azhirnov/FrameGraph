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

	template <size_t Size, uint UID>
	struct IDWithString
	{
	// variables
	private:
		StaticString<Size>	_name;
		HashVal				_hash;


	// methods
	public:
		IDWithString () {}
		explicit IDWithString (StringView name) : _name{name}, _hash{HashOf(_name)} {}
		//explicit IDWithString (const char *name) : _name{name}, _hash{HashOf(_name)} {}
		explicit constexpr IDWithString (const char *name) : _name{ name }, _hash{ name ? _CalcHash( name, 0 ) : HashVal() } {}

		ND_ bool operator == (const IDWithString &rhs) const		{ return _name == rhs._name; }
		ND_ bool operator >  (const IDWithString &rhs) const		{ return _name >  rhs._name; }
		ND_ bool operator <  (const IDWithString &rhs) const		{ return rhs._name > _name; }
		ND_ bool operator != (const IDWithString &rhs) const		{ return not (_name == rhs._name); }
		ND_ bool operator >= (const IDWithString &rhs) const		{ return not (_name <  rhs._name); }
		ND_ bool operator <= (const IDWithString &rhs) const		{ return not (_name >  rhs._name); }

		ND_ StringView  GetName ()		const						{ return _name; }

		ND_ size_t		GetHash ()		const						{ return size_t(_hash); }

		ND_ bool		IsDefined ()	const						{ return not _name.empty(); }

	private:
		constexpr HashVal _CalcHash (const char *str, const size_t idx) {
			return *str ? _CalcHash( str+1, idx+1 ) + HashVal( size_t(*str) << ((idx*7) % 28) ) : HashVal(idx);
		}
	};



	//
	// Resource ID
	//

	template <uint UID>
	struct ResourceID final
	{
	// types
	public:
		using Self		= ResourceID< UID >;
		using Index_t	= uint;

	// variables
	private:
		static constexpr Index_t	_invalidValue	= ~Index_t(0);
		Index_t						_value			= _invalidValue;

	// methods
	public:
		ResourceID () {}
		explicit ResourceID (Index_t val) : _value{val} {}

		ND_ bool		IsValid ()						const	{ return _value != _invalidValue; }
		ND_ Index_t		Get ()							const	{ return _value; }

		ND_ bool		operator == (const Self &rhs)	const	{ return _value == rhs._value; }
		ND_ bool		operator != (const Self &rhs)	const	{ return not (*this == rhs); }
	};

}	// _fg_hidden_



	using UniformID				= _fg_hidden_::IDWithString< 64, 1 >;
	using PushConstantID		= _fg_hidden_::IDWithString< 64, 2 >;
	using RenderTargetID		= _fg_hidden_::IDWithString< 64, 3 >;
	using DescriptorSetID		= _fg_hidden_::IDWithString< 64, 4 >;
	using SpecializationID		= _fg_hidden_::IDWithString< 64, 5 >;
	using VertexID				= _fg_hidden_::IDWithString< 64, 6 >;
	using VertexBufferID		= _fg_hidden_::IDWithString< 64, 7 >;
	using MemPoolID				= _fg_hidden_::IDWithString< 32, 8 >;
	
	using BufferID				= _fg_hidden_::ResourceID<1>;
	using ImageID				= _fg_hidden_::ResourceID<2>;
	using GPipelineID			= _fg_hidden_::ResourceID<3>;
	using CPipelineID			= _fg_hidden_::ResourceID<4>;
	using RTPipelineID			= _fg_hidden_::ResourceID<5>;
	using SamplerID				= _fg_hidden_::ResourceID<6>;
	using DescriptorSetLayoutID	= _fg_hidden_::ResourceID<7>;
	using PipelineResourcesID	= _fg_hidden_::ResourceID<8>;


}	// FG


namespace std
{

	template <size_t Size, uint32_t UID>
	struct hash< FG::_fg_hidden_::IDWithString<Size,UID > >
	{
		ND_ size_t  operator () (const FG::_fg_hidden_::IDWithString<Size,UID > &value) const noexcept {
			return value.GetHash();
		}
	};
	

	template <uint32_t UID>
	struct hash< FG::_fg_hidden_::ResourceID<UID> >
	{
		ND_ size_t  operator () (const FG::_fg_hidden_::ResourceID<UID> &value) const noexcept {
			return value.Get();
		}
	};

}	// std
