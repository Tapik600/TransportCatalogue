set(GTEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/external/gmock")
set(GTEST_BINARY_DIR "${GTEST_DIR}/bin")
set(GTEST_SOURCE_DIR "${GTEST_DIR}/src")

set(GMOCK_LIB_PATH "${GTEST_BINARY_DIR}/lib/libgmock.a")
set(GMOCK_MAIN_PATH "${GTEST_BINARY_DIR}/lib/libgmock_main.a") # Can be used to link in the standaard GoogleMock main()
set(GTEST_LIB_PATH "${GTEST_BINARY_DIR}/lib/libgtest.a")
set(GTEST_MAIN_PATH "${GTEST_BINARY_DIR}/lib/libgtest_main.a")# Can be used to link in the standaard GoogleTest main()

set(GTEST_TAG "release-1.10.0" CACHE STRING "Git tag to use for GoogleTest")

include(ExternalProject)

# Download GoogleTest from Github
ExternalProject_Add(
    gtest_external
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG ${GTEST_TAG}
    GIT_SHALLOW ON
    PREFIX ${GTEST_DIR}
    BINARY_DIR ${GTEST_BINARY_DIR}
    BUILD_BYPRODUCTS ${GMOCK_LIB_PATH} ${GMOCK_MAIN_PATH} ${GTEST_LIB_PATH} ${GTEST_MAIN_PATH}
    INSTALL_COMMAND "" # Disable install step
    CMAKE_ARGS ${GMOCK_EXTERNAL_CMAKE_ARGS}
)


function(target_link_googletest target)
  add_dependencies(${target} gtest_external)
  target_include_directories(${target} PRIVATE "${GTEST_SOURCE_DIR}/gtest_external/googletest/include"
                                               "${GTEST_SOURCE_DIR}/gtest_external/googlemock/include")
  target_link_libraries(${target} PRIVATE ${GTEST_LIB_PATH} ${GMOCK_LIB_PATH} pthread)
endfunction(target_link_googletest)
