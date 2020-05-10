# find or download SDL2

if (${FG_ENABLE_SDL2})
	set( FG_EXTERNAL_SDL2_PATH "" CACHE PATH "path to SDL2 source" )
	mark_as_advanced( FG_EXTERNAL_SDL2_PATH )
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_SDL2_PATH}/include/SDL.h")
		message( STATUS "SDL2 is not found in \"${FG_EXTERNAL_SDL2_PATH}\"" )
		set( FG_EXTERNAL_SDL2_PATH "${FG_EXTERNALS_PATH}/SDL2" CACHE PATH "" FORCE )
	else ()
		message( STATUS "SDL2 found in \"${FG_EXTERNAL_SDL2_PATH}\"" )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_SDL2_PATH}/include/SDL.h")
		FetchContent_Declare( ExternalSDL2
			URL					https://www.libsdl.org/release/SDL2-2.0.8.zip
			SOURCE_DIR			"${FG_EXTERNAL_SDL2_PATH}"
		)
		
		FetchContent_GetProperties( ExternalSDL2 )
		if (NOT ExternalSDL2_POPULATED)
			message( STATUS "downloading SDL2..." )
			FetchContent_Populate( ExternalSDL2 )
		endif ()
	endif ()
	
	set( SDL_SHARED OFF CACHE BOOL "SDL2 option" FORCE )
	set( SDL_STATIC ON CACHE BOOL "SDL2 option" FORCE )
	set( SDL_AUDIO OFF CACHE BOOL "SDL2 option" FORCE )
	set( SDL_RENDER OFF CACHE BOOL "SDL2 option" FORCE )
	set( SDL_TEST OFF CACHE BOOL "SDL2 option" FORCE )
	set( DIRECTX OFF CACHE BOOL "SDL2 option" FORCE )
	set( RENDER_D3D OFF CACHE BOOL "SDL2 option" FORCE )

	add_subdirectory( "${FG_EXTERNAL_SDL2_PATH}" "SDL2" )
	
	if (TARGET SDL2main)
		set_property( TARGET "SDL2main" PROPERTY FOLDER "External" )
	endif ()

	if (TARGET uninstall)
		set_property( TARGET "uninstall" PROPERTY FOLDER "External" )
	endif ()

	mark_as_advanced( 3DNOW ALSA ALTIVEC ARTS ASSEMBLY ASSERTIONS CLOCK_GETTIME DIRECTX
					  DISKAUDIO DUMMYAUDIO ESD FORCE_STATIC_VCRT FUSIONSOUND GCC_ATOMICS
					  INPUT_TSLIB JACK LIBC LIBSAMPLERATE MMX NAS NAS_SHARED OSS PTHREADS
					  PULSEAUDIO RENDER_D3D RPATH SNDIO SSE SSE2 SSE3 SSEMATH WINDRES
					  VIDEO_COCOA VIDEO_DIRECTFB VIDEO_DUMMY VIDEO_KMSDRM VIDEO_MIR VIDEO_OPENGL
					  VIDEO_OPENGLES VIDEO_RPI VIDEO_VIVANTE VIDEO_VULKAN VIDEO_WAYLAND VIDEO_X11 )
	mark_as_advanced( SDL_ATOMIC SDL_AUDIO SDL_CPUINFO SDL_DLOPEN SDL_EVENTS SDL_FILE
					  SDL_FILESYSTEM SDL_HAPTIC SDL_JOYSTICK SDL_LOADSO SDL_POWER SDL_RENDER
					  SDL_SHARED SDL_STATIC SDL_STATIC_PIC SDL_TEST SDL_THREADS SDL_TIMERS SDL_VIDEO )

	add_library( "SDL2-lib" INTERFACE )

	if (${SDL_SHARED})
		set_property( TARGET "SDL2" PROPERTY FOLDER "External" )
		set_property( TARGET "SDL2-lib" PROPERTY INTERFACE_LINK_LIBRARIES "SDL2" )
	endif ()

	if (${SDL_STATIC})
		set_property( TARGET "SDL2-static" PROPERTY FOLDER "External" )
		set_property( TARGET "SDL2-lib" PROPERTY INTERFACE_LINK_LIBRARIES "SDL2-static" )
	endif ()
	
	target_include_directories( "SDL2-lib" INTERFACE "${FG_EXTERNAL_SDL2_PATH}/include" )
	target_compile_definitions( "SDL2-lib" INTERFACE "FG_ENABLE_SDL2" )
endif ()