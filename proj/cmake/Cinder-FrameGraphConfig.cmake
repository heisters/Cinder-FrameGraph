if( NOT TARGET Cinder-FrameGraph )
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    option(ENABLE_FRAMEGRAPH_QUICKTIME "enable nodes for working with Quicktime videos" OFF)
    option(ENABLE_FRAMEGRAPH_LIBGLVIDEO "enable nodes for working with libglvideo" OFF)

    get_filename_component( FrameGraph_SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../src" ABSOLUTE )
    get_filename_component( FrameGraph_INCLUDE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../include" ABSOLUTE )
    get_filename_component( FrameGraph_LIB_PATH "${CMAKE_CURRENT_LIST_DIR}/../../lib" ABSOLUTE )
    get_filename_component( CINDER_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE )
    get_filename_component( LIBGLVIDEO_PATH "${FrameGraph_LIB_PATH}/libglvideo" ABSOLUTE )

    list( APPEND FrameGraph_SOURCES
            ${FrameGraph_INCLUDE_PATH}/cinder/FrameGraph.hpp
            ${FrameGraph_INCLUDE_PATH}/cinder/framegraph/Types.hpp
            ${FrameGraph_INCLUDE_PATH}/cinder/framegraph/ColorGradeNode.hpp
            ${FrameGraph_INCLUDE_PATH}/cinder/framegraph/LUTNode.hpp
            ${FrameGraph_INCLUDE_PATH}/cinder/framegraph/FullScreenQuadRenderer.hpp
            ${FrameGraph_INCLUDE_PATH}/cinder/framegraph/VecNode.hpp
            ${FrameGraph_SOURCE_PATH}/cinder/FrameGraph.cpp
            ${FrameGraph_SOURCE_PATH}/cinder/framegraph/LUTNode.cpp
            ${FrameGraph_SOURCE_PATH}/cinder/framegraph/ColorGradeNode.cpp
            ${FrameGraph_LIB_PATH}/libnodes/src/libnodes/Node.cpp
            )
    list( APPEND FrameGraph_INCLUDE_DIRS
            "${FrameGraph_INCLUDE_PATH}"
            "${FrameGraph_LIB_PATH}/libnodes/include"
            )
    list( APPEND FrameGraph_LIBS
            cinder
            )

    if( ENABLE_FRAMEGRAPH_QUICKTIME )
      list( APPEND FrameGraph_SOURCES
        ${FrameGraph_INCLUDE_PATH}/cinder/framegraph/QuickTime.hpp
        ${FrameGraph_SOURCE_PATH}/cinder/framegraph/QuickTime.cpp
        )
    endif()
    if( ENABLE_FRAMEGRAPH_LIBGLVIDEO )
      find_package(libglvideo REQUIRED PATHS "${LIBGLVIDEO_PATH}/cmake" NO_DEFAULT_PATH)

      list( APPEND FrameGraph_SOURCES
        ${FrameGraph_INCLUDE_PATH}/cinder/framegraph/GLVideo.hpp
        ${FrameGraph_SOURCE_PATH}/cinder/framegraph/GLVideo.cpp
        )

      list( APPEND FrameGraph_INCLUDE_DIRS "${LIBGLVIDEO_INCLUDE_DIRS}" )
      list( APPEND FrameGraph_LIBS libglvideo )
    endif()

    add_library( Cinder-FrameGraph ${FrameGraph_SOURCES} )

    target_include_directories( Cinder-FrameGraph PUBLIC "${FrameGraph_INCLUDE_DIRS}" )
    target_include_directories( Cinder-FrameGraph SYSTEM BEFORE PUBLIC "${CINDER_PATH}/include" )

    if( NOT TARGET cinder )
            include( "${CINDER_PATH}/proj/cmake/configure.cmake" )
            find_package( cinder REQUIRED PATHS
                "${CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
                "$ENV{CINDER_PATH}/${CINDER_LIB_DIRECTORY}" )
    endif()
    target_link_libraries( Cinder-FrameGraph PRIVATE "${FrameGraph_LIBS}" )

endif()
