// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VPipelineCompiler.h"
#include "SpirvCompiler.h"
#include "PrivateDefines.h"
#include "extensions/vulkan_loader/VulkanLoader.h"
#include "extensions/vulkan_loader/VulkanCheckError.h"
#include "framegraph/Shared/EnumUtils.h"
#include "framegraph/Shared/EnumToString.h"
#include "VCachedDebuggableShaderData.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VPipelineCompiler::VPipelineCompiler () :
		_spirvCompiler{ new SpirvCompiler{ _directories }},
		_vkInstance{ VK_NULL_HANDLE },
		_vkPhysicalDevice{ VK_NULL_HANDLE },
		_vkLogicalDevice{ VK_NULL_HANDLE }
	{
		EXLOCK( _lock );

		// enable all features
		_spirvCompiler->SetShaderClockFeatures( true, true );
		_spirvCompiler->SetShaderFeatures( true, true );
	}
	
/*
=================================================
	constructor
=================================================
*/
	VPipelineCompiler::VPipelineCompiler (InstanceVk_t instance, PhysicalDeviceVk_t physicalDevice, DeviceVk_t device) :
		VPipelineCompiler()
	{
		EXLOCK( _lock );

		_vkInstance			= instance;
		_vkPhysicalDevice	= physicalDevice;
		_vkLogicalDevice	= device;

		if ( _vkInstance and _vkPhysicalDevice )
		{
			auto fpGetPhysicalDeviceFeatures2 = BitCast<PFN_vkGetPhysicalDeviceFeatures2>( vkGetInstanceProcAddr( BitCast<VkInstance>(_vkInstance), "vkGetPhysicalDeviceFeatures2" ));

			_fpCreateShaderModule  = BitCast<void*>( vkGetDeviceProcAddr( BitCast<VkDevice>(_vkLogicalDevice), "vkCreateShaderModule" ));
			_fpDestroyShaderModule = BitCast<void*>( vkGetDeviceProcAddr( BitCast<VkDevice>(_vkLogicalDevice), "vkDestroyShaderModule" ));

			if ( fpGetPhysicalDeviceFeatures2 )
			{
				VkPhysicalDeviceShaderClockFeaturesKHR	clock_feat = {};
				clock_feat.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;

				VkPhysicalDeviceFeatures2	feats = {};
				feats.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				feats.pNext		= &clock_feat;

				fpGetPhysicalDeviceFeatures2( BitCast<VkPhysicalDevice>(_vkPhysicalDevice), OUT &feats );

				_spirvCompiler->SetShaderClockFeatures( clock_feat.shaderSubgroupClock, clock_feat.shaderDeviceClock );
				_spirvCompiler->SetShaderFeatures( feats.features.vertexPipelineStoresAndAtomics, feats.features.fragmentStoresAndAtomics );
			}
			else
			{
				auto fpGetPhysicalDeviceFeatures = BitCast<PFN_vkGetPhysicalDeviceFeatures>( vkGetInstanceProcAddr( BitCast<VkInstance>(_vkInstance), "vkGetPhysicalDeviceFeatures" ));

				if ( fpGetPhysicalDeviceFeatures )
				{
					VkPhysicalDeviceFeatures	feats = {};
					fpGetPhysicalDeviceFeatures( BitCast<VkPhysicalDevice>(_vkPhysicalDevice), OUT &feats );

					_spirvCompiler->SetShaderFeatures( feats.vertexPipelineStoresAndAtomics, feats.fragmentStoresAndAtomics );
				}
				_spirvCompiler->SetShaderClockFeatures( false, false );
			}
		}
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
		EXLOCK( _lock );

		_compilerFlags = flags;
		
		if ( EnumEq( flags, EShaderCompilationFlags::UseCurrentDeviceLimits ) )
			CHECK_ERR( _spirvCompiler->SetCurrentResourceLimits( _vkPhysicalDevice ))
		else
			CHECK_ERR( _spirvCompiler->SetDefaultResourceLimits() );

		_spirvCompiler->SetCompilationFlags( flags );
		return true;
	}
	
