cmake_minimum_required(VERSION 3.7)

project(Cinder-FrameGraph-Tests)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(CINDER_DIR "${PROJECT_SOURCE_DIR}/../../Cinder")
set(CFG_DIR "${PROJECT_SOURCE_DIR}/..")

set(SOURCE_FILES
        "${CFG_DIR}/src/Cinder-FrameGraph/Node.cpp"
        "${CINDER_DIR}/src/cinder/Signals.cpp"
        )


include_directories("${PROJECT_SOURCE_DIR}" "${CFG_DIR}/include" "${CINDER_DIR}/include")

add_executable(tests main.cpp test_nodes.cpp "${SOURCE_FILES}")