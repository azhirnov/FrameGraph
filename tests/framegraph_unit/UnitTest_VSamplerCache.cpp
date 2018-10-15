// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VSamplerCache.h"
#include "UnitTestCommon.h"


static void VSamplerCache_Test1 (VSamplerCache &cache)
{
    SamplerDesc		desc;
    desc.SetAddressMode( EAddressMode::ClampToEdge );
    desc.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear );

    VSamplerPtr		samp1 = cache.CreateSampler( desc );
    VSamplerPtr		samp2 = cache.CreateSampler( desc );
    TEST( samp1 == samp2 );

    desc.SetAddressMode( EAddressMode::Repeat );

    VSamplerPtr		samp3 = cache.CreateSampler( desc );
    TEST( samp3 != samp2 );
}


extern void UnitTest_VSamplerCache (const VDevice &dev)
{
	VSamplerCache	cache{ dev };

    VSamplerCache_Test1( cache );

    FG_LOGI( "UnitTest_VSamplerCache - passed" );
}
