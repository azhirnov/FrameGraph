// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{
namespace FGColorScheme
{
	
	//
	// Default Dark
	//
	struct DefaultDark
	{
		static constexpr RGBA8u		RenderPass				= HtmlColor::OrangeRed;
		static constexpr RGBA8u		Compute					= HtmlColor::MediumBlue;
		static constexpr RGBA8u		DeviceLocalTransfer		= HtmlColor::Green;
		static constexpr RGBA8u		HostToDeviceTransfer	= HtmlColor::BlueViolet;
		static constexpr RGBA8u		DeviceToHostTransfer	= HtmlColor::BlueViolet;
		static constexpr RGBA8u		Present					= HtmlColor::Red;
		static constexpr RGBA8u		RayTracing				= HtmlColor::Lime;
		static constexpr RGBA8u		BuildRayTracingStruct	= HtmlColor::Lime;
		
		static constexpr RGBA8u		Draw					= HtmlColor::Bisque;
		static constexpr RGBA8u		DrawMeshes				= HtmlColor::Bisque;
		static constexpr RGBA8u		CustomDraw				= HtmlColor::Bisque;

		static constexpr RGBA8u		CmdSubBatchBackground	= RGBA8u{ 0x28, 0x28, 0x28, 0xFF };
		static constexpr RGBA8u		CmdSubBatchLavel		= RGBA8u{ 0xdc, 0xdc, 0xdc, 0xFF };
		static constexpr RGBA8u		CmdBatchBackground		= RGBA8u{ 0x18, 0x18, 0x18, 0xFF };
		static constexpr RGBA8u		CmdBatchLabel			= RGBA8u{ 0xdc, 0xdc, 0xdc, 0xFF };
		static constexpr RGBA8u		TaskLabel				= HtmlColor::White;
		static constexpr RGBA8u		ResourceBackground		= HtmlColor::Silver;
		//static constexpr RGBA8u	ResourceToResourceEdge	= HtmlColor::Silver;
		static constexpr RGBA8u		BarrierGroupBorder		= HtmlColor::Olive;
		static constexpr RGBA8u		GroupBorder				= HtmlColor::DarkGray;
		static constexpr RGBA8u		TaskDependencyEdge		= RGBA8u{ 0xd3, 0xd3, 0xd3, 0xFF };

		static constexpr RGBA8u		Debug					= HtmlColor::Pink;		// all invalid data will be drawn with this color

		// old-style barriers
		static constexpr bool		EnabledOldStyleBarriers	= false;
		static constexpr RGBA8u		LayoutBarrier			= HtmlColor::Yellow;
		static constexpr RGBA8u		WriteAfterWriteBarrier	= HtmlColor::DodgerBlue;
		static constexpr RGBA8u		WriteAfterReadBarrier	= HtmlColor::LimeGreen;
		static constexpr RGBA8u		ReadAfterWriteBarrier	= HtmlColor::Red;
	};


}	// FGColorScheme

	using ColorScheme = FGColorScheme::DefaultDark;

}	// FG
