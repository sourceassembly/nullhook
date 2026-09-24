target_sources(cathook PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/astar.h"
	"${CMAKE_CURRENT_LIST_DIR}/CNavFile.h"
	"${CMAKE_CURRENT_LIST_DIR}/nav.h")

target_include_directories(cathook PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