/*
=================================================
	SetDebugFlags
=================================================
*/
	void VPipelineCompiler::SetDebugFlags (EShaderLangFormat flags)
	{
		EXLOCK( _lock );
		_spirvCompiler->SetDebugFlags( flags & EShaderLangFormat::_DebugModeMask );
	}

/*
=================================================
	AddDirectory
=================================================
*/
	void VPipelineCompiler::AddDirectory (StringView path)
	{
		EXLOCK( _lock );

		String	file_path;
		
#	ifdef FS_HAS_FILESYSTEM
		FS::path	fpath{ path };

		if ( not fpath.is_absolute() )
			fpath = FS::absolute( fpath );

		fpath.make_preferred();
		CHECK_ERR( FS::exists( fpath ), void());

		file_path = fpath.string();
#	else
		file_path = path;
#	endif

		for (const auto& dir : _directories) {
			if ( dir == file_path )
				return;	// already exists
		}
		_directories.push_back( std::move(file_path) );
	}

/*
=================================================
	ReleaseUnusedShaders
=================================================
*/
	void VPipelineCompiler::ReleaseUnusedShaders ()
	{
		EXLOCK( _lock );

		if ( _vkLogicalDevice == VK_NULL_HANDLE )
			return;
	
		VkDevice	dev					= BitCast<VkDevice>( _vkLogicalDevice );
		auto		DestroyShaderModule = BitCast<PFN_vkDestroyShaderModule>(_fpDestroyShaderModule);
		
		for (auto iter = _shaderCache.begin(); iter != _shaderCache.end();)
		{
			if ( not iter->second.use_count() == 1 )
			{
				++iter;
				continue;
			}

			Cast<VCachedDebuggableShaderModule>( iter->second )->Destroy( DestroyShaderModule, dev );

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
		EXLOCK( _lock );

		if ( _vkLogicalDevice == VK_NULL_HANDLE )
			return;
		
		VkDevice	dev					= BitCast<VkDevice>( _vkLogicalDevice );
		auto		DestroyShaderModule = BitCast<PFN_vkDestroyShaderModule>(_fpDestroyShaderModule);

		for (auto& sh : _shaderCache)
		{
			// pipeline may keep reference to shader module, so you need to release pipeline before
			ASSERT( sh.second.use_count() == 1 );
			
			Cast<VCachedDebuggableShaderModule>( sh.second )->Destroy( DestroyShaderModule, dev );
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
		// lock is not needed because only '_vkLogicalDevice' read access is used

		if ( ppln._shaders.empty() )
			return false;

		if ( not IsDstFormatSupported( dstFormat, (_vkLogicalDevice != VK_NULL_HANDLE) ))
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
		// lock is not needed because only '_vkLogicalDevice' read access is used

		if ( ppln._shaders.empty() )
			return false;

		if ( not IsDstFormatSupported( dstFormat, (_vkLogicalDevice != VK_NULL_HANDLE) ))
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
		// lock is not needed because only '_vkLogicalDevice' read access is used

		if ( ppln._shaders.empty() )
			return false;

		if ( not IsDstFormatSupported( dstFormat, (_vkLogicalDevice != VK_NULL_HANDLE) ))
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
		// lock is not needed because only '_vkLogicalDevice' read access is used

		if ( not IsDstFormatSupported( dstFormat, (_vkLogicalDevice != VK_NULL_HANDLE) ))
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
	MergeUniformData
=================================================
*/
	ND_ static bool  MergeUniformData (const PipelineDescription::Uniform &src, INOUT PipelineDescription::Uniform &dst)
	{
		return Visit( src.data,
				[&] (const PipelineDescription::Texture &lhs)
				{
					if ( auto* rhs = UnionGetIf<PipelineDescription::Texture>( &dst.data ) )
					{
						ASSERT( lhs.textureType	== rhs->textureType );
						ASSERT( src.index		== dst.index );

						if ( lhs.textureType	== rhs->textureType and
							 src.index			== dst.index )
						{
							dst.stageFlags	|= src.stageFlags;
							rhs->state		|= EResourceState_FromShaders( dst.stageFlags );
							return true;
						}
					}
					return false;
				},
				   
				[&] (const PipelineDescription::Sampler &)
				{
					if ( auto* rhs = UnionGetIf<PipelineDescription::Sampler>( &dst.data ) )
					{
						ASSERT( src.index == dst.index );

						if ( src.index == dst.index )
						{
							dst.stageFlags |= src.stageFlags;
							return true;
						}
					}
					return false;
				},
				
				[&] (const PipelineDescription::SubpassInput &lhs)
				{
					if ( auto* rhs = UnionGetIf<PipelineDescription::SubpassInput>( &dst.data ) )
					{
						ASSERT( lhs.attachmentIndex	== rhs->attachmentIndex );
						ASSERT( lhs.isMultisample	== rhs->isMultisample );
						ASSERT( src.index			== dst.index );
						
						if ( lhs.attachmentIndex	== rhs->attachmentIndex	and
							 lhs.isMultisample		== rhs->isMultisample	and
							 src.index				== dst.index )
						{
							dst.stageFlags	|= src.stageFlags;
							rhs->state		|= EResourceState_FromShaders( dst.stageFlags );
							return true;
						}
					}
					return false;
				},
				
				[&] (const PipelineDescription::Image &lhs)
				{
					if ( auto* rhs = UnionGetIf<PipelineDescription::Image>( &dst.data ) )
					{
						ASSERT( lhs.imageType	== rhs->imageType );
						ASSERT( lhs.format		== rhs->format );
						ASSERT( src.index		== dst.index );
						
						if ( lhs.imageType	== rhs->imageType	and
							 lhs.format		== rhs->format		and
							 src.index		== dst.index )
						{
							MergeShaderAccess( lhs.state, INOUT rhs->state );

							dst.stageFlags	|= src.stageFlags;
							rhs->state		|= EResourceState_FromShaders( dst.stageFlags );
							return true;
						}
					}
					return false;
				},
				
				[&] (const PipelineDescription::UniformBuffer &lhs)
				{
					if ( auto* rhs = UnionGetIf<PipelineDescription::UniformBuffer>( &dst.data ) )
					{
						ASSERT( lhs.size	== rhs->size );
						ASSERT( src.index	== dst.index );

						if ( lhs.size	== rhs->size	and
							 src.index	== dst.index )
						{
							dst.stageFlags	|= src.stageFlags;
							rhs->state		|= EResourceState_FromShaders( dst.stageFlags );
							return true;
						}
					}
					return false;
				},
				
				[&] (const PipelineDescription::StorageBuffer &lhs)
				{
					if ( auto* rhs = UnionGetIf<PipelineDescription::StorageBuffer>( &dst.data ) )
					{
						ASSERT( lhs.staticSize	== rhs->staticSize );
						ASSERT( lhs.arrayStride	== rhs->arrayStride );
						ASSERT( src.index		== dst.index );
						
						if ( lhs.staticSize		== rhs->staticSize	and
							 lhs.arrayStride	== rhs->arrayStride	and
							 src.index			== dst.index )
						{
							MergeShaderAccess( lhs.state, INOUT rhs->state );

							dst.stageFlags	|= src.stageFlags;
							rhs->state		|= EResourceState_FromShaders( dst.stageFlags );
							return true;
						}
					}
					return false;
				},
				
				[&] (const PipelineDescription::RayTracingScene &lhs)
				{
					if ( auto* rhs = UnionGetIf<PipelineDescription::RayTracingScene>( &dst.data ) )
					{
						ASSERT( lhs.state == rhs->state );

						if ( lhs.state == rhs->state )
						{
							dst.stageFlags |= src.stageFlags;
							return true;
						}
					}
					return false;
				},

				[] (const NullUnion &) { return false; }
			);
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

			if ( un.second.index.VKBinding() != iter->second.index.VKBinding() )
			{
				for (uint i = 0; i < 100; ++i)
				{
				# ifdef FG_OPTIMIZE_IDS
					UniformID	id { "un" + ToString(i) };	// TODO
				# else
					UniformID	id { String(un.first.GetName()) + "_" + ToString(i) };
				# endif

					if ( dstUniforms.count( id ) == 0 and srcUniforms.count( id ) == 0 )
					{
						dstUniforms.insert_or_assign( id, un.second );
						break;
					}
				}
			}
			else
			{
				COMP_CHECK_ERR( MergeUniformData( un.second, INOUT iter->second ));
			}
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

		// merge push constants
		for (auto& src_pc : srcLayout.pushConstants)
		{
			// Vulkan valid usage:
			// * offset must be a multiple of 4
			// * size must be a multiple of 4
			// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VUID-vkCmdPushConstants-offset-00368
			COMP_CHECK_ERR( src_pc.second.offset % 4 == 0 );
			COMP_CHECK_ERR( src_pc.second.size   % 4 == 0 );

			auto	iter = dstLayout.pushConstants.find( src_pc.first );

			if ( iter == dstLayout.pushConstants.end() )
			{
				// check intersections
				for (auto& dst_pc : dstLayout.pushConstants)
				{
					// Vulkan valid usage:
					// * Any two elements of pPushConstantRanges must not include the same stage in stageFlags
					// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VUID-VkPipelineLayoutCreateInfo-pPushConstantRanges-00292

					if ( IsIntersects( src_pc.second.offset, src_pc.second.offset + src_pc.second.size,
									   dst_pc.second.offset, dst_pc.second.offset + dst_pc.second.size ))
					{
						// It is forbidden because FG can't handle this case.
						COMP_RETURN_ERR( "Push constants with different names uses same memory range:\n"s <<
										 "    First  (" << ToString( src_pc.first ) << "): " << ToString(src_pc.second.offset) << " .. " << ToString(src_pc.second.offset + src_pc.second.size) << '\n' <<
										 "    Second (" << ToString( dst_pc.first ) << "): " << ToString(dst_pc.second.offset) << " .. " << ToString(dst_pc.second.offset + dst_pc.second.size) );
					}
				}

				// add new push constant
				dstLayout.pushConstants.insert( src_pc );
				continue;
			}
			
			// merge
			iter->second.size		  = Max( src_pc.second.offset + src_pc.second.size, iter->second.offset + iter->second.size );
			iter->second.offset		  = Min( src_pc.second.offset, iter->second.offset );
			iter->second.size		 -= iter->second.offset;
			iter->second.stageFlags |= src_pc.second.stageFlags;
			
			// same as above
			COMP_CHECK_ERR( iter->second.offset % 4 == 0 );
			COMP_CHECK_ERR( iter->second.size   % 4 == 0 );
		}

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
	UpdateBufferDynamicOffsets
=================================================
*/
	static void UpdateBufferDynamicOffsets (INOUT PipelineDescription::DescriptorSets_t &descriptorSets)
	{
		FixedArray< PipelineDescription::Uniform *, FG_MaxBufferDynamicOffsets >	sorted;
		
		for (auto& ds_layout : descriptorSets)
		{
			for (auto& un : *ds_layout.uniforms)
			{
				if ( auto* ubuf = UnionGetIf< PipelineDescription::UniformBuffer >( &un.second.data );
					 ubuf and EnumEq( ubuf->state, EResourceState::_BufferDynamicOffset ) )
				{
					sorted.push_back( const_cast< PipelineDescription::Uniform *>( &un.second ));
				}
				else
				if ( auto* sbuf = UnionGetIf< PipelineDescription::StorageBuffer >( &un.second.data );
					sbuf and EnumEq( sbuf->state, EResourceState::_BufferDynamicOffset ) )
				{
					sorted.push_back( const_cast< PipelineDescription::Uniform *>( &un.second ));
				}
			}
		}
		
		std::sort( sorted.begin(), sorted.end(), [](auto& lhs, auto& rhs) { return lhs->index.VKBinding() < rhs->index.VKBinding(); });
		
		uint	index = 0;
		for (auto* un : sorted)
		{
			if ( auto* ubuf = UnionGetIf< PipelineDescription::UniformBuffer >( &un->data ))
				ubuf->dynamicOffsetIndex = index++;
			else
			if ( auto* sbuf = UnionGetIf< PipelineDescription::StorageBuffer >( &un->data ))
				sbuf->dynamicOffsetIndex = index++;
		}

		CHECK( index <= FG_MaxBufferDynamicOffsets );
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
		EXLOCK( _lock );
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
			if ( auto* shader_data = UnionGetIf< StringShaderData >( &iter->second ) )
			{
				SpirvCompiler::ShaderReflection	reflection;
				String							log;
				PipelineDescription::Shader		new_shader;

				if ( not _spirvCompiler->Compile( shader.first, iter->first, spirv_format, (*shader_data)->GetEntry(),
												  (*shader_data)->GetData(), (*shader_data)->GetDebugName(),
												  OUT new_shader, OUT reflection, OUT log ))
				{
					COMP_RETURN_ERR( log );
				}
				
				if ( create_module )
					COMP_CHECK_ERR( _CreateVulkanShader( INOUT new_shader ) );

				COMP_CHECK_ERR( _MergePipelineResources( reflection.layout, INOUT new_ppln._pipelineLayout ));

				switch ( shader.first )
				{
					case EShader::MeshTask :
						new_ppln._defaultTaskGroupSize	= reflection.mesh.taskGroupSize;
						new_ppln._taskSizeSpec			= reflection.mesh.taskGroupSpecialization;
						break;

					case EShader::Mesh :
						new_ppln._maxIndices			= reflection.mesh.maxIndices;
						new_ppln._maxVertices			= reflection.mesh.maxVertices;
						new_ppln._topology				= reflection.mesh.topology;
						new_ppln._defaultMeshGroupSize	= reflection.mesh.meshGroupSize;
						new_ppln._meshSizeSpec			= reflection.mesh.meshGroupSpecialization;
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

		UpdateBufferDynamicOffsets( new_ppln._pipelineLayout.descriptorSets );

		std::swap( ppln, new_ppln );
		_CheckHashCollision( ppln );
		
		ASSERT( _CheckDescriptorBindings( ppln ));
		return true;
	}
	
/*
=================================================
	Compile
=================================================
*/
	bool VPipelineCompiler::Compile (INOUT RayTracingPipelineDesc &ppln, EShaderLangFormat dstFormat)
	{
		EXLOCK( _lock );
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
			if ( auto* shader_data = UnionGetIf< StringShaderData >( &iter->second ) )
			{
				SpirvCompiler::ShaderReflection		reflection;
				String								log;
				RayTracingPipelineDesc::RTShader	new_shader;

				if ( not _spirvCompiler->Compile( shader.second.shaderType, iter->first, spirv_format, (*shader_data)->GetEntry(),
												  (*shader_data)->GetData(), (*shader_data)->GetDebugName(),
												  OUT new_shader, OUT reflection, OUT log ))
				{
					COMP_RETURN_ERR( log );
				}
				
				if ( create_module )
					COMP_CHECK_ERR( _CreateVulkanShader( INOUT new_shader ) );

				COMP_CHECK_ERR( _MergePipelineResources( reflection.layout, INOUT new_ppln._pipelineLayout ));

				switch ( shader.second.shaderType )
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

				new_shader.shaderType = shader.second.shaderType;
				new_ppln._shaders.insert_or_assign( shader.first, std::move(new_shader) );
			}
			else
			{
				COMP_RETURN_ERR( "invalid shader data type!" );
			}
		}
		
		UpdateBufferDynamicOffsets( new_ppln._pipelineLayout.descriptorSets );

		std::swap( ppln, new_ppln );
		_CheckHashCollision( ppln );
		
		ASSERT( _CheckDescriptorBindings( ppln ));
		return true;
	}

/*
=================================================
	Compile
=================================================
*/
	bool VPipelineCompiler::Compile (INOUT GraphicsPipelineDesc &ppln, EShaderLangFormat dstFormat)
	{
		EXLOCK( _lock );
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
			if ( auto* shader_data = UnionGetIf< StringShaderData >( &iter->second ) )
			{
				SpirvCompiler::ShaderReflection	reflection;
				String							log;
				PipelineDescription::Shader		new_shader;

				if ( not _spirvCompiler->Compile( shader.first, iter->first, spirv_format, (*shader_data)->GetEntry(),
												  (*shader_data)->GetData(), (*shader_data)->GetDebugName(),
												  OUT new_shader, OUT reflection, OUT log ))
				{
					COMP_RETURN_ERR( log );
				}
				
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
		UpdateBufferDynamicOffsets( new_ppln._pipelineLayout.descriptorSets );

		std::swap( ppln, new_ppln );
		_CheckHashCollision( ppln );

		ASSERT( _CheckDescriptorBindings( ppln ));
		return true;
	}
	
/*
=================================================
	Compile
=================================================
*/
	bool VPipelineCompiler::Compile (INOUT ComputePipelineDesc &ppln, EShaderLangFormat dstFormat)
	{
		EXLOCK( _lock );
		ASSERT( IsSupported( ppln, dstFormat ) );
		
		const bool					create_module	= ((dstFormat & EShaderLangFormat::_StorageFormatMask) == EShaderLangFormat::ShaderModule);
		const EShaderLangFormat		spirv_format	= not create_module ? dstFormat :
														((dstFormat & ~EShaderLangFormat::_StorageFormatMask) | EShaderLangFormat::SPIRV);

		auto	iter = FindHighPriorityShaderFormat( ppln._shader.data, spirv_format );
		
		if ( iter == ppln._shader.data.end() )
		{
			RETURN_ERR( "no suitable shader format found!" );
		}

		if ( auto* shader_data = UnionGetIf< StringShaderData >( &iter->second ) )
		{
			SpirvCompiler::ShaderReflection	reflection;
			String							log;
			ComputePipelineDesc				new_ppln;

			if ( not _spirvCompiler->Compile( EShader::Compute, iter->first, spirv_format, (*shader_data)->GetEntry(),
											  (*shader_data)->GetData(), (*shader_data)->GetDebugName(),
											  OUT new_ppln._shader, OUT reflection, OUT log ))
			{
				COMP_RETURN_ERR( log );
			}
			
			if ( create_module )
				COMP_CHECK_ERR( _CreateVulkanShader( INOUT new_ppln._shader ) );

			new_ppln._defaultLocalGroupSize = reflection.compute.localGroupSize;
			new_ppln._localSizeSpec			= reflection.compute.localGroupSpecialization;
			new_ppln._pipelineLayout		= std::move(reflection.layout);
			
			UpdateBufferDynamicOffsets( new_ppln._pipelineLayout.descriptorSets );

			std::swap( ppln, new_ppln );
			_CheckHashCollision( ppln );

			ASSERT( _CheckDescriptorBindings( ppln ));
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
		uint count = 0;

		for (auto sh_iter = shader.data.begin(); sh_iter != shader.data.end();)
		{
			switch ( sh_iter->first & EShaderLangFormat::_StorageFormatMask )
			{
				// create shader module from SPIRV
				case EShaderLangFormat::SPIRV : {
					const auto*	spv_data_ptr = UnionGetIf<BinaryShaderData>( &sh_iter->second );
					COMP_CHECK_ERR( spv_data_ptr and *spv_data_ptr, "invalid shader data!" );

					const EShaderLangFormat	module_fmt = EShaderLangFormat::ShaderModule | EShaderLangFormat::Vulkan |
														 (sh_iter->first & EShaderLangFormat::_VersionModeFlagsMask);
					
					// search in existing shader modules
					auto	spv_data	= *spv_data_ptr;
					auto	iter		= _shaderCache.find( spv_data );

					if ( iter == _shaderCache.end() )
					{
						// create new shader module
						VkShaderModuleCreateInfo	shader_info = {};
						shader_info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
						shader_info.codeSize	= size_t(ArraySizeOf( spv_data->GetData() ));
						shader_info.pCode		= spv_data->GetData().data();

						VkShaderModule		shader_id;
						COMP_CHECK_ERR( CreateShaderModule( BitCast<VkDevice>( _vkLogicalDevice ), &shader_info, null, OUT &shader_id ) == VK_SUCCESS );

						auto	module	= MakeShared<VCachedDebuggableShaderModule>( shader_id, spv_data );
						auto	base	= Cast< PipelineDescription::IShaderData<ShaderModuleVk_t> >( module );

						iter = _shaderCache.insert({ spv_data, base }).first;
					}
					
					shader.data.erase( sh_iter );
					shader.data.insert({ module_fmt, iter->second });
					sh_iter = shader.data.begin();
					++count;
					break;
				}

				// keep other shader modules
				case EShaderLangFormat::ShaderModule :
					++sh_iter;
					break;

				// remove unknown shader data
				default :
					sh_iter = shader.data.erase( sh_iter );
					break;
			}
		}

		if ( count == 0 )
			COMP_RETURN_ERR( "SPIRV shader data is not found!" );

		return true;
	}
	
/*
=================================================
	_CheckHashCollision
=================================================
*/
	void VPipelineCompiler::_CheckHashCollision (const MeshPipelineDesc &desc)
	{
		DEBUG_ONLY(
		for (auto& sh : desc._shaders)
		{
			for (auto& sc : sh.second.specConstants) {
				_hashCollisionCheck.Add( sc.first );
			}
		})

		_CheckHashCollision2( desc );
	}

	void VPipelineCompiler::_CheckHashCollision (const RayTracingPipelineDesc &desc)
	{
		DEBUG_ONLY(
		for (auto& sh : desc._shaders)
		{
			_hashCollisionCheck.Add( sh.first );

			for (auto& sc : sh.second.specConstants) {
				_hashCollisionCheck.Add( sc.first );
			}
		})

		_CheckHashCollision2( desc );
	}

	void VPipelineCompiler::_CheckHashCollision (const GraphicsPipelineDesc &desc)
	{
		DEBUG_ONLY(
		for (auto& sh : desc._shaders)
		{
			for (auto& sc : sh.second.specConstants) {
				_hashCollisionCheck.Add( sc.first );
			}
		})

		_CheckHashCollision2( desc );
	}

	void VPipelineCompiler::_CheckHashCollision (const ComputePipelineDesc &desc)
	{
		DEBUG_ONLY(
		for (auto& sc : desc._shader.specConstants)
		{
			_hashCollisionCheck.Add( sc.first );
		})

		_CheckHashCollision2( desc );
	}
	
/*
=================================================
	_CheckHashCollision2
=================================================
*/
	void VPipelineCompiler::_CheckHashCollision2 (const PipelineDescription &desc)
	{
		DEBUG_ONLY(
		for (auto& pc : desc._pipelineLayout.pushConstants)
		{
			_hashCollisionCheck.Add( pc.first );
		}

		for (auto& ds : desc._pipelineLayout.descriptorSets)
		{
			_hashCollisionCheck.Add( ds.id );

			for (auto& un : *ds.uniforms)
			{
				_hashCollisionCheck.Add( un.first );
			}
		})
	}
	
/*
=================================================
	_CheckDescriptorBindings
=================================================
*/
	bool VPipelineCompiler::_CheckDescriptorBindings (const PipelineDescription &desc) const
	{
		HashSet<uint>	binding_indices;

		for (auto& ds : desc._pipelineLayout.descriptorSets)
		{
			binding_indices.clear();

			for (auto& un : *ds.uniforms)
			{
				// binding index already occupied
				CHECK_ERR( binding_indices.insert( un.second.index.VKBinding() ).second );
			}
		}
		return true;
	}

}	// FG
