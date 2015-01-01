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
	
	# can't use the "secure" versions as they're Windows specific
	add_definitions("/D\"_CRT_SECURE_NO_WARNINGS\"")
	add_definitions("/D\"_SCL_SECURE_NO_WARNINGS\"")

	add_definitions("/wd4290")
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

SET(source
decompiler/codegen.cpp
decompiler/codegen.h
decompiler/control_flow.cpp
decompiler/control_flow.h
decompiler/disassembler.cpp
decompiler/disassembler.h
decompiler/engine.h
decompiler/graph.cpp
decompiler/graph.h
decompiler/instruction.cpp
decompiler/instruction.h
decompiler/objectFactory.h
decompiler/refcounted.h
decompiler/simple_disassembler.cpp
decompiler/simple_disassembler.h
decompiler/stack.h
decompiler/unknown_opcode_exception.cpp
decompiler/unknown_opcode_exception.h
decompiler/value.cpp
decompiler/value.h
decompiler/wrongtype.h
decompiler/scummv6/codegen.cpp
decompiler/scummv6/codegen.h
decompiler/scummv6/disassembler.cpp
decompiler/scummv6/disassembler.h
decompiler/scummv6/engine.cpp
decompiler/scummv6/engine.h
decompiler/kyra/codegen.cpp
decompiler/kyra/codegen.h
decompiler/kyra/disassembler.cpp
decompiler/kyra/disassembler.h
decompiler/kyra/engine.cpp
decompiler/kyra/engine.h
decompiler/groovie/disassembler.cpp
decompiler/groovie/disassembler.h
decompiler/groovie/engine.cpp
decompiler/groovie/engine.h
decompiler/groovie/opcodes.h
decompiler/ff7_field/disassembler.cpp
decompiler/ff7_field/disassembler.h
decompiler/ff7_field/engine.cpp
decompiler/ff7_field/engine.h
decompiler/ff7_field/codegen.cpp
decompiler/ff7_field/codegen.h
decompiler/ff7_world/disassembler.cpp
decompiler/ff7_world/disassembler.h
decompiler/ff7_world/engine.cpp
decompiler/ff7_world/engine.h
decompiler/ff7_world/codegen.cpp
decompiler/ff7_world/codegen.h
common/algorithm.h
common/endianness.h
common/file.cpp
common/file.h
common/func.h
common/noncopyable.h
common/scummsys.h
common/str.cpp
common/str.h
common/util.h
)

find_package(Boost COMPONENTS program_options filesystem thread date_time system chrono REQUIRED)

include_directories(
${Boost_INCLUDE_DIR}
${CMAKE_CURRENT_SOURCE_DIR}
${CMAKE_CURRENT_SOURCE_DIR}/common
${CMAKE_CURRENT_SOURCE_DIR}/decompiler

)
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_ALL_DYN_LINK)

if (UNIX)
	add_definitions(-DPOSIX)
endif()

ADD_LIBRARY(Sudm_Lib STATIC ${source})

add_executable(Sudm
decompiler/decompiler.cpp
)
target_link_libraries(Sudm Sudm_Lib ${Boost_LIBRARIES})

add_executable(Sudm_Test2 
decompiler/test/cfg_test.cpp
decompiler/test/codegen.cpp
decompiler/test/disassembler_test.cpp
decompiler/test/disassembler/pasc.cpp
decompiler/test/disassembler/pasc.h
decompiler/test/disassembler/subopcode.cpp
decompiler/test/disassembler/subopcode.h
)

target_link_libraries(Sudm_Test2 Sudm_Lib gmock ${Boost_LIBRARIES})

add_test(NAME Sudm_Test2 COMMAND Sudm_Test2 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})


