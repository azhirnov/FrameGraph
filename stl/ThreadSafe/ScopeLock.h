#pragma once


#	define SCOPELOCK( _syncGetScopeLock_ ) \
		GX_STL::OS::ScopeLock	AUXDEF_UNITE_RAW( __scopeLock, __COUNTER__ ) ( _syncGetScopeLock_, true )
