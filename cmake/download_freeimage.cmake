# find or download FreeImage
# Warning: GPLv3 license!

if (${FG_ENABLE_FREEIMAGE})
	message( WARNING "Warning: GPL contagion!" )
	set( FG_EXTERNAL_FREEIMAGE_PATH "" CACHE PATH "path to FreeImage source" )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_FREEIMAGE_PATH}/Source/FreeImage.h")
		message( STATUS "FreeImage is not found in \"${FG_EXTERNAL_FREEIMAGE_PATH}\"" )
		set( FG_EXTERNAL_FREEIMAGE_PATH "${FG_EXTERNALS_PATH}/FreeImage" CACHE PATH "" FORCE )
		set( FG_FREEIMAGE_REPOSITORY "https://github.com/imazen/freeimage.git" )
	else ()
		set( FG_FREEIMAGE_REPOSITORY "" )
	endif ()
	
	# download from https://sourceforge.net/projects/freeimage/files/Source%20Distribution/3.18.0/FreeImage3180.zip/download
	
endif ()
