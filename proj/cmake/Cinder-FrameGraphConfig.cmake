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

		list( APPEND FrameGraph_SOURCES
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph.hpp
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/Types.hpp
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/ColorGradeNode.hpp
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/LUTNode.hpp
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/FullScreenQuadRenderer.hpp
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/VecNode.hpp
                ${FrameGraph_SOURCE_PATH}/Cinder-FrameGraph.cpp
                ${FrameGraph_SOURCE_PATH}/Cinder-FrameGraph/LUTNode.cpp
                ${FrameGraph_SOURCE_PATH}/Cinder-FrameGraph/ColorGradeNode.cpp
                ${FrameGraph_LIB_PATH}/libnodes/src/libnodes/Node.cpp
                )

    if( ENABLE_FRAMEGRAPH_QUICKTIME )
      list( APPEND FrameGraph_SOURCES
        ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/QuickTime.hpp
        ${FrameGraph_SOURCE_PATH}/Cinder-FrameGraph/QuickTime.cpp
        )
    endif()
    if( ENABLE_FRAMEGRAPH_LIBGLVIDEO )
      list( APPEND FrameGraph_SOURCES
        ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/GLVideo.hpp
        ${FrameGraph_SOURCE_PATH}/Cinder-FrameGraph/GLVideo.cpp
        )
    endif()

    add_library( Cinder-FrameGraph ${FrameGraph_SOURCES} )

    target_include_directories( Cinder-FrameGraph PUBLIC "${FrameGraph_INCLUDE_PATH}" )
    target_include_directories( Cinder-FrameGraph PUBLIC "${FrameGraph_LIB_PATH}/libnodes/include" )
    target_include_directories( Cinder-FrameGraph SYSTEM BEFORE PUBLIC "${CINDER_PATH}/include" )

    if( NOT TARGET cinder )
            include( "${CINDER_PATH}/proj/cmake/configure.cmake" )
            find_package( cinder REQUIRED PATHS
                "${CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
                "$ENV{CINDER_PATH}/${CINDER_LIB_DIRECTORY}" )
    endif()
    target_link_libraries( Cinder-FrameGraph PRIVATE cinder )

endif()
