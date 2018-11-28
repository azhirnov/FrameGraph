// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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
		using TestFunc_t	= bool (FGApp::*) ();
		using TestQueue_t	= Deque<Pair< TestFunc_t, uint >>;
		using DebugReport	= VulkanDeviceExt::DebugReport;


	// variables
	private:
		VulkanDeviceExt		_vulkan;
		WindowPtr			_window;
		FrameGraphPtr		_frameGraphInst;
		FGThreadPtr			_frameGraph1;
		FGThreadPtr			_frameGraph2;

		TestQueue_t			_tests;
		uint				_testInvocations	= 0;
		uint				_testsPassed		= 0;
		uint				_testsFailed		= 0;


	// methods
	public:
		FGApp ();
		~FGApp ();

		static void Run ();


	// IWindowEventListener
	private:
		void OnResize (const uint2 &size) override;
		void OnRefresh () override;
		void OnDestroy () override;
		void OnUpdate () override;
		void OnKey (StringView, EKeyAction) override {}
		void OnMouseMove (const float2 &) override {}
		

	// helpers
	private:
		bool _Initialize (WindowPtr &&wnd);
		bool _Update ();
		void _Destroy ();

		bool Visualize (StringView name, bool autoOpen = false) const;
		bool CompareDumps (StringView filename) const;
		bool SavePNG (const String &filename, const ImageView &imageData) const;

		ND_ Array<uint8_t>	CreateData (BytesU size) const;
		ND_ BufferID	CreateBuffer (BytesU size, StringView name = Default) const;

		ND_ ImageID		CreateImage2D (uint2 size, EPixelFormat fmt = EPixelFormat::RGBA8_UNorm, StringView name = Default) const;
		ND_ ImageID		CreateLogicalImage2D (uint2 size, EPixelFormat fmt = EPixelFormat::RGBA8_UNorm, StringView name = Default) const;

		template <typename ...Args>
		void DeleteResources (Args& ...args);

		template <typename Arg0, typename ...Args>
		void _RecursiveDeleteResources (Arg0 &arg0, Args& ...args);

		ND_ static String  GetFuncName (StringView src);


	// implementation tests
	private:
		bool ImplTest_Scene1 ();


	// drawing tests
	private:
		bool Test_CopyBuffer1 ();
		bool Test_CopyImage1 ();
		bool Test_CopyImage2 ();
		bool Test_CopyImage3 ();
		bool Test_CopyImage4 ();
		bool Test_Compute1 ();
		bool Test_Draw1 ();
		bool Test_Draw2 ();
		bool Test_Draw3 ();
	};

	
	template <typename ...Args>
	inline void FGApp::DeleteResources (Args& ...args)
	{
		_RecursiveDeleteResources( std::forward<Args&>( args )... );
		CHECK( _frameGraphInst->WaitIdle() );
	}


	template <typename Arg0, typename ...Args>
	inline void  FGApp::_RecursiveDeleteResources (Arg0 &arg0, Args& ...args)
	{
		_frameGraph1->DestroyResource( INOUT arg0 );

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
