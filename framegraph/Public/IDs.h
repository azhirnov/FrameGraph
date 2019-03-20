// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"
#include "stl/CompileTime/Hash.h"

namespace FG
{
namespace _fg_hidden_
{

	//
	// ID With String
	//

	template <size_t Size, uint UID, bool Optimize, uint Seed = UMax>
	struct IDWithString
	{
	// variables
	private:
		StaticString<Size>	_name;
		HashVal				_hash;


	// methods
	public:
		constexpr IDWithString () {}
		explicit constexpr IDWithString (StringView name)  : _name{name}, _hash{CT_Hash( name.data(), name.length(), Seed )} {}
		explicit constexpr IDWithString (const char *name) : _name{name}, _hash{CT_Hash( name, UMax, Seed )} {}

		template <size_t StrSize>
		explicit constexpr IDWithString (const StaticString<StrSize> &name) : _name{name}, _hash{CT_Hash( name.data(), name.length(), Seed )} {}

		ND_ constexpr bool operator == (const IDWithString &rhs) const		{ return _hash == rhs._hash and _name == rhs._name; }
		ND_ constexpr bool operator != (const IDWithString &rhs) const		{ return not (*this == rhs); }
		ND_ constexpr bool operator >  (const IDWithString &rhs) const		{ return _hash != rhs._hash ? _hash > rhs._hash : _name >  rhs._name; }
		ND_ constexpr bool operator <  (const IDWithString &rhs) const		{ return rhs > *this; }
		ND_ constexpr bool operator >= (const IDWithString &rhs) const		{ return not (*this <  rhs); }
		ND_ constexpr bool operator <= (const IDWithString &rhs) const		{ return not (*this >  rhs); }

		ND_ constexpr StringView	GetName ()		const					{ return _name; }
		ND_ constexpr HashVal		GetHash ()		const					{ return _hash; }
		ND_ constexpr bool			IsDefined ()	const					{ return not _name.empty(); }
		ND_ constexpr static bool	IsOptimized ()							{ return false; }
	};
	
	

	//
	// ID With String
	//

	template <size_t Size, uint UID, uint Seed>
	struct IDWithString< Size, UID, true, Seed >
	{
	// variables
	private:
		HashVal						_hash;
		static constexpr HashVal	_emptyHash	= CT_Hash( "", 0, Seed );


	// methods
	public:
		constexpr IDWithString () {}
		explicit constexpr IDWithString (StringView name)  : _hash{CT_Hash( name.data(), name.length(), Seed )} {}
		explicit constexpr IDWithString (const char *name) : _hash{CT_Hash( name, UMax, Seed )} {}

		ND_ constexpr bool operator == (const IDWithString &rhs) const		{ return _hash == rhs._hash; }
		ND_ constexpr bool operator != (const IDWithString &rhs) const		{ return not (*this == rhs); }
		ND_ constexpr bool operator >  (const IDWithString &rhs) const		{ return _hash > rhs._hash; }
		ND_ constexpr bool operator <  (const IDWithString &rhs) const		{ return rhs > *this; }
		ND_ constexpr bool operator >= (const IDWithString &rhs) const		{ return not (*this <  rhs); }
		ND_ constexpr bool operator <= (const IDWithString &rhs) const		{ return not (*this >  rhs); }

		ND_ constexpr HashVal		GetHash ()		const					{ return _hash; }
		ND_ constexpr bool			IsDefined ()	const					{ return _hash != _emptyHash; }
		ND_ constexpr static bool	IsOptimized ()							{ return true; }
	};



	//
	// Resource ID
	//

	template <uint UID>
	struct ResourceID
	{
	// types
	public:
		using Self			= ResourceID< UID >;
		using Index_t		= uint16_t;
		using InstanceID_t	= uint16_t;
		using Value_t		= uint32_t;


	// variables
	private:
		Value_t		_value	= UMax;

		STATIC_ASSERT( sizeof(_value) == (sizeof(Index_t) + sizeof(InstanceID_t)) );

		static constexpr Value_t	_IndexMask	= sizeof(Index_t)*8 - 1;
		static constexpr Value_t	_InstOffset	= sizeof(Index_t)*8;


	// methods
	public:
		constexpr ResourceID () {}
		constexpr ResourceID (const ResourceID &other) : _value{other._value} {}
		explicit constexpr ResourceID (Index_t val, InstanceID_t inst) : _value{Value_t(val) | (Value_t(inst) << _InstOffset)} {}

		ND_ constexpr bool			IsValid ()						const	{ return _value != UMax; }
		ND_ constexpr Index_t		Index ()						const	{ return _value & _IndexMask; }
		ND_ constexpr InstanceID_t	InstanceID ()					const	{ return _value >> _InstOffset; }
		ND_ constexpr HashVal		GetHash ()						const	{ return HashOf(_value) + HashVal{UID}; }

