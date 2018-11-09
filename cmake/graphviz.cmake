# find command line GraphViz program that supports dot language.

set( FG_GRAPHVIZ_ROOT "" CACHE PATH "Path to GraphViz binaries" )


# search installed GraphViz application
if (WIN32)
	list( APPEND searchdirs ${FG_GRAPHVIZ_ROOT} ${CMAKE_SYSTEM_PREFIX_PATH} )
	  
	foreach (path ${searchdirs})
		file( GLOB filelist RELATIVE ${path} ${path}/* )

		foreach	(subath ${filelist})
			if (IS_DIRECTORY "${path}/${subath}")
								
				string( TOLOWER ${subath} outLowerCaseSubPath )
				string( FIND ${outLowerCaseSubPath} "graphviz" outPos )
				if (outPos GREATER -1)
					set( GRAPHVIZ_PATH "${path}/${subath}" )
				endif ()
				
			endif ()
		endforeach ()
		
	endforeach ()
	
	if (GRAPHVIZ_PATH)
		find_file( FG_GRAPHVIZ_DOT_EXECUTABLE NAMES "bin/dot.exe" PATHS "${GRAPHVIZ_PATH}" )
		
		if (NOT FG_GRAPHVIZ_DOT_EXECUTABLE)
			message( WARNING "GraphViz found in ${GRAPHVIZ_PATH}, but 'dot.exe' is not found!" )
		endif ()
	endif ()
endif ()

if (UNIX)
# TODO
endif ()


if (FG_GRAPHVIZ_DOT_EXECUTABLE)
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_GRAPHVIZ_DOT_EXECUTABLE=\"${FG_GRAPHVIZ_DOT_EXECUTABLE}\"" )
else ()
	message( WARNING "GraphViz is not found, download and instal library from https://www.graphviz.org/" )
endif ()
