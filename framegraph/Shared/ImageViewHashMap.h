// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "framegraph/Public/LowLevel/ImageResource.h"

namespace FG
{


	//
	// Image View Hash Map
	//

	template <typename ViewType>
	struct ImageViewHashMap
	{
	// types
	public:
		using Key_t = ImageViewDesc;
		
	private:
		using ImageViewMap_t	= HashMap< Key_t, ViewType >;
		using Self				= ImageViewHashMap< ViewType >;
		using Description_t		= ImageResource::Description_t;


	// variables
	private:
		ImageViewMap_t		_map;


	// methods
	public:
		ImageViewHashMap () {}

		ViewType Find (const Key_t &key) const;

		void Add (const Key_t &key, ViewType view);

		void		Clear ()			{ _map.clear(); }

		ND_ bool	Empty ()	const	{ return _map.empty(); }

		ND_ auto	begin ()			{ return _map.begin(); }
		ND_ auto	begin ()	const	{ return _map.begin(); }
		ND_ auto	end ()				{ return _map.end(); }
		ND_ auto	end ()		const	{ return _map.end(); }
	};
	


/*
=================================================
	Find
=================================================
*/
	template <typename ViewType>
	inline ViewType ImageViewHashMap<ViewType>::Find (const Key_t &key) const
	{
		auto	iter = _map.find( key );

		if ( iter != _map.end() )
			return iter->second;

		return ViewType(0);
	}
	
/*
=================================================
	Add
=================================================
*/
	template <typename ViewType>
	inline void ImageViewHashMap<ViewType>::Add (const Key_t &key, ViewType view)
	{
		_map.insert({ key, view });
	}


}	//FG
