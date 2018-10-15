// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VPipelineCache.h"
#include "UnitTestCommon.h"

using DescriptorSet		= PipelineDescription::DescriptorSet;
using PipelineLayout	= PipelineDescription::PipelineLayout;


namespace FG
{
	class VPipelineCacheUnitTest
	{
	private:
		VPipelineCache		_cache;

	public:
		VPipelineCacheUnitTest (const VDevice &dev) : _cache{ dev } {}

		bool Initialize ()
		{
			return _cache.Initialize();
		}

		void Deinitialize ()
		{
			return _cache.Deinitialize();
		}
	
		VDescriptorSetLayoutPtr  CreateDescriptorSetLayout (DescriptorSet &&ds)
		{
			return _cache._CreateDescriptorSetLayout( std::move(ds) );
		}

		VPipelineLayoutPtr  CreatePipelineLayout (PipelineLayout &&pl)
		{
			return _cache._CreatePipelineLayout( std::move(pl) );
		}
	};
}	// FG


static void DescriptorSet_Test1 (VPipelineCacheUnitTest &cache)
{
	DescriptorSet	ds1;
	ds1.bindingIndex = 0;
	ds1.id			 = DescriptorSetID("1");
	ds1.uniforms.insert({ UniformID("aaa"), PipelineDescription::UniformBuffer{ 128_b, BindingIndex{0,0}, EShaderStages::Vertex } });
	ds1.uniforms.insert({ UniformID("bbb"), PipelineDescription::Texture{ EImage::Tex2D, BindingIndex{0,1}, EShaderStages::Fragment } });

	VDescriptorSetLayoutPtr	layout1 = cache.CreateDescriptorSetLayout( DescriptorSet{ds1} );
	VDescriptorSetLayoutPtr	layout2 = cache.CreateDescriptorSetLayout( DescriptorSet{ds1} );
	TEST( layout1 == layout2 );
	

	ds1.bindingIndex = 1;
	ds1.id			 = DescriptorSetID("2");
	VDescriptorSetLayoutPtr	layout3 = cache.CreateDescriptorSetLayout( DescriptorSet{ds1} );
	TEST( layout3 == layout1 );

	
	ds1.id = DescriptorSetID("3");
	ds1.uniforms.insert({ UniformID("ccc"), PipelineDescription::Image{ EImage::Tex2D, EPixelFormat::RGBA8_UNorm, EShaderAccess::ReadWrite, BindingIndex{0,2}, EShaderStages::Fragment } });
	
	VDescriptorSetLayoutPtr	layout4 = cache.CreateDescriptorSetLayout( DescriptorSet{ds1} );
	TEST( layout4 != layout1 );
}


static void PipelineLayout_Test1 (VPipelineCacheUnitTest &cache)
{
	PipelineLayout	pl1;
	{
		DescriptorSet	ds1;
		ds1.bindingIndex = 0;
		ds1.id			 = DescriptorSetID("1");
		ds1.uniforms.insert({ UniformID("aaa"), PipelineDescription::UniformBuffer{ 128_b, BindingIndex{0,0}, EShaderStages::Vertex } });
		ds1.uniforms.insert({ UniformID("bbb"), PipelineDescription::Texture{ EImage::Tex2D, BindingIndex{0,1}, EShaderStages::Fragment } });

		pl1.descriptorSets.push_back( std::move(ds1) );
	}

	VPipelineLayoutPtr	layout1 = cache.CreatePipelineLayout( PipelineLayout{pl1} );
	VPipelineLayoutPtr	layout2 = cache.CreatePipelineLayout( PipelineLayout{pl1} );
	TEST( layout1 == layout2 );


	pl1.descriptorSets.back().id = DescriptorSetID("2");

	VPipelineLayoutPtr	layout3 = cache.CreatePipelineLayout( PipelineLayout{pl1} );
	TEST( layout3 != layout1 );
	

	pl1.descriptorSets.back().uniforms
		.insert({ UniformID("ccc"), PipelineDescription::Image{ EImage::Tex2D, EPixelFormat::RGBA8_UNorm, EShaderAccess::ReadWrite, BindingIndex{0,2}, EShaderStages::Fragment } });

	VPipelineLayoutPtr	layout4 = cache.CreatePipelineLayout( PipelineLayout{pl1} );
	TEST( layout4 != layout1 );
	TEST( layout4 != layout3 );
}


extern void UnitTest_VPipelineCache (const VDevice &dev)
{
	VPipelineCacheUnitTest		cache{ dev };

	TEST( cache.Initialize() );

	DescriptorSet_Test1( cache );
	PipelineLayout_Test1( cache );

	cache.Deinitialize();

    FG_LOGI( "UnitTest_VPipelineCache - passed" );
}
