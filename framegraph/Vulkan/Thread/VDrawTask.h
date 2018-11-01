// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphDrawTask.h"
#include "VCommon.h"

namespace FG
{

	template <typename Task>
	class FGDrawTask;


	
	//
	// Draw Task interface
	//

	class IDrawTask
	{
	// types
	public:
		using TaskName_t		= DrawTask::TaskName_t;
		using ProcessFunc_t		= void (*) (void *visitor, void *taskData);
		

	// variables
	private:
		ProcessFunc_t	_pass1	= null;
		ProcessFunc_t	_pass2	= null;


	// interface
	public:
		IDrawTask (ProcessFunc_t pass1, ProcessFunc_t pass2) :
			_pass1{pass1}, _pass2{pass2} {}

		virtual ~IDrawTask () {}

		ND_ virtual StringView	GetName () const = 0;
		ND_ virtual RGBA8u		GetDebugColor () const = 0;
		
		void Process1 (void *visitor)		{ ASSERT( _pass1 );  _pass1( visitor, this ); }
		void Process2 (void *visitor)		{ ASSERT( _pass2 );  _pass2( visitor, this ); }
	};



	//
	// Draw Task
	//

	template <typename Task>
	class FGDrawTask final : public IDrawTask
	{
	// variables
	private:
		Task			_task;


	// methods
	public:
		FGDrawTask (const Task &task, ProcessFunc_t pass1, ProcessFunc_t pass2) :
			IDrawTask{pass1, pass2}, _task{task} {}
		
		StringView	GetName ()			const override	{ return _task.taskName; }
		RGBA8u		GetDebugColor ()	const override	{ return _task.debugColor; }

		Task &		Data ()								{ return _task; }
		Task const&	Data ()				const			{ return _task; }
	};


}	// FG
