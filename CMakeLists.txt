cmake_minimum_required(VERSION 2.6)

project(SUDM)

# Set to true when using SUDM as a submodule of another project such that only the lib is compiled
option(SUDM_AS_LIB     "Build only lib"     FALSE)

if (NOT SUDM_AS_LIB)
	enable_testing()
endif()

# The version number
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

if (CMAKE_BUILD_TYPE STREQUAL "")
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

if (NOT SUDM_AS_LIB)
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

	SET(gmock_src
	${CMAKE_CURRENT_SOURCE_DIR}/googlemock/src/gmock-all.cc
	${CMAKE_CURRENT_SOURCE_DIR}/googlemock/gtest/src/gtest-all.cc)
	
	add_library(gmock STATIC ${gmock_src})
endif()

if (MSVC)
	# Build cpp files on all cores
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /W4")
	
	# can't use the "secure" versions as they're Windows specific
	add_definitions("/D\"_CRT_SECURE_NO_WARNINGS\"")
	add_definitions("/D\"_SCL_SECURE_NO_WARNINGS\"")

	add_definitions("/wd4290")
else()
	# Enable C++11, you may need to use -std=c++0x if using an older gcc compiler
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	else()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-unused-parameter -fPIC -Wall -Weffc++ -pedantic")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Wcast-align")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-qual  -Wchar-subscripts  -Wcomment -Wconversion")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdisabled-optimization")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfloat-equal  -Wformat  -Wformat=2")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat-nonliteral -Wformat-security")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat-y2k")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wimport  -Winit-self")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winvalid-pch")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunsafe-loop-optimizations -Wmissing-braces")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-field-initializers -Wmissing-format-attribute")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-include-dirs -Wmissing-noreturn")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpacked  -Wparentheses  -Wpointer-arith")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wredundant-decls -Wreturn-type")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsequence-point  -Wshadow -Wsign-compare  -Wstack-protector")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstrict-aliasing -Wstrict-aliasing=2 -Wswitch  -Wswitch-default")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wswitch-enum -Wtrigraphs  -Wuninitialized")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunknown-pragmas  -Wunreachable-code -Wunused")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-function  -Wunused-label  -Wunused-parameter")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-value  -Wunused-variable  -Wvariadic-macros")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wvolatile-register-var  -Wwrite-strings")
		
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0") # debug, no optimisation
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage") # enabling coverage
		
		# we dont want high warning levels on 3rd party things since we won't be fixing those warnings
		set_source_files_properties(${gmock_src} PROPERTIES COMPILE_FLAGS -Wno-effc++)
	endif()
endif()

SET(source
decompiler/sudm.cpp
decompiler/sudm.h
decompiler/decompiler_codegen.cpp
decompiler/decompiler_codegen.h
decompiler/control_flow.cpp
decompiler/control_flow.h
decompiler/decompiler_disassembler.cpp
decompiler/decompiler_disassembler.h
decompiler/decompiler_engine.h
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
decompiler/ff7_field/ff7_field_disassembler.cpp
decompiler/ff7_field/ff7_field_disassembler.h
decompiler/ff7_field/ff7_field_engine.cpp
decompiler/ff7_field/ff7_field_engine.h
decompiler/ff7_field/ff7_field_codegen.cpp
decompiler/ff7_field/ff7_field_codegen.h
decompiler/ff7_world/ff7_world_disassembler.cpp
decompiler/ff7_world/ff7_world_disassembler.h
decompiler/ff7_world/ff7_world_engine.cpp
decompiler/ff7_world/ff7_world_engine.h
decompiler/ff7_world/ff7_world_codegen.cpp
decompiler/ff7_world/ff7_world_codegen.h
common/algorithm.h
common/noncopyable.h
common/scummsys.h
common/str.cpp
common/str.h
common/util.h
common/binaryreader.h
common/lzs.h
common/make_unique.h
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

if (NOT SUDM_AS_LIB)
	add_executable(Sudm
	decompiler/decompiler.cpp
	)
	target_link_libraries(Sudm Sudm_Lib ${Boost_LIBRARIES})

	add_executable(Sudm_Test 
	decompiler/test/cfg_test.cpp
	decompiler/test/codegen.cpp
	decompiler/test/disassembler_test.cpp
	decompiler/test/disassembler/pasc.cpp
	decompiler/test/disassembler/pasc.h
	decompiler/test/disassembler/subopcode.cpp
	decompiler/test/disassembler/subopcode.h
	decompiler/test/main.cpp
	decompiler/test/ff7_field_all_opcodes_disassembler_test.cpp
	)

	target_link_libraries(Sudm_Test Sudm_Lib gmock ${Boost_LIBRARIES})

	add_test(NAME Sudm_Test COMMAND Sudm_Test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()
