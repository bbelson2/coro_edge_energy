cmake_minimum_required(VERSION 3.16)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

project(vanet2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fcoroutines")
# set(CMAKE_CXX_FLAGS_DEBUG "-g")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# From https://stackoverflow.com/a/10055571
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # using Clang
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fcoroutines-ts -stdlib=libc++")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fcoroutines")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # using Visual Studio C++
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

include_directories(include)
include_directories(../3rdparty/fpm/include)
include_directories(../3rdparty/tclap/include)
include_directories(../3rdparty/tlx)

find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
if(NOT WIRINGPI_LIBRARIES)
  add_compile_definitions(WIRINGPI_MOCK)
endif()

set(pe_sources
  perf/pe_assist_cpp.cpp  
  perf/pe_assist.cpp
  perf/pe_assist_coll.cpp
  perf/pe_monitor.cpp
)

# add_executable(svm1 experiments/svm1.cpp)
add_executable(infer7 infer7.cpp ${pe_sources})
target_compile_definitions(infer7 PUBLIC PE_EXCLUDE_PRINTS)
add_executable(infer7a infer7.cpp ${pe_sources})
target_compile_definitions(infer7a PUBLIC USE_FPM=0)
target_compile_definitions(infer7a PUBLIC PE_EXCLUDE_PRINTS)
# add_executable(transmit transmit.cpp)

if(WIRINGPI_LIBRARIES)
  target_link_libraries(infer7 ${WIRINGPI_LIBRARIES})
  target_link_libraries(infer7a ${WIRINGPI_LIBRARIES})
endif()

add_executable(
  btree_test
  test/btree_test.cpp ../3rdparty/tlx/tlx/die/core.cpp
)
target_compile_definitions(btree_test PUBLIC TLX_MORE_TESTS)

add_executable(svm_test test/svm_test.cpp)
add_executable(svm_test_fp test/svm_test_fp.cpp)

target_link_libraries(svm_test GTest::gtest_main)
target_link_libraries(svm_test_fp GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(svm_test)

include(GoogleTest)
gtest_discover_tests(svm_test_fp)

add_custom_target(main)
add_dependencies(main infer7)


