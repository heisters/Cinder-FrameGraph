cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
#set( CMAKE_VERBOSE_MAKEFILE ON )

project( FrameGraph-ColorExample )

option(ENABLE_GLVIDEO "use video through libglvideo, instead of Cinder's built-in static image loading" OFF)

if(ENABLE_GLVIDEO)
    set(ENABLE_FRAMEGRAPH_LIBGLVIDEO ON CACHE BOOL "Enable FrameGraph libglvideo support")
    add_definitions(-DUSE_GLVIDEO)
endif(ENABLE_GLVIDEO)

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

list( APPEND SOURCES
        "${APP_PATH}/src/ColorApp.cpp"
        )


ci_make_app(
        APP_NAME    "FrameGraph-ColorExample"
        CINDER_PATH ${CINDER_PATH}
        SOURCES     ${SOURCES}
        BLOCKS      Cinder-FrameGraph
)