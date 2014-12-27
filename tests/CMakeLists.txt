if (UNIX)
	add_definitions(-DPOSIX)
endif()

include_directories(
${Boost_INCLUDE_DIR}
${CMAKE_CURRENT_SOURCE_DIR}/..
${CMAKE_CURRENT_SOURCE_DIR}/../common
${CMAKE_CURRENT_SOURCE_DIR}/../decompiler

)

add_executable(Sudm_tests
    main.cpp
)

target_link_libraries(Sudm_tests Sudm_Lib gmock ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME Sudm_tests COMMAND Sudm_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)
