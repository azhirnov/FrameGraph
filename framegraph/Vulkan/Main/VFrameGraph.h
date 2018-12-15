// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraph.h"
#include "VResourceManager.h"
#include "VDevice.h"
#include "VDebugger.h"
#include "VSubmissionGraph.h"

namespace FG
{

	//
	// Frame Graph
	//

	class VFrameGraph final : public FrameGraph
	{
	// types
	private:
		enum class EState : uint
		{
			Initial,
			Idle,
			Begin,
			RunThreads,
			End,
			Destroyed,
		};

		using VThreadWeak_t		= std::weak_ptr< VFrameGraphThread >;
		using Threads_t			= Array< VThreadWeak_t >;
		using Compilers_t		= HashSet< IPipelineCompilerPtr >;


	// variables
	private:
		VDevice					_device;
		Threads_t				_threads;
		std::mutex				_threadLock;
		VSubmissionGraph		_submissionGraph;

		uint					_ringBufferSize	= 0;
		uint					_frameId		= 0;
		uint					_visitorID		= 0;

		VResourceManager		_resourceMngr;
		std::atomic<EState>		_state;

		VDebugger				_debugger;

		Compilers_t				_pplnCompilers;
		ECompilationFlags		_defaultCompilationFlags;
		ECompilationDebugFlags	_defaultDebugFlags;

		Statistics				_statistics;
		RWRaceConditionCheck	_rcCheck;


	// methods
	public:
		explicit VFrameGraph (const VulkanDeviceInfo &);
		~VFrameGraph ();

		FGThreadPtr  CreateThread (const ThreadDesc &) override;

		// initialization
		bool  Initialize (uint ringBufferSize) override;
		void  Deinitialize () override;
		bool  AddPipelineCompiler (const IPipelineCompilerPtr &comp) override;
		void  SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags) override;
		
		// frame execution
		bool  BeginFrame (const SubmissionGraph &) override;
		bool  EndFrame () override;
		bool  WaitIdle () override;
		bool  SkipBatch (const CommandBatchID &batchId, uint indexInBatch) override;
		bool  SubmitBatch (const CommandBatchID &batchId, uint indexInBatch, const ExternalCmdBatch_t &data) override;

		// debugging
		bool  GetStatistics (OUT Statistics &result) const override;
		bool  DumpToString (OUT String &result) const override;
		bool  DumpToGraphViz (OUT String &result) const override;

		ND_ VResourceManager&	GetResourceMngr ()				{ SHAREDLOCK( _rcCheck );  return _resourceMngr; }
		ND_ VDevice const&		GetDevice ()			const	{ SHAREDLOCK( _rcCheck );  return _device; }
		ND_ VDebugger *			GetDebugger ()					{ SHAREDLOCK( _rcCheck );  return &_debugger; }

		ND_ uint				GetRingBufferSize ()	const	{ SHAREDLOCK( _rcCheck );  return _ringBufferSize; }


	private:
		ND_ bool  _IsInitialized () const;
			bool  _WaitIdle ();

		ND_ EState	_GetState () const;
			void	_SetState (EState newState);
		ND_ bool	_SetState (EState expected, EState newState);
	};


}	// FG
