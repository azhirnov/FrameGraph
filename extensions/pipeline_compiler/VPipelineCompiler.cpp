// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VPipelineCompiler.h"
#include "SpirvCompiler.h"
#include "PrivateDefines.h"
#include "extensions/vulkan_loader/VulkanLoader.h"
#include "extensions/vulkan_loader/VulkanCheckError.h"
#include "framegraph/Shared/EnumUtils.h"

namespace FG
{

	//
	// Vulkan Cached Shader Module
	//

	class VCachedShaderModule final : public PipelineDescription::IShaderData< ShaderModuleVk_t >
	{
	// variables
	private:
		VkShaderModule		_module		= VK_NULL_HANDLE;
		StaticString<64>	_entry;


	// methods
	public:
		VCachedShaderModule (VkShaderModule module, StringView entry) :
			_module{ module }, _entry{ entry } {}

		~VCachedShaderModule () {
			CHECK( _module == VK_NULL_HANDLE );
		}

		void Destroy (PFN_vkDestroyShaderModule fpDestroyShaderModule, VkDevice dev)
		{
			if ( _module )
			{
				fpDestroyShaderModule( dev, _module, null );
				_module = VK_NULL_HANDLE;
			}
		}
		
		ShaderModuleVk_t const&		GetData () const override		{ return BitCast<ShaderModuleVk_t>( _module ); }

		StringView					GetEntry () const override		{ return _entry; }

