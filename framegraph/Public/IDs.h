// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

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
		explicit IDWithString (const char *name) : _name{name}, _hash{HashOf(_name)} {}
		//explicit constexpr IDWithString (const char *name) : _name{ name }, _hash{ name ? _CalcHash( name, 0 ) : HashVal() } {}	// TODO

		ND_ bool operator == (const IDWithString &rhs) const		{ return _hash == rhs._hash and _name == rhs._name; }
		ND_ bool operator != (const IDWithString &rhs) const		{ return not (*this == rhs); }
		ND_ bool operator >  (const IDWithString &rhs) const		{ return _hash != rhs._hash ? _hash > rhs._hash : _name >  rhs._name; }
		ND_ bool operator <  (const IDWithString &rhs) const		{ return rhs > *this; }
		ND_ bool operator >= (const IDWithString &rhs) const		{ return not (*this <  rhs); }
		ND_ bool operator <= (const IDWithString &rhs) const		{ return not (*this >  rhs); }

		ND_ StringView  GetName ()		const						{ return _name; }
		ND_ HashVal		GetHash ()		const						{ return _hash; }
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
		using Self			= ResourceID< UID >;
		using Index_t		= uint;
		using InstanceID_t	= uint;


	// variables
	private:
		uint64_t	_value	= ~0ull;

		STATIC_ASSERT( sizeof(_value) == (sizeof(Index_t) + sizeof(InstanceID_t)) );


	// methods
	public:
		ResourceID () {}
		explicit ResourceID (Index_t val, InstanceID_t inst) : _value{ uint64_t(val) | (uint64_t(inst) << 32) } {}

		ND_ bool			IsValid ()						const	{ return _value != ~0ull; }
		ND_ Index_t			Index ()						const	{ return _value & 0xFFFFFFFFull; }
		ND_ InstanceID_t	InstanceID ()					const	{ return _value >> 32; }
		ND_ HashVal			GetHash ()						const	{ return HashVal{_value}; }

		ND_ bool			operator == (const Self &rhs)	const	{ return _value == rhs._value; }
		ND_ bool			operator != (const Self &rhs)	const	{ return not (*this == rhs); }

		ND_ explicit		operator bool ()				const	{ return IsValid(); }

		ND_ static constexpr uint GetUID ()							{ return UID; }
	};
	


	//
	// Wrapper for Resource ID
	//

	template <typename T>
	struct ResourceIDWrap;
	
	template <uint UID>
	struct ResourceIDWrap< ResourceID<UID> >
	{
	// types
	public:
		using ID_t	= ResourceID< UID >;
		using Self	= ResourceIDWrap< ID_t >;

	// variables
	private:
		ID_t	_id;

	// methods
	public:
		ResourceIDWrap ()										{}
		//ResourceIDWrap (const Self &other) : _id{other._id}		{}
		ResourceIDWrap (Self &&other) : _id{other._id}			{ other._id = Default; }
		explicit ResourceIDWrap (const ID_t &id) : _id{id}		{}
		~ResourceIDWrap ()										{ ASSERT(not IsValid()); }	// ID must be released

		//Self&			operator = (const Self &rhs)					{ ASSERT(not IsValid());  _id = rhs._id;  return *this; }
		Self&			operator = (Self &&rhs)							{ ASSERT(not IsValid());  _id = rhs._id;  rhs._id = Default;  return *this; }
		//Self&			operator = (const ID_t &rhs)					{ ASSERT(not IsValid());  _id = rhs;  return *this; }

		ND_ bool		IsValid ()						const			{ return _id.IsValid(); }
		ND_ auto		Index ()						const			{ return _id.Index(); }
		ND_ auto		InstanceID ()					const			{ return _id.InstanceID(); }
		ND_ HashVal		GetHash ()						const			{ return _id.GetHash(); }
		
		ND_ ID_t		Release ()										{ ASSERT(IsValid());  ID_t temp{_id};  _id = Default;  return temp; }
		ND_ ID_t const&	Get ()							const			{ ASSERT(IsValid());  return _id; }

		ND_ bool		operator == (const Self &rhs)	const			{ return _id == rhs._id; }
		ND_ bool		operator != (const Self &rhs)	const			{ return _id != rhs._id; }
		
		ND_ explicit	operator bool ()				const			{ return IsValid(); }
	};

}	// _fg_hidden_



	using UniformID					= _fg_hidden_::IDWithString< 64, 1 >;
	using PushConstantID			= _fg_hidden_::IDWithString< 64, 2 >;
	using RenderTargetID			= _fg_hidden_::IDWithString< 64, 3 >;
	using DescriptorSetID			= _fg_hidden_::IDWithString< 64, 4 >;
	using SpecializationID			= _fg_hidden_::IDWithString< 64, 5 >;
	using VertexID					= _fg_hidden_::IDWithString< 64, 6 >;
	using VertexBufferID			= _fg_hidden_::IDWithString< 64, 7 >;
	using MemPoolID					= _fg_hidden_::IDWithString< 32, 8 >;
	using CommandBatchID			= _fg_hidden_::IDWithString< 32, 9 >;
	
	using RawBufferID				= _fg_hidden_::ResourceID< 1 >;
	using RawImageID				= _fg_hidden_::ResourceID< 2 >;
	using RawGPipelineID			= _fg_hidden_::ResourceID< 3 >;
	using RawMPipelineID			= _fg_hidden_::ResourceID< 4 >;
	using RawCPipelineID			= _fg_hidden_::ResourceID< 5 >;
	using RawRTPipelineID			= _fg_hidden_::ResourceID< 6 >;
	using RawSamplerID				= _fg_hidden_::ResourceID< 7 >;
	using RawDescriptorSetLayoutID	= _fg_hidden_::ResourceID< 8 >;
	using RawPipelineResourcesID	= _fg_hidden_::ResourceID< 9 >;
	using LogicalPassID				= _fg_hidden_::ResourceID< 10 >;
	
	using BufferID					= _fg_hidden_::ResourceIDWrap< RawBufferID >;
	using ImageID					= _fg_hidden_::ResourceIDWrap< RawImageID >;
	using GPipelineID				= _fg_hidden_::ResourceIDWrap< RawGPipelineID >;
	using MPipelineID				= _fg_hidden_::ResourceIDWrap< RawMPipelineID >;
	using CPipelineID				= _fg_hidden_::ResourceIDWrap< RawCPipelineID >;
	using RTPipelineID				= _fg_hidden_::ResourceIDWrap< RawRTPipelineID >;
	using SamplerID					= _fg_hidden_::ResourceIDWrap< RawSamplerID >;


}	// FG


namespace std
{

	template <size_t Size, uint32_t UID>
	struct hash< FG::_fg_hidden_::IDWithString<Size,UID > >
	{
		ND_ size_t  operator () (const FG::_fg_hidden_::IDWithString<Size,UID > &value) const noexcept {
			return size_t(value.GetHash());
		}
	};
	
	
	template <uint32_t UID>
	struct hash< FG::_fg_hidden_::ResourceID<UID> >
	{
		ND_ size_t  operator () (const FG::_fg_hidden_::ResourceID<UID> &value) const noexcept {
			return size_t(value.GetHash());
		}
	};
	

	template <typename T>
	struct hash< FG::_fg_hidden_::ResourceIDWrap<T> >
	{
		ND_ size_t  operator () (const FG::_fg_hidden_::ResourceIDWrap<T> &value) const noexcept {
			return size_t(value.GetHash());
		}
	};

}	// std
