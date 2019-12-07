# find or download ffmpeg SDK (LGPL license)
# https://ffmpeg.zeranoe.com/builds/

if (${FG_EXTERNALS_USE_PREBUILD} AND ${FG_ENABLE_FFMPEG})
	set( FFMPEG_PATH "${FG_EXTERNALS_PATH}/ffmpeg" )
	add_library( "ffmpeg-lib" INTERFACE )
	target_include_directories( "ffmpeg-lib" INTERFACE "${FFMPEG_PATH}/include" )
	target_compile_definitions( "ffmpeg-lib" INTERFACE "FG_ENABLE_FFMPEG" )
	set_property( TARGET "ffmpeg-lib" PROPERTY INTERFACE_LINK_LIBRARIES 
		"${FFMPEG_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avcodec${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FFMPEG_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avdevice${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FFMPEG_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avfilter${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FFMPEG_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avformat${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FFMPEG_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avutil${CMAKE_STATIC_LIBRARY_SUFFIX}" 
		"${FFMPEG_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}postproc${CMAKE_STATIC_LIBRARY_SUFFIX}" 
		"${FFMPEG_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}swresample${CMAKE_STATIC_LIBRARY_SUFFIX}" 
		"${FFMPEG_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}swscale${CMAKE_STATIC_LIBRARY_SUFFIX}" )
		
