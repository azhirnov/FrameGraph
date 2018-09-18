// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/ResourceEnums.h"
#include "framegraph/Public/LowLevel/EResourceState.h"

namespace FG
{

	//
	// Buffer Resource
	//

	class BufferResource : public std::enable_shared_from_this<BufferResource>
	{
	// types
	public:
		struct BufferDescription
		{
		// variables
			BytesU			size;
			EBufferUsage	usage	= Default;

		// methods
			BufferDescription () {}
			BufferDescription (BytesU size, EBufferUsage usage) : size(size), usage(usage) {}
		};

		using Description_t = BufferDescription;


	// variables
	protected:
		EBufferUsage		_currentUsage	= Default;
		Description_t		_descr;

		bool				_isLogical   : 1;
		bool				_isExternal  : 1;
		bool				_isImmutable : 1;


	// methods
	protected:
		BufferResource () : _isLogical{false}, _isExternal{false}, _isImmutable{false} {}

	public:
		virtual ~BufferResource () {}
		
		ND_ virtual BufferPtr		GetReal (Task task, EResourceState rs) = 0;
			virtual void			SetDebugName (StringView name) = 0;
			virtual bool			MakeImmutable (bool immutable) = 0;

			void					AddUsage (EBufferUsage value)		{ _currentUsage |= value; }

		ND_ Description_t const&	Description ()		const			{ return _descr; }
		ND_ BytesU					Size ()				const			{ return _descr.size; }

		ND_ bool					IsLogical ()		const			{ return _isLogical; }
		ND_ bool					IsExternal ()		const			{ return _isExternal; }

		ND_ bool					IsImmutable ()		const			{ return _isImmutable; }
		ND_ bool					IsMutable ()		const			{ return not _isImmutable; }
	};


}	// FG
