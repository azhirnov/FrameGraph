// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VTaskGraph.h"

namespace FG
{

	//
	// Dummy Task
	//
	class VFgDummyTask final : public IFrameGraphTask
	{
	};


	inline Array<UniquePtr<VFgDummyTask>>  GenDummyTasks (size_t count)
	{
		Array<UniquePtr<VFgDummyTask>>	result;		result.reserve( count );

		for (size_t i = 0; i < count; ++i)
		{
			UniquePtr<VFgDummyTask>		task{ new VFgDummyTask()};

			task->SetExecutionOrder( ExeOrderIndex(size_t(ExeOrderIndex::First) + i) );

			result.push_back( std::move(task) );
		}

		return result;
	}

}	// FG
