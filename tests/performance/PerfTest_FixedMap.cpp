// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/FixedMap.h"
#include "PerfTestCommon.h"
#include <map>

using Clock_t = TimePoint_t::clock;


static void FixedMap_PerformanceTest1 ()
{
	const auto	RndString = [] ()
	{
		String	str;
		uint	len		= (rand() & 0xFF) + 12;
		uint	range	= ('Z' - 'A');

		for (uint i = 0; i < len; ++i) {
			uint v = rand() % (range*2);
			str += char(v > range ? 'a' + v - range : 'A' + v);
		}
		return str;
	};
	
	constexpr uint		num_items			= 64;

#ifdef FG_DEBUG
	const uint			num_search_count	= 100'000;
	const uint			num_iter			= 5'000;
#else
	const uint			num_search_count	= 1'000'000;
	const uint			num_iter			= 100'000;
#endif

	HashMap< StringView, StringView >				map1;	//map1.reserve( num_items );
	FixedMap< StringView, StringView, num_items >	map2;
	Array<Pair< String, String >>					data;
	Array<Pair<uint, bool>>							indices;	indices.resize( num_search_count );

	for (uint i = 0; i < num_iter * num_items; ++i)
	{
		data.push_back({ RndString(), RndString() });
	}

	for (size_t i = 0; i < indices.size(); ++i)
	{
		uint	rnd = rand();
		rnd = (rnd << 12) ^ (rnd >> 1);

		indices[i].first = rnd % data.size();
	}

	
	// test performance of insertion operation
	{
		auto	t1_start = Clock_t::now();
		{
			for (uint i = 0; i < num_iter; ++i) {
				for (uint j = 0; j < num_items; ++j)
				{
					const auto&	d = data[ i*num_items + j ];
					map1.insert({ d.first, d.second });
				}
				map1.clear();
			}
		}
		auto	t1_end = Clock_t::now();
	

		auto	t2_start = Clock_t::now();
		{
			for (uint i = 0; i < num_iter; ++i) {
				for (uint j = 0; j < num_items; ++j)
				{
					const auto&	d = data[ i*num_items + j ];
					map2.insert({ d.first, d.second });
				}
				map2.sort();
				map2.clear();
			}
		}
		auto	t2_end = Clock_t::now();


		auto	dt1 = t1_end - t1_start;
		auto	dt2 = t2_end - t2_start;

		FG_LOGI( "1: "s << ToString( dt1 ) << " vs " << ToString( dt2 ) );
		//TEST( dt1 > dt2 );
	}


	// test performance of foreach cycle
	{
		map1.clear();
		map2.clear();
		
		for (const auto& idx : indices)
		{
			if ( (idx.first & 3) and map1.size() < num_items )
			{
				const auto&	d = data[ idx.first ];

				map1.insert({ d.first, d.second });
				map2.insert({ d.first, d.second });
			}
		}
		map2.sort();
		TEST( map1.size() == num_items );
		TEST( map1.size() == map2.size() );

		
		auto	t1_start = Clock_t::now();
		uint	c1_found = 0;
		{
			for (auto& d : data)
			{
				for (auto& item : map1)
				{
					if ( d.second == item.second )
						++c1_found;
				}
			}
		}
		auto	t1_end = Clock_t::now();

		
		auto	t2_start = Clock_t::now();
		uint	c2_found = 0;
		{
			for (auto& d : data)
			{
				for (auto& item : map2)
				{
					if ( d.second == item.second )
						++c2_found;
				}
			}
		}
		auto	t2_end = Clock_t::now();
		
		TEST( c1_found == c2_found );


		auto	dt1 = t1_end - t1_start;
		auto	dt2 = t2_end - t2_start;

		FG_LOGI( "2: "s << ToString( dt1 ) << " vs " << ToString( dt2 ) );
		//TEST( dt1 > dt2 );
	}


	// test performance of find operation
	{
		map1.clear();
		map2.clear();
		
		for (auto& idx : indices)
		{
			idx.second = (idx.first & 7) and map1.size() < num_items;

			if ( idx.second )
			{
				const auto&	d = data[ idx.first ];

				map1.insert({ d.first, d.second });
				map2.insert({ d.first, d.second });
			}
		}
		map2.sort();
		TEST( map1.size() == num_items );
		TEST( map1.size() == map2.size() );


		auto	t1_start = Clock_t::now();
		uint	c1_found = 0;
		{
			for (auto& idx : indices)
			{
				auto	iter = map1.find( data[idx.first].first );

				if ( iter != map1.end() ) {
					++c1_found;
					ASSERT( iter->second == data[idx.first].second );
				}
			}
		}
		auto	t1_end = Clock_t::now();
	

		auto	t2_start = Clock_t::now();
		uint	c2_found = 0;
		{
			for (auto& idx : indices)
			{
				auto	iter = map2.find( data[idx.first].first );

				if ( iter != map2.end() ) {
					++c2_found;
					ASSERT( iter->second == data[idx.first].second );
				}
			}
		}
		auto	t2_end = Clock_t::now();

		TEST( c1_found == c2_found );


		auto	dt1 = t1_end - t1_start;
		auto	dt2 = t2_end - t2_start;

		FG_LOGI( "3: "s << ToString( dt1 ) << " vs " << ToString( dt2 ) );
		//TEST( (dt1*2)/3 > dt2 );
	}
}


