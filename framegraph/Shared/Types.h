// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
// TODO: rename

#pragma once

namespace FG
{
	
	class IFrameGraphTask
	{};

	class IFrameGraphCommandBatch : public std::enable_shared_from_this<IFrameGraphCommandBatch>
	{};

}	// FG
