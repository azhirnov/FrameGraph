// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/BaseSceneApp.h"

namespace FG
{

	//
	// Shadertoy Application
	//

	class FGShadertoyApp final : public BaseSceneApp
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
			vec3		iCameraPos;				// offset: 272, align: 16	// camera position in world space
			float		_padding8;
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
			String					_pplnFilename;
			String					_pplnDefines;
			Channels_t				_channels;
			Optional<float>			_surfaceScale;
			Optional<uint2>			_surfaceSize;
			Optional<EPixelFormat>	_format;

		// methods
			ShaderDescr () {}
			ShaderDescr&  Pipeline (String &&file, String &&def = "")	{ _pplnFilename = std::move(file);  _pplnDefines = std::move(def);  return *this; }
			ShaderDescr&  InChannel (const String &name, uint index)	{ _channels.push_back({ name, index });  return *this; }
			ShaderDescr&  SetScale (float value)						{ _surfaceScale = value;  return *this; }
			ShaderDescr&  SetDimension (uint2 value)					{ _surfaceSize = value;  return *this; }
			ShaderDescr&  SetFormat (EPixelFormat value)				{ _format = value;  return *this; }
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

			struct PerEye {
				PerPass_t		passes;
				BufferID		ubuffer;
			};

			using PerEye_t		= FixedArray< PerEye, 2 >;
			using Channels_t	= ShaderDescr::Channels_t;

		// variables
			GPipelineID				_pipeline;
			const String			_name;
			String					_pplnFilename;
			String					_pplnDefines;
			Channels_t				_channels;
			PerEye_t				_perEye;
			Optional<float>			_surfaceScale;
			Optional<uint2>			_surfaceSize;
			Optional<EPixelFormat>	_format;

		// methods
			explicit Shader (StringView name, ShaderDescr &&desc);

			ND_ StringView  Name ()	const	{ return _name; }
		};


		using Samples_t		= Array< std::function<void ()> >;

		using ShaderPtr		= SharedPtr< Shader >;
		using ShadersMap_t	= HashMap< String, ShaderPtr >;

		using ImageCache_t	= HashMap< String, ImageID >;


	// variables
	private:
		CommandBuffer			_cmdBuffer;

		Samples_t				_samples;
		size_t					_currSample		= UMax;
		size_t					_nextSample		= 0;
		uint					_passIdx : 1;
		bool					_pause			= false;
		bool					_freeze			= false;
		bool					_enableVRMode	= false;
		bool					_vrMode			= false;
		bool					_vrMirror		= true;

		ShadersMap_t			_shaders;
		Array< ShaderPtr >		_ordered;

		ImageCache_t			_imageCache;

		Optional<vec2>			_debugPixel;

		ShadertoyUB				_ubData;
		Task					_currTask;
		
		uint2					_surfaceSize;
		uint2					_scaledSurfaceSize;
		float					_sufaceScale	= 0.5f;
		
		uint					_frameCounter	= 0;
		TimePoint_t				_startTime;
		TimePoint_t				_lastUpdateTime;
		RGBA32f					_selectedPixel;

		SamplerID				_nearestClampSampler;
		SamplerID				_linearClampSampler;
		SamplerID				_nearestRepeatSampler;
		SamplerID				_linearRepeatSampler;

		static constexpr EPixelFormat	ImageFormat = EPixelFormat::RGBA16F;


	// methods
	public:
		FGShadertoyApp ();
		~FGShadertoyApp ();
		
		bool Initialize ();
		void Destroy ();


	// BaseSceneApp
	public:
		bool DrawScene () override;


	// IWindowEventListener
	private:
		void OnKey (StringView, EKeyAction) override;
		

	private:
		void _InitSamples ();
		void _CreateSamplers ();

		bool _AddShader (const String &name, ShaderDescr &&desc);
		void _ResetShaders ();
		bool _RecreateShaders (const uint2 &size, bool enableVR);
		bool _CreateShader (const ShaderPtr &shader, bool enableVR);
		void _DestroyShader (const ShaderPtr &shader, bool destroyPipeline);
		bool _DrawWithShader (const ShaderPtr &shader, uint eye, uint passIndex, bool isLast);
		void _UpdateShaderData ();

		bool _LoadImage (const String &filename, OUT ImageID &id);
		bool _HasImage (StringView filename) const;

		GPipelineID  _Compile (StringView name, StringView defs) const;
		bool _Recompile ();
		
		void _OnPixelReadn (const uint2 &point, const ImageView &view);
	};


}	// FG
