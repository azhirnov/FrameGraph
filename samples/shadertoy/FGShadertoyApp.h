// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framegraph/FG.h"
#include "scene/Math/FPSCamera.h"
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
		
		struct ShadertoyUB
		{
			vec3		iResolution;			// offset: 0, align: 16		// viewport resolution (in pixels)
			float		iTime;					// offset: 12, align: 4		// shader playback time (in seconds)
			float		iTimeDelta;				// offset: 16, align: 4		// render time (in seconds)
			int			iFrame;					// offset: 20, align: 4		// shader playback frame
			vec2		_padding0;
			vec4		iChannelTime[4];		// offset: 32, align: 16
			vec4		iChannelResolution[4];	// offset: 96, align: 16
			vec4		iMouse;					// offset: 160, align: 16	// mouse pixel coords. xy: current (if MLB down), zw: click
			vec4		iDate;					// offset: 176, align: 16	// (year, month, day, time in seconds)
			float		iSampleRate;			// offset: 192, align: 4	// sound sample rate (i.e., 44100)
			float		_padding1;
			float		_padding2;
			float		_padding3;
			vec3		iCameraFrustumRayLB;	// offset: 208, align: 16	// left bottom - frustum rays
			float		_padding4;
			vec3		iCameraFrustumRayRB;	// offset: 224, align: 16	// right bottom
			float		_padding5;
			vec3		iCameraFrustumRayLT;	// offset: 240, align: 16	// left top
			float		_padding6;
			vec3		iCameraFrustumRayRT;	// offset: 256, align: 16	// right top
			float		_padding7;
			quat		iCameraOrientation;		// offset: 272, align: 16	// camera orientation
			vec3		iCameraPos;				// offset: 288, align: 16	// camera position in world space
			float		iCameraFovY;
		};


		struct ShaderDescr
		{
		// types
			struct Channel {
				String	name;
				uint	index	= UMax;
			};
			using Channels_t = FixedArray< Channel, CountOf(&ShadertoyUB::iChannelResolution) >;
			
		// variables
			String				_pplnFilename;
			String				_pplnDefines;
			Channels_t			_channels;
			Optional<float>		_surfaceScale;
			Optional<uint2>		_surfaceSize;

		// methods
			ShaderDescr () {}
			ShaderDescr&  Pipeline (String &&file, String &&def = "")	{ _pplnFilename = std::move(file);  _pplnDefines = std::move(def);  return *this; }
			ShaderDescr&  InChannel (const String &name, uint index)	{ _channels.push_back({ name, index });  return *this; }
			ShaderDescr&  SetScale (float value)						{ _surfaceScale = value;  return *this; }
			ShaderDescr&  SetDimension (uint2 value)					{ _surfaceSize = value;  return *this; }
		};


		struct Shader
		{
		// types
			using ChannelImages_t	= FixedArray< ImageID, ShaderDescr::Channels_t::capacity() >;

			struct PerPass {
				PipelineResources	resources;
				ImageID				renderTarget;
				uint2				viewport;
				ChannelImages_t		images;
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
			Optional<float>		_surfaceScale;
			Optional<uint2>		_surfaceSize;

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
		FrameGraph				_frameGraph;
		CommandBuffer			_cmdBuffer;
		SwapchainID				_swapchainId;

		Samples_t				_samples;
		size_t					_currSample		= UMax;
		size_t					_nextSample		= 0;
		uint					_passIdx : 1;
		bool					_pause			= false;
		bool					_freeze			= false;

		ShadersMap_t			_shaders;
		Array< ShaderPtr >		_ordered;

		ImageCache_t			_imageCache;

		FPSCamera				_camera;
		Rad						_cameraFov		= 60_deg;
		vec3					_positionDelta;
		vec2					_mouseDelta;
		vec2					_lastMousePos;
		Optional<vec2>			_debugPixel;
		bool					_mousePressed	= false;

		ShadertoyUB				_ubData;
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
		
		String					_debugOutputPath;

		struct {
			Nanoseconds				gpuTimeSum				{0};
			Nanoseconds				cpuTimeSum				{0};
			TimePoint_t				lastUpdateTime;
			uint					frameCounter			= 0;
			const uint				UpdateIntervalMillis	= 500;
			RGBA32f					selectedPixel;
		}						_frameStat;

		static constexpr EPixelFormat	ImageFormat = EPixelFormat::RGBA16F;


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
		void OnMouseMove (const float2 &) override;
		
	private:
		//bool Visualize (StringView name, bool autoOpen = false) const;

		void _InitSamples ();
		void _CreateSamplers ();

		bool _AddShader (const String &name, ShaderDescr &&desc);
		void _ResetShaders ();
		bool _RecreateShaders (const uint2 &size);
		bool _CreateShader (const ShaderPtr &shader);
		void _DestroyShader (const ShaderPtr &shader);
		bool _DrawWithShader (const ShaderPtr &shader, uint passIndex, bool isLast);
		void _UpdateShaderData ();

		void _UpdateFrameStat ();

		bool _LoadImage (const String &filename, OUT ImageID &id);
		bool _HasImage (StringView filename) const;

		GPipelineID  _Compile (StringView name, StringView defs) const;
		bool _Recompile ();
		
		void _OnShaderTraceReady (StringView name, ArrayView<String> output) const;
		void _OnPixelReadn (const uint2 &point, const ImageView &view);
	};


}	// FG
