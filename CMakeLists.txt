cmake_minimum_required(VERSION 3.12)

project(
    tc_engine
    VERSION 1.0
    LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(
    -lstdc++
    -Wall
    -Werror
    -Wno-psabi
)

add_subdirectory(libs/json)
add_subdirectory(libs/svg)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

file(GLOB SOURCES "src/*.cpp")
file(GLOB PROTO "src/proto/*.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO})

add_library(${PROJECT_NAME} ${SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_CURRENT_BINARY_DIR}
        ${Protobuf_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" 
        Threads::Threads -lm -latomic     
        json
        svg
    PRIVATE
)

add_subdirectory(app)

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    include(cmake/gtest.cmake)
    add_subdirectory(test)
endif()
