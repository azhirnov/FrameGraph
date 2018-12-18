// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once


#define PRIVATE_COMP_RETURN_ERR( _text_, _ret_ ) \
		{if ( not EnumEq( _compilerFlags, EShaderCompilationFlags::Quiet )) { \
			FG_LOGE( _text_ ); \
		}else{ \
			FG_LOGI( _text_ ); \
		}return (_ret_); \
		}

#define COMP_RETURN_ERR( ... ) \
		PRIVATE_COMP_RETURN_ERR( FG_PRIVATE_GETARG_0( __VA_ARGS__ ), FG_PRIVATE_GETARG_1( __VA_ARGS__, ::FG::Default ) )


#define PRIVATE_COMP_CHECK_ERR( _expr_, _ret_ ) \
		{if (( _expr_ )) {}\
		 else \
			PRIVATE_COMP_RETURN_ERR( FG_PRIVATE_TOSTRING(_expr_), (_ret_) ) \
		}

#define COMP_CHECK_ERR( ... ) \
		PRIVATE_COMP_CHECK_ERR( FG_PRIVATE_GETARG_0( __VA_ARGS__ ), FG_PRIVATE_GETARG_1( __VA_ARGS__, ::FG::Default ) )