		size_t						GetHashOfData () const override	{ ASSERT(false);  return 0; }
	};
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VPipelineCompiler::VPipelineCompiler () :
		_spirvCompiler{ new SpirvCompiler() },
		_physicalDevice{ VK_NULL_HANDLE },
		_logicalDevice{ VK_NULL_HANDLE }
	{
	}
	
/*
=================================================
	constructor
=================================================
*/
	VPipelineCompiler::VPipelineCompiler (PhysicalDeviceVk_t physicalDevice, DeviceVk_t device) :
		VPipelineCompiler()
	{
		_physicalDevice	= physicalDevice;
		_logicalDevice	= device;

        _fpCreateShaderModule  = BitCast<void*>( vkGetDeviceProcAddr( BitCast<VkDevice>(_logicalDevice), "vkCreateShaderModule" ));
        _fpDestroyShaderModule = BitCast<void*>( vkGetDeviceProcAddr( BitCast<VkDevice>(_logicalDevice), "vkDestroyShaderModule" ));
	}

/*
=================================================
	destructor
=================================================
*/
	VPipelineCompiler::~VPipelineCompiler ()
	{
		ReleaseShaderCache();
	}
	
/*
=================================================
	SetCompilationFlags
=================================================
*/
	bool VPipelineCompiler::SetCompilationFlags (EShaderCompilationFlags flags)
	{
		SCOPELOCK( _lock );

		_compilerFlags = flags;
		
		if ( EnumEq( flags, EShaderCompilationFlags::UseCurrentDeviceLimits ) )
			CHECK_ERR( _spirvCompiler->SetCurrentResourceLimits( _physicalDevice ))
		else
			CHECK_ERR( _spirvCompiler->SetDefaultResourceLimits() );

		return _spirvCompiler->SetCompilationFlags( flags );
	}
	
/*
=================================================
	ReleaseUnusedShaders
=================================================
*/
	void VPipelineCompiler::ReleaseUnusedShaders ()
	{
		if ( _logicalDevice == VK_NULL_HANDLE )
			return;
	
		SCOPELOCK( _lock );

		VkDevice	dev					= BitCast<VkDevice>( _logicalDevice );
		auto		DestroyShaderModule = BitCast<PFN_vkDestroyShaderModule>(_fpDestroyShaderModule);
		
		for (auto iter = _shaderCache.begin(); iter != _shaderCache.end();)
		{
			if ( not iter->second.use_count() == 1 )
			{
				++iter;
				continue;
			}

			Cast<VCachedShaderModule>( iter->second )->Destroy( DestroyShaderModule, dev );

			iter = _shaderCache.erase( iter );
		}
	}
	
/*
=================================================
	ReleaseShaderCache
=================================================
*/
	void VPipelineCompiler::ReleaseShaderCache ()
	{
		if ( _logicalDevice == VK_NULL_HANDLE )
			return;
		
		SCOPELOCK( _lock );

		VkDevice	dev					= BitCast<VkDevice>( _logicalDevice );
		auto		DestroyShaderModule = BitCast<PFN_vkDestroyShaderModule>(_fpDestroyShaderModule);

		for (auto& sh : _shaderCache)
		{
			ASSERT( sh.second.use_count() == 1 );
			
			Cast<VCachedShaderModule>( sh.second )->Destroy( DestroyShaderModule, dev );
		}
		_shaderCache.clear();
	}

/*
=================================================
	IsSrcFormatSupported
=================================================
*/
	ND_ static bool IsSrcFormatSupported (EShaderLangFormat srcFormat)
	{
		switch ( srcFormat & EShaderLangFormat::_ApiStorageFormatMask )
		{
			case EShaderLangFormat::OpenGL | EShaderLangFormat::HighLevel :
			case EShaderLangFormat::Vulkan | EShaderLangFormat::HighLevel :
				return true;
		}
		return false;
	}

/*
=================================================
	IsDstFormatSupported
=================================================
*/
	ND_ static bool IsDstFormatSupported (EShaderLangFormat dstFormat, bool hasDevice)
	{
		switch ( dstFormat & EShaderLangFormat::_ApiStorageFormatMask )
		{
			case EShaderLangFormat::Vulkan | EShaderLangFormat::SPIRV :			return true;
			case EShaderLangFormat::Vulkan | EShaderLangFormat::ShaderModule :	return hasDevice;
		}
		return false;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool VPipelineCompiler::IsSupported (const MeshPipelineDesc &ppln, EShaderLangFormat dstFormat) const
	{
		// lock is not needed becouse only '_logicalDevice' read access is used

		if ( ppln._shaders.empty() )
			return false;

		if ( not IsDstFormatSupported( dstFormat, (_logicalDevice != VK_NULL_HANDLE) ))
			return false;

		bool	is_supported = true;

		for (auto& sh : ppln._shaders)
		{
			is_supported &= _IsSupported( sh.second.data );
		}

		return is_supported;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool VPipelineCompiler::IsSupported (const RayTracingPipelineDesc &ppln, EShaderLangFormat dstFormat) const
	{
		// lock is not needed becouse only '_logicalDevice' read access is used

		if ( ppln._shaders.empty() )
			return false;

		if ( not IsDstFormatSupported( dstFormat, (_logicalDevice != VK_NULL_HANDLE) ))
			return false;

		bool	is_supported = true;

		for (auto& sh : ppln._shaders)
		{
			is_supported &= _IsSupported( sh.second.data );
		}

		return is_supported;
	}

/*
=================================================
	IsSupported
=================================================
*/
	bool VPipelineCompiler::IsSupported (const GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat) const
	{
		// lock is not needed becouse only '_logicalDevice' read access is used

		if ( ppln._shaders.empty() )
			return false;

		if ( not IsDstFormatSupported( dstFormat, (_logicalDevice != VK_NULL_HANDLE) ))
			return false;

		bool	is_supported = true;

		for (auto& sh : ppln._shaders)
		{
			is_supported &= _IsSupported( sh.second.data );
		}

		return is_supported;
	}
	
/*
=================================================
	IsSupported
=================================================
*/
	bool VPipelineCompiler::IsSupported (const ComputePipelineDesc &ppln, EShaderLangFormat dstFormat) const
	{
		// lock is not needed becouse only '_logicalDevice' read access is used

		if ( not IsDstFormatSupported( dstFormat, (_logicalDevice != VK_NULL_HANDLE) ))
			return false;
		
		return _IsSupported( ppln._shader.data );
	}
	
/*
=================================================
	_IsSupported
=================================================
*/
	bool VPipelineCompiler::_IsSupported (const ShaderDataMap_t &shaderDataMap)
	{
		ASSERT( not shaderDataMap.empty() );

		bool	is_supported = false;

		for (auto& data : shaderDataMap)
		{
			if ( data.second.index() and IsSrcFormatSupported( data.first ) )
			{
				is_supported = true;
				break;
			}
		}
		return is_supported;
	}

/*
=================================================
	MergeShaderAccess
=================================================
*/
	static void MergeShaderAccess (const EResourceState srcAccess, INOUT EResourceState &dstAccess)
	{
		if ( srcAccess == dstAccess )
			return;

		dstAccess |= srcAccess;

		if ( EnumEq( dstAccess, EResourceState::InvalidateBefore ) and
			 EnumEq( dstAccess, EResourceState::ShaderRead ) )
		{
			dstAccess &= ~EResourceState::InvalidateBefore;
		}
	}

/*
=================================================
	_MergeUniforms
=================================================
*/
	bool VPipelineCompiler::_MergeUniforms (const PipelineDescription::UniformMap_t &srcUniforms,
											INOUT PipelineDescription::UniformMap_t &dstUniforms) const
	{
		for (auto& un : srcUniforms)
		{
			auto	iter = dstUniforms.find( un.first );

			// add new uniform
			if ( iter == dstUniforms.end() )
			{
				dstUniforms.insert( un );
				continue;
			}

			bool	type_missmatch = true;

			Visit( un.second.data,
				[&] (const PipelineDescription::Texture &lhs)
				{
					if ( auto* rhs = std::get_if<PipelineDescription::Texture>( &iter->second.data ) )
					{
						ASSERT( lhs.textureType	== rhs->textureType );
						ASSERT( un.second.index	== iter->second.index );

						if ( lhs.textureType	== rhs->textureType and
							 un.second.index	== iter->second.index )
						{
							iter->second.stageFlags |= un.second.stageFlags;
							rhs->state				|= EResourceState_FromShaders( iter->second.stageFlags );
							type_missmatch			 = false;
						}
					}
				},
				   
				[&] (const PipelineDescription::Sampler &)
				{
					if ( auto* rhs = std::get_if<PipelineDescription::Sampler>( &iter->second.data ) )
					{
						ASSERT( un.second.index == iter->second.index );

						if ( un.second.index == iter->second.index )
						{
							iter->second.stageFlags |= un.second.stageFlags;
							type_missmatch			 = false;
						}
					}
				},
				
				[&] (const PipelineDescription::SubpassInput &lhs)
				{
					if ( auto* rhs = std::get_if<PipelineDescription::SubpassInput>( &iter->second.data ) )
					{
						ASSERT( lhs.attachmentIndex	== rhs->attachmentIndex );
						ASSERT( lhs.isMultisample	== rhs->isMultisample );
						ASSERT( un.second.index		== iter->second.index );
						
						if ( lhs.attachmentIndex	== rhs->attachmentIndex	and
							 lhs.isMultisample		== rhs->isMultisample	and
							 un.second.index		== iter->second.index )
						{
							iter->second.stageFlags |= un.second.stageFlags;
							rhs->state				|= EResourceState_FromShaders( iter->second.stageFlags );
							type_missmatch			 = false;
						}
					}
				},
				
				[&] (const PipelineDescription::Image &lhs)
				{
					if ( auto* rhs = std::get_if<PipelineDescription::Image>( &iter->second.data ) )
					{
						ASSERT( lhs.imageType	== rhs->imageType );
						ASSERT( lhs.format		== rhs->format );
						ASSERT( un.second.index	== iter->second.index );
						
						if ( lhs.imageType		== rhs->imageType	and
							 lhs.format			== rhs->format		and
							 un.second.index	== iter->second.index )
						{
							MergeShaderAccess( lhs.state, INOUT rhs->state );

							iter->second.stageFlags |= un.second.stageFlags;
							rhs->state				|= EResourceState_FromShaders( iter->second.stageFlags );
							type_missmatch			 = false;
						}
					}
				},
				
				[&] (const PipelineDescription::UniformBuffer &lhs)
				{
					if ( auto* rhs = std::get_if<PipelineDescription::UniformBuffer>( &iter->second.data ) )
					{
						ASSERT( lhs.size		== rhs->size );
						ASSERT( un.second.index	== iter->second.index );

						if ( lhs.size			== rhs->size	and
							 un.second.index	== iter->second.index )
						{
							iter->second.stageFlags |= un.second.stageFlags;
							rhs->state				|= EResourceState_FromShaders( iter->second.stageFlags );
							type_missmatch			 = false;
						}
					}
				},
				
				[&] (const PipelineDescription::StorageBuffer &lhs)
				{
					if ( auto* rhs = std::get_if<PipelineDescription::StorageBuffer>( &iter->second.data ) )
					{
						ASSERT( lhs.staticSize	== rhs->staticSize );
						ASSERT( lhs.arrayStride	== rhs->arrayStride );
						ASSERT( un.second.index	== iter->second.index );
						
						if ( lhs.staticSize		== rhs->staticSize	and
							 lhs.arrayStride	== rhs->arrayStride	and
							 un.second.index	== iter->second.index )
						{
							MergeShaderAccess( lhs.state, INOUT rhs->state );

							iter->second.stageFlags |= un.second.stageFlags;
							rhs->state				|= EResourceState_FromShaders( iter->second.stageFlags );
							type_missmatch			 = false;
						}
					}
				},

				[] (const auto &) { ASSERT(false); }
			);

			COMP_CHECK_ERR( not type_missmatch );
		}
		return true;
	}

/*
=================================================
	_MergePipelineResources
=================================================
*/
	bool VPipelineCompiler::_MergePipelineResources (const PipelineDescription::PipelineLayout &srcLayout,
													 INOUT PipelineDescription::PipelineLayout &dstLayout) const
	{
		// merge descriptor sets
		for (auto& src_ds : srcLayout.descriptorSets)
		{
			bool	found = false;

			for (auto& dst_ds : dstLayout.descriptorSets)
			{
				// merge
				if ( src_ds.id == dst_ds.id )
				{
					COMP_CHECK_ERR( src_ds.bindingIndex == dst_ds.bindingIndex );

					COMP_CHECK_ERR( _MergeUniforms( *src_ds.uniforms, INOUT const_cast<PipelineDescription::UniformMap_t &>(*dst_ds.uniforms) ));

					found = true;
					break;
				}
			}

			// add new descriptor set
			if ( not found )
				dstLayout.descriptorSets.push_back( src_ds );
		}

		// TODO: merge push constants

		return true;
	}
	
/*
=================================================
	MergePrimitiveTopology
=================================================
*/
	static void MergePrimitiveTopology (const GraphicsPipelineDesc::TopologyBits_t &src, INOUT GraphicsPipelineDesc::TopologyBits_t &dst)
	{
		for (size_t i = 0; i < src.size(); ++i)
		{
			if ( src.test( i ) )
				dst.set( i );
		}
	}
	
/*
=================================================
	ValidatePrimitiveTopology
=================================================
*/
	static void ValidatePrimitiveTopology (INOUT GraphicsPipelineDesc::TopologyBits_t &topology)
	{
		if ( topology.test(uint(EPrimitive::Patch)) )
		{
			topology.reset().set(uint(EPrimitive::Patch));
			return;
		}
		
		if ( topology.test(uint(EPrimitive::TriangleListAdjacency)) or
			 topology.test(uint(EPrimitive::TriangleStripAdjacency)) )
		{
			topology.reset().set(uint(EPrimitive::TriangleListAdjacency))
							.set(uint(EPrimitive::TriangleStripAdjacency));
			return;
		}
		
		if ( topology.test(uint(EPrimitive::LineListAdjacency)) or
			 topology.test(uint(EPrimitive::LineStripAdjacency)) )
		{
			topology.reset().set(uint(EPrimitive::LineListAdjacency))
							.set(uint(EPrimitive::LineStripAdjacency));
			return;
		}

		if ( topology.none() )
		{
			topology.reset().set(uint(EPrimitive::Point))
							.set(uint(EPrimitive::LineList))
							.set(uint(EPrimitive::LineStrip))
							.set(uint(EPrimitive::TriangleList))
							.set(uint(EPrimitive::TriangleStrip))
							.set(uint(EPrimitive::TriangleFan));
			return;
		}
	}
	
/*
=================================================
	FindHighPriorityShaderFormat
=================================================
*/
	ND_ static auto  FindHighPriorityShaderFormat (const PipelineDescription::ShaderDataMap_t &shaderData, EShaderLangFormat dstFormat)
	{
		auto	result	= shaderData.find( dstFormat );

		if ( result != shaderData.end() )
			return result;

		// limit max vulkan version with considering of 'dstFormat'
		const EShaderLangFormat		dst_vulkan_ver	= (dstFormat & EShaderLangFormat::_ApiMask) == EShaderLangFormat::Vulkan ?
														(dstFormat & EShaderLangFormat::_VersionMask) :
														EShaderLangFormat::_VersionMask;

		// search nearest shader format
		for (auto iter = shaderData.begin(); iter != shaderData.end(); ++iter)
		{
			if ( not IsSrcFormatSupported( iter->first ) )
				continue;

			// vulkan has most priority than opengl
			const bool				current_is_vulkan	= (result != shaderData.end()) and (result->first & EShaderLangFormat::_ApiMask) == EShaderLangFormat::Vulkan;
			const bool				pending_is_vulkan	= (iter->first & EShaderLangFormat::_ApiMask) == EShaderLangFormat::Vulkan;
			const EShaderLangFormat	current_ver			= (result != shaderData.end()) ? (result->first & EShaderLangFormat::_VersionMask) : EShaderLangFormat::Unknown;
			const EShaderLangFormat	pending_ver			= (iter->first & EShaderLangFormat::_VersionMask);

			if ( current_is_vulkan )
			{
				if ( not pending_is_vulkan )
					continue;

				// compare vulkan versions
				if ( pending_ver > current_ver and pending_ver <= dst_vulkan_ver )
					result = iter;

				continue;
			}

			if ( pending_is_vulkan )
			{
				if ( pending_ver <= dst_vulkan_ver )
					result = iter;

				continue;
			}

			// compare opengl versions
			if ( pending_ver > current_ver )
				result = iter;
		}

		return result;
	}
	
/*
=================================================
	Compile
=================================================
*/
	bool VPipelineCompiler::Compile (INOUT MeshPipelineDesc &ppln, EShaderLangFormat dstFormat)
	{
		SCOPELOCK( _lock );
		ASSERT( IsSupported( ppln, dstFormat ) );

		const bool					create_module	= ((dstFormat & EShaderLangFormat::_StorageFormatMask) == EShaderLangFormat::ShaderModule);
		const EShaderLangFormat		spirv_format	= not create_module ? dstFormat :
														((dstFormat & ~EShaderLangFormat::_StorageFormatMask) | EShaderLangFormat::SPIRV);
		MeshPipelineDesc			new_ppln;


		for (const auto& shader : ppln._shaders)
		{
			ASSERT( not shader.second.data.empty() );

			auto	iter = FindHighPriorityShaderFormat( shader.second.data, spirv_format );

			if ( iter == shader.second.data.end() )
				RETURN_ERR( "no suitable shader format found!" );
			

			// compile glsl
			if ( auto* shader_data = std::get_if< StringShaderData >( &iter->second ) )
			{
				SpirvCompiler::ShaderReflection	reflection;
				String							log;
				PipelineDescription::Shader		new_shader;

				COMP_CHECK_ERR( _spirvCompiler->Compile( shader.first, iter->first, spirv_format,
														 (*shader_data)->GetEntry(), (*shader_data)->GetData(), OUT new_shader, OUT reflection, OUT log ));
				
				if ( create_module )
					COMP_CHECK_ERR( _CreateVulkanShader( INOUT new_shader ) );

				COMP_CHECK_ERR( _MergePipelineResources( reflection.layout, INOUT new_ppln._pipelineLayout ));

				switch ( shader.first )
				{
					case EShader::MeshTask :
					case EShader::Mesh :
						break;	// TODO

					default :
						RETURN_ERR( "unknown shader type!" );
				}

				new_ppln._shaders.insert_or_assign( shader.first, std::move(new_shader) );
			}
			else
			{
				COMP_RETURN_ERR( "invalid shader data type!" );
			}
		}

		std::swap( ppln, new_ppln ); 
		return true;
	}
	
/*
=================================================
	Compile
=================================================
*/
	bool VPipelineCompiler::Compile (INOUT RayTracingPipelineDesc &ppln, EShaderLangFormat dstFormat)
	{
		SCOPELOCK( _lock );
		ASSERT( IsSupported( ppln, dstFormat ) );
		
		const bool					create_module	= ((dstFormat & EShaderLangFormat::_StorageFormatMask) == EShaderLangFormat::ShaderModule);
		const EShaderLangFormat		spirv_format	= not create_module ? dstFormat :
														((dstFormat & ~EShaderLangFormat::_StorageFormatMask) | EShaderLangFormat::SPIRV);
		RayTracingPipelineDesc		new_ppln;


		for (const auto& shader : ppln._shaders)
		{
			ASSERT( not shader.second.data.empty() );

			auto	iter = FindHighPriorityShaderFormat( shader.second.data, spirv_format );

			if ( iter == shader.second.data.end() )
				RETURN_ERR( "no suitable shader format found!" );
			

			// compile glsl
			if ( auto* shader_data = std::get_if< StringShaderData >( &iter->second ) )
			{
				SpirvCompiler::ShaderReflection	reflection;
				String							log;
				PipelineDescription::Shader		new_shader;

				COMP_CHECK_ERR( _spirvCompiler->Compile( shader.first, iter->first, spirv_format,
														 (*shader_data)->GetEntry(), (*shader_data)->GetData(), OUT new_shader, OUT reflection, OUT log ));
				
				if ( create_module )
					COMP_CHECK_ERR( _CreateVulkanShader( INOUT new_shader ) );

				COMP_CHECK_ERR( _MergePipelineResources( reflection.layout, INOUT new_ppln._pipelineLayout ));

				switch ( shader.first )
				{
					case EShader::RayGen :
					case EShader::RayAnyHit :
					case EShader::RayClosestHit :
					case EShader::RayMiss :
					case EShader::RayIntersection :
					case EShader::RayCallable :
						break;	// TODO

					default :
						RETURN_ERR( "unknown shader type!" );
				}

				new_ppln._shaders.insert_or_assign( shader.first, std::move(new_shader) );
			}
			else
			{
				COMP_RETURN_ERR( "invalid shader data type!" );
			}
		}

		std::swap( ppln, new_ppln ); 
		return true;
	}

/*
=================================================
	Compile
=================================================
*/
	bool VPipelineCompiler::Compile (INOUT GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat)
	{
		SCOPELOCK( _lock );
		ASSERT( IsSupported( ppln, dstFormat ) );
		
		const bool					create_module	= ((dstFormat & EShaderLangFormat::_StorageFormatMask) == EShaderLangFormat::ShaderModule);
		const EShaderLangFormat		spirv_format	= not create_module ? dstFormat :
														((dstFormat & ~EShaderLangFormat::_StorageFormatMask) | EShaderLangFormat::SPIRV);
		GraphicsPipelineDesc		new_ppln;


		for (const auto& shader : ppln._shaders)
		{
			ASSERT( not shader.second.data.empty() );

			auto	iter = FindHighPriorityShaderFormat( shader.second.data, spirv_format );

			if ( iter == shader.second.data.end() )
				RETURN_ERR( "no suitable shader format found!" );
			

			// compile glsl
			if ( auto* shader_data = std::get_if< StringShaderData >( &iter->second ) )
			{
				SpirvCompiler::ShaderReflection	reflection;
				String							log;
				PipelineDescription::Shader		new_shader;

				COMP_CHECK_ERR( _spirvCompiler->Compile( shader.first, iter->first, spirv_format,
														 (*shader_data)->GetEntry(), (*shader_data)->GetData(), OUT new_shader, OUT reflection, OUT log ));
				
				if ( create_module )
					COMP_CHECK_ERR( _CreateVulkanShader( INOUT new_shader ) );

				COMP_CHECK_ERR( _MergePipelineResources( reflection.layout, INOUT new_ppln._pipelineLayout ));
						
				MergePrimitiveTopology( reflection.vertex.supportedTopology, INOUT new_ppln._supportedTopology );

				switch ( shader.first )
				{
					case EShader::Vertex :
						new_ppln._vertexAttribs			= reflection.vertex.vertexAttribs;
						break;

					case EShader::TessControl :
						new_ppln._patchControlPoints	= reflection.tessellation.patchControlPoints;
						break;

					case EShader::TessEvaluation :
						break;

					case EShader::Geometry :
						break;

					case EShader::Fragment :
						new_ppln._fragmentOutput		= reflection.fragment.fragmentOutput;
						new_ppln._earlyFragmentTests	= reflection.fragment.earlyFragmentTests;
						break;

					default :
						RETURN_ERR( "unknown shader type!" );
				}

				new_ppln._shaders.insert_or_assign( shader.first, std::move(new_shader) );
			}
			else
			{
				COMP_RETURN_ERR( "invalid shader data type!" );
			}
		}

		ValidatePrimitiveTopology( INOUT new_ppln._supportedTopology );

		std::swap( ppln, new_ppln ); 
		return true;
	}
	
/*
=================================================
	Compile
=================================================
*/
	bool VPipelineCompiler::Compile (INOUT ComputePipelineDesc &ppln, EShaderLangFormat dstFormat)
	{
		SCOPELOCK( _lock );
		ASSERT( IsSupported( ppln, dstFormat ) );
		
		const bool					create_module	= ((dstFormat & EShaderLangFormat::_StorageFormatMask) == EShaderLangFormat::ShaderModule);
		const EShaderLangFormat		spirv_format	= not create_module ? dstFormat :
														((dstFormat & ~EShaderLangFormat::_StorageFormatMask) | EShaderLangFormat::SPIRV);

		auto	iter = FindHighPriorityShaderFormat( ppln._shader.data, spirv_format );
		
		if ( iter == ppln._shader.data.end() )
		{
			RETURN_ERR( "no suitable shader format found!" );
		}

		if ( auto* shader_data = std::get_if< StringShaderData >( &iter->second ) )
		{
			SpirvCompiler::ShaderReflection	reflection;
			String							log;
			ComputePipelineDesc				new_ppln;

			COMP_CHECK_ERR( _spirvCompiler->Compile( EShader::Compute, iter->first, spirv_format,
													 (*shader_data)->GetEntry(), (*shader_data)->GetData(), OUT new_ppln._shader, OUT reflection, OUT log ) );
			
			if ( create_module )
				COMP_CHECK_ERR( _CreateVulkanShader( INOUT new_ppln._shader ) );

			new_ppln._defaultLocalGroupSize = reflection.compute.localGroupSize;
			new_ppln._localSizeSpec			= reflection.compute.localGroupSpecialization;
			new_ppln._pipelineLayout		= std::move(reflection.layout);

			std::swap( ppln, new_ppln ); 
			return true;
		}

		RETURN_ERR( "invalid shader data type!" );
	}
	
/*
=================================================
	_CreateVulkanShader
=================================================
*/
	bool VPipelineCompiler::_CreateVulkanShader (INOUT PipelineDescription::Shader &shader)
	{
		CHECK_ERR( _fpCreateShaderModule and _fpDestroyShaderModule );

		auto CreateShaderModule = BitCast<PFN_vkCreateShaderModule>(_fpCreateShaderModule);


		// find SPIRV binary
		for (auto& sh : shader.data)
		{
			if ( (sh.first & EShaderLangFormat::_StorageFormatMask) == EShaderLangFormat::SPIRV )
			{
				const auto*	spv_data_ptr = std::get_if<BinaryShaderData>( &sh.second );
				COMP_CHECK_ERR( spv_data_ptr and *spv_data_ptr, "invalid shader data!" );

				// search in existing shader modules
				auto					spv_data	= *spv_data_ptr;
				const EShaderLangFormat	module_fmt	= EShaderLangFormat::ShaderModule | EShaderLangFormat::Vulkan | (sh.first & EShaderLangFormat::_VersionMask);
				auto					iter		= _shaderCache.find( spv_data );

				if ( iter != _shaderCache.end() )
				{
					shader.data.insert({ module_fmt, iter->second });
					return true;
				}

				// create new shader module
				VkShaderModuleCreateInfo	shader_info = {};
				shader_info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				shader_info.codeSize	= size_t(ArraySizeOf( spv_data->GetData() ));
				shader_info.pCode		= spv_data->GetData().data();

				VkShaderModule		shader_id;
				COMP_CHECK_ERR( CreateShaderModule( BitCast<VkDevice>( _logicalDevice ), &shader_info, null, OUT &shader_id ) == VK_SUCCESS );

				auto	module	= MakeShared<VCachedShaderModule>( shader_id, spv_data->GetEntry() );
				auto	base	= Cast< PipelineDescription::IShaderData<ShaderModuleVk_t> >( module );

				_shaderCache.insert({ spv_data, base });

				shader.data.clear();
				shader.data.insert({ module_fmt, base });
				return true;
			}
		}
		COMP_RETURN_ERR( "SPIRV shader data is not found!" );
	}

}	// FG
