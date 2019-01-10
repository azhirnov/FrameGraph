// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "Public/FrameGraphDrawTask.h"
#include "Public/FrameGraphTask.h"
#include "Public/Pipeline.h"
#include "Public/FrameGraphThread.h"

namespace FG
{

	//
	// Vulkan Shader Debugger
	//

	class VShaderDebugger
	{
	// types
	private:
		using SharedShaderPtr	= PipelineDescription::SharedShaderPtr< ShaderModuleVk_t >;
		using TaskName_t		= _fg_hidden_::TaskName_t;

		struct StorageBuffer
		{
			BufferID				shaderTraceBuffer;
			BufferID				readBackBuffer;
			BytesU					capacity;
			BytesU					size;
			VkPipelineStageFlags	stages	= 0;
		};

		struct DebugMode
		{
			SharedShaderPtr		module;
			VkDescriptorSet		descriptorSet	= VK_NULL_HANDLE;
			BytesU				offset;
			BytesU				size;
			uint				sbIndex			= UMax;
			EShaderDebugMode	mode			= Default;
			EShader				shaderType		= Default;
			TaskName_t			taskName;
			uint				data[8]			= {};
		};

		using StorageBuffers_t		= Array< StorageBuffer >;
		using DebugModes_t			= Array< DebugMode >;
		using ShaderDebugCallback_t	= FrameGraphThread::ShaderDebugCallback_t;
		using LayoutCache_t			= HashMap< uint, DescriptorSetLayoutID >;
		using DescriptorCache_t		= HashMap< Pair<RawBufferID, RawDescriptorSetLayoutID>, VkDescriptorSet >;


	// variables
	private:
		VFrameGraphThread&		_frameGraph;

		StorageBuffers_t		_storageBuffers;
		DebugModes_t			_debugModes;

		const BytesU			_bufferAlign;
		const BytesU			_bufferSize		= 128_Mb;

		VkDescriptorPool		_descPool		= VK_NULL_HANDLE;
		DescriptorCache_t		_descCache;

		LayoutCache_t			_layoutCache;
		mutable Array<String>	_tempStrings;	// TODO: temporary allocator?


	// methods
	public:
		explicit VShaderDebugger (VFrameGraphThread &);
		~VShaderDebugger ();

		void OnBeginRecording (VkCommandBuffer cmd);
		void OnEndRecording (VkCommandBuffer cmd);
		void OnEndFrame (const ShaderDebugCallback_t &);

		bool SetShaderModule (uint id, const SharedShaderPtr &module);
		bool GetDebugModeInfo (uint id, OUT EShaderDebugMode &mode, OUT EShader &shader) const;
		bool GetDescriptotSet (uint id, OUT VkDescriptorSet &descSet, OUT uint &dynamicOffset) const;

		ND_ uint  Append (INOUT ArrayView<RectI> &, const TaskName_t &name, const _fg_hidden_::GraphicsShaderDebugMode &mode, BytesU size = 8_Mb);
		ND_ uint  Append (const TaskName_t &name, const _fg_hidden_::ComputeShaderDebugMode &mode, BytesU size = 8_Mb);
		ND_ uint  Append (const TaskName_t &name, const _fg_hidden_::RayTracingShaderDebugMode &mode, BytesU size = 8_Mb);

		ND_ RawDescriptorSetLayoutID	GetDescriptorSetLayout (EShaderDebugMode debugMode, EShader debuggableShader);


	private:
		bool _ParseDebugOutput (const ShaderDebugCallback_t &, const DebugMode &) const;

		bool _AllocStorage (INOUT DebugMode &, BytesU);
		bool _AllocDescriptorSet (EShaderDebugMode debugMode, EShader shader, RawBufferID storageBuffer, BytesU size, OUT VkDescriptorSet &descSet);

		ND_ BytesU  _GetBufferStaticSize (EShader type) const;
	};


}	// FG
