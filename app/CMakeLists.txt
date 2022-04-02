cmake_minimum_required(VERSION 3.12)

project(
    transport_catalogue
    VERSION 1.0
    LANGUAGES CXX
)

add_executable (${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE tc_engine)
target_link_libraries(${PROJECT_NAME} tc_engine)