static void FixedMap_PerformanceTest2 ()
{
	const auto	RndString = [] ()
	{
		String	str;
		uint	len		= (rand() & 0xFF) + 12;
		uint	range	= ('Z' - 'A');

		for (uint i = 0; i < len; ++i) {
			uint v = rand() % (range*2);
			str += char(v > range ? 'a' + v - range : 'A' + v);
		}
		return str;
	};
	
	constexpr uint		num_items			= 32;

#ifdef FG_DEBUG
	const uint			num_search_count	= 100'000;
	const uint			num_iter			= 10'000;
#else
	const uint			num_search_count	= 10'000'000;
	const uint			num_iter			= 100'000;
#endif

	HashMap< int, StringView >				map1;
	FixedMap< int, StringView, num_items >	map2;
	Array<Pair< int, String >>				data;
	Array<Pair<uint, bool>>					indices;	indices.resize( num_search_count );

	for (uint i = 0; i < num_iter * num_items; ++i)
	{
		data.push_back({ i<<2, RndString() });
	}

	for (size_t i = 0; i < indices.size(); ++i)
	{
		uint	rnd = rand();
		rnd = (rnd << 12) ^ (rnd >> 1);

		indices[i].first = rnd % data.size();
	}

	
	// test performance of insertion operation
	{
		auto	t1_start = Clock_t::now();
		{
			for (uint i = 0; i < num_iter; ++i) {
				for (uint j = 0; j < num_items; ++j)
				{
					const auto&	d = data[ i*num_items + j ];
					map1.insert({ d.first, d.second });
				}
				map1.clear();
			}
		}
		auto	t1_end = Clock_t::now();
	

		auto	t2_start = Clock_t::now();
		{
			for (uint i = 0; i < num_iter; ++i) {
				for (uint j = 0; j < num_items; ++j)
				{
					const auto&	d = data[ i*num_items + j ];
					map2.insert({ d.first, d.second });
				}
				map2.sort();
				map2.clear();
			}
		}
		auto	t2_end = Clock_t::now();


		auto	dt1 = t1_end - t1_start;
		auto	dt2 = t2_end - t2_start;

		FG_LOGI( "1: "s << ToString( dt1 ) << " vs " << ToString( dt2 ) );
		//TEST( dt1 > dt2 );
	}


	// test performance of foreach cycle
	{
		map1.clear();
		map2.clear();
		
		for (const auto& idx : indices)
		{
			if ( (idx.first & 3) and map1.size() < num_items )
			{
				const auto&	d = data[ idx.first ];

				map1.insert({ d.first, d.second });
				map2.insert({ d.first, d.second });
			}
		}
		map2.sort();
		TEST( map1.size() == num_items );
		TEST( map1.size() == map2.size() );

		
		auto	t1_start = Clock_t::now();
		uint	c1_found = 0;
		{
			for (auto& d : data)
			{
				for (auto& item : map1)
				{
					if ( d.second == item.second )
						++c1_found;
				}
			}
		}
		auto	t1_end = Clock_t::now();

		
		auto	t2_start = Clock_t::now();
		uint	c2_found = 0;
		{
			for (auto& d : data)
			{
				for (auto& item : map2)
				{
					if ( d.second == item.second )
						++c2_found;
				}
			}
		}
		auto	t2_end = Clock_t::now();
		
		TEST( c1_found == c2_found );


		auto	dt1 = t1_end - t1_start;
		auto	dt2 = t2_end - t2_start;

		FG_LOGI( "2: "s << ToString( dt1 ) << " vs " << ToString( dt2 ) );
		//TEST( dt1 > dt2 );
	}

	// test performance of find operation
	{
		map1.clear();
		map2.clear();
		
		for (auto& idx : indices)
		{
			idx.second = (idx.first & 7) and map1.size() < num_items;

			if ( idx.second )
			{
				const auto&	d = data[ idx.first ];

				map1.insert({ d.first, d.second });
				map2.insert({ d.first, d.second });
			}
		}
		map2.sort();
		TEST( map1.size() == num_items );
		TEST( map1.size() == map2.size() );


		auto	t1_start = Clock_t::now();
		uint	c1_found = 0;
		{
			for (auto& idx : indices)
			{
				auto	iter = map1.find( data[idx.first].first );

				if ( iter != map1.end() ) {
					++c1_found;
					ASSERT( iter->second == data[idx.first].second );
				}
			}
		}
		auto	t1_end = Clock_t::now();
	

		auto	t2_start = Clock_t::now();
		uint	c2_found = 0;
		{
			for (auto& idx : indices)
			{
				auto	iter = map2.find( data[idx.first].first );

				if ( iter != map2.end() ) {
					++c2_found;
					ASSERT( iter->second == data[idx.first].second );
				}
			}
		}
		auto	t2_end = Clock_t::now();

		TEST( c1_found == c2_found );


		auto	dt1 = t1_end - t1_start;
		auto	dt2 = t2_end - t2_start;

		FG_LOGI( "3: "s << ToString( dt1 ) << " vs " << ToString( dt2 ) );
		//TEST( (dt1*2) > dt2 );
	}
}


extern void PerformanceTest_FixedMap ()
{
	FixedMap_PerformanceTest1();
	FixedMap_PerformanceTest2();

    FG_LOGI( "PerformanceTest_FixedMap - finished" );
}
