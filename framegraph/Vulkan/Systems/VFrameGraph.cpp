// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraph.h"
#include "VFrameGraphDebugger.h"

namespace FG
{
	
/*
=================================================
	constructor
=================================================
*/
	VFrameGraph::VFrameGraph (const VulkanDeviceInfo &vdi) :
		_device{ vdi },
		_renderPassCache{ _device },	_pipelineCache{ _device },
		_samplerCache{ _device },		_memoryMngr{ _device },
		_stagingMngr{ *this },
		_compilationFlags{ Default }
	{
		_SetupQueues( vdi.queues );

		constexpr BytesU	DrawTasksBlockSize { Max(sizeof(DrawTask), sizeof(DrawIndexedTask)) };

		_drawTaskAllocator.SetBlockSize( AlignToLarger( DrawTasksBlockSize, 4_Mb ));
	}

/*
=================================================
	CreateVulkanFrameGraph
=================================================
*/
	FrameGraphPtr  FrameGraph::CreateVulkanFrameGraph (const VulkanDeviceInfo &vdi)
	{
		CHECK_ERR( vdi.instance and vdi.physicalDevice and vdi.device and vdi.surface and not vdi.queues.empty() );
		
		CHECK_ERR( VulkanLoader::Initialize() );

		return FrameGraphPtr{ new VFrameGraph( vdi )};
	}

/*
=================================================
	destructor
=================================================
*/
	VFrameGraph::~VFrameGraph ()
	{
		// missing call of 'Deinitialize'
		CHECK( _perFrame.empty() );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VFrameGraph::Initialize (uint swapchainLength)
	{
		CHECK_ERR( _perFrame.empty() );

		_device._Initialize( swapchainLength );

		for (auto& q : _cmdQueues) {
			CHECK_ERR( q.Initialize( swapchainLength, true ));
		}

		CHECK_ERR( _memoryMngr.Initialize() );
		CHECK_ERR( _pipelineCache.Initialize() );
		CHECK_ERR( _renderPassCache.Initialize() );
		CHECK_ERR( _stagingMngr.Initialize( swapchainLength ) );

		_currFrame	= 0;
		_perFrame.resize( swapchainLength );

		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VFrameGraph::Deinitialize ()
	{
		// don't unload twise
		const bool	free_loader	= _perFrame.size() or _device.GetVkInstance();

		for (auto& frame : _perFrame) {
			_DestroyLogicalResources( frame );
		}
		_perFrame.clear();

		_renderPassCache.Deinitialize();
		_pipelineCache.Deinitialize();
		_stagingMngr.Deinitialize();
		_memoryMngr.Deinitialize();
		
		for (auto& q : _cmdQueues) {
			q.Deinitialize();
		}
		_cmdQueues.clear();

		_device._Deinitialize();

		if ( free_loader ) {
			VulkanLoader::Unload();
		}
	}

/*
=================================================
	AddPipelineCompiler
=================================================
*/
	void VFrameGraph::AddPipelineCompiler (const IPipelineCompilerPtr &comp)
	{
		_pipelineCache.AddCompiler( comp );
	}
	
/*
=================================================
	SetCompilationFlags
=================================================
*/
	void VFrameGraph::SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags)
	{
		_compilationFlags = flags;

		if ( EnumEq( _compilationFlags, ECompilationFlags::EnableDebugger ) )
		{
			if ( not _debugger )
				_debugger.reset( new VFrameGraphDebugger() );

			_debugger->Setup( debugFlags );
		}
		else
			_debugger.reset();
	}

/*
=================================================
	CreatePipeline
=================================================
*/
	PipelinePtr  VFrameGraph::CreatePipeline (MeshPipelineDesc &&desc)
	{
		return _pipelineCache.CreatePipeline( GetSelfShared(), std::move(desc) );
	}

	PipelinePtr  VFrameGraph::CreatePipeline (RayTracingPipelineDesc &&desc)
	{
		return null;
		//return _pipelineCache.CreatePipeline( GetSelfShared(), std::move(desc) );
	}

	PipelinePtr  VFrameGraph::CreatePipeline (GraphicsPipelineDesc &&desc)
	{
		return _pipelineCache.CreatePipeline( GetSelfShared(), std::move(desc) );
	}
	
	PipelinePtr  VFrameGraph::CreatePipeline (ComputePipelineDesc &&desc)
	{
		return _pipelineCache.CreatePipeline( GetSelfShared(), std::move(desc) );
	}

/*
=================================================
	CreateImage
=================================================
*/
	ImagePtr  VFrameGraph::CreateImage (EMemoryType		memType,
										EImage			imageType,
										const uint3		&dimension,
										EPixelFormat	format,
										EImageUsage		usage,
										ImageLayer		arrayLayer,
										MipmapLevel		maxLevel,
										MultiSamples	samples)
	{
		VImagePtr	image = MakeShared<VImage>( GetSelfShared() );

		CHECK_ERR( image->_CreateImage( ImageResource::ImageDescription{ imageType, dimension, format, usage, arrayLayer, maxLevel, samples },
										EMemoryTypeExt(memType), _memoryMngr ));

		return image;
	}
	
/*
=================================================
	CreateLogicalImage
=================================================
*/
	ImagePtr  VFrameGraph::CreateLogicalImage (EMemoryType		memType,
											   EImage			imageType,
											   const uint3		&dimension,
											   EPixelFormat		format,
											   ImageLayer		arrayLayer,
											   MipmapLevel		maxLevel,
											   MultiSamples		samples)
	{
		// TODO
		ASSERT(false);
		return null;
	}
	
/*
=================================================
	CreateBuffer
=================================================
*/
	BufferPtr  VFrameGraph::CreateBuffer (EMemoryType	memType,
										  BytesU		size,
										  EBufferUsage	usage)
	{
		VBufferPtr	buffer = MakeShared<VBuffer>( GetSelfShared() );

		CHECK_ERR( buffer->_CreateBuffer( BufferResource::BufferDescription{ size, usage }, EMemoryTypeExt(memType), _memoryMngr ) );

		return buffer;
	}
	
/*
=================================================
	CreateLogicalBuffer
=================================================
*/
	BufferPtr  VFrameGraph::CreateLogicalBuffer (EMemoryType	memType,
												 BytesU				size)
	{
		// TODO
		ASSERT(false);
		return null;
	}
	
/*
=================================================
	CreateSampler
=================================================
*/
	SamplerPtr  VFrameGraph::CreateSampler (const SamplerDesc &desc)
	{
		return _samplerCache.CreateSampler( desc );
	}

/*
=================================================
	CreateRenderPass
=================================================
*/
	RenderPass  VFrameGraph::CreateRenderPass (const RenderPassDesc &desc)
	{
		CHECK_ERR( not desc.renderTargets.empty() );
		CHECK_ERR(All( desc.area.Size() > int2(0) ));

		_renderPasses.push_back(UniquePtr<VLogicalRenderPass>{ new VLogicalRenderPass{ desc } });
		return _renderPasses.back().get();
	}
	
/*
=================================================
	GetStatistics
=================================================
*/
	VFrameGraph::Statistics const&  VFrameGraph::GetStatistics () const
	{
		return _statistics;
	}

/*
=================================================
	DumpToString
=================================================
*/
	bool VFrameGraph::DumpToString (OUT String &result) const
	{
		result.clear();

		if ( not _debugger )
			return false;

		_debugger->DumpFrame( OUT result );
		return true;
	}
	
/*
=================================================
	DumpToGraphViz
=================================================
*/
	bool VFrameGraph::DumpToGraphViz (EGraphVizFlags flags, OUT String &result) const
	{
		return false;
	}

}	// FG
