// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framegraph/VFG.h"
#include <chrono>

namespace FG
{

	//
	// Shadertoy Application
	//

	class FGShadertoyApp final : public IWindowEventListener
	{
	// types
	private:

		struct ShaderData
		{
			float3	iResolution;			// viewport resolution (in pixels)
			float	iTime;					// shader playback time (in seconds)
			float	iTimeDelta;				// render time (in seconds)
			int		iFrame;					// shader playback frame
			float4	iMouse;					// mouse pixel coords. xy: current (if MLB down), zw: click
			float4	iDate;					// (year, month, day, time in seconds)
			float	iSampleRate;			// sound sample rate (i.e., 44100)
			float3	iCameraFrustumRay0;		// left bottom - frustum rays
			float3	iCameraFrustumRay1;		// right bottom
			float3	iCameraFrustumRay2;		// left top
			float3	iCameraFrustumRay3;		// right top
			float3	iCameraPos;				// camera position in world space
		};


		struct ShaderDescr
		{
			using Channels_t = FixedArray< Pair<String, uint>, 8 >;
			
			String			_pplnFilename;
			String			_pplnDefines;
			Channels_t		_channels;

			ShaderDescr () {}
			ShaderDescr& Pipeline (String &&file, String &&def = "")	{ _pplnFilename = std::move(file);  _pplnDefines = std::move(def);  return *this; }
			ShaderDescr& InChannel (const String &name, uint index)		{ _channels.push_back({ name, index });  return *this; }
		};


		struct Shader
		{
		// types
			struct PerPass
			{
				PipelineResources	resources;
				ImageID				renderTarget;
				uint2				viewport;
			};
			using PerPass_t		= StaticArray< PerPass, 4 >;
			using Channels_t	= ShaderDescr::Channels_t;

		// variables
			GPipelineID			_pipeline;
			const String		_name;
			String				_pplnFilename;
			String				_pplnDefines;
			Channels_t			_channels;
			PerPass_t			_perPass;
			BufferID			_ubuffer;

		// methods
			explicit Shader (StringView name, ShaderDescr &&desc);

			ND_ StringView  Name ()	const	{ return _name; }
		};


		using Samples_t		= Array< std::function<void ()> >;

		using ShaderPtr		= SharedPtr< Shader >;
		using ShadersMap_t	= HashMap< String, ShaderPtr >;

		using ImageCache_t	= HashMap< String, ImageID >;

		using TimePoint_t	= std::chrono::high_resolution_clock::time_point;
		using SecondsF		= std::chrono::duration< float >;


	// variables
	private:
		VulkanDeviceExt			_vulkan;
		WindowPtr				_window;
		FrameGraphPtr			_frameGraphInst;
		FGThreadPtr				_frameGraph;

		Samples_t				_samples;
		size_t					_currSample		= UMax;
		size_t					_nextSample		= 0;
		uint					_passIdx : 1;
		bool					_pause			= false;

		ShadersMap_t			_shaders;
		Array< ShaderPtr >		_ordered;

		ImageCache_t			_imageCache;

		ShaderData				_ubData;
		Task					_currTask;
		
		uint2					_surfaceSize;
		uint2					_scaledSurfaceSize;
		float					_sufaceScale	= 0.5f;
		
		uint					_frameCounter	= 0;
		TimePoint_t				_startTime;
		TimePoint_t				_lastUpdateTime;

		SamplerID				_nearestClampSampler;
		SamplerID				_linearClampSampler;
		SamplerID				_nearestRepeatSampler;
		SamplerID				_linearRepeatSampler;
		
		// FPS counter
		struct {
			TimePoint_t				lastUpdateTime;
			uint					frameCounter			= 0;
			static constexpr uint	UpdateIntervalMillis	= 500;
		}						_fps;


	// methods
	public:
		FGShadertoyApp ();
		~FGShadertoyApp ();
		
		bool Initialize (WindowPtr &&wnd);
		bool Update ();
		void Destroy ();


	// IWindowEventListener
	private:
		void OnResize (const uint2 &size) override;
		void OnRefresh () override {}
		void OnDestroy () override {}
		void OnUpdate () override {}
		void OnKey (StringView, EKeyAction) override;
		void OnMouseMove (const float2 &) override {}
		
	private:
		//bool Visualize (StringView name, bool autoOpen = false) const;

		void _InitSamples ();
		void _CreateSamplers ();

		bool _AddShader (const String &name, ShaderDescr &&desc);
		void _ResetShaders ();
		bool _RecreateShaders (const uint2 &size);
		bool _CreateShader (const ShaderPtr &shader);
		void _DestroyShader (const ShaderPtr &shader);
		bool _DrawWithShader (const ShaderPtr &shader, uint passIndex);
		void _UpdateShaderData ();

		void _UpdateFPS ();

		bool _LoadImage (const String &filename, OUT ImageID &id);
		bool _HasImage (StringView filename) const;

		GPipelineID  _Compile (StringView name, StringView defs) const;
		bool _Recompile ();
	};


}	// FG