		ND_ constexpr bool			operator == (const Self &rhs)	const	{ return _value == rhs._value; }
		ND_ constexpr bool			operator != (const Self &rhs)	const	{ return not (*this == rhs); }

		ND_ explicit constexpr		operator bool ()				const	{ return IsValid(); }

		ND_ static constexpr uint	GetUID ()								{ return UID; }
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
		ResourceIDWrap (Self &&other) : _id{other._id}			{ other._id = Default; }
		explicit ResourceIDWrap (const ID_t &id) : _id{id}		{}
		~ResourceIDWrap ()										{ ASSERT(not IsValid()); }	// ID must be released

		Self&			operator = (Self &&rhs)					{ ASSERT(not IsValid());  _id = rhs._id;  rhs._id = Default;  return *this; }

		ND_ bool		IsValid ()						const	{ return _id.IsValid(); }
		ND_ HashVal		GetHash ()						const	{ return _id.GetHash(); }
		
		ND_ ID_t		Release ()								{ ASSERT(IsValid());  ID_t temp{_id};  _id = Default;  return temp; }
		ND_ ID_t const&	Get ()							const	{ return _id; }

		ND_ bool		operator == (const Self &rhs)	const	{ return _id == rhs._id; }
		ND_ bool		operator != (const Self &rhs)	const	{ return _id != rhs._id; }
		
		ND_ explicit	operator bool ()				const	{ return IsValid(); }

		ND_				operator ID_t ()				const	{ return _id; }
	};

}	// _fg_hidden_


	enum class RenderTargetID : uint
	{
		DepthStencil	= FG_MaxColorBuffers,
		Depth			= DepthStencil,
		Unknown			= ~0u
	};


	using UniformID					= _fg_hidden_::IDWithString< 32,  1, FG_OPTIMIZE_IDS >;
	using PushConstantID			= _fg_hidden_::IDWithString< 32,  2, FG_OPTIMIZE_IDS >;
	//using RenderTargetID			= _fg_hidden_::IDWithString< 32,  3, FG_OPTIMIZE_IDS >;
	using DescriptorSetID			= _fg_hidden_::IDWithString< 32,  4, FG_OPTIMIZE_IDS >;
	using SpecializationID			= _fg_hidden_::IDWithString< 32,  5, FG_OPTIMIZE_IDS >;
	using VertexID					= _fg_hidden_::IDWithString< 32,  6, FG_OPTIMIZE_IDS >;
	using VertexBufferID			= _fg_hidden_::IDWithString< 32,  7, FG_OPTIMIZE_IDS >;
	using MemPoolID					= _fg_hidden_::IDWithString< 32,  8, FG_OPTIMIZE_IDS >;
	using RTShaderID				= _fg_hidden_::IDWithString< 32, 10, FG_OPTIMIZE_IDS >;
	using GeometryID				= _fg_hidden_::IDWithString< 32, 11, FG_OPTIMIZE_IDS >;
	using InstanceID				= _fg_hidden_::IDWithString< 32, 12, FG_OPTIMIZE_IDS >;
	
	// weak references
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
	using RawRTSceneID				= _fg_hidden_::ResourceID< 11 >;
	using RawRTGeometryID			= _fg_hidden_::ResourceID< 12 >;
	using RawRTShaderTableID		= _fg_hidden_::ResourceID< 13 >;
	using RawSwapchainID			= _fg_hidden_::ResourceID< 14 >;
	
	// strong references
	using BufferID					= _fg_hidden_::ResourceIDWrap< RawBufferID >;
	using ImageID					= _fg_hidden_::ResourceIDWrap< RawImageID >;
	using GPipelineID				= _fg_hidden_::ResourceIDWrap< RawGPipelineID >;
	using MPipelineID				= _fg_hidden_::ResourceIDWrap< RawMPipelineID >;
	using CPipelineID				= _fg_hidden_::ResourceIDWrap< RawCPipelineID >;
	using RTPipelineID				= _fg_hidden_::ResourceIDWrap< RawRTPipelineID >;
	using SamplerID					= _fg_hidden_::ResourceIDWrap< RawSamplerID >;
	using RTSceneID					= _fg_hidden_::ResourceIDWrap< RawRTSceneID >;
	using RTGeometryID				= _fg_hidden_::ResourceIDWrap< RawRTGeometryID >;
	using RTShaderTableID			= _fg_hidden_::ResourceIDWrap< RawRTShaderTableID >;
	using SwapchainID				= _fg_hidden_::ResourceIDWrap< RawSwapchainID >;


}	// FG


namespace std
{

	template <size_t Size, uint32_t UID, bool Optimize, uint32_t Seed>
	struct hash< FG::_fg_hidden_::IDWithString<Size, UID, Optimize, Seed> >
	{
		ND_ size_t  operator () (const FG::_fg_hidden_::IDWithString<Size, UID, Optimize, Seed> &value) const noexcept {
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
