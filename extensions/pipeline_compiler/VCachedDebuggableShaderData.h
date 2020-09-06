// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_GLSL_TRACE
#	include "ShaderTrace.h"
#endif

#ifdef FG_ENABLE_VULKAN
#	include "extensions/vulkan_loader/VulkanLoader.h"
#endif

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

	// types
	public:
		#ifdef FG_ENABLE_GLSL_TRACE
		using ShaderDebugUtils_t	= Union< NullUnion, ShaderTrace >;
		using ShaderDebugUtilsPtr	= UniquePtr< ShaderDebugUtils_t >;
		#endif


	// variables
	private:
		T						_data;
		StaticString<64>		_entry;
		StaticString<64>		_debugName;
		
		#ifdef FG_ENABLE_GLSL_TRACE
		ShaderDebugUtilsPtr		_debugInfo;
		#endif


	// methods
	public:
		VCachedDebuggableShaderData (StringView entry, T &&data, StringView dbgName) :
			_data{std::move(data)}, _entry{entry}, _debugName{dbgName}
		{}
		
		#ifdef FG_ENABLE_GLSL_TRACE
		VCachedDebuggableShaderData (StringView entry, T &&data, StringView dbgName, ShaderDebugUtilsPtr &&debugUtilsPtr) :
			_data{std::move(data)}, _entry{entry}, _debugName{dbgName}, _debugInfo{std::move(debugUtilsPtr)}
		{}
		#endif

		#ifdef FG_ENABLE_VULKAN
		VCachedDebuggableShaderData (VkShaderModule module, const PipelineDescription::SharedShaderPtr<Array<uint>> &spirvCache)
		{
			if constexpr( IsSameTypes< T, ShaderModuleVk_t > )
			{
				_data		= BitCast<ShaderModuleVk_t>( module );
				_entry		= spirvCache->GetEntry();
				_debugName	= spirvCache->GetDebugName();
				
				#ifdef FG_ENABLE_GLSL_TRACE
				if ( auto other = DynCast<VCachedDebuggableSpirv>( spirvCache ))
					_debugInfo = std::move(other->_debugInfo);
				#endif
			}
		}
		#endif

		~VCachedDebuggableShaderData ()
		{
			if constexpr( IsSameTypes< T, ShaderModuleVk_t > )
			{
				CHECK( _data == VK_NULL_HANDLE );
			}
		}

		#ifdef FG_ENABLE_VULKAN
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
		#endif

		bool ParseDebugOutput (EShaderDebugMode mode, ArrayView<uint8_t> debugOutput, OUT Array<String> &result) const override
		{
		#ifdef FG_ENABLE_GLSL_TRACE
			CHECK_ERR( mode == EShaderDebugMode::Trace	or
					   mode == EShaderDebugMode::Profiling );

			if ( not _debugInfo )
				return false;

			return Visit( *_debugInfo,
						  [&] (const ShaderTrace &trace) { return trace.ParseShaderTrace( debugOutput.data(), debugOutput.size(), OUT result ); },
						  []  (const NullUnion &)		 { return false; }
						);
		#else
			Unused( mode, debugOutput, result );
			return false;
		#endif
		}


		T const&	GetData () const override		{ return _data; }

		StringView	GetEntry () const override		{ return _entry; }

		StringView	GetDebugName () const override	{ return _debugName; }


		size_t		GetHashOfData () const override
		{
			if constexpr( IsSameTypes< T, ShaderModuleVk_t > )
			{
				ASSERT(!"not supported");
				return 0;
			}
			else
			{
				return size_t(HashOf( _data ));
			}
		}
	};


}	// FG
