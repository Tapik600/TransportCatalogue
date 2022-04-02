add_executable(tc_tests 
    test_catalogue.cpp 
    test_router.cpp
)

target_link_libraries(tc_tests PRIVATE ${GMOCK_MAIN_PATH} tc_engine)
target_link_googletest(tc_tests)

add_test(tc_tests tc_tests)