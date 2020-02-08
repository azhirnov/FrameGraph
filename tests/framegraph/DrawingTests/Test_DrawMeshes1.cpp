// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_DrawMeshes1 ()
	{
		if ( not _vulkan.GetDeviceMeshShaderFeatures().meshShader )
			return true;

		MeshPipelineDesc	ppln;

		ppln.AddShader( EShader::Mesh, EShaderLangFormat::VKSL_100, "main", R"#(
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_NV_mesh_shader : require

layout(local_size_x=3) in;
layout(triangles) out;
layout(max_vertices=3, max_primitives=1) out;

out gl_MeshPerVertexNV {
	vec4	gl_Position;
} gl_MeshVerticesNV[]; // [max_vertices]

layout(location=0) out MeshOutput {
	vec4	color;
} Output[]; // [max_vertices]

const vec2	g_Positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

const vec3	g_Colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main ()
{
	const uint I = gl_LocalInvocationID.x;

	gl_MeshVerticesNV[I].gl_Position	= vec4( g_Positions[I], 0.0, 1.0 );
	Output[I].color						= vec4( g_Colors[I], 1.0 );
	gl_PrimitiveIndicesNV[I]			= I;

	if ( I == 0 )
		gl_PrimitiveCountNV = 1;
}
)#" );
		
		ppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location=0) in MeshOutput {
	vec4	color;
} Input;

layout(location=0) out vec4  out_Color;

void main() {
	out_Color = Input.color;
}
)#" );
		
		const uint2		view_size	= {800, 600};
		ImageID			image		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																			EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "RenderTarget" );

		MPipelineID		pipeline	= _frameGraph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );

		
		bool		data_is_correct = false;

		const auto	OnLoaded =	[OUT &data_is_correct] (const ImageView &imageData)
		{
			const auto	TestPixel = [&imageData] (float x, float y, const RGBA32f &color)
			{
				uint	ix	 = uint( (x + 1.0f) * 0.5f * float(imageData.Dimension().x) + 0.5f );
				uint	iy	 = uint( (y + 1.0f) * 0.5f * float(imageData.Dimension().y) + 0.5f );

				RGBA32f	col;
				imageData.Load( uint3(ix, iy, 0), OUT col );
				
				bool	is_equal = All(Equals( col, color, 0.1f ));
				ASSERT( is_equal );
				return is_equal;
			};

			data_is_correct  = true;
			data_is_correct &= TestPixel( 0.00f, -0.49f, RGBA32f{1.0f, 0.0f, 0.0f, 1.0f} );
			data_is_correct &= TestPixel( 0.49f,  0.49f, RGBA32f{0.0f, 1.0f, 0.0f, 1.0f} );
			data_is_correct &= TestPixel(-0.49f,  0.49f, RGBA32f{0.0f, 0.0f, 1.0f, 1.0f} );
			
			data_is_correct &= TestPixel( 0.00f, -0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.51f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel(-0.51f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.00f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.51f, -0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel(-0.51f, -0.51f, RGBA32f{0.0f} );
		};

		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( EDebugFlags::Default ));
		CHECK_ERR( cmd );

		LogicalPassID	render_pass	= cmd->CreateRenderPass( RenderPassDesc( view_size )
											.AddTarget( RenderTargetID::Color_0, image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
											.AddViewport( view_size ) );
		
		cmd->AddTask( render_pass, DrawMeshes().Draw( 1 ).SetPipeline( pipeline ));

		Task	t_draw	= cmd->AddTask( SubmitRenderPass{ render_pass });
		Task	t_read	= cmd->AddTask( ReadImage().SetImage( image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_draw ) );
		FG_UNUSED( t_read );

		CHECK_ERR( _frameGraph->Execute( cmd ));
		CHECK_ERR( _frameGraph->WaitIdle() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( data_is_correct );

		DeleteResources( image, pipeline );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG
