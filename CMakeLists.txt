cmake_minimum_required(VERSION 2.6)

project(SUDM)

enable_testing()

# The version number
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

if (CMAKE_BUILD_TYPE STREQUAL "")
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

if (MSVC)
	# Build cpp files on all cores
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
else()
	# Enable C++11, you may need to use -std=c++0x if using an older gcc compiler
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
endif()

# On Linux gtest needs pthreads
if (UNIX)
	find_package (Threads)
endif()

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/googlemock
    ${CMAKE_CURRENT_SOURCE_DIR}/googlemock/include
	${CMAKE_CURRENT_SOURCE_DIR}/googlemock/gtest/include
	${CMAKE_CURRENT_SOURCE_DIR}/googlemock/gtest
)

add_library(gmock STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/googlemock/src/gmock-all.cc
	${CMAKE_CURRENT_SOURCE_DIR}/googlemock/gtest/src/gtest-all.cc
)

add_subdirectory(tests)