elseif (${FG_ENABLE_FFMPEG})
	set( FG_EXTERNAL_FFMPEG_PATH "" CACHE PATH "path to ffmpeg SDK" )
	mark_as_advanced( FG_EXTERNAL_FFMPEG_PATH )

	if ((NOT WIN32) OR (NOT MSVC))
		message( FATAL_ERROR "only win64 and Visual Stuido are supported" )
	endif ()
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_FFMPEG_PATH}/ffmpeg/dev/README.txt")
		message( STATUS "ffmpeg SDK is not found in \"${FG_EXTERNAL_FFMPEG_PATH}\"" )
		set( FG_EXTERNAL_FFMPEG_PATH "${FG_EXTERNALS_PATH}/ffmpeg" CACHE PATH "" FORCE )
	else ()
		message( STATUS "ffmpeg SDK found in \"${FG_EXTERNAL_FFMPEG_PATH}\"" )
	endif ()
	
	ExternalProject_Add( "External.FFmpeg-dev"
		LOG_OUTPUT_ON_FAILURE 1
		# download
		URL					"https://ffmpeg.zeranoe.com/builds/win64/dev/ffmpeg-4.2.1-win64-dev.zip"
		DOWNLOAD_DIR		"${FG_EXTERNAL_FFMPEG_PATH}/dev-temp"
		LOG_DOWNLOAD		1
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_FFMPEG_PATH}/dev"
		CONFIGURE_COMMAND	${CMAKE_COMMAND} -E tar -xf "${FG_EXTERNAL_FFMPEG_PATH}/dev-temp/ffmpeg-4.2.1-win64-dev.zip"
		LOG_CONFIGURE 		1
		# build
		BUILD_COMMAND		${CMAKE_COMMAND} -E remove_directory "${FG_EXTERNAL_FFMPEG_PATH}/dev-temp"
		LOG_BUILD 			1
		# install
		INSTALL_COMMAND 	""
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)
	ExternalProject_Add( "External.FFmpeg-shared"
		LOG_OUTPUT_ON_FAILURE 1
		# download
		URL					"https://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-4.2.1-win64-shared.zip"
		DOWNLOAD_DIR		"${FG_EXTERNAL_FFMPEG_PATH}/shared-temp"
		LOG_DOWNLOAD		1
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_FFMPEG_PATH}/shared"
		CONFIGURE_COMMAND	${CMAKE_COMMAND} -E tar -xf "${FG_EXTERNAL_FFMPEG_PATH}/shared-temp/ffmpeg-4.2.1-win64-shared.zip"
		LOG_CONFIGURE 		1
		# build
		BUILD_COMMAND		${CMAKE_COMMAND} -E remove_directory "${FG_EXTERNAL_FFMPEG_PATH}/shared-temp"
		LOG_BUILD 			1
		# install
		INSTALL_COMMAND 	${CMAKE_COMMAND} -E copy "${FG_EXTERNAL_FFMPEG_PATH}/shared/bin/avcodec-58.dll" "${MAIN_BINARY_DIR}/$<CONFIG>/avcodec-58.dll"
							COMMAND ${CMAKE_COMMAND} -E copy "${FG_EXTERNAL_FFMPEG_PATH}/shared/bin/avdevice-58.dll" "${MAIN_BINARY_DIR}/$<CONFIG>/avdevice-58.dll"
							COMMAND ${CMAKE_COMMAND} -E copy "${FG_EXTERNAL_FFMPEG_PATH}/shared/bin/avfilter-7.dll" "${MAIN_BINARY_DIR}/$<CONFIG>/aavfilter-7.dll"
							COMMAND ${CMAKE_COMMAND} -E copy "${FG_EXTERNAL_FFMPEG_PATH}/shared/bin/avformat-58.dll" "${MAIN_BINARY_DIR}/$<CONFIG>/avformat-58.dll"
							COMMAND ${CMAKE_COMMAND} -E copy "${FG_EXTERNAL_FFMPEG_PATH}/shared/bin/avutil-56.dll" "${MAIN_BINARY_DIR}/$<CONFIG>/avutil-56.dll"
							COMMAND ${CMAKE_COMMAND} -E copy "${FG_EXTERNAL_FFMPEG_PATH}/shared/bin/postproc-55.dll" "${MAIN_BINARY_DIR}/$<CONFIG>/postproc-55.dll"
							COMMAND ${CMAKE_COMMAND} -E copy "${FG_EXTERNAL_FFMPEG_PATH}/shared/bin/swresample-3.dll" "${MAIN_BINARY_DIR}/$<CONFIG>/swresample-3.dll"
							COMMAND ${CMAKE_COMMAND} -E copy "${FG_EXTERNAL_FFMPEG_PATH}/shared/bin/swscale-5.dll" "${MAIN_BINARY_DIR}/$<CONFIG>/swscale-5.dll"
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)

	set_property( TARGET "External.FFmpeg-dev" PROPERTY FOLDER "External" )
	set_property( TARGET "External.FFmpeg-shared" PROPERTY FOLDER "External" )
	
	add_library( "ffmpeg-lib" INTERFACE )
	target_include_directories( "ffmpeg-lib" INTERFACE "${FG_EXTERNAL_FFMPEG_PATH}/dev/include" )
	target_compile_definitions( "ffmpeg-lib" INTERFACE "FG_ENABLE_FFMPEG" )
	add_dependencies( "ffmpeg-lib" "External.FFmpeg-dev" "External.FFmpeg-shared" )
	set_property( TARGET "ffmpeg-lib" PROPERTY INTERFACE_LINK_LIBRARIES 
		"${FG_EXTERNAL_FFMPEG_PATH}/dev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avcodec${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_EXTERNAL_FFMPEG_PATH}/dev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avdevice${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_EXTERNAL_FFMPEG_PATH}/dev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avfilter${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_EXTERNAL_FFMPEG_PATH}/dev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avformat${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_EXTERNAL_FFMPEG_PATH}/dev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avutil${CMAKE_STATIC_LIBRARY_SUFFIX}" 
		"${FG_EXTERNAL_FFMPEG_PATH}/dev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}postproc${CMAKE_STATIC_LIBRARY_SUFFIX}" 
		"${FG_EXTERNAL_FFMPEG_PATH}/dev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}swresample${CMAKE_STATIC_LIBRARY_SUFFIX}" 
		"${FG_EXTERNAL_FFMPEG_PATH}/dev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}swscale${CMAKE_STATIC_LIBRARY_SUFFIX}" )

endif ()
