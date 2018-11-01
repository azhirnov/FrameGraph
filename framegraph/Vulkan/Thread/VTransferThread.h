// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VFrameGraphThread.h"

namespace FG
{

	//
	// Frame Graph Transfer Thread
	//

	class VTransferThread final : public VFrameGraphThread
	{
	// types
	private:


	// variables
	private:


	// methods
	public:
		explicit VTransferThread (VFrameGraph &);
		~VTransferThread ();
			

		// initialization
		bool		Initialize (uint ringBufferSize) override;
		void		Deinitialize () override;
		void		SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags) override;

		// frame execution
		bool		Begin () override;
		bool		Compile () override;
		bool		Execute () override;
		bool		Wait () override;


		// tasks
		Task		AddTask (const UpdateBuffer &) override;
		Task		AddTask (const UpdateImage &) override;
		Task		AddTask (const ReadBuffer &) override;
		Task		AddTask (const ReadImage &) override;
	};


}	// FG
