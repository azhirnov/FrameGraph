// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framegraph/VFG.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

	//
	// Frame Graph Test Application
	//

	class FGApp final : public IWindowEventListener
	{
	// types
	private:
		using TestFunc_t			= bool (FGApp::*) ();
		using TestQueue_t			= Deque<Pair< TestFunc_t, uint >>;
		using DebugReport			= VulkanDeviceExt::DebugReport;
		using VPipelineCompilerPtr	= SharedPtr< class VPipelineCompiler >;


	// variables
	private:
		VulkanDeviceExt			_vulkan;
		WindowPtr				_window;
		FGInstancePtr			_fgInstance;
		FGThreadPtr				_fgGraphics1;
		FGThreadPtr				_fgGraphics2;
		FGThreadPtr				_fgCompute;
		FGThreadPtr				_fgGraphicsCompute;
		VPipelineCompilerPtr	_pplnCompiler;

		TestQueue_t				_tests;
		uint					_testInvocations	= 0;
		uint					_testsPassed		= 0;
		uint					_testsFailed		= 0;


	// methods
	public:
		FGApp ();
		~FGApp ();

		static void Run ();


	// IWindowEventListener
	private:
		void OnResize (const uint2 &size) override;
		void OnRefresh () override {}
		void OnDestroy () override {}
		void OnUpdate () override {}
		void OnKey (StringView, EKeyAction) override {}
		void OnMouseMove (const float2 &) override {}
		

	// helpers
	private:
		bool _Initialize (WindowPtr &&wnd);
		bool _Update ();
		void _Destroy ();

		bool Visualize (StringView name) const;
		bool CompareDumps (StringView filename) const;
		bool SavePNG (const String &filename, const ImageView &imageData) const;

		template <typename ...Args>
		void DeleteResources (Args& ...args);

		template <typename Arg0, typename ...Args>
		void _RecursiveDeleteResources (Arg0 &arg0, Args& ...args);

		ND_ Array<uint8_t>	CreateData (BytesU size) const;

		ND_ static String  GetFuncName (StringView src);


	// implementation tests
	private:
		bool ImplTest_Scene1 ();
		bool ImplTest_CacheOverflow1 ();
		bool ImplTest_Multithreading1 ();
		bool ImplTest_Multithreading2 ();
		bool ImplTest_Multithreading3 ();


	// drawing tests
	private:
		bool Test_CopyBuffer1 ();
		bool Test_CopyImage1 ();
		bool Test_CopyImage2 ();
		bool Test_CopyImage3 ();
		bool Test_CopyImage4 ();
		bool Test_PushConst1 ();
		bool Test_Compute1 ();		// compute + specialization
		bool Test_Compute2 ();
		bool Test_DynamicOffset ();	// buffer dynamic offset
		bool Test_Draw1 ();
		bool Test_Draw2 ();			// with swapchain
		bool Test_Draw3 ();			// with scissor
		bool Test_Draw4 ();
		bool Test_ExternalCmdBuf1 ();
		bool Test_InvalidID ();
		bool Test_ReadAttachment1 ();
		bool Test_AsyncCompute1 ();
		bool Test_ShaderDebugger1 ();
		bool Test_ShaderDebugger2 ();
		bool Test_ArrayOfTextures1 ();
		bool Test_ArrayOfTextures2 ();

		// RTX only
		bool Test_DrawMeshes1 ();
		bool Test_TraceRays1 ();
		bool Test_TraceRays2 ();
		bool Test_TraceRays3 ();
		bool Test_ShadingRate1 ();
		bool Test_RayTracingDebugger1 ();
	};

	
	template <typename ...Args>
	inline void FGApp::DeleteResources (Args& ...args)
	{
		_RecursiveDeleteResources( std::forward<Args&>( args )... );
		CHECK( _fgInstance->WaitIdle() );
	}


	template <typename Arg0, typename ...Args>
	inline void  FGApp::_RecursiveDeleteResources (Arg0 &arg0, Args& ...args)
	{
		_fgGraphics1->ReleaseResource( INOUT arg0 );

		if constexpr ( CountOf<Args...>() )
			_RecursiveDeleteResources( std::forward<Args&>( args )... );
	}
	

	inline String  FGApp::GetFuncName (StringView src)
	{
		size_t	pos = src.find_last_of( "::" );

		if ( pos != StringView::npos )
			return String{ src.substr( pos+1 )};
		else
			return String{ src };
	}

#	define TEST_NAME	GetFuncName( FG_FUNCTION_NAME )


}	// FG
