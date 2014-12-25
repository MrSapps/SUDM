add_executable(Sudm_tests
    main.cpp
)

target_link_libraries(Sudm_tests gmock)
add_test(Sudm_tests Sudm_tests)
