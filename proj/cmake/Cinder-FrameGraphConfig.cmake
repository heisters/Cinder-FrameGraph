if( NOT TARGET Cinder-FrameGraph )
    get_filename_component( FrameGraph_SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../src" ABSOLUTE )
    get_filename_component( FrameGraph_INCLUDE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../include" ABSOLUTE )
    get_filename_component( CINDER_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE )

		list( APPEND SOURCES
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph.hpp
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/Types.hpp
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/ColorGradeShaderIONode.hpp
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/LUTShaderIONode.hpp
                ${FrameGraph_INCLUDE_PATH}/Cinder-FrameGraph/FullScreenQuadRenderer.hpp
				${FrameGraph_SOURCE_PATH}/Cinder-FrameGraph.cpp
                ${FrameGraph_SOURCE_PATH}/libnodes/Node.cpp
                ${FrameGraph_SOURCE_PATH}/Cinder-FrameGraph/LUTShaderIONode.cpp
                ${FrameGraph_SOURCE_PATH}/Cinder-FrameGraph/ColorGradeShaderIONode.cpp
		)

    add_library( Cinder-FrameGraph ${SOURCES} )

    target_include_directories( Cinder-FrameGraph PUBLIC "${FrameGraph_INCLUDE_PATH}" )
    target_include_directories( Cinder-FrameGraph SYSTEM BEFORE PUBLIC "${CINDER_PATH}/include" )

    if( NOT TARGET cinder )
            include( "${CINDER_PATH}/proj/cmake/configure.cmake" )
            find_package( cinder REQUIRED PATHS
                "${CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
                "$ENV{CINDER_PATH}/${CINDER_LIB_DIRECTORY}" )
    endif()
    target_link_libraries( Cinder-FrameGraph PRIVATE cinder )

endif()