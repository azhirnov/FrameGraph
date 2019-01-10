// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "extensions/glsl_trace/ShaderTrace.h"
#include "extensions/vulkan_loader/VulkanLoader.h"

namespace FG
{
	template <typename T>
	class VCachedDebuggableShaderData;
	
	using VCachedDebuggableShaderModule	= VCachedDebuggableShaderData< ShaderModuleVk_t >;
	using VCachedDebuggableSpirv		= VCachedDebuggableShaderData< Array<uint> >;



	//
	// Vulkan Cached Shader Data (with debug info)
	//

	template <typename T>
	class VCachedDebuggableShaderData final : public PipelineDescription::IShaderData<T>
	{
		template <typename B> friend class VCachedDebuggableShaderData;

	// variables
	private:
		T					_data;
		StaticString<64>	_entry;
		ShaderTrace			_trace;


	// methods
	public:
		VCachedDebuggableShaderData (StringView entry, T &&data) :
			_data{std::move(data)}, _entry{entry}, _trace{std::move(trace)}
		{}

		VCachedDebuggableShaderData (StringView entry, T &&data, ShaderTrace &&trace) :
			_data{std::move(data)}, _entry{entry}, _trace{std::move(trace)}
		{}

		VCachedDebuggableShaderData (VkShaderModule module, const PipelineDescription::SharedShaderPtr<Array<uint>> &spirvCache)
		{
			if constexpr( IsSameTypes< T, ShaderModuleVk_t > )
			{
				_data	= BitCast<ShaderModuleVk_t>( module );
				_entry	= spirvCache->GetEntry();

				if ( auto other = DynCast<VCachedDebuggableSpirv>( spirvCache ))
					_trace = std::move(other->_trace);
			}
		}
		

		~VCachedDebuggableShaderData ()
		{
			if constexpr( IsSameTypes< T, ShaderModuleVk_t > )
			{
				CHECK( _data == VK_NULL_HANDLE );
			}
		}


		void Destroy (PFN_vkDestroyShaderModule fpDestroyShaderModule, VkDevice dev)
		{
			if constexpr( IsSameTypes< T, ShaderModuleVk_t > )
			{
				if ( _data )
				{
					fpDestroyShaderModule( dev, BitCast<VkShaderModule>(_data), null );
					_data = ShaderModuleVk_t(0);
				}
			}
		}


		bool ParseDebugOutput (EShaderDebugMode mode, ArrayView<uint8_t> debugOutput, OUT Array<String> &result) const override
		{
			CHECK_ERR( mode == EShaderDebugMode::Trace );

			return _trace.GetDebugOutput( debugOutput.data(), debugOutput.size(), OUT result );
		}


		T const&	GetData () const override		{ return _data; }

		StringView	GetEntry () const override		{ return _entry; }

		size_t		GetHashOfData () const override	{ ASSERT(false);  return 0; }
	};


}	// FG
