// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/IDs.h"
#include "stl/ThreadSafe/DummyLock.h"
#include <random>

namespace FG
{

	//
	// Hash Collision Check
	//

	template <typename GuardType = DummySharedLock>
	class HashCollisionCheck
	{
	// types
	private:
		using StString_t	= StaticString<64>;
		using UniqueIDs_t	= std::unordered_multimap< size_t, StString_t >;
		
		struct Info
		{
			UniqueIDs_t	data;
			uint		seed			= 0;
			bool		hasCollisions	= false;

			Info () {}
			explicit Info (uint seed) : seed{seed} {}
		};
		using IdMap_t	= HashMap< /*UID*/uint, Info >;


	// variables
	private:
		GuardType	_guard;
		IdMap_t		_idMap;


	// methods
	public:
		~HashCollisionCheck ();
		
		template <size_t Size, uint UID, bool Optimize, uint Seed>
		void  Add (const _fg_hidden_::IDWithString<Size, UID, Optimize, Seed> &id);
		
		template <size_t Size, uint UID, bool Optimize, uint Seed>
		ND_ uint  RecalculateSeed (const _fg_hidden_::IDWithString<Size, UID, Optimize, Seed> &);
		
	private:
		ND_ uint  _RecalculateSeed (Info &) const;
	};

	
/*
=================================================
	Add
=================================================
*/
	template <typename GuardType>
	template <size_t Size, uint UID, bool Optimize, uint Seed>
	inline void  HashCollisionCheck<GuardType>::Add (const _fg_hidden_::IDWithString<Size, UID, Optimize, Seed> &id)
	{
		if constexpr( not Optimize )
		{
			EXLOCK( _guard );
			STATIC_ASSERT( Size <= StString_t::capacity() );

			auto&	info	 = _idMap.insert({ UID, Info{Seed} }).first->second;
			size_t	key		 = size_t(id.GetHash());
			auto	iter	 = info.data.find( key );
			bool	inserted = false;

			for (; iter != info.data.end() and iter->first == key; ++iter)
			{
				if ( iter->second != id.GetName() )
				{
					ASSERT( !"hash collision detected" );
					info.hasCollisions = true;
				}
				inserted = true;
			}

			if ( not inserted )
				info.data.insert({ key, id.GetName() });
		}
	}
	
/*
=================================================
	destructor
=================================================
*/
	template <typename GuardType>
	inline HashCollisionCheck<GuardType>::~HashCollisionCheck ()
	{
		EXLOCK( _guard );
		for (auto& id : _idMap)
		{
			if ( id.second.hasCollisions )
			{
				uint	seed = _RecalculateSeed( id.second );
				CHECK( seed );
			}
		}
	}

/*
=================================================
	RecalculateSeed
=================================================
*/
	template <typename GuardType>
	template <size_t Size, uint UID, bool Optimize, uint Seed>
	inline uint  HashCollisionCheck<GuardType>::RecalculateSeed (const _fg_hidden_::IDWithString<Size, UID, Optimize, Seed> &)
	{
		SHAREDLOCK( _guard );

		auto	iter = _idMap.find( UID );
		if ( iter == _idMap.end() )
			return Seed;
		
		if ( iter->second.seed != Seed )
			return iter->second.seed;

		return _RecalculateSeed( iter->second );
	}
	
	template <typename GuardType>
	inline uint  HashCollisionCheck<GuardType>::_RecalculateSeed (Info &info) const
	{
		std::mt19937	gen{ std::random_device{}() };
		for (;;)
		{
			// generate new seed
			uint	new_seed		= gen();
			bool	has_collision	= false;

			HashMap< size_t, StString_t >	map;
			for (auto& item : info.data)
			{
				size_t	key = size_t(CT_Hash( item.second.data(), item.second.length(), new_seed ));

				if ( not map.insert({ key, StringView{item.second} }).second )
				{
					has_collision = true;
					break;
				}
			}

			if ( not has_collision )
			{
				info.seed = new_seed;
				return new_seed;
			}
		}
	}


}	// FG
