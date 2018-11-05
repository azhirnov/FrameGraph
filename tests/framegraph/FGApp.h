// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framegraph/VFG.h"

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
		FGThreadPtr			_frameGraph;

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
		

	// helpers
	private:
		bool _Initialize (WindowPtr &&wnd);
		bool _Update ();
		void _Destroy ();

		bool _Visualize (const FrameGraphPtr &fg, EGraphVizFlags flags, StringView name = "temp/framegraph", bool autoOpen = true) const;
		
		bool _SavePNG (const String &filename, const ImageView &imageData) const;

		ND_ Array<uint8_t>	_CreateData (BytesU size) const;
		ND_ BufferID	_CreateBuffer (BytesU size, StringView name = Default) const;

		ND_ ImageID		_CreateImage2D (uint2 size, EPixelFormat fmt = EPixelFormat::RGBA8_UNorm, StringView name = Default) const;
		ND_ ImageID		_CreateLogicalImage2D (uint2 size, EPixelFormat fmt = EPixelFormat::RGBA8_UNorm, StringView name = Default) const;

		template <typename Arg0, typename ...Args>
		void _DeleteResources (Arg0 &arg0, Args& ...args);


	// implementation tests
	private:
		bool _ImplTest_Scene1 ();
		bool _ImplTest_Scene2 ();


	// drawing tests
	private:
		//bool _DrawTest_Scene1 ();
		bool _DrawTest_CopyBuffer ();
		bool _DrawTest_CopyImage2D ();
		bool _DrawTest_GLSLCompute ();
		bool _DrawTest_FirstTriangle ();
	};

	
	template <typename Arg0, typename ...Args>
	inline void FGApp::_DeleteResources (Arg0 &arg0, Args& ...args)
	{
		_frameGraph->DestroyResource( INOUT arg0 );

		if constexpr ( CountOf<Args...>() )
			_DeleteResources( std::forward<Args&>( args )... );
	}

}	// FG
