# setup build on CI

if (FG_CI_BUILD)
	message( STATUS "configured CI build" )

	set( FG_ENABLE_OPENVR OFF CACHE INTERNAL "" FORCE )
	set( FG_ENABLE_SIMPLE_COMPILER_OPTIONS ON CACHE INTERNAL "" FORCE )
	set( FG_ENABLE_TESTS ON CACHE INTERNAL "" FORCE )
	set( FG_ENABLE_ASSIMP OFF CACHE INTERNAL "" FORCE )
	set( FG_ENABLE_GLM OFF CACHE INTERNAL "" FORCE )
	set( FG_ENABLE_DEVIL OFF CACHE INTERNAL "" FORCE )
	set( FG_ENABLE_FREEIMAGE OFF CACHE INTERNAL "" FORCE )

	if (WIN32 AND MSVC)
		set( FG_EXTERNALS_USE_PREBUILD ON CACHE INTERNAL "" FORCE )
	else ()
		set( FG_EXTERNALS_USE_PREBUILD OFF CACHE INTERNAL "" FORCE )
	endif ()

	if (UNIX)
		set( ENABLE_OPT OFF CACHE INTERNAL "" FORCE )	# temp
	endif ()

	enable_testing()
	add_test( NAME "Tests.STL" COMMAND "Tests.STL" )
	add_test( NAME "Tests.PipelineCompiler" COMMAND "Tests.PipelineCompiler" )
	#add_test( NAME "Tests.FrameGraph" COMMAND "Tests.FrameGraph" )

endif ()
