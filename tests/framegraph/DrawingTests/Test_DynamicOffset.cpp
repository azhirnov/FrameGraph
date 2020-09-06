// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_DynamicOffset ()
	{
		if ( not _pplnCompiler )
		{
			FG_LOGI( TEST_NAME << " - skipped" );
			return true;
		}

		ComputePipelineDesc	ppln;

		ppln.AddShader( EShaderLangFormat::VKSL_100, "main", R"#(
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// @dynamic-offset
layout (binding=0, std140) uniform UB {
	vec4	data[4];
} ub;

// @dynamic-offset
layout (binding=1, std430) writeonly buffer SSB {
	vec4	data[4];
} ssb;

void main ()
{
	ssb.data[0] = ub.data[1];
	ssb.data[3] = ub.data[2];
	ssb.data[1] = ub.data[3];
	ssb.data[2] = ub.data[0];
}
)#" );
		
		const BytesU	sb_align		= _properties.minStorageBufferOffsetAlignment;
		const BytesU	ub_align		= _properties.minUniformBufferOffsetAlignment;
		const BytesU	base_off		= Max( sb_align, ub_align );
		const BytesU	buf_off			= base_off;
		const BytesU	src_size		= SizeOf<float4> * 4;
		const BytesU	dst_size		= SizeOf<float4> * 4;
		const BytesU	ub_offset		= AlignToLarger( base_off + buf_off, ub_align );
		const BytesU	sb_offset		= AlignToLarger( base_off + buf_off + src_size, sb_align );
		BufferID		buffer			= _frameGraph->CreateBuffer( BufferDesc{ Max( ub_offset + src_size, sb_offset + dst_size ),
																				 EBufferUsage::Uniform | EBufferUsage::Storage | EBufferUsage::Transfer },
																	 Default, "SharedBuffer" );
		const float		src_data[]		= { 1.0f, 2.0f, 3.0f, 4.0f,
											5.0f, 6.0f, 7.0f, 8.0f,
											1.1f, 2.2f, 2.3f, 2.4f,
											1.5f, 1.6f, 1.7f, 1.8f };
		CPipelineID		pipeline		= _frameGraph->CreatePipeline( ppln );
		CHECK_ERR( buffer and pipeline );

		PipelineResources	resources;
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources ));
		

		bool	cb_was_called	= false;
		bool	data_is_correct	= false;

		const auto	OnLoaded = [&src_data, dst_size, OUT &cb_was_called, OUT &data_is_correct] (BufferView data)
		{
			cb_was_called	= true;
			data_is_correct	= (data.size() == size_t(dst_size));

			const float*	dst_data = Cast<float>( data.Parts().front().data() );

			data_is_correct &= (src_data[1*4+0] == dst_data[0*4+0]);
			data_is_correct &= (src_data[1*4+1] == dst_data[0*4+1]);
			data_is_correct &= (src_data[1*4+2] == dst_data[0*4+2]);
			data_is_correct &= (src_data[1*4+3] == dst_data[0*4+3]);
			
			data_is_correct &= (src_data[2*4+0] == dst_data[3*4+0]);
			data_is_correct &= (src_data[2*4+1] == dst_data[3*4+1]);
			data_is_correct &= (src_data[2*4+2] == dst_data[3*4+2]);
			data_is_correct &= (src_data[2*4+3] == dst_data[3*4+3]);
			
			data_is_correct &= (src_data[3*4+0] == dst_data[1*4+0]);
			data_is_correct &= (src_data[3*4+1] == dst_data[1*4+1]);
			data_is_correct &= (src_data[3*4+2] == dst_data[1*4+2]);
			data_is_correct &= (src_data[3*4+3] == dst_data[1*4+3]);
			
			data_is_correct &= (src_data[0*4+0] == dst_data[2*4+0]);
			data_is_correct &= (src_data[0*4+1] == dst_data[2*4+1]);
			data_is_correct &= (src_data[0*4+2] == dst_data[2*4+2]);
			data_is_correct &= (src_data[0*4+3] == dst_data[2*4+3]);
		};

		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( EDebugFlags::Default ));
		CHECK_ERR( cmd );
		
		resources.SetBufferBase( UniformID("UB"),  0_b );
		resources.SetBufferBase( UniformID("SSB"), base_off );

		resources.BindBuffer( UniformID("UB"),  buffer, ub_offset, src_size );
		resources.BindBuffer( UniformID("SSB"), buffer, sb_offset, dst_size );

		Task	t_write		= cmd->AddTask( UpdateBuffer{}.SetBuffer( buffer ).AddData( src_data, CountOf(src_data), ub_offset ));
		Task	t_dispatch	= cmd->AddTask( DispatchCompute{}.SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ).Dispatch({ 1, 1 }).DependsOn( t_write ));
		Task	t_read		= cmd->AddTask( ReadBuffer{}.SetBuffer( buffer, sb_offset, dst_size ).SetCallback( OnLoaded ).DependsOn( t_dispatch ));
		Unused( t_read );

		CHECK_ERR( _frameGraph->Execute( cmd ));
		
		CHECK_ERR( not cb_was_called );
		CHECK_ERR( _frameGraph->WaitIdle() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));
		
		CHECK_ERR( cb_was_called );
		CHECK_ERR( data_is_correct );
		
		DeleteResources( pipeline, buffer );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG
